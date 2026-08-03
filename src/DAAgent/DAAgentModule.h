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
    /**
     * @brief 构造函数
     * @param core 核心接口指针
     * @param parent 父对象
     */
    explicit DAAgentModule(DACoreInterface* core, QObject* parent = nullptr);
    /**
     * @brief 析构函数
     */
    ~DAAgentModule();

    /**
     * @brief 使用核心接口初始化模块
     * @param core 核心接口指针
     */
    void initialize(DACoreInterface* core);

    /**
     * @brief 注入由 DAAppDockingArea::buildDockingArea() 创建的 Dock Widget
     *
     * initialize() 不再 new DAAgentDockWidget，避免与 plan-03 产生两个实例。
     * @param dock 由 DAAppDockingArea 创建的 dock 窗口
     */
    void setDockWidget(DAAgentDockWidget* dock);

    /// @copydoc DAAgentInterface::registerTool
    void registerTool(DAAbstractAgentTool* tool) override;
    /// @copydoc DAAgentInterface::registerSystemPrompt
    void registerSystemPrompt(const QString& name, const QString& content) override;
    /// @copydoc DAAgentInterface::showDockWidget
    void showDockWidget() override;
    /// @copydoc DAAgentInterface::hideDockWidget
    void hideDockWidget() override;
    /// @copydoc DAAgentInterface::sendMessage
    void sendMessage(const QString& text) override;
    /// @copydoc DAAgentInterface::isRunning
    bool isRunning() const override;
    /// @copydoc DAAgentInterface::getLLMConfig
    QJsonObject getLLMConfig() const override;
    /// @copydoc DAAgentInterface::setLLMConfig
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
