// DAAgentWebChannel.cpp
#include "DAAgentWebChannel.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>
#include <QWebEngineView>

namespace DA
{

// 把 QString 转义为可安全嵌入 JS 字符串字面量的形式（含双引号外层由调用方提供）
static QString toJsString(const QString& str)
{
    QString result;
    result.reserve(str.size() + 8);
    for (const QChar& ch : str) {
        ushort code = ch.unicode();
        switch (code) {
        case '"':  result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (code < 0x20) {
                // 控制字符用 \uXXXX 表示
                result += QString("\\u%1").arg(code, 4, 16, QChar('0'));
            } else {
                result += ch;  // 含 CJK 在内的 BMP 字符直接保留
            }
        }
    }
    return result;
}

// 把 JSON 字符串解析为 QJsonObject；解析失败返回空对象（plan-04 loadHistory 用）
static QJsonObject parseJsonStr(const QString& str)
{
    if (str.isEmpty()) return QJsonObject();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return QJsonObject();
    }
    return doc.object();
}

/**
 * @brief 构造函数
 * @param view 关联的 WebEngineView
 * @param parent 父对象
 */
DAAgentWebChannel::DAAgentWebChannel(QWebEngineView* view, QObject* parent)
    : QObject(parent), mView(view)
{
}

/**
 * @brief 调用 JS 函数
 * @param funcCall JS 函数调用字符串
 */
void DAAgentWebChannel::callJS(const QString& funcCall)
{
    if (mView && mView->page()) {
        mView->page()->runJavaScript(funcCall);
    }
}

/**
 * @brief JS 调用：用户选择了答案
 * @param answer 用户选择的答案
 */
void DAAgentWebChannel::onUserSelect(const QString& answer)
{
    emit userAnswerSelected(answer);
}

/**
 * @brief JS 调用：用户发送了消息
 * @param text 用户输入的消息文本
 */
void DAAgentWebChannel::onUserMessage(const QString& text)
{
    emit userMessageSent(text);
}

/**
 * @brief JS 调用：用户点击了绘图引用超链接（da-figure: 协议）
 * @param href 超链接 href，形如 da-figure:&lt;figure_name&gt; 或 da-figure:id=&lt;uuid&gt;
 */
void DAAgentWebChannel::onFigureLink(const QString& href)
{
    emit figureLinkRequested(href);
}

/**
 * @brief JS 调用：web 侧初始化完成（chat.js init() 建立 QWebChannel 后回调）
 *
 * 握手信号：通知 C++ web 已就绪可接收状态推送。C++ 收到后回推
 * setI18nLabels/setBusy/setModel/setTokenStats，缓解 webview 异步加载期间的
 * JS-ready 竞态（agent 信号若在 chat.html 加载完成前触发会丢失）。
 */
void DAAgentWebChannel::onReady()
{
    emit webReady();
}

/**
 * @brief JS 调用：用户在 web 输入区点击 Stop 按钮（忙碌态）
 *
 * web 输入区按钮 Send/Stop 切换后，Stop 走此通道直达 C++ 终止流程，
 * 替代旧原生 m_sendButton 分流。
 */
void DAAgentWebChannel::onStopRequested()
{
    emit stopRequested();
}

/**
 * @brief 追加用户消息到聊天界面
 * @param text 消息文本
 */
void DAAgentWebChannel::appendUserMessage(const QString& text)
{
    callJS(QString("appendUserMessage(\"%1\")").arg(toJsString(text)));
}

/**
 * @brief 追加 Agent 流式 token 到聊天界面
 * @param token 当前 token 文本
 */
void DAAgentWebChannel::appendToken(const QString& token)
{
    callJS(QString("appendToken(\"%1\")").arg(toJsString(token)));
}

/**
 * @brief 完成 Agent 消息（标记消息结束）
 * @param fullText 完整消息文本
 */
void DAAgentWebChannel::finalizeAgentMessage(const QString& fullText)
{
    callJS(QString("finalizeAgentMessage(\"%1\")").arg(toJsString(fullText)));
}

/**
 * @brief 追加工具调用信息到聊天界面
 * @param toolName 工具名称
 * @param args 工具参数
 */
void DAAgentWebChannel::appendToolCall(const QString& toolName, const QJsonObject& args)
{
    QJsonDocument doc(args);
    QString argsJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    callJS(QString("appendToolCall(\"%1\",%2)").arg(toJsString(toolName), argsJson));
}

/**
 * @brief 追加工具执行结果到聊天界面
 * @param toolName 工具名称
 * @param result 工具执行结果
 */
void DAAgentWebChannel::appendToolResult(const QString& toolName, const QJsonObject& result)
{
    QJsonDocument doc(result);
    QString resultJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    callJS(QString("appendToolResult(\"%1\",%2)").arg(toJsString(toolName), resultJson));
}

/**
 * @brief 追加提问信息到聊天界面
 * @param text 问题文本
 * @param options 选项列表
 * @param multiSelect 是否允许多选
 */
void DAAgentWebChannel::appendQuestion(const QString& text, const QStringList& options, bool multiSelect)
{
    QString arr;
    for (int i = 0; i < options.size(); ++i) {
        if (i) arr += ",";
        arr += QString("\"%1\"").arg(toJsString(options.at(i)));
    }
    // 问题底部的提交按钮与自定义输入框占位符（本地化标签，由 JS 渲染）
    QString submitLabel = toJsString(tr("Submit"));  // cn:提交
    QString customPlaceholder = toJsString(tr("Type your own answer..."));  // cn:输入自定义回答...
    // 第 5 个参数为 multiSelect 布尔字面量（true/false），JS 据此切换单选/多选交互
    callJS(QString("appendQuestion(\"%1\",[%2],\"%3\",\"%4\",%5)")
               .arg(toJsString(text), arr, submitLabel, customPlaceholder,
                    multiSelect ? QStringLiteral("true") : QStringLiteral("false")));
}

/**
 * @brief 显示重试状态条（LLM 调用重试期间）
 * @param attempt 当前重试次数（1-based）
 * @param maxAttempts 最大重试次数
 * @param delayMs 本次退避延迟毫秒数
 * @param errorType 触发重试的错误类型
 * @param errorMessage 触发重试的错误消息
 */
void DAAgentWebChannel::showRetryStatus(int attempt, int maxAttempts, int delayMs,
                                         const QString& errorType, const QString& errorMessage)
{
    callJS(QString("showRetryStatus(%1, %2, %3, \"%4\", \"%5\")")
        .arg(attempt)
        .arg(maxAttempts)
        .arg(delayMs)
        .arg(toJsString(errorType))
        .arg(toJsString(errorMessage)));
}

/**
 * @brief 追加错误信息到聊天界面（独立错误卡片）
 * @param message 错误消息（经 mapErrorMessage 映射后的用户文案）
 * @param errorType 错误类型（用于 JS 端样式/图标选择）
 */
void DAAgentWebChannel::appendError(const QString& message, const QString& errorType)
{
    callJS(QString("appendError(\"%1\", \"%2\")")
        .arg(toJsString(message))
        .arg(toJsString(errorType)));
}

/**
 * @brief 追加系统消息到聊天界面（用户可见但不作为 LLM 对话内容）
 * @param text 消息文本
 * @param level 级别："info" / "warning" / "error"
 */
void DAAgentWebChannel::appendSystemMessage(const QString& text, const QString& level)
{
    callJS(QString("appendSystemMessage(\"%1\", \"%2\")")
        .arg(toJsString(text))
        .arg(toJsString(level)));
}

/**
 * @brief 清空聊天界面
 */
void DAAgentWebChannel::clearChat()
{
    callJS(QStringLiteral("clearChat()"));
}

/**
 * @brief 批量重放历史会话记录到聊天界面（plan-04）
 *
 * 遍历 plan-03 原始 JSONL 记录，把连续的 `assistant(tool_call)` + 紧随的 `tool_result`
 * 合并为单个 UI 事件（type:user/assistant/tool/question/usage），序列化为 JSON 数组
 * 调 `callJS("loadHistory(" + jsonArrayStr + ")")`。JS 端 loadHistory(events)
 * 据合并后 type 分发渲染。MAJOR2: 配对在 C++ 完成，JS 不再读 _toolName/_toolArgs。
 *
 * @param records plan-03 原始 JSONL 记录（QVector<QJsonObject>）
 */
void DAAgentWebChannel::loadHistory(const QVector<QJsonObject>& records)
{
    // MAJOR2: C++ 合并连续 assistant(tool_call) + 紧随 tool_result 为单个 UI 事件。
    // JS 端 loadHistory(events) 据合并后 type 分发；不再读 _toolName/_toolArgs。
    // tool_calls 用简化格式（call.value("name")/("args") 顶层，对齐 plan-01/03 事件映射表）。
    QJsonArray uiEvents;
    QHash<QString, QJsonObject> pendingToolCalls;  // toolCallId → {toolName,args}

    for (const QJsonObject& rec : records) {
        const QString t = rec.value("type").toString();
        const QJsonObject msg = rec.value("message").toObject();

        if (t == "user" || t == "usage" || t == "summary") {
            // 原样透传（usage/summary JS 端不再渲染，仅 user 渲染）
            uiEvents.append(rec);
        } else if (t == "assistant") {
            const QJsonArray tcs = msg.value("tool_calls").toArray();
            if (!tcs.isEmpty()) {
                // 含 tool_calls：非空 content 留作 assistant 事件；每个 tool_call 缓存等 result
                if (!msg.value("content").toString().isEmpty()) {
                    QJsonObject aEv;
                    aEv.insert("type", "assistant");
                    aEv.insert("message", msg);
                    uiEvents.append(aEv);
                }
                for (const QJsonValue& tc : tcs) {
                    const QJsonObject call = tc.toObject();
                    const QString id = call.value("id").toString();
                    const QString name = call.value("name").toString();  // 简化格式顶层 name
                    const QJsonObject args = call.value("args").toObject();  // 简化格式顶层 args（已 object）
                    QJsonObject meta;
                    meta.insert("toolName", name);
                    meta.insert("args", args);
                    pendingToolCalls.insert(id, meta);  // 缓存等 result
                }
            } else {
                // 纯文本 assistant
                uiEvents.append(rec);
            }
        } else if (t == "tool_result") {
            const QString id = msg.value("tool_call_id").toString();
            const QJsonObject meta = pendingToolCalls.take(id);
            if (meta.isEmpty()) {
                // 边界：无配对 tool_call（中断），一期跳过不入 uiEvents
                continue;
            }
            const QString name = meta.value("toolName").toString();
            const QJsonObject args = meta.value("args").toObject();
            QJsonObject ev;
            // MAJOR5: ask_user 的 tool_call+tool_result 标 type:"question"
            ev.insert("type", (name == QStringLiteral("ask_user")) ? QStringLiteral("question") : QStringLiteral("tool"));
            ev.insert("toolName", name);
            ev.insert("args", args);
            ev.insert("toolCallId", id);
            if (name == QStringLiteral("ask_user")) {
                // ask_user 答案是纯文本 str(answer)（plan-03），parseJsonStr 会失败返回空对象；
                // 构造 {"answer":content} 匹配 JS question 分支读 ev.result.answer 的取值逻辑。
                QJsonObject ansObj;
                ansObj.insert("answer", msg.value("content").toString());
                ev.insert("result", ansObj);
            } else {
                // 普通工具结果 content 是 json.dumps(result) 字符串，解析为 object
                ev.insert("result", parseJsonStr(msg.value("content").toString()));
            }
            uiEvents.append(ev);
        }
        // 其他类型（answer 等）一期不入 uiEvents
    }

    QByteArray json = QJsonDocument(uiEvents).toJson(QJsonDocument::Compact);
    callJS(QStringLiteral("loadHistory(") + QString::fromUtf8(json) + QStringLiteral(")"));
}

/**
 * @brief 设置忙碌状态（busy 打包：JS 解释按钮 Send/Stop 切换+输入禁用+状态文案）
 * @param busy 是否忙碌
 */
void DAAgentWebChannel::setBusy(bool busy)
{
    // busy 打包：JS 解释按钮 Send/Stop 切换 + 输入禁用 + 状态文案
    callJS(QStringLiteral("setBusy(%1)").arg(busy ? QStringLiteral("true")
                                                   : QStringLiteral("false")));
}

/**
 * @brief 设置停止过渡态（onStopClicked 后、onAgentBusy(false)/Ready 前）
 *
 * JS 侧禁用按钮防重复点击 + 状态文案置 Stopping...。
 */
void DAAgentWebChannel::setStopping()
{
    // 停止过渡态：按钮禁用防重复点 + 状态 Stopping...
    callJS(QStringLiteral("setStopping()"));
}

/**
 * @brief 设置启动过渡态（agentStarting 信号触发，ready/error 清除）
 *
 * JS 侧禁用按钮+输入 + 状态文案置"启动中"。区别于 setBusy(thinking)——
 * 启动中并非思考中，UI 应明确提示用户等待启动完成。
 */
void DAAgentWebChannel::setStarting()
{
    callJS(QStringLiteral("setStarting()"));
}

/**
 * @brief JS 调用：用户在 web 两级模型选择器选定供应商+模型
 * @param provider 供应商名称
 * @param model 模型 id
 */
void DAAgentWebChannel::onModelSelect(const QString& provider, const QString& model)
{
    emit modelChangeRequested(provider, model);
}

/**
 * @brief 推送可用模型列表到 web（flat 数组，JS 据供应商分组渲染两级选择器）
 * @param models 每元素 QVariantMap{provider,model,context_window,max_output_tokens}
 */
void DAAgentWebChannel::setAvailableModels(const QVariantList& models)
{
    // flat 数组序列化为 JSON 推给 JS：setAvailableModels([{provider,model,...},...])
    // JS 端按 provider 分组渲染两级选择器（第一层供应商、第二层模型）
    QJsonArray arr;
    for (const QVariant& v : models) {
        arr.append(QJsonValue::fromVariant(v));
    }
    QJsonDocument doc(arr);
    QString json = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    callJS(QStringLiteral("setAvailableModels(") + json + QStringLiteral(")"));
}

/**
 * @brief 推送激活供应商+模型到 web（JS 更新触发按钮文案 + 选中高亮）
 * @param provider 激活供应商名称
 * @param model 激活模型 id
 */
void DAAgentWebChannel::setActiveModel(const QString& provider, const QString& model)
{
    // JS 据此更新触发按钮文案（"provider · model"）+ 在两级选择器中标记选中
    callJS(QString("setActiveModel(\"%1\",\"%2\")")
               .arg(toJsString(provider), toJsString(model)));
}

/**
 * @brief 设置 token 计量（右）+ 缓存明细供 popover
 * @param label 已由 C++ 格式化的 "tokens: N / window" 串（streaming_estimate 带 ~ 前缀）
 * @param inputTokens 输入 token
 * @param outputTokens 输出 token
 * @param totalTokens 总 token
 * @param contextWindow 上下文窗口大小
 * @param source 来源（tiktoken / usage_metadata / streaming_estimate）
 */
void DAAgentWebChannel::setTokenStats(const QString& label, int inputTokens, int outputTokens,
                                     int totalTokens, int contextWindow, const QString& source)
{
    // label 已由 C++ 格式化；5 值随推供 popover 缓存
    callJS(QString("setTokenStats(\"%1\",%2,%3,%4,%5,\"%6\")")
               .arg(toJsString(label))
               .arg(inputTokens)
               .arg(outputTokens)
               .arg(totalTokens)
               .arg(contextWindow)
               .arg(toJsString(source)));
}

/**
 * @brief 复位 token 计量到无活跃会话初始态（新会话/清空时）
 */
void DAAgentWebChannel::resetTokenStats()
{
    callJS(QStringLiteral("resetTokenStats()"));
}

/**
 * @brief 注入静态 UI 标签（握手时 C++ 一次性推送，C++ 仍是唯一 i18n 拥有者）
 * @param labels QVariantMap，键见 chat.js setI18nLabels 注释
 */
void DAAgentWebChannel::setI18nLabels(const QVariantMap& labels)
{
    // 序列化为 JSON 对象推给 JS：setI18nLabels({send:"...",stop:"...",...})
    QJsonObject obj;
    for (auto it = labels.constBegin(); it != labels.constEnd(); ++it) {
        obj.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    QJsonDocument doc(obj);
    QString json = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    callJS(QStringLiteral("setI18nLabels(") + json + QStringLiteral(")"));
}

/**
 * @brief 聚焦 web 输入框（新会话/切换会话后）
 */
void DAAgentWebChannel::focusInput()
{
    callJS(QStringLiteral("focusInput()"));
}

/**
 * @brief 终止时定稿当前流式消息 + 关闭工具分组
 *
 * 调用 JS flushAgentMessage()（保留已累积的 token 文本）+ closeToolGroup()
 *（将未完成的工具卡片标记为 incomplete），避免半截消息悬挂。
 */
void DAAgentWebChannel::onAgentStopped()
{
    // 复用已有 JS 函数：flushAgentMessage 定稿半截流式消息（保留已累积文本），
    // closeToolGroup 关闭未完成的工具分组（标记为 incomplete）。
    callJS(QStringLiteral("flushAgentMessage()"));
    callJS(QStringLiteral("closeToolGroup()"));
}

} // namespace DA
