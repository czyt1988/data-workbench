#pragma once
#include "DAAgentAPI.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include "DAAbstractAgentTool.h"

namespace DA
{
class DAAgent_API DAAgentInterface : public QObject
{
    Q_OBJECT
public:
    explicit DAAgentInterface(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~DAAgentInterface() = default;
    // 插件扩展
    virtual void registerTool(DAAbstractAgentTool* tool) = 0;
    virtual void registerSystemPrompt(const QString& name, const QString& content) = 0;
    // UI
    virtual void showDockWidget() = 0;
    virtual void hideDockWidget() = 0;
    // Agent 控制
    virtual void sendMessage(const QString& text) = 0;
    virtual bool isRunning() const = 0;
    // 配置
    virtual QJsonObject getLLMConfig() const = 0;
    virtual void setLLMConfig(const QJsonObject& config) = 0;
};
} // namespace DA
