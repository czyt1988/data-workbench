#include "DAPyWorkFlowNodeItemSettingWidget.h"
#include "ui_DAPyWorkFlowNodeItemSettingWidget.h"
#include "DAPyNodeProxy.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyWorkFlowOperateWidget.h"
#include "DAGraphicsPixmapItemSettingWidget.h"
#include "DAGraphicsStandardTextItem.h"
#include "DAPyWorkFlowEditWidget.h"
#include "DAGraphicsPixmapItem.h"
#include "DANodeParamSettingPanelWidget.h"
#include "DANodeParamSettingPanelFactory.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyWorkFlowNodeItemSettingWidget
//===================================================
DAPyWorkFlowNodeItemSettingWidget::DAPyWorkFlowNodeItemSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAPyWorkFlowNodeItemSettingWidget), _lastTabIndex(0)
{
    ui->setupUi(this);
    init();
}

DAPyWorkFlowNodeItemSettingWidget::~DAPyWorkFlowNodeItemSettingWidget()
{
    delete ui;
}

void DAPyWorkFlowNodeItemSettingWidget::init()
{
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DAPyWorkFlowNodeItemSettingWidget::onTabBarCurrentIndexChanged);

    // 创建参数设置面板调度器，插入为第一个tab页（index 0）
    mParamSettingWidget = new DANodeParamSettingPanelWidget(this);
    ui->tabWidget->insertTab(0, mParamSettingWidget, tr("参数"));
    ui->tabWidget->setCurrentIndex(0);

    // 注册默认面板（当前为空实现，后续任务将注册具体面板）
    DANodeParamSettingPanelFactory::instance().registerDefaultPanels();

    // 参数值变化信号 → 实时写入代理配置
    // 注意: DANodeParamSettingPanel::collectConfig() 为 protected，无法从外部调用
    // 实际配置持久化将由 3-hop 信号链第三跳 (onPropertyValueChanged) 完成
    // 此连接作为外部通知接口，后续 collectConfig 可访问时补充 setConfig 调用
    connect(mParamSettingWidget, &DANodeParamSettingPanelWidget::propertyValueChanged,
            this, [this](int propertyId) {
                Q_UNUSED(propertyId);
                // TODO: 当 collectConfig() 可外部访问时，补充 proxy->setConfig(panel->collectConfig())
            });
}

void DAPyWorkFlowNodeItemSettingWidget::bindWorkFlowEditWidget(DAPyWorkFlowEditWidget* w)
{
    if (_workflowEditWidget) {
        DAPyWorkFlowGraphicsScene* s = _workflowEditWidget->getWorkFlowGraphicsScene();
        disconnect(s, &DAPyWorkFlowGraphicsScene::selectionChanged, this, &DAPyWorkFlowNodeItemSettingWidget::onSceneSelectionChanged);
        disconnect(s, &DAPyWorkFlowGraphicsScene::nodeDoubleClicked, this, &DAPyWorkFlowNodeItemSettingWidget::onSceneNodeDoubleClicked);
        disconnect(s,
                   &DAPyWorkFlowGraphicsScene::itemsPositionChanged,
                   this,
                   &DAPyWorkFlowNodeItemSettingWidget::onSceneItemsPositionChanged);
        disconnect(s,
                   &DAPyWorkFlowGraphicsScene::itemBodySizeChanged,
                   this,
                   &DAPyWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged);
        disconnect(s,
                   &DAPyWorkFlowGraphicsScene::itemRotationChanged,
                   this,
                   &DAPyWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged);
    }
    _workflowEditWidget = w;
    if (_workflowEditWidget) {
        DAPyWorkFlowGraphicsScene* s = _workflowEditWidget->getWorkFlowGraphicsScene();
        connect(s, &DAPyWorkFlowGraphicsScene::selectionChanged, this, &DAPyWorkFlowNodeItemSettingWidget::onSceneSelectionChanged);
        connect(s, &DAPyWorkFlowGraphicsScene::nodeDoubleClicked, this, &DAPyWorkFlowNodeItemSettingWidget::onSceneNodeDoubleClicked);
        connect(s,
                &DAPyWorkFlowGraphicsScene::itemsPositionChanged,
                this,
                &DAPyWorkFlowNodeItemSettingWidget::onSceneItemsPositionChanged);
        connect(s,
                &DAPyWorkFlowGraphicsScene::itemBodySizeChanged,
                this,
                &DAPyWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged);
        connect(s,
                &DAPyWorkFlowGraphicsScene::itemRotationChanged,
                this,
                &DAPyWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged);
        ui->tabItemSetting->setScene(s);
        ui->tabItemSetting->updateData();
    } else {
        ui->tabItemSetting->setScene(nullptr);
    }
}

void DAPyWorkFlowNodeItemSettingWidget::addWidget(QWidget* w, const QIcon& icon, const QString& title)
{
    tabWidget()->addTab(w, icon, title);
}

/**
 * @brief 移除设置页
 * @note 注意，页面并不会被删除
 * @param w
 */
void DAPyWorkFlowNodeItemSettingWidget::removeWidget(QWidget* w)
{
    int index = tabWidget()->indexOf(w);
    if (index >= 0) {
        tabWidget()->removeTab(index);
    }
}

QTabWidget* DAPyWorkFlowNodeItemSettingWidget::tabWidget()
{
    return ui->tabWidget;
}

/**
 * @brief 设置关联的场景
 * @param s
 */
void DAPyWorkFlowNodeItemSettingWidget::setWorkFlowOperateWidget(DAPyWorkFlowOperateWidget* wf)
{
    if (_workflowOptWidget) {
        disconnect(_workflowOptWidget.data(),
                   &DAPyWorkFlowOperateWidget::currentWorkFlowWidgetChanged,
                   this,
                   &DAPyWorkFlowNodeItemSettingWidget::onWorkFlowEditWidgetChanged);
    }
    _workflowOptWidget = wf;
    if (_workflowOptWidget) {
        connect(_workflowOptWidget.data(),
                &DAPyWorkFlowOperateWidget::currentWorkFlowWidgetChanged,
                this,
                &DAPyWorkFlowNodeItemSettingWidget::onWorkFlowEditWidgetChanged);
    }
}
/**
 * @brief 设置节点设置可用
 * @param on
 */
void DAPyWorkFlowNodeItemSettingWidget::setNodeSettingEnable(bool on)
{
    if (!isTabContainWidget(ui->tabNodeSetting)) {
        return;
    }
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabNodeSetting), on);
}
/**
 * @brief 设置item设置可用
 * @param on
 */
void DAPyWorkFlowNodeItemSettingWidget::setItemSettingEnable(bool on)
{
    if (!isTabContainWidget(ui->tabItemSetting)) {
        return;
    }
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabItemSetting), on);
}
/**
 * @brief 设置link设置可用
 * @param on
 */
void DAPyWorkFlowNodeItemSettingWidget::setLinkSettingEnable(bool on)
{
    if (!isTabContainWidget(ui->tabLinkSetting)) {
        return;
    }
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabLinkSetting), on);
}
/**
 * @brief 设置PixmapItem设置可用
 * @param on
 */
void DAPyWorkFlowNodeItemSettingWidget::setPixmapItemSettingEnable(bool on)
{
    if (!isTabContainWidget(ui->tabPictureItemSetting)) {
        return;
    }
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabPictureItemSetting), on);
}

DAPyWorkFlowGraphicsScene* DAPyWorkFlowNodeItemSettingWidget::getCurrentScene() const
{
    if (_workflowEditWidget) {
        return _workflowEditWidget->getWorkFlowGraphicsScene();
    }
    return nullptr;
}

/**
 * @brief 移除tab页
 * @param w
 */
void DAPyWorkFlowNodeItemSettingWidget::removeTab(QWidget* w)
{
    int index = ui->tabWidget->indexOf(w);
    if (index >= 0) {
        ui->tabWidget->removeTab(index);
    }
}

/**
 * @brief 移除节点设置tab，移除后将不显示
 *
 * 某些情况不想让用户看到节点设置tab，可以使用此函数移除掉
 */
void DAPyWorkFlowNodeItemSettingWidget::removeNodeSettingTab()
{
    removeTab(ui->tabNodeSetting);
}

/**
 * @brief 移除元件设置tab，移除后将不显示
 */
void DAPyWorkFlowNodeItemSettingWidget::removeItemSettingTab()
{
    removeTab(ui->tabItemSetting);
}

/**
 * @brief 移除链接设置tab
 */
void DAPyWorkFlowNodeItemSettingWidget::removeLinkSettingTab()
{
    removeTab(ui->tabLinkSetting);
}

/**
 * @brief 移除图片设置窗口
 */
void DAPyWorkFlowNodeItemSettingWidget::removePictureItemSettingTab()
{
    removeTab(ui->tabPictureItemSetting);
}

/**
 * @brief 判断tab是否包含此窗口
 * @param w
 * @return
 */
bool DAPyWorkFlowNodeItemSettingWidget::isTabContainWidget(QWidget* w)
{
    return (ui->tabWidget->indexOf(w) >= 0);
}

/**
 * @brief 节点双击处理：切换到参数tab并设置节点代理
 * @param proxy 双击的节点代理
 */
void DAPyWorkFlowNodeItemSettingWidget::onSceneNodeDoubleClicked(DAPyNodeProxy* proxy)
{
    if (!proxy) {
        return;
    }
    // 设置参数面板的节点代理
    mParamSettingWidget->setNodeProxy(proxy);
    // 切换到参数tab（index 0）
    ui->tabWidget->setCurrentIndex(0);
}

/**
 * @brief 选择改变
 */
void DAPyWorkFlowNodeItemSettingWidget::onSceneSelectionChanged()
{
    DAPyWorkFlowGraphicsScene* scene = getCurrentScene();
    if (nullptr == scene) {
        ui->tabNodeSetting->setNode(nullptr);
        ui->tabItemSetting->setItem(nullptr);
        ui->tabLinkSetting->setLinkItem(nullptr);
        mParamSettingWidget->setNodeProxy(nullptr);
        return;
    }
    QList< QGraphicsItem* > its = scene->selectedItems();
    if (its.empty()) {
        setLinkSettingEnable(false);
        setNodeSettingEnable(false);
        setItemSettingEnable(false);
        setPixmapItemSettingEnable(false);
        mParamSettingWidget->setNodeProxy(nullptr);
        return;
    }
    QGraphicsItem* item = its.last();
    // 此注释给新手：
    //  注意C语言的条件赋值，如果是nullptr就不会执行if内容
    if (DAGraphicsResizeableItem* rsItem = dynamic_cast< DAGraphicsResizeableItem* >(item)) {
        setItemSettingEnable(true);
        if (isTabContainWidget(ui->tabItemSetting)) {
            ui->tabItemSetting->setItem(rsItem);
        }
        // 图片有单独设置
        if (DAGraphicsPixmapItem* pitem = dynamic_cast< DAGraphicsPixmapItem* >(item)) {
            setPixmapItemSettingEnable(true);
            if (isTabContainWidget(ui->tabPictureItemSetting)) {
                ui->tabPictureItemSetting->setItem(pitem);
            }
        } else {
            setPixmapItemSettingEnable(false);
        }
    }
    if (DAPyNodeGraphicsItem* nodeItem = dynamic_cast< DAPyNodeGraphicsItem* >(item)) {
        // 说明是node
        setLinkSettingEnable(false);
        setNodeSettingEnable(true);

        setPixmapItemSettingEnable(false);
        DAPyNodeProxy* proxy = nodeItem->getProxy();
        if (isTabContainWidget(ui->tabNodeSetting)) {
            ui->tabNodeSetting->setNode(proxy);
        }
        // 设置参数面板调度器的节点代理
        mParamSettingWidget->setNodeProxy(proxy);

        if (isTabContainWidget(ui->tabLinkSetting)) {
            ui->tabLinkSetting->setLinkItem(nullptr);
        }
    } else if (DAPyLinkGraphicsItem* linkItem = dynamic_cast< DAPyLinkGraphicsItem* >(item)) {
        // 说明是link
        setLinkSettingEnable(true);
        setNodeSettingEnable(false);
        setItemSettingEnable(false);
        setPixmapItemSettingEnable(false);
        // 选中连线时，移除节点参数配置tab并清理widget
        mParamSettingWidget->setNodeProxy(nullptr);
        if (isTabContainWidget(ui->tabNodeSetting)) {
            ui->tabNodeSetting->setNode(nullptr);
        }
        if (isTabContainWidget(ui->tabItemSetting)) {
            ui->tabItemSetting->setItem(nullptr);
        }
        if (isTabContainWidget(ui->tabLinkSetting)) {
            ui->tabLinkSetting->setLinkItem(linkItem);
        }
    } else {
        // 说明是其他item
        setLinkSettingEnable(false);
        setNodeSettingEnable(false);
        // 选中其他item时，移除节点参数配置tab并清理widget
        mParamSettingWidget->setNodeProxy(nullptr);
        if (isTabContainWidget(ui->tabNodeSetting)) {
            ui->tabNodeSetting->setNode(nullptr);
        }
        if (isTabContainWidget(ui->tabLinkSetting)) {
            ui->tabLinkSetting->setLinkItem(nullptr);
        }
    }
    // 把最后用户点击的页面记录下来并显示
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
void DAPyWorkFlowNodeItemSettingWidget::onSceneItemsPositionChanged(const QList< QGraphicsItem* >& items,
                                                                  const QList< QPointF >& oldPos,
                                                                  const QList< QPointF >& newPos)
{
    Q_UNUSED(oldPos);
    Q_UNUSED(newPos);
    // 刷新item维护的数据
    if (!isTabContainWidget(ui->tabItemSetting)) {
        return;
    }
    if (items.contains(static_cast< QGraphicsItem* >(ui->tabItemSetting->getItem()))) {
        ui->tabItemSetting->updatePosition();
    }
}
/**
 * @brief DAPyWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged
 * @param item
 * @param oldSize
 * @param newSize
 */
void DAPyWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged(DAGraphicsResizeableItem* item,
                                                                 const QSizeF& oldSize,
                                                                 const QSizeF& newSize)
{
    Q_UNUSED(item);
    Q_UNUSED(oldSize);
    Q_UNUSED(newSize);
    if (!isTabContainWidget(ui->tabItemSetting)) {
        return;
    }
    if (ui->tabItemSetting->getItem() == item) {
        ui->tabItemSetting->updateBodySize();
    }
}
/**
 * @brief DAPyWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged
 * @param item
 * @param rotation
 */
void DAPyWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged(DAGraphicsResizeableItem* item, const qreal& rotation)
{
    Q_UNUSED(item);
    Q_UNUSED(rotation);
    if (!isTabContainWidget(ui->tabItemSetting)) {
        return;
    }
    if (ui->tabItemSetting->getItem() == item) {
        ui->tabItemSetting->updateRotation();
    }
}

void DAPyWorkFlowNodeItemSettingWidget::onTabBarCurrentIndexChanged(int index)
{
    _lastTabIndex = index;
}

void DAPyWorkFlowNodeItemSettingWidget::onWorkFlowEditWidgetChanged(DAPyWorkFlowEditWidget* w)
{
    if (w) {
        bindWorkFlowEditWidget(w);
    }
}
