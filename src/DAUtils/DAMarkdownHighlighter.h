#ifndef DAMARKDOWNHIGHLIGHTER_H
#define DAMARKDOWNHIGHLIGHTER_H
#include "DAUtilsAPI.h"
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

namespace DA
{
/**
 * @brief 通用 Markdown 语法高亮器，可用于任意 QPlainTextEdit 文档
 *
 * 着色规则：
 * - 标题(#..######)：蓝色加粗
 * - 列表(- * +)：灰蓝
 * - 行内代码 / 代码块：灰底等宽
 * - 加粗(**text**)：深灰加粗
 * - 链接 [text](url)：蓝色
 * - 表格行(|)：浅灰
 */
class DAUTILS_API DAMarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit DAMarkdownHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    enum BlockState { Normal = 0, InCodeBlock = 1 };

    void applyRegex(const QString& text, const QRegularExpression& rx, const QTextCharFormat& fmt);
    void highlightInline(const QString& text);

    QTextCharFormat m_headingFmt;
    QTextCharFormat m_listFmt;
    QTextCharFormat m_codeFmt;
    QTextCharFormat m_boldFmt;
    QTextCharFormat m_linkFmt;
    QTextCharFormat m_tableFmt;
    QTextCharFormat m_codeBlockFmt;
};
} // namespace DA

#endif // DAMARKDOWNHIGHLIGHTER_H
