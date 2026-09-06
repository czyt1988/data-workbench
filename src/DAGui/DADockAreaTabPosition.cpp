#include "DADockAreaTabPosition.h"
#include <QBoxLayout>
#include <QPointer>
// Qt-Advanced-Docking-System
#include "DockAreaWidget.h"
#include "DockAreaTitleBar.h"
#include "DockAreaTabBar.h"
#include "DockManager.h"
#include "ElidingLabel.h"

namespace DA
{

// manager 关联属性：当前标签页方位（true=底部）
static const char* const cInnerDockTabsAtBottomProp = "daInnerDockTabsAtBottom";
// manager 关联属性：dockAreaCreated 信号是否已绑定（保证只 connect 一次）
static const char* const cInnerDockTabsBoundProp = "daInnerDockTabsBound";

/**
 * @brief 把单个 dock 区域的标签栏移到底部（内容下方）
 *
 * ADS 原生 TabsAtBottom 模式下标签栏位于 dock 区域主布局末尾（内容容器恒插入
 * index 1，末尾即内容下方），且标题栏显示 autoHideTitleLabel 作为标题。此处按
 * 相同结构做布局调整，与原生模式的观感保持一致。
 * @param dockArea
 */
void DADockAreaTabPosition::moveTabsToBottom(ads::CDockAreaWidget* dockArea)
{
    if (nullptr == dockArea) {
        return;
    }
    ads::CDockAreaTitleBar* titleBar = dockArea->titleBar();
    if (nullptr == titleBar) {
        return;
    }
    ads::CDockAreaTabBar* tabBar = titleBar->tabBar();
    if (nullptr == tabBar) {
        return;
    }
    QBoxLayout* titleLayout = qobject_cast< QBoxLayout* >(titleBar->layout());
    QBoxLayout* areaLayout  = qobject_cast< QBoxLayout* >(dockArea->layout());
    if (nullptr == titleLayout || nullptr == areaLayout) {
        return;
    }
    if (areaLayout->indexOf(tabBar) >= 0) {
        // 已位于底部
        return;
    }
    titleLayout->removeWidget(tabBar);
    areaLayout->addWidget(tabBar);
    tabBar->show();
    if (ads::CElidingLabel* label = titleBar->autoHideTitleLabel()) {
        label->show();
    }
}

/**
 * @brief 把单个 dock 区域的标签栏恢复到顶部（标题栏内）
 *
 * ADS 构建标题栏时 TabBar 是标题栏布局的第一项（createTabBar 先于
 * createAutoHideTitleLabel 执行），恢复时插回 index 0 即还原原始结构。
 * @param dockArea
 */
void DADockAreaTabPosition::moveTabsToTop(ads::CDockAreaWidget* dockArea)
{
    if (nullptr == dockArea) {
        return;
    }
    ads::CDockAreaTitleBar* titleBar = dockArea->titleBar();
    if (nullptr == titleBar) {
        return;
    }
    ads::CDockAreaTabBar* tabBar = titleBar->tabBar();
    if (nullptr == tabBar) {
        return;
    }
    QBoxLayout* titleLayout = qobject_cast< QBoxLayout* >(titleBar->layout());
    QBoxLayout* areaLayout  = qobject_cast< QBoxLayout* >(dockArea->layout());
    if (nullptr == titleLayout || nullptr == areaLayout) {
        return;
    }
    if (titleLayout->indexOf(tabBar) >= 0) {
        // 已位于标题栏
        return;
    }
    areaLayout->removeWidget(tabBar);
    titleLayout->insertWidget(0, tabBar);
    tabBar->show();
    if (ads::CElidingLabel* label = titleBar->autoHideTitleLabel()) {
        label->hide();
    }
}

/**
 * @brief 按 atBottom 设置单个 dock 区域的标签页方位
 * @param dockArea
 * @param atBottom true=底部，false=顶部
 */
void DADockAreaTabPosition::applyToDockArea(ads::CDockAreaWidget* dockArea, bool atBottom)
{
    if (atBottom) {
        moveTabsToBottom(dockArea);
    } else {
        moveTabsToTop(dockArea);
    }
}

/**
 * @brief 应用到 manager：重定位已有 dock 区域，之后新建的区域自动应用当前方位
 *
 * 通过 manager 的动态属性记录当前方位，dockAreaCreated 信号只绑定一次，
 * 后续调用仅更新属性值并重定位已存在的区域，可安全重复调用（含运行期切换）。
 * @param dockManager
 * @param atBottom true=底部，false=顶部
 */
void DADockAreaTabPosition::applyToDockManager(ads::CDockManager* dockManager, bool atBottom)
{
    if (nullptr == dockManager) {
        return;
    }
    dockManager->setProperty(cInnerDockTabsAtBottomProp, atBottom);
    if (!dockManager->property(cInnerDockTabsBoundProp).toBool()) {
        dockManager->setProperty(cInnerDockTabsBoundProp, true);
        QPointer< ads::CDockManager > guard(dockManager);
        QObject::connect(dockManager, &ads::CDockManager::dockAreaCreated, dockManager, [guard](ads::CDockAreaWidget* dockArea) {
            if (guard) {
                applyToDockArea(dockArea, guard->property(cInnerDockTabsAtBottomProp).toBool());
            }
        });
    }
    // 重定位已存在的 dock 区域（含配置运行期切换的场景）
    const QList< ads::CDockAreaWidget* > dockAreas = dockManager->openedDockAreas();
    for (ads::CDockAreaWidget* dockArea : dockAreas) {
        applyToDockArea(dockArea, atBottom);
    }
}
}  // namespace DA
