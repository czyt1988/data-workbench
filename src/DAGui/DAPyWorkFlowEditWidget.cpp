#include "DAPyWorkFlowEditWidget.h"
#include "ui_DAPyWorkFlowEditWidget.h"
// qt
#include <QUrl>
#include <QImage>
#include <QDebug>
#include <QUndoStack>
#include <QColor>
#include <QList>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include <QFileInfo>
#include <QThread>
#include "DAGraphicsTextItem.h"
// workflow
#include "DAPyWorkFlowGraphicsView.h"
#include "DAPyWorkFlowGraphicsScene.h"
#include "DAGraphicsLinkItem.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyWorkFlowExecuter.h"
#include "DAGraphicsPixmapItem.h"
//
#include "Commands/DACommandsForWorkFlow.h"
#include "DAXmlHelper.h"
namespace DA
{

DAPyWorkFlowEditWidget::DAPyWorkFlowEditWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAPyWorkFlowEditWidget), mScene(nullptr)
{
	ui->setupUi(this);
	createScene();
}

DAPyWorkFlowEditWidget::~DAPyWorkFlowEditWidget()
{
	qDebug() << "destroy DAPyWorkFlowEditWidget";
	// 断开子对象到 this 的信号连接，防止析构期间信号发给已析构对象
	const auto allChildren = findChildren<QObject*>();
	for (auto* obj : allChildren) {
		obj->disconnect(this);
	}
	// 停止工作流执行线程
	if (mWorkFlowThread && mWorkFlowThread->isRunning()) {
		mWorkFlowThread->quit();
		mWorkFlowThread->wait();
	}
	delete ui;
}

/**
 * @brief 获取工作流
 * @return
 */
DAPyWorkFlow* DAPyWorkFlowEditWidget::getWorkflow() const
{
    return ui->workflowGraphicsView->getWorkflow();
}

/**
 * @brief DAPyWorkFlowEditWidget::setWorkFlow
 * @param w
 */
void DAPyWorkFlowEditWidget::setWorkFlow(DAPyWorkFlow* w)
{
	ui->workflowGraphicsView->setWorkFlow(w);
	// TODO: DAPyWorkFlow no longer has startExecute/nodeExecuteFinished/finished signals.
	// These signals now belong to DAPyWorkFlowExecuter. Execution signal connections
	// should be set up when a DAPyWorkFlowExecuter is created for this workflow.
	// The DAPyWorkFlowEditWidget's own signals (startExecute, nodeExecuteFinished, finished)
	// remain defined and can be emitted manually or connected from an executer later.
}

DAPyWorkFlowGraphicsView* DAPyWorkFlowEditWidget::getWorkFlowGraphicsView() const
{
    return ui->workflowGraphicsView;
}

/**
 * @brief 获取场景
 *
 * 每个DAPyWorkFlowEditWidget必定有一个场景，但可能有多个view
 * @return
 */
DAPyWorkFlowGraphicsScene* DAPyWorkFlowEditWidget::getWorkFlowGraphicsScene() const
{
	return mScene;
}

void DAPyWorkFlowEditWidget::setUndoStackActive()
{
	getWorkFlowGraphicsView()->setUndoStackActive();
}

void DAPyWorkFlowEditWidget::setEnableShowGrid(bool on)
{
	DAPyWorkFlowGraphicsScene* scene = getWorkFlowGraphicsScene();
	if (scene) {
		scene->showGridLine(on);
		scene->update();
	}
}

QUndoStack* DAPyWorkFlowEditWidget::getUndoStack()
{
	return getWorkFlowGraphicsView()->getUndoStack();
}

void DAPyWorkFlowEditWidget::runWorkFlow()
{
	auto scene = getWorkFlowGraphicsScene();
	if (!scene || !scene->hasPyWorkflow()) {
		qCritical() << tr("no workflow set");
		return;
	}

	// 创建执行器并设置 Python 工作流对象
	auto executer = new DA::DAPyWorkFlowExecuter(this);
	executer->setWorkflow(scene->getPyWorkflow());

	// 创建独立线程
	mWorkFlowThread = new QThread(this);
	executer->moveToThread(mWorkFlowThread);

	// 连接信号：线程启动时开始执行
	connect(mWorkFlowThread, &QThread::started, executer, &DA::DAPyWorkFlowExecuter::startExecute);
	// 节点执行完成信号转发（类型适配：shared_ptr -> raw pointer）
	connect(executer, &DA::DAPyWorkFlowExecuter::nodeExecuteFinished,
	        this, [this](std::shared_ptr< DA::DAPyNodeProxy > nodeProxy, bool success) {
		        emit nodeExecuteFinished(nodeProxy.get(), success);
	        });
	// 执行完成信号转发
	connect(executer, &DA::DAPyWorkFlowExecuter::finished, this, [this](bool success) {
		emit finished(success);
	});
	// 线程结束后清理资源
	connect(mWorkFlowThread, &QThread::finished, mWorkFlowThread, &QThread::deleteLater);
	connect(mWorkFlowThread, &QThread::finished, executer, &QObject::deleteLater);
	// 执行完成时停止线程
	connect(executer, &DA::DAPyWorkFlowExecuter::finished, mWorkFlowThread, &QThread::quit);
	// 进度信号转发
	connect(executer, &DA::DAPyWorkFlowExecuter::progressChanged,
	        this, [](int current, int total) {
			qDebug() << "Workflow progress:" << current << "/" << total;
		});

	// 启动线程
	mWorkFlowThread->start();
}

void DAPyWorkFlowEditWidget::setPreDefineSceneAction(DAPyWorkFlowGraphicsScene::SceneActionFlag mf)
{
	auto sc = getWorkFlowGraphicsScene();
	if (sc) {
		sc->setPreDefineSceneAction(mf);
	}
}

void DAPyWorkFlowEditWidget::addBackgroundPixmap(const QString& pixmapPath)
{
	auto sc = getWorkFlowGraphicsScene();
	if (!sc) {
		return;
	}

	QImage img(pixmapPath);
	QPixmap px;
	px.convertFromImage(img);
	DAGraphicsPixmapItem* item = sc->setBackgroundPixmap(px);
	item->setSelectable(true);
	item->setMoveable(true);
	// connect(item, &DAGraphicsPixmapItem::itemPosChange, this, &DAPyWorkFlowOperateWidget::onItemPosChange);
}

void DAPyWorkFlowEditWidget::setBackgroundPixmapLock(bool on)
{
	auto sc = getWorkFlowGraphicsScene();
	if (!sc) {
		return;
	}
	DAGraphicsPixmapItem* item = sc->getBackgroundPixmapItem();
	if (nullptr == item) {
		return;
	}
	item->setSelectable(!on);
	item->setMoveable(!on);
}

void DAPyWorkFlowEditWidget::setSelectTextToBold(bool on)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	const auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextBold(on);
	}

	const auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextBold(on);
	}
}

void DAPyWorkFlowEditWidget::setSelectTextToItalic(bool on)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	const auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextItalic(on);
	}

	const auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextItalic(on);
	}
}

void DAPyWorkFlowEditWidget::setSelectTextColor(const QColor& color)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	const auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextColor(color);
	}

	const auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextColor(color);
	}
}

void DAPyWorkFlowEditWidget::setSelectTextFamily(const QString& family)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	const auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextFamily(family);
	}

	const auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextFamily(family);
	}
}

void DAPyWorkFlowEditWidget::setSelectTextPointSize(const int size)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	const auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextPointSize(size);
	}
	const auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextPointSize(size);
	}
}

/**
 * @brief 设置选中的textitem的字体
 *
 * @note 此操作自带redo/undo，DAGraphicsStandardTextItem会自动把命令放入scene的undo stack中
 * @param f
 */
void DAPyWorkFlowEditWidget::setSelectTextItemFont(const QFont& f)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	auto standarditems = getSelectStandardTextItems();
	for (auto item : standarditems) {
		item->setSelectTextFont(f);
	}
	auto items = getSelectTextItems();
	for (auto item : items) {
		item->setSelectTextFont(f);
	}
}

/**
 * @brief 设置当前选中图元的背景
 *
 * @note 支持redo/undo
 * @param b
 */
void DAPyWorkFlowEditWidget::setSelectShapeBackgroundBrush(const QBrush& b)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	QList< DAGraphicsItem* > items = getSelectDAItems();
	if (items.isEmpty()) {
		return;
	}
	auto cmd = new DA::DACommandGraphicsShapeBackgroundBrushChange(items, b);
	secen->push(cmd);
}
/**
 * @brief 设置当前选中图元的边框
 * @param v
 */
void DAPyWorkFlowEditWidget::setSelectShapeBorderPen(const QPen& v)
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return;
	}
	QList< DAGraphicsItem* > items = getSelectDAItems();
	if (items.isEmpty()) {
		return;
	}
	DA::DACommandGraphicsShapeBorderPenChange* cmd = new DA::DACommandGraphicsShapeBorderPenChange(items, v);
	secen->push(cmd);
}

/**
 * @brief 全选
 */
void DAPyWorkFlowEditWidget::selectAll()
{
    ui->workflowGraphicsView->selectAll();
}

/**
 * @brief 取消选中
 */
void DAPyWorkFlowEditWidget::clearSelection()
{
    ui->workflowGraphicsView->clearSelection();
}

/**
 * @brief 复制当前选中的items
 */
void DAPyWorkFlowEditWidget::copySelectItems()
{
    ui->workflowGraphicsView->copySelectItems();
}

/**
 * @brief 剪切
 */
void DAPyWorkFlowEditWidget::cutSelectItems()
{
    ui->workflowGraphicsView->cutSelectItems();
}

/**
 * @brief 粘贴动作，把目标粘贴到view中心区域
 */
void DAPyWorkFlowEditWidget::pasteToViewCenter()
{
    ui->workflowGraphicsView->pasteToViewCenter();
}

/**
 * @brief 移除选中的条目
 */
void DAPyWorkFlowEditWidget::removeSelectItems()
{
    getWorkFlowGraphicsScene()->removeSelectedItems_();
}

/**
 * @brief 执行取消动作
 */
void DAPyWorkFlowEditWidget::cancel()
{
	DAPyWorkFlowGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {
		if (sc->isStartLink()) {
			sc->cancelLink();
		} else {
			// 不在连线状态按下esc，就取消选择
			sc->clearSelection();
		}
	}
}

QFont DAPyWorkFlowEditWidget::getDefaultTextFont() const
{
	return getWorkFlowGraphicsScene()->getDefaultTextFont();
}

void DAPyWorkFlowEditWidget::setDefaultTextFont(const QFont& f)
{
	getWorkFlowGraphicsScene()->setDefaultTextFont(f);
}

QColor DAPyWorkFlowEditWidget::getDefaultTextColor() const
{
	return getWorkFlowGraphicsScene()->getDefaultTextColor();
}

void DAPyWorkFlowEditWidget::setDefaultTextColor(const QColor& c)
{
    getWorkFlowGraphicsScene()->setDefaultTextColor(c);
}

/**
 * @brief 添加一个图片
 * @param img
 */
DAGraphicsPixmapItem* DAPyWorkFlowEditWidget::addPixmapItem_(const QImage& img)
{
	if (img.isNull()) {
		return nullptr;
	}
	QPixmap pixmap = QPixmap::fromImage(img);
	if (pixmap.isNull()) {
		return nullptr;
	}
	DAGraphicsPixmapItem* pixmapItem = new DAGraphicsPixmapItem(pixmap);
	getWorkFlowGraphicsScene()->addItem_(pixmapItem);
	return pixmapItem;
}

/**
 * @brief 获取当前view视图下的scene中心
 * @return
 */
QPointF DAPyWorkFlowEditWidget::getViewCenterMapToScene() const
{
	auto r = ui->workflowGraphicsView->viewport()->rect().center();
	return ui->workflowGraphicsView->mapToScene(r);
}

/**
 * @brief 把item移动到屏幕中心
 * @param item
 */
void DAPyWorkFlowEditWidget::moveItemToViewSceneCenter(QGraphicsItem* item)
{
	QPointF c = getViewCenterMapToScene();
	auto br   = item->boundingRect();
	c.rx() -= (br.width() / 2);
	c.ry() -= (br.height() / 2);
	item->setPos(c);
}

/**
 * @brief 计算item所包含的范围，这个范围存入xml中，以便让scene第一时间知道总体范围
 * @param its
 * @return
 */
QRectF DAPyWorkFlowEditWidget::calcAllItemsSceneRange(const QList< QGraphicsItem* >& its)
{
	if (its.empty()) {
		return QRectF();
	}
	QRectF range = its.first()->sceneBoundingRect();
	for (int i = 1; i < its.size(); ++i) {
        range = range.united(its[ i ]->sceneBoundingRect());
	}
	return range;
}

QList< QGraphicsItem* > DAPyWorkFlowEditWidget::cast(const QList< DAGraphicsItem* >& its)
{
	QList< QGraphicsItem* > res;
	res.reserve(its.size());
	for (DAGraphicsItem* i : its) {
		res.append(static_cast< QGraphicsItem* >(i));
	}
	return res;
}

/**
 * @brief 获取选中的文本
 * @return
 */
QList< DAGraphicsStandardTextItem* > DAPyWorkFlowEditWidget::getSelectStandardTextItems()
{
	QList< DAGraphicsStandardTextItem* > res;
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
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

/**
 * @brief getSelectTextItems
 * @return
 */
QList< DAGraphicsTextItem* > DAPyWorkFlowEditWidget::getSelectTextItems()
{
	QList< DAGraphicsTextItem* > res;
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return res;
	}
	const QList< QGraphicsItem* > its = secen->selectedItems();
	if (its.size() == 0) {
		return res;
	}
	for (QGraphicsItem* item : its) {
		if (DAGraphicsTextItem* textItem = dynamic_cast< DAGraphicsTextItem* >(item)) {
			res.append(textItem);
		}
	}
	return res;
}

/**
 * @brief 获取选中的基本图元
 * @return
 */
QList< DAGraphicsItem* > DAPyWorkFlowEditWidget::getSelectDAItems()
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return QList< DAGraphicsItem* >();
	}
	return secen->selectedDAItems();
}

void DAPyWorkFlowEditWidget::createScene()
{
	DAPyWorkFlowGraphicsScene* sc = new DAPyWorkFlowGraphicsScene(this);

	mScene = sc;
	ui->workflowGraphicsView->setScene(sc);
	//    connect(_scene, &DAPyWorkFlowGraphicsScene::selectNodeItemChanged, this, [ this ](DAGraphicsItem* i) {
	//        if (DAPyNodeGraphicsItem* ni = dynamic_cast< DAPyNodeGraphicsItem* >(i)) {
	//            emit selectNodeItemChanged(ni);
	//        }
	//    });

	connect(sc, &DAPyWorkFlowScene::selectPyNodeItemChanged, this, &DAPyWorkFlowEditWidget::selectNodeItemChanged);
	connect(sc, &DAPyWorkFlowGraphicsScene::sceneActionActived, this, &DAPyWorkFlowEditWidget::sceneActionActived);
	connect(sc, &DAPyWorkFlowGraphicsScene::sceneActionDeactived, this, &DAPyWorkFlowEditWidget::sceneActionDeactived);
}

}  // end of DA
