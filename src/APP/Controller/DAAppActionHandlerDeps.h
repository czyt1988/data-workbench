#ifndef DAAPPACTIONHANDLERDEPS_H
#define DAAPPACTIONHANDLERDEPS_H

namespace DA
{
class DAAppController;
class DAAppRibbonArea;
class DAAppDockingArea;
class DAAppCommand;
class DAAppDataManager;
class DAAppConfig;
class DAAppActions;
class AppMainWindow;

/**
 * @brief 动作分发器共享的应用上下文依赖
 *
 * 分发器只通过这里拿到协作者，不持有 DAAppController 的其他分发器，
 * 依赖图保持以 DAAppController 为中心的星形：控制器负责装配与跨域协调，
 * 分发器负责把某一域的动作转成对当前活动对象的调用。
 *
 * @note controller 承担"当前活动对象查询 + 脏标志 + 焦点判定"这三件协调者职责，
 *       分发器不复制这些逻辑，统一经它调用。
 */
struct DAAppActionHandlerDeps
{
    DAAppController* controller { nullptr };  ///< 当前对象查询、setDirty 等协调者门面
    DAAppActions* actions { nullptr };        ///< 动作注册表
    DAAppRibbonArea* ribbon { nullptr };      ///< Ribbon 区域（控件状态回写）
    DAAppDockingArea* dock { nullptr };       ///< 停靠区域（显隐、raise、取子窗口）
    DAAppCommand* command { nullptr };        ///< 撤销命令组
    DAAppDataManager* datas { nullptr };      ///< 数据集管理
    DAAppConfig* config { nullptr };          ///< 应用配置
    AppMainWindow* mainWindow { nullptr };    ///< 主窗口（对话框父窗口）
};
}  // namespace DA

#endif  // DAAPPACTIONHANDLERDEPS_H
