﻿#include "DAWorkFlowNodeItemSettingWidget.h"
#include "ui_DAWorkFlowNodeItemSettingWidget.h"
#include "DAAbstractNode.h"
#include "DAAbstractNodeGraphicsItem.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAGraphicsPixmapItemSettingWidget.h"
#include "DAGraphicsStandardTextItem.h"
#include "DAWorkFlowEditWidget.h"
#include "DAGraphicsPixmapItem.h"
#include "DAAbstractNodeWidget.h"
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
}

void DAWorkFlowNodeItemSettingWidget::bindWorkFlowEditWidget(DAWorkFlowEditWidget* w)
{
    if (_workflowEditWidget) {
        DAWorkFlowGraphicsScene* s = _workflowEditWidget->getWorkFlowGraphicsScene();
        disconnect(s, &DAWorkFlowGraphicsScene::selectionChanged, this, &DAWorkFlowNodeItemSettingWidget::onSceneSelectionChanged);
        disconnect(s,
                   &DAWorkFlowGraphicsScene::itemsPositionChanged,
                   this,
                   &DAWorkFlowNodeItemSettingWidget::onSceneItemsPositionChanged);
        disconnect(s,
                   &DAWorkFlowGraphicsScene::itemBodySizeChanged,
                   this,
                   &DAWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged);
        disconnect(s,
                   &DAWorkFlowGraphicsScene::itemRotationChanged,
                   this,
                   &DAWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged);
    }
    _workflowEditWidget = w;
    if (_workflowEditWidget) {
        DAWorkFlowGraphicsScene* s = _workflowEditWidget->getWorkFlowGraphicsScene();
        connect(s, &DAWorkFlowGraphicsScene::selectionChanged, this, &DAWorkFlowNodeItemSettingWidget::onSceneSelectionChanged);
        connect(s,
                &DAWorkFlowGraphicsScene::itemsPositionChanged,
                this,
                &DAWorkFlowNodeItemSettingWidget::onSceneItemsPositionChanged);
        connect(s,
                &DAWorkFlowGraphicsScene::itemBodySizeChanged,
                this,
                &DAWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged);
        connect(s,
                &DAWorkFlowGraphicsScene::itemRotationChanged,
                this,
                &DAWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged);
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

/**
 * @brief 移除设置页
 * @note 注意，页面并不会被删除
 * @param w
 */
void DAWorkFlowNodeItemSettingWidget::removeWidget(QWidget* w)
{
    int index = tabWidget()->indexOf(w);
    if (index >= 0) {
        tabWidget()->removeTab(index);
    }
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
    if (!isTabContainWidget(ui->tabNodeSetting)) {
        return;
    }
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabNodeSetting), on);
}
/**
 * @brief 设置item设置可用
 * @param on
 */
void DAWorkFlowNodeItemSettingWidget::setItemSettingEnable(bool on)
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
void DAWorkFlowNodeItemSettingWidget::setLinkSettingEnable(bool on)
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
void DAWorkFlowNodeItemSettingWidget::setPixmapItemSettingEnable(bool on)
{
    if (!isTabContainWidget(ui->tabPictureItemSetting)) {
        return;
    }
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
 * @brief 移除tab页
 * @param w
 */
void DAWorkFlowNodeItemSettingWidget::removeTab(QWidget* w)
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
void DAWorkFlowNodeItemSettingWidget::removeNodeSettingTab()
{
    removeTab(ui->tabNodeSetting);
}

/**
 * @brief 移除元件设置tab，移除后将不显示
 */
void DAWorkFlowNodeItemSettingWidget::removeItemSettingTab()
{
    removeTab(ui->tabItemSetting);
}

/**
 * @brief 移除链接设置tab
 */
void DAWorkFlowNodeItemSettingWidget::removeLinkSettingTab()
{
    removeTab(ui->tabLinkSetting);
}

/**
 * @brief 移除图片设置窗口
 */
void DAWorkFlowNodeItemSettingWidget::removePictureItemSettingTab()
{
    removeTab(ui->tabPictureItemSetting);
}

/**
 * @brief 判断tab是否包含此窗口
 * @param w
 * @return
 */
bool DAWorkFlowNodeItemSettingWidget::isTabContainWidget(QWidget* w)
{
    return (ui->tabWidget->indexOf(w) >= 0);
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
    if (DAAbstractNodeGraphicsItem* nodeItem = dynamic_cast< DAAbstractNodeGraphicsItem* >(item)) {
        // 说明是node
        setLinkSettingEnable(false);
        setNodeSettingEnable(true);

        setPixmapItemSettingEnable(false);
        if (isTabContainWidget(ui->tabNodeSetting)) {
            ui->tabNodeSetting->setNode(nodeItem->node());
        }

        if (isTabContainWidget(ui->tabLinkSetting)) {
            ui->tabLinkSetting->setLinkItem(nullptr);
        }
        //! 节点选择发生了变化,
        //! 获取节点对应的设置窗口，把设置窗口作为一个tab设置进去
        DAAbstractNodeWidget* w = nodeItem->getNodeWidget();
        if (nullptr == w) {
            // 没有设置窗口，看看当前是否显示着设置窗口，如果有就取消掉
            if (nullptr != mLastSetNodeWidget) {
                removeWidget(mLastSetNodeWidget.data());
            }
        } else {
            // 设置窗口不为空，就把
            if (mLastSetNodeWidget != w) {
                removeWidget(mLastSetNodeWidget.data());
                addWidget(w, QIcon(":/DAGui/icon/node-settting.svg"), tr("property"));
            }
        }
        mLastSetNodeWidget = w;
    } else if (DAAbstractNodeLinkGraphicsItem* linkItem = dynamic_cast< DAAbstractNodeLinkGraphicsItem* >(item)) {
        // 说明是link
        setLinkSettingEnable(true);
        setNodeSettingEnable(false);
        setItemSettingEnable(false);
        setPixmapItemSettingEnable(false);
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
void DAWorkFlowNodeItemSettingWidget::onSceneItemsPositionChanged(const QList< QGraphicsItem* >& items,
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
 * @brief DAWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged
 * @param item
 * @param oldSize
 * @param newSize
 */
void DAWorkFlowNodeItemSettingWidget::onSceneItemBodySizeChanged(DAGraphicsResizeableItem* item,
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
 * @brief DAWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged
 * @param item
 * @param rotation
 */
void DAWorkFlowNodeItemSettingWidget::onSceneItemRotationChanged(DAGraphicsResizeableItem* item, const qreal& rotation)
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
