#include "DAMarkdownHighlighter.h"
// Qt
#include <QRegularExpression>
#include <QTextDocument>
#include <QColor>
#include <QFont>

namespace DA
{
DAMarkdownHighlighter::DAMarkdownHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent)
{
    // 标题：蓝色加粗
    m_headingFmt.setForeground(QColor("#5280C1"));
    m_headingFmt.setFontWeight(QFont::Bold);
    m_headingFmt.setFontPointSize(12);

    // 列表标记：灰蓝
    m_listFmt.setForeground(QColor("#497CAD"));

    // 行内代码：灰底等宽
    m_codeFmt.setBackground(QColor("#F0F0F0"));
    m_codeFmt.setForeground(QColor("#515151"));
    m_codeFmt.setFontFamily("Consolas");

    // 代码块整体：浅灰背景等宽
    m_codeBlockFmt.setBackground(QColor("#F7F7F7"));
    m_codeBlockFmt.setForeground(QColor("#515151"));
    m_codeBlockFmt.setFontFamily("Consolas");

    // 加粗：深灰加粗
    m_boldFmt.setForeground(QColor("#515151"));
    m_boldFmt.setFontWeight(QFont::Bold);

    // 链接：蓝色
    m_linkFmt.setForeground(QColor("#5280C1"));

    // 表格分隔行：浅灰
    m_tableFmt.setForeground(QColor("#7F7F7F"));
}

void DAMarkdownHighlighter::highlightBlock(const QString& text)
{
    // 1. 代码块（``` 围栏，跨行状态）
    static const QRegularExpression fenceRx("^```+");
    bool isFence = fenceRx.match(text).hasMatch();

    int prevState = previousBlockState();
    if (prevState == InCodeBlock) {
        // 上一行在代码块内
        if (isFence) {
            // 围栏结束行
            setFormat(0, text.length(), m_codeBlockFmt);
            setCurrentBlockState(Normal);
            return;
        }
        // 代码块正文行
        setFormat(0, text.length(), m_codeBlockFmt);
        setCurrentBlockState(InCodeBlock);
        return;
    } else {
        if (isFence) {
            // 围栏开始行
            setFormat(0, text.length(), m_codeBlockFmt);
            setCurrentBlockState(InCodeBlock);
            return;
        }
        setCurrentBlockState(Normal);
    }

    // 2. 标题
    static const QRegularExpression headingRx("^#{1,6}\\s.*$");
    applyRegex(text, headingRx, m_headingFmt);

    // 3. 表格行（含 | 且非纯空白）
    static const QRegularExpression tableRx("^\\s*\\|");
    applyRegex(text, tableRx, m_tableFmt);

    // 4. 行内规则
    highlightInline(text);
}

void DAMarkdownHighlighter::applyRegex(const QString& text, const QRegularExpression& rx, const QTextCharFormat& fmt)
{
    QRegularExpressionMatchIterator it = rx.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), fmt);
    }
}

void DAMarkdownHighlighter::highlightInline(const QString& text)
{
    // 列表标记
    static const QRegularExpression listRx("^(\\s*)([-*+])\\s");
    QRegularExpressionMatch lm = listRx.match(text);
    if (lm.hasMatch()) {
        setFormat(lm.capturedStart(2), lm.capturedLength(2), m_listFmt);
    }

    // 行内代码 `code`
    static const QRegularExpression inlineCodeRx("`[^`\\n]+`");
    applyRegex(text, inlineCodeRx, m_codeFmt);

    // 加粗 **text** 或 __text__
    static const QRegularExpression boldRx("(\\*\\*|__)[^\\n]+?(\\*\\*|__)");
    applyRegex(text, boldRx, m_boldFmt);

    // 链接 [text](url)
    static const QRegularExpression linkRx("\\[[^\\]]+\\]\\([^)]+\\)");
    applyRegex(text, linkRx, m_linkFmt);
}
} // namespace DA
