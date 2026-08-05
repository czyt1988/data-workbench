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
/**
 * @brief DAAgent 模块的公共接口
 *
 * 此接口由 DAAgentModule 实现，插件通过此接口注册工具、系统提示词、
 * 控制 agent 的 UI 显隐与消息收发，以及配置 LLM 参数（base_url、api_key、model）。
 */
class DAAgent_API DAAgentInterface : public QObject
{
    Q_OBJECT
public:
    explicit DAAgentInterface(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~DAAgentInterface() = default;

    /**
     * @brief 注册工具供 agent 使用
     * @param tool 工具实现指针
     */
    virtual void registerTool(DAAbstractAgentTool* tool) = 0;
    /**
     * @brief 注册命名系统提示词片段
     * @param name 提示词片段名称
     * @param content 提示词内容
     */
    virtual void registerSystemPrompt(const QString& name, const QString& content) = 0;

    /**
     * @brief 显示 agent dock 窗口
     */
    virtual void showDockWidget() = 0;
    /**
     * @brief 隐藏 agent dock 窗口
     */
    virtual void hideDockWidget() = 0;

    /**
     * @brief 发送用户消息给 agent
     * @param text 用户消息文本
     */
    virtual void sendMessage(const QString& text) = 0;
    /**
     * @brief 停止正在运行的 agent（用户主动终止）
     *
     * 非阻塞调用，进程退出后通过 agentBusy(false) 信号通知 UI 恢复。
     */
    virtual void stop() = 0;
    /**
     * @brief 检查 agent 是否正在运行
     * @return 若 agent 正在运行返回 true
     */
    virtual bool isRunning() const = 0;

    /**
     * @brief 获取 LLM 配置
     * @return LLM 配置 JSON（base_url、api_key、model）
     */
    virtual QJsonObject getLLMConfig() const = 0;
    /**
     * @brief 设置 LLM 配置
     * @param config LLM 配置 JSON（base_url、api_key、model）
     */
    virtual void setLLMConfig(const QJsonObject& config) = 0;
};
} // namespace DA
