#ifndef DAAGENTSESSIONCOMPONENTSFACTORY_H
#define DAAGENTSESSIONCOMPONENTSFACTORY_H
#include "DAGuiAPI.h"
// ADS
#include "DockComponentsFactory.h"

namespace DA
{
class DAAgentDockWidget;

/**
 * @brief Agent 会话 dock 自定义组件工厂
 *
 * 继承 ads::CDockComponentsFactory，重写 createDockAreaTitleBar 返回
 * DAAgentDockAreaTitleBar（标题栏带「+/会话管理」按钮）、createDockWidgetTab
 * 返回 DAAgentSessionDockWidgetTab（右键菜单：重命名/停止/删除会话）。
 *
 * 由 DAAgentDockWidget 注册到其嵌套 ads::CDockManager，仅影响 Agent 会话
 * dock，不影响顶层停靠区。工厂持有宿主指针（QPointer 防悬空，正常生命周期
 * 内宿主与嵌套管理器同生共死）。
 */
class DAGUI_API DAAgentSessionComponentsFactory : public ads::CDockComponentsFactory
{
public:
    using Super = ads::CDockComponentsFactory;
    explicit DAAgentSessionComponentsFactory(DAAgentDockWidget* host);
    ~DAAgentSessionComponentsFactory() override;
    // 会话 dock 区域标题栏（带「+/会话管理」按钮）
    ads::CDockAreaTitleBar* createDockAreaTitleBar(ads::CDockAreaWidget* dockArea) const override;
    // 会话 dock 选项卡（右键菜单携带会话操作）
    ads::CDockWidgetTab* createDockWidgetTab(ads::CDockWidget* dockWidget) const override;

private:
    DAAgentDockWidget* mHost;  ///< 宿主（按钮/菜单入口），弱引用语义（不持有所有权）
};
}  // namespace DA
#endif  // DAAGENTSESSIONCOMPONENTSFACTORY_H
