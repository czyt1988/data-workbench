#pragma once
#include <QWidget>
#include <QWebEngineView>
#include <QWebChannel>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QProgressBar>
#include <QMenu>
#include <QJsonObject>
#include <QVector>
#include <QStringList>
#include <QVariantList>
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
    void onFigureLink(const QString& href);  // 绘图引用超链接点击转发
    // 会话栏按钮与下拉
    void onSessionComboActivated(int index);
    void onNewSessionClicked();
    void onDeleteSessionClicked();
    void onRenameSessionClicked();
    void onTokenLabelClicked();

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

    // ---- plan-04 多会话 + token UI 槽 ----
    /**
     * @brief 处理 token 使用量更新（契约2：5 参含 contextWindow 与 source）
     * @param inputTokens 输入 token
     * @param outputTokens 输出 token
     * @param totalTokens 总 token
     * @param contextWindow 上下文窗口大小
     * @param source 来源（tiktoken / usage_metadata）
     */
    void onAgentUsage(int inputTokens, int outputTokens, int totalTokens, int contextWindow, const QString& source);

    /**
     * @brief Python load_session 重建完成，解除 UI 切换守卫
     * @param sessionId 会话 ID
     */
    void onAgentSessionLoaded(const QString& sessionId);

    /**
     * @brief 会话切换完成（Module::sessionSwitched），UI 侧守卫 + clearChat + loadHistory
     * @param sessionId 新会话 ID
     * @param allRecords 新会话完整 JSONL 记录
     */
    void onSessionSwitched(const QString& sessionId, const QVector<QJsonObject>& allRecords);

    /**
     * @brief 会话列表变化（契约3：payload 每元素 QVariantMap{id,title}），直接填充下拉
     * @param sessions 会话列表 payload
     */
    void onSessionListChanged(QVariantList sessions);

    /**
     * @brief 新会话创建（newSession 路径），清空聊天 + 复位守卫 + 复位 token 控件
     * @param sessionId 新会话 ID
     */
    void onSessionCreated(const QString& sessionId);

    /**
     * @brief 当前无活跃会话（restoreLastActiveSession 未命中且无工程会话）
     *
     * 清空残留聊天区、复位 token 控件、下拉不选中、解除切换守卫。
     * 触发场景：打开一个无内嵌会话的工程时，清空残留的游离会话聊天区。
     */
    void onSessionCleared();

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

    /// 用户点击绘图引用超链接（da-figure: 协议），转发给 DAAppController 处理
    void figureLinkRequested(const QString& href);

    // ---- plan-04 会话操作信号（→ Module） ----
    /// 用户在会话下拉切换会话
    void sessionSwitchRequested(const QString& sessionId);
    /// 用户点击 "+" 新建会话
    void sessionCreateRequested();
    /// 用户点击 "×" 删除会话
    void sessionDeleteRequested(const QString& sessionId);
    /// 用户点击 Rename 重命名会话
    void sessionRenameRequested(const QString& sessionId, const QString& newTitle);
    /// 会话切换开始时请求停止当前 agent 流式输出（MAJOR4 切换时请求停止）
    void agentStopRequested();

protected:
    /**
     * @brief 事件过滤器：m_tokenLabel 鼠标点击弹出 token 分类明细 QMenu
     */
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    /**
     * @brief 初始化 UI 界面
     */
    void setupUI();

    /**
     * @brief 初始化 WebChannel
     */
    void setupWebChannel();

    /// 用 sessionListChanged payload 填充 m_sessionCombo（保留当前选择）
    void refreshSessionCombo(const QVariantList& sessions);
    /// 按会话 ID 在 m_sessionCombo 中查找索引（未找到返回 -1）
    int findSessionIndex(const QString& sid) const;
    /// 重建 token 分类明细 QMenu（input/output/total/window/source）
    void rebuildTokenMenu(int inT, int outT, int tot, int window, const QString& source);
    /// 复位 token 控件到无活跃会话初始态（进度条 0%、标签 "tokens: -"、菜单清空）
    void resetTokenStats();

    QWebEngineView* m_webView;
    DAAgentWebChannel* m_channel;
    QTextEdit* m_inputEdit;
    QPushButton* m_sendButton;
    QLabel* m_statusLabel;
    bool m_agentBusy = false;

    // ---- plan-04 会话栏 ----
    QComboBox* m_sessionCombo = nullptr;
    QPushButton* m_newSessionBtn = nullptr;
    QPushButton* m_deleteSessionBtn = nullptr;
    QPushButton* m_renameSessionBtn = nullptr;
    // ---- plan-04 token 占比状态栏 ----
    QProgressBar* m_tokenBar = nullptr;
    QLabel* m_tokenLabel = nullptr;
    QMenu* m_tokenMenu = nullptr;
    // ---- MAJOR4 UI 侧切换守卫：true 时渲染槽跳过，避免旧会话残余 token 渲染到新聊天区 ----
    bool m_switching = false;
};
} // namespace DA
