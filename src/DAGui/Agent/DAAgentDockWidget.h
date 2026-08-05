#pragma once
#include <QWidget>
#include <QWebEngineView>
#include <QWebChannel>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QJsonObject>
#include <QStringList>
#include "DAGuiAPI.h"

namespace DA
{
class DAAgentWebChannel;

/**
 * @brief Agent 对话窗口部件，继承 QWidget（非 QDockWidget，平台使用 ADS 管理停靠）
 *
 * 本项目使用 Qt-Advanced-Docking-System (ADS)，平台 DAAppDockingArea 的
 * createDockWidget(QWidget*, ...) 会把本 QWidget 包装进 ads::CDockWidget。
 * 若继承 QDockWidget 再交给 createDockWidget()，会出现双标题栏、float/拖拽冲突。
 *
 * @note 本类继承 QWidget，而非 QDockWidget
 */
class DAGUI_API DAAgentDockWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit DAAgentDockWidget(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~DAAgentDockWidget();

    /**
     * @brief 获取关联的 WebChannel 对象
     * @return WebChannel 指针
     */
    DAAgentWebChannel* webChannel() const { return m_channel; }

private Q_SLOTS:
    void onSendClicked();
    void onStopClicked();
    void onUserAnswer(const QString& answer);

public Q_SLOTS:
    /**
     * @brief 处理 Agent 流式 token 信号
     * @param token 当前 token 文本
     */
    void onAgentToken(const QString& token);

    /**
     * @brief 处理 Agent 消息完成信号
     * @param fullText 完整消息文本
     */
    void onAgentMessageComplete(const QString& fullText);

    /**
     * @brief 处理 Agent 工具调用信号
     * @param toolName 工具名称
     * @param args 工具参数
     */
    void onAgentToolCall(const QString& toolName, const QJsonObject& args);

    /**
     * @brief 处理 Agent 工具结果信号
     * @param toolName 工具名称
     * @param result 工具执行结果
     */
    void onAgentToolResult(const QString& toolName, const QJsonObject& result);

    /**
     * @brief 处理 Agent 提问信号
     * @param text 问题文本
     * @param options 选项列表
     * @param multiSelect 是否允许多选
     */
    void onAgentQuestion(const QString& text, const QStringList& options, bool multiSelect);

    /**
     * @brief 处理 Agent 错误信号
     * @param message 错误信息
     */
    void onAgentError(const QString& message);

    /**
     * @brief 处理 Agent 就绪信号
     * @param model 模型名称
     */
    void onAgentReady(const QString& model);

    /**
     * @brief 处理 Agent 忙碌状态信号
     * @param busy 是否忙碌
     */
    void onAgentBusy(bool busy);

Q_SIGNALS:
    /**
     * @brief 用户请求发送消息信号
     * @param text 消息文本
     */
    void sendMessageRequested(const QString& text);

    /**
     * @brief 用户请求终止 agent 信号
     */
    void stopRequested();

    /**
     * @brief 用户选择答案信号
     * @param answer 用户选择的答案
     */
    void userAnswerSelected(const QString& answer);

private:
    /**
     * @brief 初始化 UI 界面
     */
    void setupUI();

    /**
     * @brief 初始化 WebChannel
     */
    void setupWebChannel();

    QWebEngineView* m_webView;
    DAAgentWebChannel* m_channel;
    QTextEdit* m_inputEdit;
    QPushButton* m_sendButton;
    QLabel* m_statusLabel;
    bool m_agentBusy = false;
};
} // namespace DA
