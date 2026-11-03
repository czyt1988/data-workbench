// DABadgeDockComponentsFactory.cpp
#include "DABadgeDockComponentsFactory.h"
#include "DABadgeAutoHideTab.h"
// ADS（DockWidget 需完整定义：构造参数 ads::CDockWidget* 上行转换为 QWidget*）
#include "DockWidget.h"

namespace DA
{

/**
 * @brief 创建侧边栏选项卡
 * @param dockWidget ADS 传入的 dock 指针（当前实现传 nullptr，无法按 dock 区分，
 *                   DABadgeAutoHideTab 绑定 dock 后按静态名单决定是否激活徽章）
 * @return DABadgeAutoHideTab 实例（所有权归 ADS 侧边栏）
 */
ads::CAutoHideTab* DABadgeDockComponentsFactory::createDockWidgetSideTab(ads::CDockWidget* dockWidget) const
{
    return new DABadgeAutoHideTab(dockWidget);
}
}  // namespace DA
