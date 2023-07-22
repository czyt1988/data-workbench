#include "DAWorkFlowNodeItemSettingWidget.h"
#include "ui_DAWorkFlowNodeItemSettingWidget.h"
#include "DAAbstractNode.h"
#include "DAAbstractNodeGraphicsItem.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAGraphicsPixmapItemSettingWidget.h"
#include "DAStandardGraphicsTextItem.h"
#include "DAWorkFlowEditWidget.h"
#include "DAGraphicsResizeablePixmapItem.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAWorkFlowNodeItemSettingWidget
//===================================================
DAWorkFlowNodeItemSettingWidget::DAWorkFlowNodeItemSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAWorkFlowNodeItemSettingWidget), _lastTabIndex(0)
{
    ui->setupUi(this);
    init();
}

DAWorkFlowNodeItemSettingWidget::~DAWorkFlowNodeItemSettingWidget()
{
    delete ui;
}

void DAWorkFlowNodeItemSettingWidget::init()
{
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DAWorkFlowNodeItemSettingWidget::onTabBarCurrentIndexChanged);
    connect(ui->tabPictureItemSetting,
            &DAGraphicsPixmapItemSettingWidget::pixmapAlphaValueChanged,
            this,
            &DAWorkFlowNodeItemSettingWidget::onPixmapItemAlphaChanged);
}

void DAWorkFlowNodeItemSettingWidget::bindWorkFlowEditWidget(DAWorkFlowEditWidget* w)
{
    if (_workflowEditWidget) {
        DAWorkFlowGraphicsScene* s = _workflowEditWidget->getWorkFlowGraphicsScene();
        disconnect(s, &DAWorkFlowGraphicsScene::selectionChanged, this, &DAWorkFlowNodeItemSettingWidget::onSceneSelectionChanged);
        disconnect(s, &DAWorkFlowGraphicsScene::itemsPositionChanged, this, &DAWorkFlowNodeItemSettingWidget::onSceneItemsPositionChanged);
        disconnect(s, &DAWorkFlowGraphicsScene::itemBodySizeChanged, this, &DAWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged);
        disconnect(s, &DAWorkFlowGraphicsScene::itemRotationChanged, this, &DAWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged);
    }
    _workflowEditWidget = w;
    if (_workflowEditWidget) {
        DAWorkFlowGraphicsScene* s = _workflowEditWidget->getWorkFlowGraphicsScene();
        connect(s, &DAWorkFlowGraphicsScene::selectionChanged, this, &DAWorkFlowNodeItemSettingWidget::onSceneSelectionChanged);
        connect(s, &DAWorkFlowGraphicsScene::itemsPositionChanged, this, &DAWorkFlowNodeItemSettingWidget::onSceneItemsPositionChanged);
        connect(s, &DAWorkFlowGraphicsScene::itemBodySizeChanged, this, &DAWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged);
        connect(s, &DAWorkFlowGraphicsScene::itemRotationChanged, this, &DAWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged);
        ui->tabItemSetting->setScene(s);
        ui->tabItemSetting->updateData();
    } else {
        ui->tabItemSetting->setScene(nullptr);
    }
}

void DAWorkFlowNodeItemSettingWidget::addWidget(QWidget* w, const QIcon& icon, const QString& title)
{
    tabWidget()->addTab(w, icon, title);
}

void DAWorkFlowNodeItemSettingWidget::removeWidget(QWidget* w)
{
    tabWidget()->removeTab(tabWidget()->indexOf(w));
}

QTabWidget* DAWorkFlowNodeItemSettingWidget::tabWidget()
{
    return ui->tabWidget;
}

/**
 * @brief 设置关联的场景
 * @param s
 */
void DAWorkFlowNodeItemSettingWidget::setWorkFlowOperateWidget(DAWorkFlowOperateWidget* wf)
{
    if (_workflowOptWidget) {
        disconnect(_workflowOptWidget.data(),
                   &DAWorkFlowOperateWidget::currentWorkFlowWidgetChanged,
                   this,
                   &DAWorkFlowNodeItemSettingWidget::onWorkFlowEditWidgetChanged);
    }
    _workflowOptWidget = wf;
    if (_workflowOptWidget) {
        connect(_workflowOptWidget.data(),
                &DAWorkFlowOperateWidget::currentWorkFlowWidgetChanged,
                this,
                &DAWorkFlowNodeItemSettingWidget::onWorkFlowEditWidgetChanged);
    }
}
/**
 * @brief 设置节点设置可用
 * @param on
 */
void DAWorkFlowNodeItemSettingWidget::setNodeSettingEnable(bool on)
{
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabNodeSetting), on);
}
/**
 * @brief 设置item设置可用
 * @param on
 */
void DAWorkFlowNodeItemSettingWidget::setItemSettingEnable(bool on)
{
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabItemSetting), on);
}
/**
 * @brief 设置link设置可用
 * @param on
 */
void DAWorkFlowNodeItemSettingWidget::setLinkSettingEnable(bool on)
{
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabLinkSetting), on);
}
/**
 * @brief 设置PixmapItem设置可用
 * @param on
 */
void DAWorkFlowNodeItemSettingWidget::setPixmapItemSettingEnable(bool on)
{
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabPictureItemSetting), on);
}

DAWorkFlowGraphicsScene* DAWorkFlowNodeItemSettingWidget::getCurrentScene() const
{
    if (_workflowEditWidget) {
        return _workflowEditWidget->getWorkFlowGraphicsScene();
    }
    return nullptr;
}
/**
 * @brief 选择改变
 */
void DAWorkFlowNodeItemSettingWidget::onSceneSelectionChanged()
{
    DAWorkFlowGraphicsScene* scene = getCurrentScene();
    if (nullptr == scene) {
        ui->tabNodeSetting->setNode(nullptr);
        ui->tabItemSetting->setItem(nullptr);
        ui->tabLinkSetting->setLinkItem(nullptr);
        return;
    }
    QList< QGraphicsItem* > its = scene->selectedItems();
    if (its.empty()) {
        setLinkSettingEnable(false);
        setNodeSettingEnable(false);
        setItemSettingEnable(false);
        setPixmapItemSettingEnable(false);
        return;
    }
    QGraphicsItem* item = its.last();
    //此注释给新手：
    // 注意C语言的条件赋值，如果是nullptr就不会执行if内容
    if (DAAbstractNodeGraphicsItem* nodeItem = dynamic_cast< DAAbstractNodeGraphicsItem* >(item)) {
        //说明是node
        setLinkSettingEnable(false);
        setNodeSettingEnable(true);
        setItemSettingEnable(true);
        setPixmapItemSettingEnable(false);
        ui->tabNodeSetting->setNode(nodeItem->node());
        ui->tabItemSetting->setItem(nodeItem);
        ui->tabLinkSetting->setLinkItem(nullptr);
    } else if (DAAbstractNodeLinkGraphicsItem* linkItem = dynamic_cast< DAAbstractNodeLinkGraphicsItem* >(item)) {
        //说明是link
        setLinkSettingEnable(true);
        setNodeSettingEnable(false);
        setItemSettingEnable(false);
        setPixmapItemSettingEnable(false);
        ui->tabNodeSetting->setNode(nullptr);
        ui->tabItemSetting->setItem(nullptr);
        ui->tabLinkSetting->setLinkItem(linkItem);
    } else {
        //说明是其他item
        setLinkSettingEnable(false);
        setNodeSettingEnable(false);
        setItemSettingEnable(true);
        ui->tabNodeSetting->setNode(nullptr);
        ui->tabItemSetting->setItem(nullptr);
        ui->tabLinkSetting->setLinkItem(nullptr);
        if (DAGraphicsResizeablePixmapItem* pitem = dynamic_cast< DAGraphicsResizeablePixmapItem* >(item)) {
            setPixmapItemSettingEnable(true);
            updatePixmapItemSettingWidget(pitem);
        } else {
            setPixmapItemSettingEnable(false);
        }
    }
    //把最后用户点击的页面记录下来并显示
    if (_lastTabIndex >= 0 && _lastTabIndex < ui->tabWidget->count()) {
        if (ui->tabWidget->isTabEnabled(_lastTabIndex)) {
            ui->tabWidget->setCurrentIndex(_lastTabIndex);
        }
    }
}

/**
 * @brief 条目的位置改变触发的槽
 * @param items
 * @param oldPos
 * @param newPos
 */
void DAWorkFlowNodeItemSettingWidget::onSceneItemsPositionChanged(const QList< QGraphicsItem* >& items,
                                                                  const QList< QPointF >& oldPos,
                                                                  const QList< QPointF >& newPos)
{
    Q_UNUSED(oldPos);
    Q_UNUSED(newPos);
    //刷新item维护的数据
    if (items.contains(static_cast< QGraphicsItem* >(ui->tabItemSetting->getItem()))) {
        ui->tabItemSetting->updatePosition();
    }
}
/**
 * @brief DAWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged
 * @param item
 * @param oldSize
 * @param newSize
 */
void DAWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged(DAGraphicsResizeableItem* item, const QSizeF& oldSize, const QSizeF& newSize)
{
    Q_UNUSED(item);
    Q_UNUSED(oldSize);
    Q_UNUSED(newSize);
    if (ui->tabItemSetting->getItem() == item) {
        ui->tabItemSetting->updateBodySize();
    }
}
/**
 * @brief DAWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged
 * @param item
 * @param rotation
 */
void DAWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged(DAGraphicsResizeableItem* item, const qreal& rotation)
{
    Q_UNUSED(item);
    Q_UNUSED(rotation);
    if (ui->tabItemSetting->getItem() == item) {
        ui->tabItemSetting->updateRotation();
    }
}

void DAWorkFlowNodeItemSettingWidget::onTabBarCurrentIndexChanged(int index)
{
    _lastTabIndex = index;
}

void DAWorkFlowNodeItemSettingWidget::onWorkFlowEditWidgetChanged(DAWorkFlowEditWidget* w)
{
    if (w) {
        bindWorkFlowEditWidget(w);
    }
}

/**
 * @brief 图片元素的透明度 改变
 * @param v
 */
void DAWorkFlowNodeItemSettingWidget::onPixmapItemAlphaChanged(int v)
{
    DAWorkFlowGraphicsScene* scene = getCurrentScene();
    if (nullptr == scene) {
        return;
    }
    DAGraphicsResizeablePixmapItem* pixmapItem = scene->getBackgroundPixmapItem();
    if (nullptr == pixmapItem) {
        return;
    }
    pixmapItem->setAlpha(v);
}

/**
 * @brief 更新PixmapItemSettingWidget
 * @param pitem
 */
void DAWorkFlowNodeItemSettingWidget::updatePixmapItemSettingWidget(DAGraphicsResizeablePixmapItem* pitem)
{
    ui->tabPictureItemSetting->setCurrentAlphaValue(pitem->getAlpha());
}
