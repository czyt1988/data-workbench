#include "DAWorkFlowOperateWidget.h"
#include "ui_DAWorkFlowOperateWidget.h"
// qt
#include <QAction>
#include <QImage>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QUndoStack>
// workflow
#include "DAWorkFlowGraphicsView.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DAGraphicsPixmapItem.h"
//
#include "DAWorkFlowEditWidget.h"
#include "DACommandsForWorkFlowNodeGraphics.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAWorkFlowOperateWidget
//===================================================
DAWorkFlowOperateWidget::DAWorkFlowOperateWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAWorkFlowOperateWidget), mIsShowGrid(true), mDefaultTextColor(Qt::black)
{
	mIsDestorying = false;
	ui->setupUi(this);
	initActions();
	connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DAWorkFlowOperateWidget::onTabWidgetCurrentChanged);
	connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &DAWorkFlowOperateWidget::onTabWidgetTabCloseRequested);
}

DAWorkFlowOperateWidget::~DAWorkFlowOperateWidget()
{
	qDebug() << "DAWorkFlowOperateWidget begin delete ui";
	mIsDestorying = true;
	delete ui;
	qDebug() << "DAWorkFlowOperateWidget end delete ui";
}

/**
 * @brief 创建工作流，创建完后通过getWorkflow获取
 *
 * 如果对DAWorkFlow如果有继承，那么重载此函数创建自己的workflow就行
 *
 * 此函数会在@ref appendWorkflow 中调用
 * @return
 */
DAWorkFlow* DAWorkFlowOperateWidget::createWorkflow()
{
    return (new DAWorkFlow());
}

/**
 * @brief 添加一个工作流编辑窗口
 *
 * 此函数发射信号workflowCreated（先），也会触发currentWorkFlowWidgetChanged（后）
 * @param wfe
 */
DAWorkFlowEditWidget* DAWorkFlowOperateWidget::appendWorkflow(const QString& name)
{
	if (isOnlyOneWorkflow()) {
		if (ui->tabWidget->count() >= 1) {
			return nullptr;
		}
	}
	DAWorkFlowEditWidget* wfe = new DAWorkFlowEditWidget(ui->tabWidget);
	DAWorkFlow* wf            = createWorkflow();
	wf->setParent(wfe);
	wfe->setWorkFlow(wf);
	// 把undo添加进去
	wfe->setEnableShowGrid(mIsShowGrid);
	wfe->setDefaultTextColor(mDefaultTextColor);
	wfe->setDefaultTextFont(mDefaultFont);
	DAWorkFlowGraphicsScene* scene = wfe->getWorkFlowGraphicsScene();
	connect(wfe, &DAWorkFlowEditWidget::selectNodeItemChanged, this, &DAWorkFlowOperateWidget::selectNodeItemChanged);
	connect(wfe, &DAWorkFlowEditWidget::mouseActionFinished, this, &DAWorkFlowOperateWidget::mouseActionFinished);
	connect(scene, &DAWorkFlowGraphicsScene::selectionChanged, this, &DAWorkFlowOperateWidget::onSelectionChanged);
	connect(wfe, &DAWorkFlowEditWidget::startExecute, this, [ this, wfe ]() { emit workflowStartExecute(wfe); });
	connect(wfe,
			&DAWorkFlowEditWidget::nodeExecuteFinished,
			this,
			[ this, wfe ](DAAbstractNode::SharedPointer n, bool state) { emit nodeExecuteFinished(wfe, n, state); });
	connect(wfe, &DAWorkFlowEditWidget::finished, this, [ this, wfe ](bool s) { emit workflowFinished(wfe, s); });
	ui->tabWidget->addTab(wfe, name);
	// 把名字保存到DAWorkFlowEditWidget中，在DAProject保存的时候会用到
	wfe->setWindowTitle(name);
	emit workflowCreated(wfe);
	ui->tabWidget->setCurrentIndex(ui->tabWidget->indexOf(wfe));
	return wfe;
}

/**
 * @brief 创建一个新的工作流窗口
 * @note 此函数带有交互
 * @return
 */
DAWorkFlowEditWidget* DAWorkFlowOperateWidget::appendWorkflowWithDialog()
{
	bool ok = false;
	QString text = QInputDialog::getText(this, tr("Title of new workflow"), tr("Title:"), QLineEdit::Normal, QString(), &ok);
	if (!ok || text.isEmpty()) {
		return nullptr;
	}
	return appendWorkflow(text);
}

/**
 * @brief 获取当前工作流的索引
 * @return
 */
int DAWorkFlowOperateWidget::getCurrentWorkflowIndex() const
{
    return ui->tabWidget->currentIndex();
}

/**
 * @brief 设置当前的工作流
 * @param index
 */
void DAWorkFlowOperateWidget::setCurrentWorkflow(int index)
{
    ui->tabWidget->setCurrentIndex(index);
}

/**
 * @brief 设置当前的页面
 * @param wf
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowWidget(DAWorkFlowEditWidget* wf)
{
    ui->tabWidget->setCurrentWidget(wf);
}

/**
 * @brief 获取当前选中的工作流窗口
 * @return
 */
DAWorkFlowEditWidget* DAWorkFlowOperateWidget::getCurrentWorkFlowWidget() const
{
	QWidget* w = ui->tabWidget->currentWidget();
	if (nullptr == w) {
		return nullptr;
	}
	return qobject_cast< DAWorkFlowEditWidget* >(w);
}

void DAWorkFlowOperateWidget::setCurrentWorkflowName(const QString& name)
{
	int i = getCurrentWorkflowIndex();
	renameWorkFlowWidget(i, name);
}

/**
 * @brief 获取所有的工作流编辑窗口
 * @return
 */
QList< DAWorkFlowEditWidget* > DAWorkFlowOperateWidget::getAllWorkFlowWidgets() const
{
	QList< DAWorkFlowEditWidget* > res;
	for (int i = 0; i < ui->tabWidget->count(); ++i) {
		auto w = qobject_cast< DAWorkFlowEditWidget* >(ui->tabWidget->widget(i));
		res.append(w);
	}
	return res;
}

/**
 * @brief 获取scene
 * @return
 */
DAWorkFlowGraphicsScene* DAWorkFlowOperateWidget::getCurrentWorkFlowScene() const
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		return nullptr;
	}
	return w->getWorkFlowGraphicsScene();
}

/**
 * @brief 获取所有的工作流窗口
 * @return
 */
QList< DAWorkFlowGraphicsScene* > DAWorkFlowOperateWidget::getAllWorkFlowScene() const
{
	QList< DAWorkFlowGraphicsScene* > res;
	int c = ui->tabWidget->count();
	for (int i = 0; i < c; ++i) {
		DAWorkFlowEditWidget* we = qobject_cast< DAWorkFlowEditWidget* >(ui->tabWidget->widget(i));
		if (we) {
			DAWorkFlowGraphicsScene* sc = we->getWorkFlowGraphicsScene();
			if (sc) {
				res.append(sc);
			}
		}
	}
	return res;
}

/**
 * @brief 获取当前视图
 * @return
 */
DAWorkFlowGraphicsView* DAWorkFlowOperateWidget::getCurrentWorkFlowView() const
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		return nullptr;
	}
	return w->getWorkFlowGraphicsView();
}

/**
 * @brief 获取工作流窗口
 * @param index
 * @return 如果超出索引范围返回nullptr
 */
DAWorkFlowEditWidget* DAWorkFlowOperateWidget::getWorkFlowWidget(int index) const
{
    return qobject_cast< DAWorkFlowEditWidget* >(ui->tabWidget->widget(index));
}

/**
 * @brief 获取工作流窗口的名称
 * @param index
 * @return
 */
QString DAWorkFlowOperateWidget::getWorkFlowWidgetName(int index) const
{
    return ui->tabWidget->tabText(index);
}

/**
 * @brief 给工作流重命名
 * @param index
 * @param name
 */
void DAWorkFlowOperateWidget::renameWorkFlowWidget(int index, const QString& name)
{
    ui->tabWidget->setTabText(index, name);
}

/**
 * @brief 获取编辑窗口数量
 * @return
 */
int DAWorkFlowOperateWidget::count() const
{
    return ui->tabWidget->count();
}

/**
 * @brief 移除工作流
 * @param index
 */
void DAWorkFlowOperateWidget::removeWorkflow(int index)
{
	QWidget* w = ui->tabWidget->widget(index);
	if (nullptr == w) {
		return;
	}
	QMessageBox::StandardButton btn = QMessageBox::question(this,
															tr("question"),  // 疑问
															tr("Confirm to delete workflow:%1")
																.arg(getWorkFlowWidgetName(index))  // 是否确认删除工作流:%1
	);
	if (btn != QMessageBox::Yes) {
		return;
	}
	// 发射移除信号
	emit workflowRemoving(qobject_cast< DA::DAWorkFlowEditWidget* >(w));
	ui->tabWidget->removeTab(index);
	w->hide();
	w->deleteLater();
}

/**
 * @brief 激活当前的回退功能
 */
void DAWorkFlowOperateWidget::setUndoStackActive()
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (w) {
		w->setUndoStackActive();
	}
}

/**
 * @brief 设置显示grid
 *
 * 此函数影响所有工作流编辑窗口
 * @param on
 */
void DAWorkFlowOperateWidget::setEnableShowGrid(bool on)
{
	mIsShowGrid = on;
	const int c = count();
	for (int i = 0; i < c; ++i) {
		DAWorkFlowEditWidget* w = getWorkFlowWidget(i);
		if (w == nullptr) {
			continue;
		}
		w->setEnableShowGrid(mIsShowGrid);
	}
}

/**
 * @brief 是否显示网格
 * @return
 */
bool DAWorkFlowOperateWidget::isEnableShowGrid() const
{
	return mIsShowGrid;
}

/**
 * @brief 获取undostack
 * @return
 */
QUndoStack* DAWorkFlowOperateWidget::getUndoStack()
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (w) {
		return w->getUndoStack();
	}
	return nullptr;
}

void DAWorkFlowOperateWidget::addBackgroundPixmap(const QString& pixmapPath)
{
	DAWorkFlowGraphicsScene* s = getCurrentWorkFlowScene();
	if (nullptr == s) {
		return;
	}
	QImage img(pixmapPath);
	QPixmap px;
	px.convertFromImage(img);
	DAGraphicsPixmapItem* item = s->setBackgroundPixmap(px);
	item->setSelectable(true);
	item->setMoveable(true);
}

void DAWorkFlowOperateWidget::setBackgroundPixmapLock(bool on)
{
	DAWorkFlowGraphicsScene* s = getCurrentWorkFlowScene();
	if (nullptr == s) {
		return;
	}
	DAGraphicsPixmapItem* item = s->getBackgroundPixmapItem();
	if (nullptr == item) {
		return;
	}
	item->setSelectable(!on);
	item->setMoveable(!on);
}

void DAWorkFlowOperateWidget::setSelectTextColor(const QColor& color)
{
	DAWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
	if (ww) {
		ww->setSelectTextColor(color);
	}
}

void DAWorkFlowOperateWidget::setSelectShapeBackgroundBrush(const QBrush& b)
{
	DAWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
	if (ww) {
		ww->setSelectShapeBackgroundBrush(b);
	}
}

void DAWorkFlowOperateWidget::setSelectShapeBorderPen(const QPen& v)
{
	DAWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
	if (ww) {
		ww->setSelectShapeBorderPen(v);
	}
}

void DAWorkFlowOperateWidget::setSelectTextFont(const QFont& f)
{
	DAWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
	if (ww) {
		ww->setSelectTextItemFont(f);
	}
}

/**
 * @brief 设置当前工作流的网格显示与否
 * @param on
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowShowGrid(bool on)
{
	DAWorkFlowGraphicsScene* secen = getCurrentWorkFlowScene();
	if (nullptr == secen) {
		return;
	}
	secen->showGridLine(on);
	secen->update();
}

/**
 * @brief 设置当前工作流全部显示
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowWholeView()
{
	DAWorkFlowGraphicsView* view = getCurrentWorkFlowView();
	if (!view) {
		qWarning() << tr("Loss View");  // cn:缺少视图
		return;
	}
	view->setWholeView();
}

/**
 * @brief 放大
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowZoomIn()
{
	DAWorkFlowGraphicsView* view = getCurrentWorkFlowView();
	if (!view) {
		qWarning() << tr("Loss View");  // cn:缺少视图
		return;
	}
	qDebug() << "zoomIn";
	view->zoomIn();
}

/**
 * @brief 缩小
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowZoomOut()
{
	DAWorkFlowGraphicsView* view = getCurrentWorkFlowView();
	if (!view) {
		qWarning() << tr("Loss View");  // cn:缺少视图
		return;
	}
	qDebug() << "zoomOut";
	view->zoomOut();
}

/**
 * @brief 全选
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowSelectAll()
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		qWarning() << tr("No active workflow detected");  // 未检测到激活的工作流
		return;
	}
	w->selectAll();
}

/**
 * @brief 运行工作流
 */
void DAWorkFlowOperateWidget::runCurrentWorkFlow()
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		qWarning() << tr("No active workflow detected");  // 未检测到激活的工作流
		return;
	}
	DAWorkFlow* wf = w->getWorkflow();
	if (nullptr == wf) {
		qCritical() << tr("Unable to get workflow correctly");  // 无法正确获取工作流
		return;
	}
	wf->exec();
}

/**
 * @brief 终止当前工作流
 */
void DAWorkFlowOperateWidget::terminateCurrentWorkFlow()
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		qWarning() << tr("No active workflow detected");  // 未检测到激活的工作流
		return;
	}
	DAWorkFlow* wf = w->getWorkflow();
	if (nullptr == wf) {
		qCritical() << tr("Unable to get workflow correctly");  // 无法正确获取工作流
		return;
	}
	wf->terminate();
}

/**
 * @brief 复制当前选中的items
 */
void DAWorkFlowOperateWidget::copyCurrentSelectItems()
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		qWarning() << tr("No active workflow detected");  // 未检测到激活的工作流
		return;
	}
	w->copySelectItems();
}

/**
 * @brief 剪切当前选中的items
 */
void DAWorkFlowOperateWidget::cutCurrentSelectItems()
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		qWarning() << tr("No active workflow detected");  // 未检测到激活的工作流
		return;
	}
	w->cutSelectItems();
}

/**
 * @brief ctrl+v动作
 */
void DAWorkFlowOperateWidget::pasteFromClipBoard()
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		qWarning() << tr("No active workflow detected");  // 未检测到激活的工作流
		return;
	}
	w->paste(DAWorkFlowEditWidget::PaseteRangeCenterToViewCenter);
}

/**
 * @brief 删除当前的item
 */
void DAWorkFlowOperateWidget::removeCurrentSelectItems()
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		qWarning() << tr("No active workflow detected");  // 未检测到激活的工作流
		return;
	}
	w->removeSelectItems();
}

/**
 * @brief 当前的wf执行取消动作
 */
void DAWorkFlowOperateWidget::cancelCurrent()
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		qWarning() << tr("No active workflow detected");  // 未检测到激活的工作流
		return;
	}
	w->cancel();
}

/**
 * @brief 文本字体
 * @param c
 */
QFont DAWorkFlowOperateWidget::getDefaultTextFont() const
{
	return mDefaultFont;
}
/**
 * @brief 设置文本字体
 * @param c
 */
void DAWorkFlowOperateWidget::setDefaultTextFont(const QFont& f)
{
	mDefaultFont                             = f;
	QList< DAWorkFlowGraphicsScene* > secens = getAllWorkFlowScene();
	for (DAWorkFlowGraphicsScene* sc : qAsConst(secens)) {
		sc->setDefaultTextFont(f);
	}
}
/**
 * @brief 文本颜色
 * @param c
 */
QColor DAWorkFlowOperateWidget::getDefaultTextColor() const
{
	return mDefaultTextColor;
}
/**
 * @brief 设置默认的文本颜色
 * @param c
 */
void DAWorkFlowOperateWidget::setDefaultTextColor(const QColor& c)
{
	mDefaultTextColor                        = c;
	QList< DAWorkFlowGraphicsScene* > secens = getAllWorkFlowScene();
	for (DAWorkFlowGraphicsScene* sc : secens) {
		sc->setDefaultTextColor(c);
	}
}

/**
 * @brief tab窗口发送了变化
 * @param index
 */
void DAWorkFlowOperateWidget::onTabWidgetCurrentChanged(int index)
{
	DAWorkFlowEditWidget* w = getWorkFlowWidget(index);
	if (nullptr == w) {
		return;
	}
	emit currentWorkFlowWidgetChanged(w);
}

/**
 * @brief 请求关闭
 * @param index
 */
void DAWorkFlowOperateWidget::onTabWidgetTabCloseRequested(int index)
{
    removeWorkflow(index);
}

/**
 * @brief 场景条目选择变化触发的槽
 */
void DAWorkFlowOperateWidget::onSelectionChanged()
{
	if (mIsDestorying) {
		//! 很奇怪，DAWorkFlowGraphicsScene已经析构了，但此槽函数还是能调用，在DAWorkFlowOperateWidget
		//! 开始delete ui的时候，先析构DAWorkFlowGraphicsView，再析构DAWorkFlowGraphicsScene
		//! 然后就会调用此槽函数，这时导致错误，从qt原理上，在析构时应该会把槽函数都断开连接才合理
		return;
	}
	DAWorkFlowGraphicsScene* scene = getCurrentWorkFlowScene();
	if (nullptr == scene) {
		return;
	}
	QList< QGraphicsItem* > sits = scene->selectedItems();
	if (sits.isEmpty()) {
		return;
	}
	emit selectionItemChanged(sits.last());
}

QList< DAGraphicsStandardTextItem* > DAWorkFlowOperateWidget::getSelectTextItems()
{
	QList< DAGraphicsStandardTextItem* > res;
	DAWorkFlowGraphicsScene* secen = getCurrentWorkFlowScene();
	if (nullptr == secen) {
		return res;
	}
	QList< QGraphicsItem* > its = secen->selectedItems();
	if (its.size() == 0) {
		return res;
	}
	for (QGraphicsItem* item : qAsConst(its)) {
		if (DAGraphicsStandardTextItem* textItem = dynamic_cast< DAGraphicsStandardTextItem* >(item)) {
			res.append(textItem);
		}
	}
	return res;
}

void DAWorkFlowOperateWidget::initActions()
{
	mActionCopy = new QAction(this);
	mActionCopy->setObjectName(QStringLiteral("actionCopyToDAWorkFlowOperateWidget"));
	mActionCopy->setIcon(QIcon(QStringLiteral(":/DAGui/icon/copy.svg")));
	mActionCopy->setShortcuts(QKeySequence::Copy);
	connect(mActionCopy, &QAction::triggered, this, &DAWorkFlowOperateWidget::copyCurrentSelectItems);

	mActionCut = new QAction(this);
	mActionCut->setObjectName(QStringLiteral("actionCutToDAWorkFlowOperateWidget"));
	mActionCut->setIcon(QIcon(QStringLiteral(":/DAGui/icon/cut.svg")));
	mActionCut->setShortcuts(QKeySequence::Cut);
	connect(mActionCut, &QAction::triggered, this, &DAWorkFlowOperateWidget::cutCurrentSelectItems);

	mActionPaste = new QAction(this);
	mActionPaste->setObjectName(QStringLiteral("actionPasteToDAWorkFlowOperateWidget"));
	mActionPaste->setIcon(QIcon(QStringLiteral(":/DAGui/icon/paste.svg")));
	mActionPaste->setShortcuts(QKeySequence::Paste);
	connect(mActionPaste, &QAction::triggered, this, &DAWorkFlowOperateWidget::pasteFromClipBoard);

	mActionDelete = new QAction(this);
	mActionDelete->setObjectName(QStringLiteral("actionDeleteToDAWorkFlowOperateWidget"));
	mActionDelete->setIcon(QIcon(QStringLiteral(":/DAGui/icon/delete.svg")));
	mActionDelete->setShortcuts(QKeySequence::Delete);
	connect(mActionDelete, &QAction::triggered, this, &DAWorkFlowOperateWidget::removeCurrentSelectItems);

	mActionCancel = new QAction(this);
	mActionCancel->setObjectName(QStringLiteral("actionCancelToDAWorkFlowOperateWidget"));
	mActionCancel->setIcon(QIcon(QStringLiteral(":/DAGui/icon/cancel.svg")));
	mActionCancel->setShortcuts(QKeySequence::Cancel);
	connect(mActionCancel, &QAction::triggered, this, &DAWorkFlowOperateWidget::cancelCurrent);

	mActionSelectAll = new QAction(this);
	mActionSelectAll->setObjectName(QStringLiteral("actionSelectAllToDAWorkFlowOperateWidget"));
	mActionSelectAll->setIcon(QIcon(QStringLiteral(":/DAGui/icon/select-all.svg")));
	mActionSelectAll->setShortcuts(QKeySequence::SelectAll);
	connect(mActionSelectAll, &QAction::triggered, this, &DAWorkFlowOperateWidget::setCurrentWorkflowSelectAll);

	mActionZoomIn = new QAction(this);
	mActionZoomIn->setObjectName(QStringLiteral("actionZoomInToDAWorkFlowOperateWidget"));
	mActionZoomIn->setIcon(QIcon(QStringLiteral(":/DAGui/icon/zoomIn.svg")));
	mActionZoomIn->setShortcuts({ QKeySequence(QKeySequence::ZoomIn), QKeySequence(QStringLiteral("CTRL+=")) });
	connect(mActionZoomIn, &QAction::triggered, this, &DAWorkFlowOperateWidget::setCurrentWorkflowZoomIn);

	mActionZoomOut = new QAction(this);
	mActionZoomOut->setObjectName(QStringLiteral("actionZoomOutToDAWorkFlowOperateWidget"));
	mActionZoomOut->setIcon(QIcon(QStringLiteral(":/DAGui/icon/zoomOut.svg")));
	mActionZoomOut->setShortcuts(QKeySequence::ZoomOut);
	connect(mActionZoomOut, &QAction::triggered, this, &DAWorkFlowOperateWidget::setCurrentWorkflowZoomOut);

	// 缩放到适合屏幕
	mActionZoomFit = new QAction(this);
	mActionZoomFit->setObjectName(QStringLiteral("actionZoomFullToDAWorkFlowOperateWidget"));
	mActionZoomFit->setIcon(QIcon(QStringLiteral(":/DAGui/icon/viewAll.svg")));
	mActionZoomFit->setShortcut(QKeySequence(QStringLiteral("CTRL+0")));
	connect(mActionZoomFit, &QAction::triggered, this, &DAWorkFlowOperateWidget::setCurrentWorkflowWholeView);

	addAction(mActionCopy);
	addAction(mActionCut);
	addAction(mActionPaste);
	addAction(mActionDelete);
	addAction(mActionCancel);
	addAction(mActionSelectAll);
	addAction(mActionZoomIn);
	addAction(mActionZoomOut);
	addAction(mActionZoomFit);
	retranslateUi();
}

void DAWorkFlowOperateWidget::retranslateUi()
{
	mActionCopy->setText(tr("Copy"));                             // cn:复制
	mActionCopy->setStatusTip(tr("Copy"));                        // cn:复制
	mActionCut->setText(tr("Cut"));                               // cn:剪切
	mActionCut->setStatusTip(tr("Cut"));                          // cn:剪切
	mActionPaste->setText(tr("Paste"));                           // cn:粘贴
	mActionPaste->setStatusTip(tr("Paste"));                      // cn:粘贴
	mActionDelete->setText(tr("Delete"));                         // cn:删除
	mActionDelete->setStatusTip(tr("Delete"));                    // cn:删除
	mActionCancel->setText(tr("Cancel"));                         // cn:取消
	mActionCancel->setStatusTip(tr("Cancel"));                    // cn:取消
	mActionSelectAll->setText(tr("Select All"));                  // cn:全选
	mActionSelectAll->setStatusTip(tr("Select all items"));       // cn:全选所有图元
	mActionZoomIn->setText(tr("Zoom In"));                        // cn:放大
	mActionZoomIn->setStatusTip(tr("Zoom in graphics view"));     // cn:放大画布
	mActionZoomOut->setText(tr("Zoom Out"));                      // cn:缩小
	mActionZoomOut->setStatusTip(tr("Zoom Out graphics view"));   // cn:缩小画布
	mActionZoomFit->setText(tr("Zoom to Fit"));                   // cn:适合屏幕
	mActionZoomFit->setStatusTip(tr("Zoom to fit screen size"));  // cn:缩放到适合屏幕大小
}

bool DAWorkFlowOperateWidget::isOnlyOneWorkflow() const
{
	return mOnlyOneWorkflow;
}

void DAWorkFlowOperateWidget::setOnlyOneWorkflow(bool v)
{
    mOnlyOneWorkflow = v;
}

/**
 * @brief 获取窗口内置的action，一般这个函数用来把action设置到工具栏或者菜单中
 * @param act
 * @return
 */
QAction* DAWorkFlowOperateWidget::getInnerAction(DAWorkFlowOperateWidget::InnerActions act)
{
	switch (act) {
	case ActionCopy:
		return mActionCopy;
	case ActionCut:
		return mActionCut;
	case ActionPaste:
		return mActionPaste;
	case ActionDelete:
		return mActionDelete;
	case ActionCancel:
		return mActionCancel;
	case ActionSelectAll:
		return mActionSelectAll;
	case ActionZoomIn:
		return mActionZoomIn;
	case ActionZoomOut:
		return mActionZoomOut;
	case ActionZoomFit:
		return mActionZoomFit;
	default:
		break;
	}
	return nullptr;
}

/**
 * @brief 设置鼠标动作
 *
 * 一旦设置鼠标动作，鼠标点击后就会触发此动作，continuous来标记动作结束后继续保持还是还原为无动作
 * @param mf 鼠标动作
 * @param continuous 是否连续执行
 */
bool DAWorkFlowOperateWidget::setMouseActionFlag(DAWorkFlowGraphicsScene::MouseActionFlag mf, bool continous)
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		qWarning() << tr("No active workflow detected");  // 未检测到激活的工作流
		return false;
	}
	w->setMouseActionFlag(mf, continous);
	return true;
}

/**
 * @brief 清空
 * @note 此函数会发射@ref workflowClearing 信号
 */
void DAWorkFlowOperateWidget::clear()
{
	emit workflowClearing();
	int count = ui->tabWidget->count();
	QList< DAWorkFlowEditWidget* > wfes;
	for (int i = 0; i < count; ++i) {
		DAWorkFlowEditWidget* wfe = getWorkFlowWidget(i);
		wfes.append(wfe);
	}
	// 清空tab
	while (ui->tabWidget->count() > 0) {
		ui->tabWidget->removeTab(0);
	}
	// 清空
	for (DAWorkFlowEditWidget* w : wfes) {
		w->hide();
		w->deleteLater();
	}
}

/**
 * @brief 获取所有工作流的名字
 * @return
 */
QList< QString > DAWorkFlowOperateWidget::getAllWorkflowNames() const
{
	QList< QString > names;
	int c = ui->tabWidget->count();
	for (int i = 0; i < c; ++i) {
		names.append(ui->tabWidget->tabText(i));
	}
	return names;
}

}
