// DAAgentSessionComponentsFactory.cpp
#include "DAAgentSessionComponentsFactory.h"
#include "DAAgentDockWidget.h"
#include "DAAgentDockAreaTitleBar.h"
#include "DAAgentSessionDockWidgetTab.h"

namespace DA
{

DAAgentSessionComponentsFactory::DAAgentSessionComponentsFactory(DAAgentDockWidget* host)
    : mHost(host)
{
}

DAAgentSessionComponentsFactory::~DAAgentSessionComponentsFactory()
{
}

/**
 * @brief 创建会话区域标题栏（带「+/会话管理」按钮）
 * @param dockArea 所属 dock area
 * @return DAAgentDockAreaTitleBar 实例（所有权归 ADS dock area）
 */
ads::CDockAreaTitleBar* DAAgentSessionComponentsFactory::createDockAreaTitleBar(ads::CDockAreaWidget* dockArea) const
{
    return new DAAgentDockAreaTitleBar(dockArea, mHost);
}

/**
 * @brief 创建会话 dock 选项卡（右键菜单携带会话操作）
 * @param dockWidget 所属 dock
 * @return DAAgentSessionDockWidgetTab 实例（所有权归 ADS dock）
 */
ads::CDockWidgetTab* DAAgentSessionComponentsFactory::createDockWidgetTab(ads::CDockWidget* dockWidget) const
{
    return new DAAgentSessionDockWidgetTab(dockWidget);
}

}  // namespace DA
