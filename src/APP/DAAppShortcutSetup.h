#ifndef DAAPPSHORTCUTSETUP_H
#define DAAPPSHORTCUTSETUP_H

namespace DA
{
class DAShortcutManager;
class DAAppActions;
class DAPyWorkFlowOperateWidget;

/**
 * @brief 把主程序与工作流编辑器的可配置命令注册进快捷键管理器
 *
 * 命令 ID 为 action 的 objectName（契约常量见 DAUiObjectNames.h），默认快捷键在各 action
 * 创建处就地设置，注册时由管理器自动收集。分类使用稳定英文键（Main/Data/Figure/Chart/View/
 * Workflow/WorkflowEditor/Other），显示文本由设置页翻译；注册表中未列入分类表的 action
 * （如颜色主题、插件动态注册的 action）归入 Other。
 *
 * 后续扩展：新窗口或插件的命令只要 QAction 带唯一 objectName，经 @ref DAShortcutManager::attachAction
 * 注册即可出现在「设置-快捷键」页
 * @param mgr 快捷键管理器
 * @param actions 主程序 action 管理器（可为 nullptr）
 * @param wfo 工作流操作窗口（可为 nullptr）
 * @return 成功注册的命令数量
 */
int setupAppShortcutCommands(DAShortcutManager* mgr, DAAppActions* actions, DAPyWorkFlowOperateWidget* wfo);
}
#endif  // DAAPPSHORTCUTSETUP_H
