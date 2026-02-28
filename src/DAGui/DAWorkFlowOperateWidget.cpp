#include "DAWorkFlowOperateWidget.h"
#include "ui_DAWorkFlowOperateWidget.h"
// qt
#include <QAction>
#include <QActionGroup>
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

namespace DA
{

class DAWorkFlowOperateWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAWorkFlowOperateWidget)
public:
	PrivateData(DAWorkFlowOperateWidget* p);

	bool mIsShowGrid { true };
	QColor mDefaultTextColor { Qt::black };
	QFont mDefaultFont;
	bool mIsDestorying { false };
	bool mOnlyOneWorkflow { false };    ///< 设置只允许一个工作流
	bool mEnableWorkflowLink { true };  ///< 是否允许工作流连接
	QAction* mActionCopy { nullptr };
	QAction* mActionCut { nullptr };
	QAction* mActionPaste { nullptr };
	QAction* mActionDelete { nullptr };              ///< 删除选中
	QAction* mActionCancel { nullptr };              ///< 取消动作
	QAction* mActionSelectAll { nullptr };           ///< 全选
	QAction* mActionZoomIn { nullptr };              ///< 放大
	QAction* mActionZoomOut { nullptr };             ///< 缩小
	QAction* mActionZoomFit { nullptr };             ///< 全部显示
	QAction* actionViewCrossLineMarker { nullptr };  ///< 视图的十字标记线
	QAction* actionViewHLineMarker { nullptr };      ///< 视图的水平标记线
	QAction* actionViewVLineMarker { nullptr };      ///< 视图的垂直标记线
	QAction* actionViewNoneMarker { nullptr };       ///< 无标记线
	QActionGroup* actionGroupViewLineMarkers { nullptr };
};

DAWorkFlowOperateWidget::PrivateData::PrivateData(DAWorkFlowOperateWidget* p) : q_ptr(p)
{
}

//===================================================
// DAWorkFlowOperateWidget
//===================================================
DAWorkFlowOperateWidget::DAWorkFlowOperateWidget(QWidget* parent)
	: DAAbstractOperateWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAWorkFlowOperateWidget)
{
	ui->setupUi(this);
	initActions();
	connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DAWorkFlowOperateWidget::onTabWidgetCurrentChanged);
	connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &DAWorkFlowOperateWidget::onTabWidgetTabCloseRequested);
}

DAWorkFlowOperateWidget::~DAWorkFlowOperateWidget()
{
	qDebug() << "DAWorkFlowOperateWidget begin delete ui";
	d_ptr->mIsDestorying = true;
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
	DA_D(d);
	DAWorkFlowEditWidget* wfe = new DAWorkFlowEditWidget(ui->tabWidget);
	DAWorkFlow* wf            = createWorkflow();
	wf->setParent(wfe);
	wfe->setWorkFlow(wf);
	// 把undo添加进去
	wfe->setEnableShowGrid(d->mIsShowGrid);
	wfe->setDefaultTextColor(d->mDefaultTextColor);
	wfe->setDefaultTextFont(d->mDefaultFont);
	DAWorkFlowGraphicsScene* scene = wfe->getWorkFlowGraphicsScene();
	// 同步状态
	scene->setIgnoreLinkEvent(!isEnableWorkflowLink());

	connect(wfe, &DAWorkFlowEditWidget::selectNodeItemChanged, this, &DAWorkFlowOperateWidget::selectNodeItemChanged);
	connect(wfe, &DAWorkFlowEditWidget::sceneActionActived, this, &DAWorkFlowOperateWidget::sceneActionActived);
	connect(wfe, &DAWorkFlowEditWidget::sceneActionDeactived, this, &DAWorkFlowOperateWidget::sceneActionDeactived);
	connect(scene, &DAWorkFlowGraphicsScene::selectionChanged, this, &DAWorkFlowOperateWidget::onSelectionChanged);
	connect(scene, &DAWorkFlowGraphicsScene::itemsAdded, this, &DAWorkFlowOperateWidget::onSceneItemsAdded);
	connect(scene, &DAWorkFlowGraphicsScene::itemsRemoved, this, &DAWorkFlowOperateWidget::onSceneItemsRemoved);
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
 * @brief 获取当前的工作流
 * @return
 */
DAWorkFlow* DAWorkFlowOperateWidget::getCurrentWorkflow() const
{
	if (auto w = getCurrentWorkFlowWidget()) {
		return w->getWorkflow();
	}
	return nullptr;
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
	QMessageBox::StandardButton btn = QMessageBox::question(
		this,
		tr("question"),                                                        // 疑问
		tr("Confirm to delete workflow:%1").arg(getWorkFlowWidgetName(index))  // 是否确认删除工作流:%1
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
 * @brief 是否显示网格
 * @return
 */
bool DAWorkFlowOperateWidget::isCurrentWorkflowShowGrid() const
{
	DAWorkFlowGraphicsScene* sc = getCurrentWorkFlowScene();
	if (!sc) {
		return false;
	}
	return sc->isShowGridLine();
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
	DAWorkFlowGraphicsScene* sc = getCurrentWorkFlowScene();
	if (!sc) {
		return;
	}
	sc->showGridLine(on);
	sc->update();
	d_ptr->mIsShowGrid = on;  // 记录最后的状态
}

/**
 * @brief 设置当前工作流锁定
 * @param on
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowReadOnly(bool on)
{
	DAWorkFlowGraphicsScene* sc = getCurrentWorkFlowScene();
	if (!sc) {
		return;
	}
	sc->setReadOnly(on);
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
	view->zoomFit();
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
	w->pasteToViewCenter();
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
 * @brief 设置是否允许连接
 * @param on
 */
void DAWorkFlowOperateWidget::setEnableWorkflowLink(bool on)
{
	d_ptr->mEnableWorkflowLink = on;
	iteratorScene([ on ](DAWorkFlowGraphicsScene* sc) -> bool {
		sc->setIgnoreLinkEvent(!on);
		return true;
	});
}

/**
 * @brief 是否允许连接
 * @return
 */
bool DAWorkFlowOperateWidget::isEnableWorkflowLink() const
{
	return d_ptr->mEnableWorkflowLink;
}

/**
 * @brief 文本字体
 * @param c
 */
QFont DAWorkFlowOperateWidget::getDefaultTextFont() const
{
	return d_ptr->mDefaultFont;
}
/**
 * @brief 设置文本字体
 * @param c
 */
void DAWorkFlowOperateWidget::setDefaultTextFont(const QFont& f)
{
	d_ptr->mDefaultFont = f;
	iteratorScene([ f ](DAWorkFlowGraphicsScene* sc) -> bool {
		sc->setDefaultTextFont(f);
		return true;
	});
}
/**
 * @brief 文本颜色
 * @param c
 */
QColor DAWorkFlowOperateWidget::getDefaultTextColor() const
{
	return d_ptr->mDefaultTextColor;
}
/**
 * @brief 设置默认的文本颜色
 * @param c
 */
void DAWorkFlowOperateWidget::setDefaultTextColor(const QColor& c)
{
	d_ptr->mDefaultTextColor = c;
	iteratorScene([ c ](DAWorkFlowGraphicsScene* sc) -> bool {
		sc->setDefaultTextColor(c);
		return true;
	});
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
	// 激活undostack
	auto un = w->getUndoStack();
	if (un) {
		if (!un->isActive()) {
			un->setActive(true);
		}
	}
	// 更新action的状态
	if (DAWorkFlowGraphicsView* view = w->getWorkFlowGraphicsView()) {
		auto markerStyle = view->getCurrentMarkerStyle();
		switch (markerStyle) {
		case DAGraphicsViewOverlayMouseMarker::CrossLine:
			d_ptr->actionViewCrossLineMarker->setChecked(true);
		case DAGraphicsViewOverlayMouseMarker::VLine:
			d_ptr->actionViewVLineMarker->setChecked(true);
		case DAGraphicsViewOverlayMouseMarker::HLine:
			d_ptr->actionViewHLineMarker->setChecked(true);
		case DAGraphicsViewOverlayMouseMarker::NoMarkerStyle:
			d_ptr->actionViewNoneMarker->setChecked(true);
		default:
			break;
		}
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
	if (d_ptr->mIsDestorying) {
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

void DAWorkFlowOperateWidget::onSceneItemsAdded(const QList< QGraphicsItem* >& its)
{
	DAGraphicsScene* sc = qobject_cast< DAGraphicsScene* >(sender());
	if (sc) {
		emit itemsAdded(sc, its);
	}
}

void DAWorkFlowOperateWidget::onSceneItemsRemoved(const QList< QGraphicsItem* >& its)
{
	DAGraphicsScene* sc = qobject_cast< DAGraphicsScene* >(sender());
	if (sc) {
		emit itemsRemoved(sc, its);
	}
}

void DAWorkFlowOperateWidget::onActionGroupViewLineMarkersTriggered(QAction* act)
{
	if (act == d_ptr->actionViewCrossLineMarker) {
		setCurrentViewLineMarker(act->isChecked() ? DAGraphicsViewOverlayMouseMarker::CrossLine
												  : DAGraphicsViewOverlayMouseMarker::NoMarkerStyle);
	} else if (act == d_ptr->actionViewHLineMarker) {
		setCurrentViewLineMarker(act->isChecked() ? DAGraphicsViewOverlayMouseMarker::HLine
												  : DAGraphicsViewOverlayMouseMarker::NoMarkerStyle);
	} else if (act == d_ptr->actionViewVLineMarker) {
		setCurrentViewLineMarker(act->isChecked() ? DAGraphicsViewOverlayMouseMarker::VLine
												  : DAGraphicsViewOverlayMouseMarker::NoMarkerStyle);
	} else if (act == d_ptr->actionViewNoneMarker) {
		setCurrentViewLineMarker(DAGraphicsViewOverlayMouseMarker::NoMarkerStyle);
	}
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
	for (QGraphicsItem* item : std::as_const(its)) {
		if (DAGraphicsStandardTextItem* textItem = dynamic_cast< DAGraphicsStandardTextItem* >(item)) {
			res.append(textItem);
		}
	}
	return res;
}

void DAWorkFlowOperateWidget::initActions()
{
	DA_D(d);

	d->mActionCopy = new QAction(this);
	d->mActionCopy->setObjectName(QStringLiteral("actionCopyToDAWorkFlowOperateWidget"));
	d->mActionCopy->setIcon(QIcon(QStringLiteral(":/DAGui/icon/copy.svg")));
	d->mActionCopy->setShortcuts(QKeySequence::Copy);
	connect(d->mActionCopy, &QAction::triggered, this, &DAWorkFlowOperateWidget::copyCurrentSelectItems);

	d->mActionCut = new QAction(this);
	d->mActionCut->setObjectName(QStringLiteral("actionCutToDAWorkFlowOperateWidget"));
	d->mActionCut->setIcon(QIcon(QStringLiteral(":/DAGui/icon/cut.svg")));
	d->mActionCut->setShortcuts(QKeySequence::Cut);
	connect(d->mActionCut, &QAction::triggered, this, &DAWorkFlowOperateWidget::cutCurrentSelectItems);

	d->mActionPaste = new QAction(this);
	d->mActionPaste->setObjectName(QStringLiteral("actionPasteToDAWorkFlowOperateWidget"));
	d->mActionPaste->setIcon(QIcon(QStringLiteral(":/DAGui/icon/paste.svg")));
	d->mActionPaste->setShortcuts(QKeySequence::Paste);
	connect(d->mActionPaste, &QAction::triggered, this, &DAWorkFlowOperateWidget::pasteFromClipBoard);

	d->mActionDelete = new QAction(this);
	d->mActionDelete->setObjectName(QStringLiteral("actionDeleteToDAWorkFlowOperateWidget"));
	d->mActionDelete->setIcon(QIcon(QStringLiteral(":/DAGui/icon/delete.svg")));
	d->mActionDelete->setShortcuts(QKeySequence::Delete);
	connect(d->mActionDelete, &QAction::triggered, this, &DAWorkFlowOperateWidget::removeCurrentSelectItems);

	d->mActionCancel = new QAction(this);
	d->mActionCancel->setObjectName(QStringLiteral("actionCancelToDAWorkFlowOperateWidget"));
	d->mActionCancel->setIcon(QIcon(QStringLiteral(":/DAGui/icon/cancel.svg")));
	d->mActionCancel->setShortcuts(QKeySequence::Cancel);
	connect(d->mActionCancel, &QAction::triggered, this, &DAWorkFlowOperateWidget::cancelCurrent);

	d->mActionSelectAll = new QAction(this);
	d->mActionSelectAll->setObjectName(QStringLiteral("actionSelectAllToDAWorkFlowOperateWidget"));
	d->mActionSelectAll->setIcon(QIcon(QStringLiteral(":/DAGui/icon/select-all.svg")));
	d->mActionSelectAll->setShortcuts(QKeySequence::SelectAll);
	connect(d->mActionSelectAll, &QAction::triggered, this, &DAWorkFlowOperateWidget::setCurrentWorkflowSelectAll);

	d->mActionZoomIn = new QAction(this);
	d->mActionZoomIn->setObjectName(QStringLiteral("actionZoomInToDAWorkFlowOperateWidget"));
	d->mActionZoomIn->setIcon(QIcon(QStringLiteral(":/DAGui/icon/zoomIn.svg")));
	d->mActionZoomIn->setShortcuts({ QKeySequence(QKeySequence::ZoomIn), QKeySequence(QStringLiteral("CTRL+=")) });
	connect(d->mActionZoomIn, &QAction::triggered, this, &DAWorkFlowOperateWidget::setCurrentWorkflowZoomIn);

	d->mActionZoomOut = new QAction(this);
	d->mActionZoomOut->setObjectName(QStringLiteral("actionZoomOutToDAWorkFlowOperateWidget"));
	d->mActionZoomOut->setIcon(QIcon(QStringLiteral(":/DAGui/icon/zoomOut.svg")));
	d->mActionZoomOut->setShortcuts(QKeySequence::ZoomOut);
	connect(d->mActionZoomOut, &QAction::triggered, this, &DAWorkFlowOperateWidget::setCurrentWorkflowZoomOut);

	// 缩放到适合屏幕
	d->mActionZoomFit = new QAction(this);
	d->mActionZoomFit->setObjectName(QStringLiteral("actionZoomFullToDAWorkFlowOperateWidget"));
	d->mActionZoomFit->setIcon(QIcon(QStringLiteral(":/DAGui/icon/viewAll.svg")));
	d->mActionZoomFit->setShortcut(QKeySequence(QStringLiteral("CTRL+0")));
	connect(d->mActionZoomFit, &QAction::triggered, this, &DAWorkFlowOperateWidget::setCurrentWorkflowWholeView);

	// 十字标记线
	d->actionViewCrossLineMarker = new QAction(this);
	d->actionViewCrossLineMarker->setCheckable(true);
	d->actionViewCrossLineMarker->setChecked(false);
	d->actionViewCrossLineMarker->setObjectName(QStringLiteral("actionViewCrossLineMarker"));
	d->actionViewCrossLineMarker->setIcon(QIcon(QStringLiteral(":/DAGui/icon/view-corss-marker.svg")));

	// 水平标记线
	d->actionViewHLineMarker = new QAction(this);
	d->actionViewHLineMarker->setCheckable(true);
	d->actionViewHLineMarker->setChecked(false);
	d->actionViewHLineMarker->setObjectName(QStringLiteral("actionViewHLineMarker"));
	d->actionViewHLineMarker->setIcon(QIcon(QStringLiteral(":/DAGui/icon/view-hline-marker.svg")));

	// 竖直标记线
	d->actionViewVLineMarker = new QAction(this);
	d->actionViewVLineMarker->setCheckable(true);
	d->actionViewVLineMarker->setChecked(false);
	d->actionViewVLineMarker->setObjectName(QStringLiteral("actionViewVLineMarker"));
	d->actionViewVLineMarker->setIcon(QIcon(QStringLiteral(":/DAGui/icon/view-vline-marker.svg")));

	// 无标记线
	d->actionViewNoneMarker = new QAction(this);
	d->actionViewNoneMarker->setCheckable(true);
	d->actionViewNoneMarker->setChecked(false);
	d->actionViewNoneMarker->setObjectName(QStringLiteral("actionViewNoneMarker"));
	d->actionViewNoneMarker->setIcon(QIcon(QStringLiteral(":/DAGui/icon/view-none-marker.svg")));

	d->actionGroupViewLineMarkers = new QActionGroup(this);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	d->actionGroupViewLineMarkers->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
#endif
	d->actionGroupViewLineMarkers->setExclusive(true);
	d->actionGroupViewLineMarkers->addAction(d->actionViewCrossLineMarker);
	d->actionGroupViewLineMarkers->addAction(d->actionViewHLineMarker);
	d->actionGroupViewLineMarkers->addAction(d->actionViewVLineMarker);
	d->actionGroupViewLineMarkers->addAction(d->actionViewNoneMarker);

	connect(d->actionGroupViewLineMarkers,
	        &QActionGroup::triggered,
	        this,
	        &DAWorkFlowOperateWidget::onActionGroupViewLineMarkersTriggered);

	addAction(d->mActionCopy);
	addAction(d->mActionCut);
	addAction(d->mActionPaste);
	addAction(d->mActionDelete);
	addAction(d->mActionCancel);
	addAction(d->mActionSelectAll);
	addAction(d->mActionZoomIn);
	addAction(d->mActionZoomOut);
	addAction(d->mActionZoomFit);
	addAction(d->actionViewCrossLineMarker);
	addAction(d->actionViewHLineMarker);
	addAction(d->actionViewVLineMarker);
	addAction(d->actionViewNoneMarker);
	retranslateUi();
}

void DAWorkFlowOperateWidget::retranslateUi()
{
	DA_D(d);
	d->mActionCopy->setText(tr("Copy"));                                   // cn:复制
	d->mActionCopy->setStatusTip(tr("Copy"));                              // cn:复制
	d->mActionCut->setText(tr("Cut"));                                     // cn:剪切
	d->mActionCut->setStatusTip(tr("Cut"));                                // cn:剪切
	d->mActionPaste->setText(tr("Paste"));                                 // cn:粘贴
	d->mActionPaste->setStatusTip(tr("Paste"));                            // cn:粘贴
	d->mActionDelete->setText(tr("Delete"));                               // cn:删除
	d->mActionDelete->setStatusTip(tr("Delete"));                          // cn:删除
	d->mActionCancel->setText(tr("Cancel"));                               // cn:取消
	d->mActionCancel->setStatusTip(tr("Cancel"));                          // cn:取消
	d->mActionSelectAll->setText(tr("Select All"));                        // cn:全选
	d->mActionSelectAll->setStatusTip(tr("Select all items"));             // cn:全选所有图元
	d->mActionZoomIn->setText(tr("Zoom In"));                              // cn:放大
	d->mActionZoomIn->setStatusTip(tr("Zoom in graphics view"));           // cn:放大画布
	d->mActionZoomOut->setText(tr("Zoom Out"));                            // cn:缩小
	d->mActionZoomOut->setStatusTip(tr("Zoom Out graphics view"));         // cn:缩小画布
	d->mActionZoomFit->setText(tr("Zoom to Fit"));                         // cn:适合屏幕
	d->mActionZoomFit->setStatusTip(tr("Zoom to fit screen size"));        // cn:缩放到适合屏幕大小
	d->actionViewCrossLineMarker->setText(tr("Cross Line Marker"));        // cn:十字标记线
	d->actionViewCrossLineMarker->setStatusTip(tr("Cross Line Marker"));   // cn:十字标记线
	d->actionViewHLineMarker->setText(tr("Horizontal Line Marker"));       // cn:水平标记线
	d->actionViewHLineMarker->setStatusTip(tr("Horizontal Line Marker"));  // cn:水平标记线
	d->actionViewVLineMarker->setText(tr("Vertical Line Marker"));         // cn:垂直标记线
	d->actionViewVLineMarker->setStatusTip(tr("Vertical Line Marker"));    // cn:垂直标记线
	d->actionViewNoneMarker->setText(tr("None Marker"));                   // cn:无标记线
	d->actionViewNoneMarker->setStatusTip(tr("None Marker"));              // cn:无标记线
}

bool DAWorkFlowOperateWidget::isOnlyOneWorkflow() const
{
	return d_ptr->mOnlyOneWorkflow;
}

void DAWorkFlowOperateWidget::setOnlyOneWorkflow(bool v)
{
	d_ptr->mOnlyOneWorkflow = v;
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
		return d_ptr->mActionCopy;
	case ActionCut:
		return d_ptr->mActionCut;
	case ActionPaste:
		return d_ptr->mActionPaste;
	case ActionDelete:
		return d_ptr->mActionDelete;
	case ActionCancel:
		return d_ptr->mActionCancel;
	case ActionSelectAll:
		return d_ptr->mActionSelectAll;
	case ActionZoomIn:
		return d_ptr->mActionZoomIn;
	case ActionZoomOut:
		return d_ptr->mActionZoomOut;
	case ActionZoomFit:
		return d_ptr->mActionZoomFit;
	case ActionCrossLineMarker:
		return d_ptr->actionViewCrossLineMarker;
	case ActionHLineMarker:
		return d_ptr->actionViewHLineMarker;
	case ActionVLineMarker:
		return d_ptr->actionViewVLineMarker;
	case ActionNoneMarker:
		return d_ptr->actionViewNoneMarker;
	default:
		break;
	}
	return nullptr;
}

/**
 * @brief 迭代场景操作
 * @param fp 函数指：bool(DAWorkFlowGraphicsScene*)，返回false代表迭代结束，返回true，代表迭代继续
 * @sa FpScenesOpt
 */
void DAWorkFlowOperateWidget::iteratorScene(FpScenesOpt fp)
{
	const QList< DAWorkFlowGraphicsScene* > secens = getAllWorkFlowScene();
	for (DAWorkFlowGraphicsScene* sc : secens) {
		if (!fp(sc)) {
			return;
		}
	}
}

/**
 * @brief 设置当前视图的标记线
 * @param s
 */
void DAWorkFlowOperateWidget::setCurrentViewLineMarker(DAGraphicsViewOverlayMouseMarker::MarkerStyle s)
{
	DAWorkFlowGraphicsView* v = getCurrentWorkFlowView();
	if (!v) {
		return;
	}
	v->setViewMarkerStyle(s);
}

QActionGroup* DAWorkFlowOperateWidget::getLineMarkerActionGroup() const
{
    return d_ptr->actionGroupViewLineMarkers;
}

/**
 * @brief 设置鼠标动作
 *
 * 一旦设置鼠标动作，鼠标点击后就会触发此动作，continuous来标记动作结束后继续保持还是还原为无动作
 * @param mf 鼠标动作
 * @param continuous 是否连续执行
 */
bool DAWorkFlowOperateWidget::setPreDefineSceneAction(DAWorkFlowGraphicsScene::SceneActionFlag mf)
{
	DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
	if (nullptr == w) {
		qWarning() << tr("No active workflow detected");  // 未检测到激活的工作流
		return false;
	}
	w->setPreDefineSceneAction(mf);
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
