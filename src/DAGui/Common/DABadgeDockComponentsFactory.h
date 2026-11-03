#ifndef DABADGEDOCKCOMPONENTSFACTORY_H
#define DABADGEDOCKCOMPONENTSFACTORY_H
#include "DAGuiAPI.h"
// ADS
#include "DockComponentsFactory.h"

namespace DA
{
/**
 * @brief 带徽章能力的 ADS 组件工厂
 *
 * 继承 ads::CDockComponentsFactory，仅将侧边栏选项卡（auto-hide side tab）
 * 替换为 DABadgeAutoHideTab（带未读计数徽章；未命中名单时绘制行为与
 * ads::CAutoHideTab 完全一致），其余组件全部转发 ADS 默认实现。
 *
 * 经 ads::CDockManager::setComponentsFactory 注册到单个 dock manager，
 * 只影响该 manager（嵌套 dock manager 使用各自/默认工厂，互不影响）。
 * 必须在创建任何 dock widget 之前注册。
 */
class DAGUI_API DABadgeDockComponentsFactory : public ads::CDockComponentsFactory
{
public:
    using Super = ads::CDockComponentsFactory;
    DABadgeDockComponentsFactory() = default;
    ~DABadgeDockComponentsFactory() override = default;

    // 侧边栏选项卡：返回带徽章能力的 tab（ADS 调用时 dock 参数为 nullptr，延迟按名单激活）
    ads::CAutoHideTab* createDockWidgetSideTab(ads::CDockWidget* dockWidget) const override;
    // 其余组件（普通选项卡/选项卡条/区域标题栏）走 ADS 默认实现
};
}  // namespace DA
#endif  // DABADGEDOCKCOMPONENTSFACTORY_H
