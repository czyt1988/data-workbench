#include "DAAppContextRoutePolicy.h"
#include <QWidget>

namespace DA
{
/**
 * @brief 解析 dock 内容激活后的上下文路由目标
 *
 * 判定顺序：插件设置上下文提供者优先（命中后由 provider 接管、不执行 hide-all），
 * 其次依次比较固定的数据操作/工作流操作/绘图操作/数据管理窗口，其余归为工具 dock。
 * @param widget 被激活的 dock 内容窗口
 * @param hasSettingContextProvider 该内容是否注册了设置上下文提供者
 * @param dataOperateWidget 数据操作窗口指针
 * @param workFlowOperateWidget 工作流操作窗口指针
 * @param chartOperateWidget 绘图操作窗口指针
 * @param dataManageWidget 数据管理窗口指针
 * @return 路由目标
 */
DAAppContextRoutePolicy resolveAppContextRoutePolicy(QWidget* widget,
                                                     bool hasSettingContextProvider,
                                                     QWidget* dataOperateWidget,
                                                     QWidget* workFlowOperateWidget,
                                                     QWidget* chartOperateWidget,
                                                     QWidget* dataManageWidget)
{
    if (nullptr == widget) {
        return DAAppContextRoutePolicy::UtilityDock;
    }
    if (hasSettingContextProvider) {
        return DAAppContextRoutePolicy::SettingContextProvider;
    }
    if (widget == dataOperateWidget) {
        return DAAppContextRoutePolicy::DataOperate;
    }
    if (widget == workFlowOperateWidget) {
        return DAAppContextRoutePolicy::WorkFlowOperate;
    }
    if (widget == chartOperateWidget) {
        return DAAppContextRoutePolicy::ChartOperate;
    }
    if (widget == dataManageWidget) {
        return DAAppContextRoutePolicy::DataManage;
    }
    return DAAppContextRoutePolicy::UtilityDock;
}

}  // namespace DA
