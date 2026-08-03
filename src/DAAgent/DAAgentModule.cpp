#include "DAAgentModule.h"
#include "DAAgentSettingsWidget.h"
#include <QSettings>
namespace DA
{
DAAgentModule::DAAgentModule(DACoreInterface* core, QObject* parent)
    : DAAgentInterface(parent), m_core(core) {}
DAAgentModule::~DAAgentModule() {}
void DAAgentModule::initialize(DACoreInterface* core) { m_core = core; }
void DAAgentModule::registerTool(DAAbstractAgentTool*) {}
void DAAgentModule::registerSystemPrompt(const QString&, const QString&) {}
void DAAgentModule::showDockWidget() {}
void DAAgentModule::hideDockWidget() {}
void DAAgentModule::sendMessage(const QString&) {}
bool DAAgentModule::isRunning() const { return false; }
QJsonObject DAAgentModule::getLLMConfig() const
{
    // 从 QSettings 读取（与设置页 DAAgentSettingsWidget 同一存储源，保持一致）
    // DAAgent 库不持有 DAAppConfig*（库无法链接 APP 可执行文件中的 DAAppConfig）
    QSettings s;
    QJsonObject config;
    config["base_url"] = s.value("agent/llm_base_url").toString();
    config["model"]    = s.value("agent/llm_model").toString();
    QByteArray encKey = s.value("agent/llm_api_key").toByteArray();
    if (!encKey.isEmpty()) {
        config["api_key"] = DAAgentSettingsWidget::decryptApiKey(encKey);
    }
    return config;
}
void DAAgentModule::setLLMConfig(const QJsonObject& config)
{
    // 与 getLLMConfig() 对称的 key 写入 QSettings（可用于运行时覆盖配置）
    QSettings s;
    s.setValue("agent/llm_base_url", config.value("base_url").toString());
    s.setValue("agent/llm_model", config.value("model").toString());
    QString apiKey = config.value("api_key").toString();
    if (!apiKey.isEmpty()) {
        s.setValue("agent/llm_api_key", DAAgentSettingsWidget::encryptApiKey(apiKey));
    }
}
} // namespace DA
