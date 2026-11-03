#pragma once
#include <QString>

namespace DA
{
/**
 * @brief markdown 相关的 JS 工具函数
 *
 * DAMarkdownView 与 DAMarkdownExporter 共用（DAAgentWebChannel 保留自己的副本，
 * 模块内聚不跨目录引用 MarkdownView 头文件）。
 */
namespace DAMarkdownJsUtils
{
/**
 * @brief 把 QString 转义为可安全嵌入 JS 字符串字面量的形式
 *
 * 处理双引号、反斜杠、换行、制表符及控制字符；CJK 等 BMP 字符直通。
 * 分片传输时按原始文本切片后再逐片转义（转义是字符局部的，切片安全），
 * 但切片边界不能落在代理对中间（见 DAMarkdownView::renderMarkdown）。
 * @param str 原始字符串
 * @return 转义后的字符串（外层双引号由调用方提供）
 */
inline QString toJsString(const QString& str)
{
    QString result;
    result.reserve(str.size() + 8);
    for (const QChar& ch : str) {
        ushort code = ch.unicode();
        switch (code) {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            if (code < 0x20) {
                // 控制字符用 \uXXXX 表示
                result += QString("\\u%1").arg(code, 4, 16, QChar('0'));
            } else {
                // 含 CJK 在内的 BMP 字符直接保留
                result += ch;
            }
        }
    }
    return result;
}
}  // namespace DAMarkdownJsUtils
}  // namespace DA
