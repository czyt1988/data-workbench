#ifndef DAAPPCONTEXTROUTEPOLICY_H
#define DAAPPCONTEXTROUTEPOLICY_H
class QWidget;

namespace DA
{
/**
 * @brief dock 内容激活后的上下文路由目标
 */
enum class DAAppContextRoutePolicy
{
    SettingContextProvider,  ///< 插件注册的设置上下文提供者接管（不执行 hide-all）
    DataOperate,             ///< 数据操作窗口
    WorkFlowOperate,         ///< 工作流操作窗口
    ChartOperate,            ///< 绘图操作窗口
    DataManage,              ///< 数据管理窗口
    UtilityDock              ///< 其它工具 dock（设置/日志/图元树等）：不切换上下文
};

/**
 * @brief 解析 dock 内容激活后的上下文路由目标
 *
 * 纯函数、不依赖界面对象，便于单元测试。nullptr 内容由调用方先走「失活 + hide-all」
 * 分支，本函数对 nullptr 返回 UtilityDock。
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
                                                     QWidget* dataManageWidget);
}  // namespace DA
#endif  // DAAPPCONTEXTROUTEPOLICY_H
