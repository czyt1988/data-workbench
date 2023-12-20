#include "DAWorkFlowEditWidget.h"
#include "ui_DAWorkFlowEditWidget.h"
// qt
#include <QImage>
#include <QDebug>
#include <QUndoStack>
// moc
#include "ui_DAWorkFlowOperateWidget.h"
// workflow
#include "DAWorkFlowGraphicsView.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DAGraphicsPixmapItem.h"
//
#include "Commands/DACommandsForWorkFlow.h"
namespace DA
{

DAWorkFlowEditWidget::DAWorkFlowEditWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAWorkFlowEditWidget), _scene(nullptr)
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
    return _scene;
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

void DAWorkFlowEditWidget::setMouseActionFlag(DAWorkFlowGraphicsScene::MouseActionFlag mf, bool continous)
{
    auto sc = getWorkFlowGraphicsScene();
    if (sc) {
        sc->setMouseAction(mf, continous);
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
    QList< DAStandardGraphicsTextItem* > items = getSelectTextItems();
    QList< QFont > newFonts;
    for (DAStandardGraphicsTextItem* item : qAsConst(items)) {
        QFont font = item->font();
        font.setBold(on);
        newFonts.append(font);
    }
    DA::DACommandGraphicsTextItemsChangeFont* cmd = new DA::DACommandGraphicsTextItemsChangeFont(items, newFonts);
    secen->push(cmd);
}

void DAWorkFlowEditWidget::setSelectTextToItalic(bool on)
{
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return;
    }
    QList< DAStandardGraphicsTextItem* > items = getSelectTextItems();
    QList< QFont > newFonts;
    for (DAStandardGraphicsTextItem* item : qAsConst(items)) {
        QFont font = item->font();
        font.setItalic(on);
        newFonts.append(font);
    }
    DA::DACommandGraphicsTextItemsChangeFont* cmd = new DA::DACommandGraphicsTextItemsChangeFont(items, newFonts);
    secen->push(cmd);
}

void DAWorkFlowEditWidget::setSelectTextColor(const QColor& color)
{
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return;
    }
    QList< DAStandardGraphicsTextItem* > items = getSelectTextItems();
    QList< QColor > colors;
    for (int i = 0; i < items.size(); ++i) {
        colors.append(color);
    }
    DA::DACommandGraphicsTextItemsChangeColor* cmd = new DA::DACommandGraphicsTextItemsChangeColor(items, colors);
    secen->push(cmd);
}

void DAWorkFlowEditWidget::setSelectTextFamily(const QString& family)
{
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return;
    }
    QList< DAStandardGraphicsTextItem* > items = getSelectTextItems();
    QList< QFont > newFonts;
    for (DAStandardGraphicsTextItem* item : qAsConst(items)) {
        QFont font = item->font();
        font.setFamily(family);
        newFonts.append(font);
    }
    DA::DACommandGraphicsTextItemsChangeFont* cmd = new DA::DACommandGraphicsTextItemsChangeFont(items, newFonts);
    secen->push(cmd);
}

void DAWorkFlowEditWidget::setTextSize(const int size)
{
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return;
    }
    QList< DAStandardGraphicsTextItem* > items = getSelectTextItems();
    QList< QFont > newFonts;
    for (DAStandardGraphicsTextItem* item : qAsConst(items)) {
        QFont font = item->font();
        font.setPointSize(size);
        newFonts.append(font);
    }
    DA::DACommandGraphicsTextItemsChangeFont* cmd = new DA::DACommandGraphicsTextItemsChangeFont(items, newFonts);
    secen->push(cmd);
}

/**
 * @brief 设置选中的textitem的字体
 * @param f
 */
void DAWorkFlowEditWidget::setSelectTextItemFont(const QFont& f)
{
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return;
    }
    QList< DAStandardGraphicsTextItem* > items = getSelectTextItems();
    if (items.isEmpty()) {
        return;
    }
    QList< QFont > newFonts;
    for (int i = 0; i < items.size(); ++i) {
        newFonts.append(f);
    }
    DA::DACommandGraphicsTextItemsChangeFont* cmd = new DA::DACommandGraphicsTextItemsChangeFont(items, newFonts);
    secen->push(cmd);
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
    QList< DAGraphicsItem* > items = getSelectBaseItems();
    if (items.isEmpty()) {
        return;
    }
    DA::DACommandGraphicsShapeBackgroundBrushChange* cmd = new DA::DACommandGraphicsShapeBackgroundBrushChange(items, b);
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
    QList< DAGraphicsItem* > items = getSelectBaseItems();
    if (items.isEmpty()) {
        return;
    }
    DA::DACommandGraphicsShapeBorderPenChange* cmd = new DA::DACommandGraphicsShapeBorderPenChange(items, v);
    secen->push(cmd);
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
 * @brief 获取选中的文本
 * @return
 */
QList< DAStandardGraphicsTextItem* > DAWorkFlowEditWidget::getSelectTextItems()
{
    QList< DAStandardGraphicsTextItem* > res;
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return res;
    }
    QList< QGraphicsItem* > its = secen->selectedItems();
    if (its.size() == 0) {
        return res;
    }
    for (QGraphicsItem* item : qAsConst(its)) {
        if (DAStandardGraphicsTextItem* textItem = dynamic_cast< DAStandardGraphicsTextItem* >(item)) {
            res.append(textItem);
        }
    }
    return res;
}

/**
 * @brief 获取选中的基本图元
 * @return
 */
QList< DAGraphicsItem* > DAWorkFlowEditWidget::getSelectBaseItems()
{
    QList< DAGraphicsItem* > res;
    auto secen = getWorkFlowGraphicsScene();
    if (!secen) {
        return res;
    }
    QList< QGraphicsItem* > its = secen->selectedItems();
    if (its.size() == 0) {
        return res;
    }
    for (QGraphicsItem* item : qAsConst(its)) {
        if (DAGraphicsItem* i = dynamic_cast< DAGraphicsItem* >(item)) {
            res.append(i);
        }
    }
    return res;
}

void DAWorkFlowEditWidget::createScene()
{
    _scene = new DAWorkFlowGraphicsScene(this);
    ui->workflowGraphicsView->setScene(_scene);
    //    connect(_scene, &DAWorkFlowGraphicsScene::selectNodeItemChanged, this, [ this ](DAGraphicsItem* i) {
    //        if (DAAbstractNodeGraphicsItem* ni = dynamic_cast< DAAbstractNodeGraphicsItem* >(i)) {
    //            emit selectNodeItemChanged(ni);
    //        }
    //    });

    connect(_scene, &DAWorkFlowGraphicsScene::selectNodeItemChanged, this, &DAWorkFlowEditWidget::selectNodeItemChanged);
    connect(_scene, &DAWorkFlowGraphicsScene::mouseActionFinished, this, &DAWorkFlowEditWidget::mouseActionFinished);
}

}  // end of DA
