#include "DAFigureComponentsFactory.h"
#include "DAFigureDockWidgetTab.h"
namespace DA
{
/**
 * @brief 创建绘图 dock 专用的自定义选项卡
 * @param dockWidget 所属 dock
 * @return DAFigureDockWidgetTab 实例（由调用方父级接管）
 */
ads::CDockWidgetTab* DAFigureComponentsFactory::createDockWidgetTab(ads::CDockWidget* dockWidget) const
{
    return new DAFigureDockWidgetTab(dockWidget);
}

}  // namespace DA
