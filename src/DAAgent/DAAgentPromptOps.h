#ifndef DAAGENTPROMPTOPS_H
#define DAAGENTPROMPTOPS_H
#include "DAAgentAPI.h"
#include <QList>
#include <QString>
#include "DAAgentPrompt.h"

namespace DA
{
/**
 * @brief Agent 提示词库操作回调接口
 *
 * 由 DAAgentManager（DAAgent 模块）实现，经 DAAgentInterface::agentPromptOps()
 * 以 DAAgentPromptOps* 暴露。DAGui 层的管理对话框通过此接口执行提示词的增删改查，
 * 从而在不依赖 DAAgent 模块的前提下完成 CRUD（遵循 DAGui↔DAAgent 互不依赖的
 * 既有约定，由 APP 层注入实例）。
 *
 * 纯虚接口；析构函数定义在 DAUtils 模块（DAAgentPromptOps.cpp），vtable 锚定在
 * DAUtils DLL，跨 DLL 通过指针调用安全。
 */
class DAAgent_API DAAgentPromptOps
{
public:
    virtual ~DAAgentPromptOps();

    /// 当前所有 agent 提示词
    virtual QList<DAAgentPrompt> agentPrompts() const = 0;

    /// 保存（新增或更新）agent，title 变更时 oldTitle 用于定位旧文件；成功返回 true
    virtual bool saveAgent(const QString& title, const QString& content, const QString& oldTitle = QString()) = 0;

    /// 删除指定标题的 agent；成功返回 true
    virtual bool deleteAgent(const QString& title) = 0;
};
} // namespace DA

#endif // DAAGENTPROMPTOPS_H
