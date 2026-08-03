// DAAgentModule.h
#pragma once
#include "DAAgentInterface.h"
#include "DACoreInterface.h"
#include <QHash>
#include <QMap>
#include <QStringList>
#include "DAAbstractAgentTool.h"

namespace DA
{
class DAAgentBridge;
class DAAgentDockWidget;

/**
 * @brief DAAgent 模块的完整实现：工具注册、系统提示词组装、懒启动生命周期管理
 *
 * DAAgentModule 作为 DAAgentInterface 的实现类，协调 DAAgentBridge（子进程管理）
 * 和 DAAgentDockWidget（聊天 UI）之间的信号链。插件通过 registerTool /
 * registerSystemPrompt 注入领域内容。
 */
class DAAgent_API DAAgentModule : public DAAgentInterface
{
    Q_OBJECT
public:
    explicit DAAgentModule(DACoreInterface* core, QObject* parent = nullptr);
    ~DAAgentModule();
    // 辅助方法（不属于 DAAgentInterface）：注入核心接口
    void initialize(DACoreInterface* core);
    // 注入由 DAAppDockingArea::buildDockingArea()（plan-03）创建的 Dock Widget。
    // initialize() 不再 new DAAgentDockWidget，避免与 plan-03 产生两个实例。
    void setDockWidget(DAAgentDockWidget* dock);
    // DAAgentInterface overrides
    void registerTool(DAAbstractAgentTool* tool) override;
    void registerSystemPrompt(const QString& name, const QString& content) override;
    void showDockWidget() override;
    void hideDockWidget() override;
    void sendMessage(const QString& text) override;
    bool isRunning() const override;
    QJsonObject getLLMConfig() const override;
    void setLLMConfig(const QJsonObject& config) override;
private:
    DACoreInterface* m_core;
    DAAgentBridge* m_bridge = nullptr;
    DAAgentDockWidget* m_dockWidget = nullptr;  // 由 DAAppDockingArea::buildDockingArea 创建，经 setDockWidget() 注入
    QMap<QString, DAAbstractAgentTool*> m_tools;        // tool name → impl
    QHash<QString, QString> m_systemPrompts;            // prompt name → content
    // Helper methods（plan-04 填充实现）
    void connectSignals();
    void registerBuiltinTools();      // plan-05 填充真实工具注册
    void startAgentInternal();
    QString assembleSystemPrompt() const;
    QJsonArray assembleToolSpecs() const;
    // Python 解释器与 agent 脚本路径解析（plan-04 §4）
    QString detectPythonExePath() const;
    QString detectAgentScriptPath() const;
};
} // namespace DA
