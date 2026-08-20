#ifndef DAFIGURECOMPONENTSFACTORY_H
#define DAFIGURECOMPONENTSFACTORY_H
#include "DAGuiAPI.h"
// ADS
#include "DockComponentsFactory.h"
namespace DA
{
/**
 * @brief 绘图 dock 自定义组件工厂
 *
 * 继承 ads::CDockComponentsFactory，重写 createDockWidgetTab 返回
 * DAFigureDockWidgetTab，使绘图 dock 的选项卡右键菜单携带自定义 action
 * （如重命名）。由 DAChartOperateWidget 注册到其嵌套 ads::CDockManager，
 * 仅影响绘图 dock。
 */
class DAGUI_API DAFigureComponentsFactory : public ads::CDockComponentsFactory
{
public:
    using Super = ads::CDockComponentsFactory;
    // 创建绘图 dock 专用的自定义选项卡
    ads::CDockWidgetTab* createDockWidgetTab(ads::CDockWidget* dockWidget) const override;
};
}  // namespace DA
#endif  // DAFIGURECOMPONENTSFACTORY_H
