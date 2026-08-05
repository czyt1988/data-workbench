// DAAgentModule.cpp
#include "DAAgentModule.h"
#include "DAAgentBridge.h"
#include "DAAgentDockWidget.h"
#include "DAAbstractAgentTool.h"
#include "DAAgentInterface.h"
#include "DACoreInterface.h"
#include "DAPyInterpreter.h"
#include "DAAgentSettingsWidget.h"
#include "DADir.h"
#include "DALogCategory.h"
// Platform built-in tools (plan-05)
#include "tools/DAAgentToolListData.h"
#include "tools/DAAgentToolDataInfo.h"
#include "tools/DAAgentToolQueryData.h"
#include "tools/DAAgentToolColumnStats.h"
#include "tools/DAAgentToolExportData.h"
#include "tools/DAAgentToolCreateChart.h"
#include "tools/DAAgentToolAddCurve.h"
#include "tools/DAAgentToolSetChartStyle.h"
#include "tools/DAAgentToolAddAnnotation.h"
#include "tools/DAAgentToolAddRegion.h"
#include "tools/DAAgentToolCreateSubplots.h"
#include "tools/DAAgentToolSaveChartImage.h"
#include "tools/DAAgentToolListFigures.h"
#include "tools/DAAgentToolReadFile.h"
#include "tools/DAAgentToolWriteFile.h"
#include "tools/DAAgentToolSaveReport.h"
#include <QFile>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QStandardPaths>

namespace DA
{

DAAgentModule::DAAgentModule(DACoreInterface* core, QObject* parent)
    : DAAgentInterface(parent), m_core(core) {}

DAAgentModule::~DAAgentModule() {}

void DAAgentModule::initialize(DACoreInterface* core)
{
    m_core = core;

    // 不在此 new DAAgentDockWidget——Dock 由 DAAppDockingArea::buildDockingArea()
    // （plan-03）创建，并通过 setDockWidget() 回调注入。若此处再 new，会与
    // plan-03 产生两个 Dock 实例，本模块的信号链将连到隐藏的（未注册的）那个。

    // 创建 Bridge（不依赖 Dock 存在）
    m_bridge = new DAAgentBridge(this);

    // 连接信号链——connectSignals 内部对 m_dockWidget 为空时提前返回，
    // 真正的连接在 setDockWidget() 传入 Dock 后完成
    connectSignals();

    // 注册平台内置工具（plan-05；未完成时为空体）
    registerBuiltinTools();
}

void DAAgentModule::setDockWidget(DAAgentDockWidget* dock)
{
    // 由 DAAppDockingArea::buildDockingArea()（plan-03）在创建 Dock 后回调注入。
    m_dockWidget = dock;
    // Dock 就绪后重新连接信号链（connectSignals 在 dock 为空时会提前返回）
    connectSignals();
}

void DAAgentModule::registerTool(DAAbstractAgentTool* tool)
{
    QString name = tool->getToolSpec()["name"].toString();
    m_tools[name] = tool;
    if (m_bridge) m_bridge->setTools(m_tools);
}

void DAAgentModule::registerSystemPrompt(const QString& name, const QString& content)
{
    m_systemPrompts[name] = content;
}

QString DAAgentModule::assembleSystemPrompt() const
{
    // 平台基础提示词
    QString base = R"(你是 data-workbench 的 AI 数据分析助手。
你可以使用提供的工具来查询数据、绘制图表、分析数据。
请使用 markdown 格式输出你的回复。
当需要用户提供信息时，使用 ask_user 工具提问。)";

    // 拼接插件注入的提示词
    QStringList parts;
    parts << base;
    for (auto it = m_systemPrompts.begin(); it != m_systemPrompts.end(); ++it) {
        parts << it.value();
    }
    return parts.join("\n\n");
}

QJsonArray DAAgentModule::assembleToolSpecs() const
{
    QJsonArray specs;
    for (auto* tool : m_tools) {
        specs.append(tool->getToolSpec());
    }
    return specs;
}

void DAAgentModule::sendMessage(const QString& text)
{
    // 注意：不要在此 emit agentBusy(true)——DAAgentModule.h 无 signals: 段，
    // DAAgentInterface 也未声明 agentBusy 信号，此处 emit 无法编译。
    // busy 状态改由 DAAgentBridge::sendMessage() 统一发射：Bridge 的 agentBusy 信号
    // 已在 connectSignals() 中连接到 DAAgentDockWidget::onAgentBusy。
    if (!m_bridge->isRunning()) {
        // 懒启动
        startAgentInternal();
    }
    m_bridge->sendMessage(text);
}

void DAAgentModule::startAgentInternal()
{
    // 获取 LLM 配置（plan-06 提供真实实现）
    QJsonObject config = getLLMConfig();

    // 通过 detect 方法解析路径（不依赖 config 是否包含这些键）
    QString pythonExe = detectPythonExePath();
    QString scriptPath = detectAgentScriptPath();

    // 路径缺失时提前返回并报错——daCritical 会路由到 UI 日志窗口
    if (pythonExe.isEmpty()) {
        daCritical << tr("无法找到 Python 解释器路径，请在设置页配置 Python 解释器");
        return;
    }
    if (scriptPath.isEmpty() || !QFile::exists(scriptPath)) {
        daCritical << tr("无法找到 agent_runner.py 路径: %1").arg(scriptPath);
        return;
    }

    // 读取可配超时(与 DAAgentSettingsWidget 共用 agent-config.ini,默认值一致)
    // 单位:秒→毫秒。ready 超时默认 60s 覆盖 langchain 冷启动导入(~17s)+余量;
    // stop 超时默认 5s 保持原有行为。
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    int readyTimeoutMs = s.value("agent/ready_timeout_sec", 60).toInt() * 1000;
    int stopTimeoutMs   = s.value("agent/stop_timeout_sec", 5).toInt() * 1000;

    // 启动
    m_bridge->startAgent(config, assembleToolSpecs(), assembleSystemPrompt(),
                         pythonExe, scriptPath, readyTimeoutMs, stopTimeoutMs);
}

bool DAAgentModule::isRunning() const
{
    // 转发 Bridge 的子进程运行状态——避免桩始终返回 false，
    // 导致外部（如 UI 忙状态判断）在子进程活跃期间误判为未运行
    return m_bridge ? m_bridge->isRunning() : false;
}

void DAAgentModule::registerBuiltinTools()
{
    // Data tools (5)
    registerTool(new DAAgentToolListData(m_core, this));
    registerTool(new DAAgentToolDataInfo(m_core, this));
    registerTool(new DAAgentToolQueryData(m_core, this));
    registerTool(new DAAgentToolColumnStats(m_core, this));
    registerTool(new DAAgentToolExportData(m_core, this));
    // Plotting tools (8)
    registerTool(new DAAgentToolCreateChart(m_core, this));
    registerTool(new DAAgentToolAddCurve(m_core, this));
    registerTool(new DAAgentToolSetChartStyle(m_core, this));
    registerTool(new DAAgentToolAddAnnotation(m_core, this));
    registerTool(new DAAgentToolAddRegion(m_core, this));
    registerTool(new DAAgentToolCreateSubplots(m_core, this));
    registerTool(new DAAgentToolSaveChartImage(m_core, this));
    registerTool(new DAAgentToolListFigures(m_core, this));
    // File/report tools (3)
    registerTool(new DAAgentToolReadFile(m_core, this));
    registerTool(new DAAgentToolWriteFile(m_core, this));
    registerTool(new DAAgentToolSaveReport(m_core, this));
}

void DAAgentModule::connectSignals()
{
    // initialize() 阶段 m_dockWidget 尚未由 DAAppDockingArea 注入（为 nullptr），
    // 此时跳过；setDockWidget() 在 Dock 就绪后会再次调用本函数完成连接。
    if (!m_bridge || !m_dockWidget) {
        return;
    }
    // Bridge → DockWidget
    connect(m_bridge, &DAAgentBridge::agentToken, m_dockWidget, &DAAgentDockWidget::onAgentToken);
    connect(m_bridge, &DAAgentBridge::agentMessageComplete, m_dockWidget, &DAAgentDockWidget::onAgentMessageComplete);
    connect(m_bridge, &DAAgentBridge::agentToolCall, m_dockWidget, &DAAgentDockWidget::onAgentToolCall);
    // 工具结果推送：plan-03 的 DAAgentDockWidget::onAgentToolResult /
    // DAAgentWebChannel::appendToolResult 依赖此信号，否则工具结果不显示在对话流中
    connect(m_bridge, &DAAgentBridge::agentToolResult, m_dockWidget, &DAAgentDockWidget::onAgentToolResult);
    connect(m_bridge, &DAAgentBridge::agentQuestion, m_dockWidget, &DAAgentDockWidget::onAgentQuestion);
    connect(m_bridge, &DAAgentBridge::agentError, m_dockWidget, &DAAgentDockWidget::onAgentError);
    connect(m_bridge, &DAAgentBridge::agentReady, m_dockWidget, &DAAgentDockWidget::onAgentReady);
    connect(m_bridge, &DAAgentBridge::agentBusy, m_dockWidget, &DAAgentDockWidget::onAgentBusy);

    // DockWidget → Module (用户消息)
    connect(m_dockWidget, &DAAgentDockWidget::sendMessageRequested, this, [this](const QString& text) {
        // appendUserMessage 已由 DAAgentDockWidget::onSendClicked() 在 emit
        // sendMessageRequested 之前调用（见 plan-03 onSendClicked 流程及
        // DAAgentWebChannel::appendUserMessage 实现）。此处切勿重复调用，
        // 否则每条用户消息会在对话流中渲染两次。
        sendMessage(text);
    });

    // DockWidget → Bridge (用户回答问题)
    connect(m_dockWidget, &DAAgentDockWidget::userAnswerSelected, m_bridge, &DAAgentBridge::sendUserAnswer);
}

QString DAAgentModule::detectPythonExePath() const
{
    // 平台已有规范的解释器解析器：DA::DAPyInterpreter::getPythonInterpreterPath()
    // （静态方法，DAPyInterpreter.h:46 / DAPyInterpreter.cpp:327-341）。
    // 它内部按两步优先级解析：
    //   1. python-config.json 中的 interpreter 路径（wherePythonFromConfig()）
    //   2. 系统 PATH 中的 python（wherePython()）
    QString path = DA::DAPyInterpreter::getPythonInterpreterPath();
    if (!path.isEmpty() && QFile::exists(path)) {
        return path;
    }
    // 兜底：在系统 PATH 中查找 python / python3
    return QStandardPaths::findExecutable("python");
}

QString DAAgentModule::detectAgentScriptPath() const
{
    // agent 脚本位于 bin/PyScripts/DAWorkbench/agent/agent_runner.py（plan-02）。
    // DACoreInterface::getPythonScriptsPath() 是静态方法，返回 PyScripts 目录。
    QString scriptsDir = DACoreInterface::getPythonScriptsPath();
    return scriptsDir + "/DAWorkbench/agent/agent_runner.py";
}

void DAAgentModule::showDockWidget()
{
    if (m_dockWidget) {
        m_dockWidget->show();
    }
}

void DAAgentModule::hideDockWidget()
{
    if (m_dockWidget) {
        m_dockWidget->hide();
    }
}

QJsonObject DAAgentModule::getLLMConfig() const
{
    // 从 agent-config.ini 读取（与设置页 DAAgentSettingsWidget 同一存储源，保持一致）
    // DAAgent 库不持有 DAAppConfig*（库无法链接 APP 可执行文件中的 DAAppConfig）
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    QJsonObject config;
    config["base_url"] = s.value("agent/llm_base_url").toString();
    config["model"]    = s.value("agent/llm_model").toString();
    // IniFormat 原生支持 QByteArray(@ByteArray 注解),api_key 直接读取
    QByteArray encKey  = s.value("agent/llm_api_key").toByteArray();
    if (!encKey.isEmpty()) {
        config["api_key"] = DAAgentSettingsWidget::decryptApiKey(encKey);
    }
    return config;
}

void DAAgentModule::setLLMConfig(const QJsonObject& config)
{
    // 与 getLLMConfig() 对称的 key 写入 agent-config.ini（可用于运行时覆盖配置）
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    s.setValue("agent/llm_base_url", config.value("base_url").toString());
    s.setValue("agent/llm_model", config.value("model").toString());
    QString apiKey = config.value("api_key").toString();
    if (!apiKey.isEmpty()) {
        // IniFormat 原生支持 QByteArray,加密 blob 直接存储
        s.setValue("agent/llm_api_key", DAAgentSettingsWidget::encryptApiKey(apiKey));
    }
}

} // namespace DA
