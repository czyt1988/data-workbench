#include "DAAgentModule.h"
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
QJsonObject DAAgentModule::getLLMConfig() const { return {}; }
void DAAgentModule::setLLMConfig(const QJsonObject&) {}
} // namespace DA
