// DAAgentWebChannel.cpp
#include "DAAgentWebChannel.h"
#include <QJsonDocument>

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

void DAAgentWebChannel::appendQuestion(const QString& text, const QStringList& options)
{
    QString arr;
    for (int i = 0; i < options.size(); ++i) {
        if (i) arr += ",";
        arr += QString("\"%1\"").arg(toJsString(options.at(i)));
    }
    // 问题底部的提交按钮与自定义输入框占位符（本地化标签，由 JS 渲染）
    QString submitLabel = toJsString(tr("Submit"));  // cn:提交
    QString customPlaceholder = toJsString(tr("Type your own answer..."));  // cn:输入自定义回答...
    callJS(QString("appendQuestion(\"%1\",[%2],\"%3\",\"%4\")")
               .arg(toJsString(text), arr, submitLabel, customPlaceholder));
}

void DAAgentWebChannel::clearChat()
{
    callJS(QStringLiteral("clearChat()"));
}

void DAAgentWebChannel::setBusy(bool busy)
{
    // placeholder: JS 侧可扩展 loading 指示器
    Q_UNUSED(busy);
}

void DAAgentWebChannel::onAgentStopped()
{
    // 复用已有 JS 函数：flushAgentMessage 定稿半截流式消息（保留已累积文本），
    // closeToolGroup 关闭未完成的工具分组（标记为 incomplete）。
    callJS(QStringLiteral("flushAgentMessage()"));
    callJS(QStringLiteral("closeToolGroup()"));
}

} // namespace DA
