#ifndef DAAGENTMANAGER_H
#define DAAGENTMANAGER_H
#include "DAAgentAPI.h"
#include "DAAgentPromptOps.h"
#include <QObject>
#include <QList>
#include <QString>

namespace DA
{
/**
 * @brief Agent 提示词库管理器，负责从 <exe>/daAgent 目录加载/保存/删除 agent md 文件
 *
 * 作为 DAAgentPromptOps 的实现，由 DAAgentModule 组合持有，经
 * DAAgentInterface::agentPromptOps() 以 DAAgentPromptOps* 暴露给 DAGui 层对话框。
 * 首次启动若目录为空，从内置 qrc 资源播种通用默认 agent；插件可通过
 * registerBuiltin 注入领域提示词（仅当对应文件不存在时写入，尊重用户已有编辑）。
 */
class DAAgent_API DAAgentManager : public QObject, public DAAgentPromptOps
{
    Q_OBJECT
public:
    explicit DAAgentManager(QObject* parent = nullptr);

    /// agent 所在目录（<exe>/daAgent）
    QString agentDir() const;

    /// 扫描目录加载所有 *.md，按文件名排序
    void loadAgents();

    /// 当前加载的 agent 列表
    QList<DAAgentPrompt> agents() const;

    /// 按标题查找 agent，找不到返回 nullptr
    const DAAgentPrompt* findAgent(const QString& title) const;

    // ---- DAAgentPromptOps 实现 ----
    QList<DAAgentPrompt> agentPrompts() const override;
    bool saveAgent(const QString& title, const QString& content, const QString& oldTitle = QString()) override;
    bool deleteAgent(const QString& title) override;

    /// 若 daAgent 目录不存在或无任何 md，则从 qrc 播种通用默认 agent
    void ensureDefaultAgent();

    /// 注册内置 agent：仅当 <daAgent>/<name>.md 不存在时写入（尊重用户编辑）
    void registerBuiltin(const QString& name, const QString& content);

    /// 通用默认 agent 的显示标题
    static const QString DEFAULT_AGENT_TITLE;

Q_SIGNALS:
    /// agent 列表发生变化（加载/保存/删除/注入后发射），供 UI 刷新 gallery
    void agentListChanged();

private:
    QList<DAAgentPrompt> m_agents;
};
} // namespace DA

#endif // DAAGENTMANAGER_H
