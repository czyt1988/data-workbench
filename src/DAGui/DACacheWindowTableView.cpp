#include "DACacheWindowTableView.h"
#include <QAbstractItemDelegate>
#include <QApplication>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QScrollBar>
#include "DAVirtualScrollBar.h"
#include "Models/DAAbstractCacheWindowTableModel.h"
namespace DA
{
DACacheWindowTableView::DACacheWindowTableView(QWidget* parent) : QTableView(parent)
{
	setAlternatingRowColors(false);
	QFontMetrics fm = fontMetrics();
	if (QHeaderView* vheader = verticalHeader()) {
		vheader->setDefaultSectionSize(fm.lineSpacing() * 1.2);
		vheader->setSectionResizeMode(QHeaderView::Interactive);
	}
	// 用整表坐标滚动条替换默认纵向滚动条：QScrollBar 的 value/range 仍保持「窗口内行号」
	// 语义（QTableView/QHeaderView 内部依赖它），但绘制与拖动按整表坐标，见 DAVirtualScrollBar
	mVirtualVBar = new DAVirtualScrollBar(this);
	setVerticalScrollBar(mVirtualVBar);
	connect(mVirtualVBar, &DAVirtualScrollBar::seekRequested, this, &DACacheWindowTableView::onSeekRequested);
	connect(mVirtualVBar, &DAVirtualScrollBar::stepRowsRequested, this, &DACacheWindowTableView::onStepRowsRequested);
	connect(mVirtualVBar, &DAVirtualScrollBar::pageRowsRequested, this, &DACacheWindowTableView::onPageRowsRequested);
	connect(mVirtualVBar, &DAVirtualScrollBar::gotoRowRequested, this, &DACacheWindowTableView::onGotoRowRequested);
	// 值变化来自方向键/滚轮/翻页/拖拽选择的自动滚动，统一在这里维持缓冲区
	connect(mVirtualVBar, &QScrollBar::valueChanged, this, [ this ](int) {
		if (mAdjustingScrollBar) {
			return;
		}
		ensureWindowCoversViewport();
		recenterWindowIfNeeded();
		syncScrollBarState();
	});
}

DACacheWindowTableView::~DACacheWindowTableView()
{
}

DAAbstractCacheWindowTableModel* DACacheWindowTableView::getCacheModel() const
{
	return qobject_cast< DAAbstractCacheWindowTableModel* >(model());
}

DAVirtualScrollBar* DACacheWindowTableView::getVirtualScrollBar() const
{
	return mVirtualVBar;
}

/**
 * @brief 视口首行的绝对行号 T = W + v
 * @return 绝对行号
 */
int DACacheWindowTableView::getTopActualRow() const
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return 0;
	}
	return cacheModel->getCacheWindowStartRow() + verticalScrollBar()->value();
}

/**
 * @brief 一屏可见行数
 * @return 行数（取不到时按行高估算）
 */
int DACacheWindowTableView::getViewportRowCount() const
{
	if (const QScrollBar* vbar = verticalScrollBar()) {
		if (vbar->pageStep() > 0) {
			return vbar->pageStep();
		}
	}
	const int rowHeightPx = qMax(1, rowHeight(0));
	return qMax(1, viewport()->height() / rowHeightPx);
}

/**
 * @brief 显示指定绝对行：把它放到视口首行
 *
 * 目标行靠近数据尾部、下面凑不满一屏时只能贴到最后一屏。
 * @param actualRow 目标绝对行
 */
void DACacheWindowTableView::showActualRow(int actualRow)
{
	placeViewportAt(actualRow);
}

/**
 * @brief 把视口首行放到指定绝对行
 *
 * 窗口重新摆位，使目标行上方留约 1/4 窗口的缓冲区余量（贴数据两端时自动收紧），
 * 这样定位之后上下都还能继续滚动。
 * @param actualRow 目标绝对行
 */
void DACacheWindowTableView::placeViewportAt(int actualRow)
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return;
	}
	const int totalRows = cacheModel->actualRowCount();
	if (totalRows <= 0) {
		return;
	}
	const int target = qBound(0, actualRow, totalRows - 1);
	const int rows   = qMax(1, cacheModel->rowCount());
	const int rowGap = getViewportRowCount();
	// 行数变化过（如惰性源扩容）会导致滚动条范围陈旧，先刷新再定位
	QScrollBar* vbar = verticalScrollBar();
	if (vbar->maximum() != qMax(0, rows - rowGap)) {
		updateGeometries();
	}
	// 窗口摆位：目标行上方留 1/4 窗口余量，保证上下都还能继续滚
	const int newW = qBound(0, target - rows / 4, qMax(0, cacheModel->maxCacheWindowStartRow()));
	const int oldW = cacheModel->getCacheWindowStartRow();
	// 期望的窗口内行号；贴到数据尾部时会被滚动条上界收紧（此时展示最后一屏）
	const int newV = qBound(0, target - newW, qMax(0, vbar->maximum()));
	if (newW == oldW && newV == vbar->value()) {
		return;
	}
	mAdjustingScrollBar = true;
	cacheModel->setCacheWindowStartRow(newW);
	syncViewStateAfterWindowShift(oldW - newW);
	vbar->setValue(newV);
	mAdjustingScrollBar = false;
	syncScrollBarState();
}

/**
 * @brief 高亮选中目标单元格（会先把它显示出来）
 * @param row 绝对行号
 * @param col 列号
 */
void DACacheWindowTableView::selectActualCell(int actualRow, int col)
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return;
	}
	// 先把真实行显示出来再进行高亮
	showActualRow(actualRow);
	showColumn(col);

	const int logicalRow = toLogicalRow(actualRow);
	if (logicalRow < 0 || logicalRow >= cacheModel->rowCount()) {
		// 超出范围
		return;
	}
	QModelIndex index = cacheModel->index(logicalRow, col);

	// 高亮选中项
	QItemSelectionModel* selModel = selectionModel();
	if (selModel) {
		selModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
	}
}

/**
 * @brief 转换为逻辑行（窗口内行号）
 * @param actualRow 绝对行号
 * @return 窗口内行号，可能为负或超出窗口（调用方需要判断）
 */
int DACacheWindowTableView::toLogicalRow(int actualRow) const
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return actualRow;
	}
	actualRow -= cacheModel->getCacheWindowStartRow();
	return actualRow;
}

/**
 * @brief 转换为绝对行
 * @param logicalRow 窗口内行号
 * @return 数据源绝对行号
 */
int DACacheWindowTableView::toActualRow(int logicalRow) const
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return logicalRow;
	}
	return cacheModel->getCacheWindowStartRow() + logicalRow;
}

/**
 * @brief 判断绝对行是否落在窗口缓冲区内
 * @param actualRow 绝对行号
 * @return 在窗口内返回 true
 */
bool DACacheWindowTableView::isActualRowInViewRange(int actualRow) const
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return false;
	}
	const int row = toLogicalRow(actualRow);
	if (row < 0) {
		return false;
	}
	if (row >= cacheModel->rowCount()) {
		return false;
	}
	return true;
}

/**
 * @brief 判断绝对行是否真的显示在视口里
 * @param actualRow 绝对行号
 * @return 可见返回 true
 */
bool DACacheWindowTableView::isActualRowVisible(int actualRow) const
{
	const int logicalRow = toLogicalRow(actualRow);
	if (logicalRow < 0 || !model() || logicalRow >= model()->rowCount()) {
		return false;
	}
	const int y = rowViewportPosition(logicalRow);
	return (y + rowHeight(logicalRow)) > 0 && y < viewport()->height();
}

/**
 * @brief 相对移动视口
 *
 * 缓冲区装得下时只改滚动条 value（纯滚动，一次一行/一屏，精确到行）；
 * 只有当移动会把目标行挤出缓冲区时才搬窗。
 * @param deltaRows 正数向下，负数向上
 */
void DACacheWindowTableView::moveViewport(int deltaRows)
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel || deltaRows == 0) {
		return;
	}
	const int totalRows = cacheModel->actualRowCount();
	if (totalRows <= 0) {
		return;
	}
	QScrollBar* vbar = verticalScrollBar();
	const int newV   = vbar->value() + deltaRows;
	if (newV >= 0 && newV <= vbar->maximum()) {
		vbar->setValue(newV);
		return;
	}
	// 缓冲区到头：把视口搬到目标位置（窗口随之重摆，给两个方向都留出余量）
	placeViewportAt(qBound(0, getTopActualRow() + deltaRows, totalRows - 1));
}

/**
 * @brief 返回真实行对应的表头名
 * @param actualRow 绝对行号
 * @return 表头文本
 */
QString DACacheWindowTableView::actualRowName(int actualRow) const
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return model() ? model()->headerData(actualRow, Qt::Vertical).toString() : QString();
	}
	return cacheModel->actualHeaderData(actualRow, Qt::Vertical).toString();
}

/**
 * @brief 返回真实列对应的表头名
 * @param actualCol 列号
 * @return 表头文本
 */
QString DACacheWindowTableView::actualColumnName(int actualCol) const
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return model() ? model()->headerData(actualCol, Qt::Horizontal).toString() : QString();
	}
	return cacheModel->headerData(actualCol, Qt::Horizontal).toString();
}

/**
 * @brief 视口滚动后同步滚动条状态
 * @param dx 水平增量（像素）
 * @param dy 垂直增量（像素）
 */
void DACacheWindowTableView::scrollContentsBy(int dx, int dy)
{
	QTableView::scrollContentsBy(dx, dy);
	if (mAdjustingScrollBar || dy == 0) {
		return;
	}
	syncScrollBarState();
}

/**
 * @brief 键盘导航
 *
 * 方向键/滚轮/Tab 等交给 QTableView（滚动条 value 是窗口内行号，天然精确到行）；
 * 这里只接管需要整表语义的键：翻页（光标跟随一屏）、Home/End 家族、Ctrl+G。
 * @param event 键盘事件
 */
void DACacheWindowTableView::keyPressEvent(QKeyEvent* event)
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (cacheModel && cacheModel->actualRowCount() > 0) {
		const int totalRows       = cacheModel->actualRowCount();
		const QModelIndex current = currentIndex();
		switch (event->key()) {
		case Qt::Key_PageDown:
		case Qt::Key_PageUp: {
			// 翻一屏，并让光标跟随翻页（保持它在视口中的相对位置，Excel 语义）
			const int delta      = (event->key() == Qt::Key_PageDown) ? getViewportRowCount() : -getViewportRowCount();
			const int cursorSlot = current.isValid() ? (toActualRow(current.row()) - getTopActualRow()) : 0;
			moveViewport(delta);
			if (current.isValid()) {
				setCurrentActualCell(getTopActualRow() + cursorSlot, current.column());
			}
			event->accept();
			return;
		}
		case Qt::Key_Home: {
			if (event->modifiers() & Qt::ControlModifier) {
				// Ctrl+Home：数据首行首列
				selectActualCell(0, 0);
			} else if (current.isValid()) {
				// Home：当前行首列
				setCurrentActualCell(toActualRow(current.row()), 0);
			}
			event->accept();
			return;
		}
		case Qt::Key_End: {
			if (event->modifiers() & Qt::ControlModifier) {
				// Ctrl+End：数据末行末列
				selectActualCell(totalRows - 1, qMax(0, cacheModel->columnCount() - 1));
			} else if (current.isValid()) {
				// End：当前行末列
				setCurrentActualCell(toActualRow(current.row()), qMax(0, cacheModel->columnCount() - 1));
			}
			event->accept();
			return;
		}
		case Qt::Key_G:
			if (event->modifiers() & Qt::ControlModifier) {
				promptGotoRow();
				event->accept();
				return;
			}
			break;
		default:
			break;
		}
	}
	QTableView::keyPressEvent(event);
}

/**
 * @brief 换模型时重新接线（只断开本类建立的连接，不动 QTableView 自身与模型的连接）
 * @param model 新模型
 */
void DACacheWindowTableView::setModel(QAbstractItemModel* model)
{
	for (const QMetaObject::Connection& c : mModelConnections) {
		disconnect(c);
	}
	mModelConnections.clear();
	QTableView::setModel(model);
	if (model) {
		mModelConnections.append(connect(model,
		                                 &QAbstractItemModel::modelReset,
		                                 this,
		                                 &DACacheWindowTableView::onModelReset,
		                                 Qt::UniqueConnection));
		if (DAAbstractCacheWindowTableModel* cacheModel = getCacheModel()) {
			mModelConnections.append(connect(cacheModel,
			                                 &DAAbstractCacheWindowTableModel::cacheWindowChanged,
			                                 this,
			                                 &DACacheWindowTableView::onCacheWindowChanged,
			                                 Qt::UniqueConnection));
			mModelConnections.append(connect(cacheModel,
			                                 &DAAbstractCacheWindowTableModel::tableStructureChanged,
			                                 this,
			                                 &DACacheWindowTableView::onTableStructureChanged,
			                                 Qt::UniqueConnection));
		}
	}
	syncScrollBarState();
}

/**
 * @brief 滚动条拖动滑块：把目标行显示为视口首行
 * @param actualRow 目标绝对行
 */
void DACacheWindowTableView::onSeekRequested(int actualRow)
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return;
	}
	const int totalRows = cacheModel->actualRowCount();
	if (totalRows <= 0) {
		return;
	}
	const int target = qBound(0, actualRow, totalRows - 1);
	// 目标行还在缓冲区里就只滚 value（连续拖动时避免每帧搬窗）
	const int v = target - cacheModel->getCacheWindowStartRow();
	if (v >= 0 && v <= verticalScrollBar()->maximum()) {
		verticalScrollBar()->setValue(v);
		return;
	}
	placeViewportAt(target);
}

/**
 * @brief 滚动条上下箭头：相对滚动若干行
 * @param rows 行数
 */
void DACacheWindowTableView::onStepRowsRequested(int rows)
{
	moveViewport(rows);
}

/**
 * @brief 滚动条轨道点击：相对翻若干屏
 * @param pages 屏数
 */
void DACacheWindowTableView::onPageRowsRequested(int pages)
{
	moveViewport(pages * getViewportRowCount());
}

/**
 * @brief 滚动条右键菜单请求「定位到行」
 */
void DACacheWindowTableView::onGotoRowRequested()
{
	promptGotoRow();
}

/**
 * @brief 模型窗口起始行变化：同步滚动条状态
 * @param oldStartRow 原起始行
 * @param newStartRow 新起始行
 */
void DACacheWindowTableView::onCacheWindowChanged(int oldStartRow, int newStartRow)
{
	Q_UNUSED(oldStartRow);
	Q_UNUSED(newStartRow);
	syncScrollBarState();
}

/**
 * @brief 模型结构变化（插删行列）：行数可能已变，自愈窗口与滚动范围
 */
void DACacheWindowTableView::onTableStructureChanged()
{
	ensureWindowCoversViewport();
}

/**
 * @brief 模型重置（数据被替换）：位置回到首行
 */
void DACacheWindowTableView::onModelReset()
{
	mPendingRecenter = false;
	placeViewportAt(0);
	syncScrollBarState();
}

/**
 * @brief v 逼近缓冲区两端时整体平移窗口
 *
 * 平移保持 T = W + v 不变，因此屏幕内容一动不动；只是把缓冲搬到数据源的新位置，
 * 让继续滚动始终有空间 —— 这是方向键能逐行走完整个数据源的关键。
 */
void DACacheWindowTableView::recenterWindowIfNeeded()
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	QScrollBar* vbar                            = verticalScrollBar();
	if (!cacheModel || !vbar) {
		return;
	}
	const int maxV = vbar->maximum();
	if (maxV <= 0) {
		return;  // 整表都装得进窗口，不需要缓冲区
	}
	if (mPendingRecenter) {
		if (!commitOpenEditor()) {
			return;  // 还在编辑，等编辑器关掉后再补
		}
		mPendingRecenter = false;
	}
	const int v      = vbar->value();
	const int margin = qMax(1, maxV / 8);
	if (v > margin && v < maxV - margin) {
		return;  // 缓冲区还有余量
	}
	const int delta = v - maxV / 2;  // 目标：把 v 摆回中间
	if (delta == 0) {
		return;
	}
	shiftWindow(delta);
}

/**
 * @brief 平移窗口：W 与 v 同时加减 deltaRows，视口首行 T 不变
 * @param deltaRows 窗口起始行的增量
 * @return 发生了平移返回 true
 */
bool DACacheWindowTableView::shiftWindow(int deltaRows)
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel || deltaRows == 0) {
		return false;
	}
	const int oldW = cacheModel->getCacheWindowStartRow();
	const int newW = qBound(0, oldW + deltaRows, qMax(0, cacheModel->maxCacheWindowStartRow()));
	if (newW == oldW) {
		return false;
	}
	// 编辑器持有的是逻辑行，窗口一搬就会指向别的数据，必须先提交
	if (!commitOpenEditor()) {
		mPendingRecenter = true;
		return false;
	}
	QScrollBar* vbar    = verticalScrollBar();
	mAdjustingScrollBar = true;
	cacheModel->setCacheWindowStartRow(newW);
	syncViewStateAfterWindowShift(oldW - newW);
	vbar->setValue(qBound(0, vbar->value() - (newW - oldW), vbar->maximum()));
	mAdjustingScrollBar = false;
	syncScrollBarState();
	return true;
}

/**
 * @brief 窗口平移后同步当前项与选区，使它们仍指向同一批绝对行
 *
 * 移出窗口的部分丢弃；当前项整个被移出时，把光标放到视口首行（同列），
 * 保证后续键盘操作有落点。
 * @param deltaRows 逻辑行的平移量（新逻辑行 = 旧逻辑行 + deltaRows）
 */
void DACacheWindowTableView::syncViewStateAfterWindowShift(int deltaRows)
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	QItemSelectionModel* selModel               = selectionModel();
	if (!cacheModel || !selModel || !model() || deltaRows == 0) {
		return;
	}
	const int rows = model()->rowCount();
	const int cols = model()->columnCount();
	if (rows <= 0 || cols <= 0) {
		return;
	}
	const QModelIndex current   = selModel->currentIndex();
	const QItemSelection before = selModel->selection();
	// 1) 选区整体平移后与窗口求交
	QItemSelection moved;
	for (const QItemSelectionRange& r : before) {
		const int top    = qMax(0, r.top() + deltaRows);
		const int bottom = qMin(rows - 1, r.bottom() + deltaRows);
		if (bottom < top) {
			continue;
		}
		moved.append(QItemSelectionRange(model()->index(top, r.left()), model()->index(bottom, r.right())));
	}
	if (!before.isEmpty()) {
		selModel->select(moved, QItemSelectionModel::ClearAndSelect);
	}
	// 2) 当前项
	if (current.isValid()) {
		const int logicalRow = current.row() + deltaRows;
		if (logicalRow >= 0 && logicalRow < rows) {
			selModel->setCurrentIndex(model()->index(logicalRow, current.column()), QItemSelectionModel::NoUpdate);
			return;
		}
	}
	// 当前项被移出窗口：光标落到视口首行同列
	const int topLogical = qBound(0, getTopActualRow() - cacheModel->getCacheWindowStartRow(), rows - 1);
	selModel->setCurrentIndex(model()->index(topLogical, qMin(cols - 1, qMax(0, current.column()))),
	                          QItemSelectionModel::ClearAndSelect);
}

/**
 * @brief 窗口与滚动范围自愈
 *
 * 结构变化（插删行列）后行数可能变了：滚动条范围、窗口合法上界、视口首行都可能失效。
 * 正常路径下这些判断都是几次整数比较，不做任何搬动。
 */
void DACacheWindowTableView::ensureWindowCoversViewport()
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel || !model()) {
		return;
	}
	QScrollBar* vbar = verticalScrollBar();
	// 1) 行数变了 → 滚动条范围可能陈旧（Qt 的范围基于表头段数，只在安装模型/重置时更新）
	const int wantMax = qMax(0, qMax(1, cacheModel->rowCount()) - getViewportRowCount());
	if (vbar->maximum() != wantMax) {
		updateGeometries();
	}
	// 2) 数据变少导致窗口越界 → 夹回合法范围，并保持视口首行不变
	const int oldW = cacheModel->getCacheWindowStartRow();
	const int maxW = qMax(0, cacheModel->maxCacheWindowStartRow());
	if (oldW > maxW) {
		const int topRow    = getTopActualRow();
		mAdjustingScrollBar = true;
		cacheModel->setCacheWindowStartRow(maxW);
		const int newW = cacheModel->getCacheWindowStartRow();
		syncViewStateAfterWindowShift(oldW - newW);
		vbar->setValue(qBound(0, topRow - newW, vbar->maximum()));
		mAdjustingScrollBar = false;
	}
	// 3) 视口首行越过数据末尾 → 贴到最后一屏
	const int totalRows = cacheModel->actualRowCount();
	const int topRow    = getTopActualRow();
	if (totalRows > 0 && topRow > totalRows - 1) {
		placeViewportAt(totalRows - 1);
		return;
	}
	syncScrollBarState();
}

/**
 * @brief 同步滚动条的整表坐标信息（滑块位置、总行数、一屏行数）
 */
void DACacheWindowTableView::syncScrollBarState(){
	if (!mVirtualVBar) {
		return;
	}
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		mVirtualVBar->setDatasetPosition(0, 0, 1);
		return;
	}
	mVirtualVBar->setDatasetPosition(getTopActualRow(), cacheModel->actualRowCount(), getViewportRowCount());
}

/**
 * @brief 提交并关闭正在编辑的编辑器
 * @return 当前没有未提交的编辑返回 true
 */
bool DACacheWindowTableView::commitOpenEditor()
{
	if (state() != QAbstractItemView::EditingState) {
		return true;
	}
	QWidget* editor = currentEditor();
	if (!editor) {
		return false;  // 找不到编辑器就别搬窗，等它关掉
	}
	closeEditor(editor, QAbstractItemDelegate::SubmitModelCache);
	return state() != QAbstractItemView::EditingState;
}

/**
 * @brief 取当前编辑器部件（编辑器是 viewport 的子部件，且打开时获得焦点）
 * @return 编辑器部件，取不到返回 nullptr
 */
QWidget* DACacheWindowTableView::currentEditor() const
{
	QWidget* focusWidget = QApplication::focusWidget();
	if (focusWidget && viewport()->isAncestorOf(focusWidget)) {
		return focusWidget;
	}
	const auto children = viewport()->findChildren< QWidget* >(QString(), Qt::FindDirectChildrenOnly);
	for (QWidget* w : children) {
		if (w->isVisible() && w->hasFocus()) {
			return w;
		}
	}
	return nullptr;
}

/**
 * @brief 把光标移到指定绝对行/列（不改变视口的纵向位置，必要时才滚动）
 * @param actualRow 绝对行号
 * @param actualColumn 列号
 */
void DACacheWindowTableView::setCurrentActualCell(int actualRow, int actualColumn)
{
	if (!model() || !selectionModel()) {
		return;
	}
	const int logicalRow = toLogicalRow(actualRow);
	if (logicalRow < 0 || logicalRow >= model()->rowCount()) {
		return;
	}
	if (actualColumn < 0 || actualColumn >= model()->columnCount()) {
		return;
	}
	const QModelIndex index = model()->index(logicalRow, actualColumn);
	selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
	scrollTo(index, QAbstractItemView::EnsureVisible);
}

/**
 * @brief 「定位到行」输入
 */
void DACacheWindowTableView::promptGotoRow()
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel || cacheModel->actualRowCount() <= 0) {
		return;
	}
	const int totalRows = cacheModel->actualRowCount();
	bool ok             = false;
	const int row       = QInputDialog::getInt(this,
                                         tr("Go To Row"),                             // cn:定位到行
                                         tr("Row number (1 - %1):").arg(totalRows),   // cn:行号（1 - %1）：
                                         getTopActualRow() + 1,
                                         1,
                                         totalRows,
                                         1,
                                         &ok);
	if (!ok) {
		return;
	}
	selectActualCell(row - 1, qMax(0, currentIndex().column()));
}
}
