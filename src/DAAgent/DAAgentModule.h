// DAAgentModule.h
#pragma once
#include "DAAgentInterface.h"
#include "DACoreInterface.h"

namespace DA
{
class DAAgentModule : public DAAgentInterface
{
    Q_OBJECT
public:
    explicit DAAgentModule(DACoreInterface* core, QObject* parent = nullptr);
    ~DAAgentModule();
    // 辅助方法（不属于 DAAgentInterface）：注入核心接口
    void initialize(DACoreInterface* core);
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
};
} // namespace DA
