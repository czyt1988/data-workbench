#include "DAWorkFlowEditWidget.h"
#include "ui_DAWorkFlowEditWidget.h"
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
#include "DAGraphicsTextItem.h"
// workflow
#include "DAWorkFlowGraphicsView.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DAGraphicsLinkItem.h"
#include "DAGraphicsPixmapItem.h"
//
#include "Commands/DACommandsForWorkFlow.h"
#include "DAXmlHelper.h"
namespace DA
{

DAWorkFlowEditWidget::DAWorkFlowEditWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAWorkFlowEditWidget), mScene(nullptr)
{
	ui->setupUi(this);
	createScene();
}

DAWorkFlowEditWidget::~DAWorkFlowEditWidget()
{
	qDebug() << "destroy DAWorkFlowEditWidget";
	delete ui;
}

/**
 * @brief 获取工作流
 * @return
 */
DAWorkFlow* DAWorkFlowEditWidget::getWorkflow() const
{
    return ui->workflowGraphicsView->getWorkflow();
}

/**
 * @brief DAWorkFlowEditWidget::setWorkFlow
 * @param w
 */
void DAWorkFlowEditWidget::setWorkFlow(DAWorkFlow* w)
{
	ui->workflowGraphicsView->setWorkFlow(w);
	connect(w, &DAWorkFlow::startExecute, this, &DAWorkFlowEditWidget::startExecute);
	connect(w, &DAWorkFlow::nodeExecuteFinished, this, &DAWorkFlowEditWidget::nodeExecuteFinished);
	connect(w, &DAWorkFlow::finished, this, &DAWorkFlowEditWidget::finished);
}

DAWorkFlowGraphicsView* DAWorkFlowEditWidget::getWorkFlowGraphicsView() const
{
    return ui->workflowGraphicsView;
}

/**
 * @brief 获取场景
 *
 * 每个DAWorkFlowEditWidget必定有一个场景，但可能有多个view
 * @return
 */
DAWorkFlowGraphicsScene* DAWorkFlowEditWidget::getWorkFlowGraphicsScene() const
{
#if 1
	return mScene;
#else
	return qobject_cast< DAWorkFlowGraphicsScene* >(ui->workflowGraphicsView->scene());
#endif
}

void DAWorkFlowEditWidget::setUndoStackActive()
{
	getWorkFlowGraphicsView()->setUndoStackActive();
}

void DAWorkFlowEditWidget::setEnableShowGrid(bool on)
{
	DAWorkFlowGraphicsScene* scene = getWorkFlowGraphicsScene();
	if (scene) {
		scene->showGridLine(on);
		scene->update();
	}
}

QUndoStack* DAWorkFlowEditWidget::getUndoStack()
{
	return getWorkFlowGraphicsView()->getUndoStack();
}

void DAWorkFlowEditWidget::runWorkFlow()
{
	DAWorkFlow* wf = ui->workflowGraphicsView->getWorkflow();
	if (nullptr == wf) {
		qCritical() << tr("no workflow set");
		return;
	}
	wf->exec();
}

void DAWorkFlowEditWidget::setPreDefineSceneAction(DAWorkFlowGraphicsScene::SceneActionFlag mf)
{
	auto sc = getWorkFlowGraphicsScene();
	if (sc) {
		sc->setPreDefineSceneAction(mf);
	}
}

void DAWorkFlowEditWidget::addBackgroundPixmap(const QString& pixmapPath)
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
	// connect(item, &DAGraphicsPixmapItem::itemPosChange, this, &DAWorkFlowOperateWidget::onItemPosChange);
}

void DAWorkFlowEditWidget::setBackgroundPixmapLock(bool on)
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

void DAWorkFlowEditWidget::setSelectTextToBold(bool on)
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

void DAWorkFlowEditWidget::setSelectTextToItalic(bool on)
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

void DAWorkFlowEditWidget::setSelectTextColor(const QColor& color)
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

void DAWorkFlowEditWidget::setSelectTextFamily(const QString& family)
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

void DAWorkFlowEditWidget::setSelectTextPointSize(const int size)
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
void DAWorkFlowEditWidget::setSelectTextItemFont(const QFont& f)
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
void DAWorkFlowEditWidget::setSelectShapeBackgroundBrush(const QBrush& b)
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
void DAWorkFlowEditWidget::setSelectShapeBorderPen(const QPen& v)
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
void DAWorkFlowEditWidget::selectAll()
{
    ui->workflowGraphicsView->selectAll();
}

/**
 * @brief 取消选中
 */
void DAWorkFlowEditWidget::clearSelection()
{
    ui->workflowGraphicsView->clearSelection();
}

/**
 * @brief 复制当前选中的items
 */
void DAWorkFlowEditWidget::copySelectItems()
{
    ui->workflowGraphicsView->copySelectItems();
}

/**
 * @brief 剪切
 */
void DAWorkFlowEditWidget::cutSelectItems()
{
    ui->workflowGraphicsView->cutSelectItems();
}

/**
 * @brief 粘贴动作，把目标粘贴到view中心区域
 */
void DAWorkFlowEditWidget::pasteToViewCenter()
{
    ui->workflowGraphicsView->pasteToViewCenter();
}

/**
 * @brief 移除选中的条目
 */
void DAWorkFlowEditWidget::removeSelectItems()
{
    getWorkFlowGraphicsScene()->removeSelectedItems_();
}

/**
 * @brief 执行取消动作
 */
void DAWorkFlowEditWidget::cancel()
{
	DANodeGraphicsScene* sc = getWorkFlowGraphicsScene();
	if (sc) {
		if (sc->isStartLink()) {
			sc->cancelLink();
		} else {
			// 不在连线状态按下esc，就取消选择
			sc->clearSelection();
		}
	}
}

QFont DAWorkFlowEditWidget::getDefaultTextFont() const
{
	return getWorkFlowGraphicsScene()->getDefaultTextFont();
}

void DAWorkFlowEditWidget::setDefaultTextFont(const QFont& f)
{
	getWorkFlowGraphicsScene()->setDefaultTextFont(f);
}

QColor DAWorkFlowEditWidget::getDefaultTextColor() const
{
	return getWorkFlowGraphicsScene()->getDefaultTextColor();
}

void DAWorkFlowEditWidget::setDefaultTextColor(const QColor& c)
{
    getWorkFlowGraphicsScene()->setDefaultTextColor(c);
}

/**
 * @brief 添加一个图片
 * @param img
 */
DAGraphicsPixmapItem* DAWorkFlowEditWidget::addPixmapItem_(const QImage& img)
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
QPointF DAWorkFlowEditWidget::getViewCenterMapToScene() const
{
	auto r = ui->workflowGraphicsView->viewport()->rect().center();
	return ui->workflowGraphicsView->mapToScene(r);
}

/**
 * @brief 把item移动到屏幕中心
 * @param item
 */
void DAWorkFlowEditWidget::moveItemToViewSceneCenter(QGraphicsItem* item)
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
QRectF DAWorkFlowEditWidget::calcAllItemsSceneRange(const QList< QGraphicsItem* >& its)
{
	if (its.empty()) {
		return QRectF();
	}
	QRectF range = its.first()->sceneBoundingRect();
	for (int i = 1; i < its.size(); ++i) {
		range.united(its[ i ]->sceneBoundingRect());
	}
	return range;
}

QList< QGraphicsItem* > DAWorkFlowEditWidget::cast(const QList< DAGraphicsItem* >& its)
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
QList< DAGraphicsStandardTextItem* > DAWorkFlowEditWidget::getSelectStandardTextItems()
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
	for (QGraphicsItem* item : qAsConst(its)) {
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
QList< DAGraphicsTextItem* > DAWorkFlowEditWidget::getSelectTextItems()
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
QList< DAGraphicsItem* > DAWorkFlowEditWidget::getSelectDAItems()
{
	auto secen = getWorkFlowGraphicsScene();
	if (!secen) {
		return QList< DAGraphicsItem* >();
	}
	return secen->selectedDAItems();
}

void DAWorkFlowEditWidget::createScene()
{
	DAWorkFlowGraphicsScene* sc = new DAWorkFlowGraphicsScene(this);

	mScene = sc;
	ui->workflowGraphicsView->setScene(sc);
	//    connect(_scene, &DAWorkFlowGraphicsScene::selectNodeItemChanged, this, [ this ](DAGraphicsItem* i) {
	//        if (DAAbstractNodeGraphicsItem* ni = dynamic_cast< DAAbstractNodeGraphicsItem* >(i)) {
	//            emit selectNodeItemChanged(ni);
	//        }
	//    });

	connect(sc, &DAWorkFlowGraphicsScene::selectNodeItemChanged, this, &DAWorkFlowEditWidget::selectNodeItemChanged);
	connect(sc, &DAWorkFlowGraphicsScene::sceneActionActived, this, &DAWorkFlowEditWidget::sceneActionActived);
	connect(sc, &DAWorkFlowGraphicsScene::sceneActionDeactived, this, &DAWorkFlowEditWidget::sceneActionDeactived);
}

}  // end of DA
