// DAAgentWebChannel.cpp
#include "DAAgentWebChannel.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>

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

DAAgentWebChannel::DAAgentWebChannel(QWebEngineView* view, QObject* parent)
    : QObject(parent), m_view(view)
{
}

void DAAgentWebChannel::callJS(const QString& funcCall)
{
    if (m_view && m_view->page()) {
        m_view->page()->runJavaScript(funcCall);
    }
}

void DAAgentWebChannel::onUserSelect(const QString& answer)
{
    emit userAnswerSelected(answer);
}

void DAAgentWebChannel::onUserMessage(const QString& text)
{
    emit userMessageSent(text);
}

void DAAgentWebChannel::onFigureLink(const QString& href)
{
    emit figureLinkRequested(href);
}

void DAAgentWebChannel::onReady()
{
    emit webReady();
}

void DAAgentWebChannel::onStopRequested()
{
    emit stopRequested();
}

void DAAgentWebChannel::appendUserMessage(const QString& text)
{
    callJS(QString("appendUserMessage(\"%1\")").arg(toJsString(text)));
}

void DAAgentWebChannel::appendToken(const QString& token)
{
    callJS(QString("appendToken(\"%1\")").arg(toJsString(token)));
}

void DAAgentWebChannel::finalizeAgentMessage(const QString& fullText)
{
    callJS(QString("finalizeAgentMessage(\"%1\")").arg(toJsString(fullText)));
}

void DAAgentWebChannel::appendToolCall(const QString& toolName, const QJsonObject& args)
{
    QJsonDocument doc(args);
    QString argsJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    callJS(QString("appendToolCall(\"%1\",%2)").arg(toJsString(toolName), argsJson));
}

void DAAgentWebChannel::appendToolResult(const QString& toolName, const QJsonObject& result)
{
    QJsonDocument doc(result);
    QString resultJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    callJS(QString("appendToolResult(\"%1\",%2)").arg(toJsString(toolName), resultJson));
}

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

void DAAgentWebChannel::appendError(const QString& message, const QString& errorType)
{
    callJS(QString("appendError(\"%1\", \"%2\")")
        .arg(toJsString(message))
        .arg(toJsString(errorType)));
}

void DAAgentWebChannel::clearChat()
{
    callJS(QStringLiteral("clearChat()"));
}

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

void DAAgentWebChannel::setBusy(bool busy)
{
    // busy 打包：JS 解释按钮 Send/Stop 切换 + 输入禁用 + 状态文案
    callJS(QStringLiteral("setBusy(%1)").arg(busy ? QStringLiteral("true")
                                                   : QStringLiteral("false")));
}

void DAAgentWebChannel::setStopping()
{
    // 停止过渡态：按钮禁用防重复点 + 状态 Stopping...
    callJS(QStringLiteral("setStopping()"));
}

void DAAgentWebChannel::setModel(const QString& label)
{
    // label 已由 C++ 格式化为 "Model: <name>"，JS 仅显示（CSS ellipsis 截断）
    callJS(QString("setModel(\"%1\")").arg(toJsString(label)));
}

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

void DAAgentWebChannel::resetTokenStats()
{
    callJS(QStringLiteral("resetTokenStats()"));
}

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

void DAAgentWebChannel::focusInput()
{
    callJS(QStringLiteral("focusInput()"));
}

void DAAgentWebChannel::onAgentStopped()
{
    // 复用已有 JS 函数：flushAgentMessage 定稿半截流式消息（保留已累积文本），
    // closeToolGroup 关闭未完成的工具分组（标记为 incomplete）。
    callJS(QStringLiteral("flushAgentMessage()"));
    callJS(QStringLiteral("closeToolGroup()"));
}

} // namespace DA
