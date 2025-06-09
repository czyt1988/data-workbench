#include "DACacheWindowTableView.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QElapsedTimer>
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
	if (QScrollBar* vsc = verticalScrollBar()) {
		connect(vsc, &QScrollBar::valueChanged, this, &DACacheWindowTableView::verticalScrollBarValueChanged);
	}
}

DACacheWindowTableView::~DACacheWindowTableView()
{
}

DAAbstractCacheWindowTableModel* DACacheWindowTableView::getCacheModel() const
{
	return qobject_cast< DAAbstractCacheWindowTableModel* >(model());
}

void DACacheWindowTableView::showActualRow(int actualRow)
{
	// 首先计算行的占比
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return;
	}
	const int totalRows = cacheModel->actualRowCount();
	if (actualRow >= totalRows) {
		actualRow = totalRows - 1;
	}
	if (totalRows < 0) {
		actualRow = 0;
	}
	const int maxScroll         = qMax(0, totalRows);
	const double ratio          = static_cast< double >(actualRow) / (totalRows);
	const int targetScrollValue = qMin(static_cast< int >(ratio * maxScroll), maxScroll);
	if (QScrollBar* vsc = verticalScrollBar()) {
		QSignalBlocker b(vsc);
		vsc->setValue(targetScrollValue);
	}
	// 设置startRow
	cacheModel->setCacheWindowStartRow(actualRow);
}

void DACacheWindowTableView::selectActualCell(int actualRow, int col)
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return;
	}
	// 先把真实行显示出来再进行高亮
	showActualRow(actualRow);
	showColumn(col);

	QItemSelection selection;
	int logicalRow = toLogicalRow(actualRow);
	if (logicalRow < 0 || logicalRow >= cacheModel->rowCount()) {
		// 超出范围
		return;
	}
	QModelIndex index = cacheModel->index(logicalRow, col);
	selection.select(index, index);

	// 高亮选中项
	QItemSelectionModel* selModel = selectionModel();
	if (selModel) {
		selModel->clearSelection();
		selModel->select(selection, QItemSelectionModel::Select);
	}
}

/**
 * @brief 转换为逻辑行
 * @param actualRow
 * @return
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
 * @brief 判断当前的真实行是否再视图的可见范围内
 * @param actualRow
 * @return
 */
bool DACacheWindowTableView::isActualRowInViewRange(int actualRow) const
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return false;
	}
	int row = toLogicalRow(actualRow);
	if (row < 0) {
		return false;
	}
	if (row >= cacheModel->rowCount()) {
		return false;
	}
	return true;
}

/**
 * @brief 返回真实行对应的表头名
 * @param actualRow
 * @return
 */
QString DACacheWindowTableView::actualRowName(int actualRow) const
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return model()->headerData(actualRow, Qt::Vertical).toString();
	}
	return cacheModel->actualHeaderData(actualRow, Qt::Vertical).toString();
}

/**
 * @brief 返回真实列对应的表头名
 * @param actualCol
 * @return
 */
QString DACacheWindowTableView::actualColumnName(int actualCol) const
{
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return model()->headerData(actualCol, Qt::Horizontal).toString();
	}
	return cacheModel->headerData(actualCol, Qt::Horizontal).toString();
}

void DACacheWindowTableView::verticalScrollBarValueChanged(int v)
{
	static QElapsedTimer s_elasped;
	if (s_elasped.elapsed() < 50) {
		return;  // 50ms内的重复变化只处理一次,防止抖动
	}
	s_elasped.start();
	DAAbstractCacheWindowTableModel* cacheModel = getCacheModel();
	if (!cacheModel) {
		return;
	}
	// 计算滚动比例
	QScrollBar* vsc       = verticalScrollBar();
	const int totalRows   = cacheModel->actualRowCount();
	const int maxScroll   = qMax(0, totalRows);
	const double ratio    = static_cast< double >(v) / (vsc->maximum() - vsc->minimum());
	const int targetStart = qMin(static_cast< int >(ratio * maxScroll), maxScroll);
	cacheModel->setCacheWindowStartRow(targetStart);
}
}
