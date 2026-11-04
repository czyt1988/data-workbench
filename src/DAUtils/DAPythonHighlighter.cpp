#include "DAPythonHighlighter.h"
#include "DAPythonSyntax.h"
// Qt
#include <QColor>
#include <QFont>
#include <QRegularExpression>
#include <QTextDocument>

namespace DA
{
namespace
{
// 三引号字符串的跨行状态
enum BlockState
{
    Normal = 0,
    InTripleSingle = 1,
    InTripleDouble = 2
};
}  // namespace

DAPythonHighlighter::DAPythonHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent)
{
    // 颜色全部取自 DESIGN.md 官方色板（新增色值需先更新色板）
    m_keywordFormat.setForeground(QColor(QStringLiteral("#5280C1")));  // 主蓝
    m_keywordFormat.setFontWeight(QFont::Bold);

    m_builtinFormat.setForeground(QColor(QStringLiteral("#497CAD")));  // 深蓝

    m_stringFormat.setForeground(QColor(QStringLiteral("#669E8B")));   // 绿

    m_commentFormat.setForeground(QColor(QStringLiteral("#727272")));  // 中灰
    m_commentFormat.setFontItalic(true);

    m_numberFormat.setForeground(QColor(QStringLiteral("#80559F")));   // 紫

    m_definitionFormat.setForeground(QColor(QStringLiteral("#5D6D7E")));  // 深蓝灰
    m_definitionFormat.setFontWeight(QFont::Bold);

    m_decoratorFormat.setForeground(QColor(QStringLiteral("#80559F")));  // 紫

    m_columnFormat.setForeground(QColor(QStringLiteral("#3CAED5")));  // 青蓝
    m_columnFormat.setFontWeight(QFont::Bold);
}

DAPythonHighlighter::~DAPythonHighlighter()
{
}

/**
 * @brief 逐行高亮
 *
 * 扫描顺序：注释 → 字符串 → 反引号列名 → 装饰器 → 数字 → 标识符。
 * 三引号字符串跨行时用 blockState 记录状态，下一行继续按字符串着色。
 * @param text 当前行文本
 */
void DAPythonHighlighter::highlightBlock(const QString& text)
{
    const int n = text.size();
    int i       = 0;
    // 上一行未闭合的三引号字符串：先找到闭合位置
    int prevState = previousBlockState();
    if (prevState == InTripleSingle || prevState == InTripleDouble) {
        const QString delim = (prevState == InTripleSingle) ? QStringLiteral("'''") : QStringLiteral("\"\"\"");
        const int end       = text.indexOf(delim);
        if (end < 0) {
            setFormat(0, n, m_stringFormat);
            setCurrentBlockState(prevState);
            return;
        }
        setFormat(0, end + 3, m_stringFormat);
        i = end + 3;
    }
    setCurrentBlockState(Normal);

    bool expectDefinitionName = false;
    while (i < n) {
        const QChar ch = text.at(i);
        // 注释：# 直到行尾（字符串分支在前，不会把 # 误判为注释）
        if (ch == QLatin1Char('#')) {
            setFormat(i, n - i, m_commentFormat);
            return;
        }
        // 字符串：单/双引号，三引号允许跨行
        if (ch == QLatin1Char('\'') || ch == QLatin1Char('"')) {
            const bool triple = (i + 2 < n) && (text.at(i + 1) == ch) && (text.at(i + 2) == ch);
            if (triple) {
                const QString delim = text.mid(i, 3);
                const int end       = text.indexOf(delim, i + 3);
                if (end < 0) {
                    setFormat(i, n - i, m_stringFormat);
                    setCurrentBlockState(ch == QLatin1Char('\'') ? InTripleSingle : InTripleDouble);
                    return;
                }
                setFormat(i, end + 3 - i, m_stringFormat);
                i = end + 3;
                continue;
            }
            int j = i + 1;
            while (j < n) {
                if (text.at(j) == QLatin1Char('\\')) {
                    j += 2;
                    continue;
                }
                if (text.at(j) == ch) {
                    ++j;
                    break;
                }
                ++j;
            }
            j = qMin(j, n);
            setFormat(i, j - i, m_stringFormat);
            i = j;
            continue;
        }
        // pandas 反引号列名：`列 名`
        if (ch == QLatin1Char('`')) {
            const int end = text.indexOf(QLatin1Char('`'), i + 1);
            if (end > i) {
                setFormat(i, end - i + 1, m_columnFormat);
                i = end + 1;
                continue;
            }
        }
        // 装饰器：行首（可带空白）的 @name / @a.b
        if (ch == QLatin1Char('@') && text.left(i).trimmed().isEmpty()) {
            int j = i + 1;
            while (j < n && (text.at(j).isLetterOrNumber() || text.at(j) == QLatin1Char('_') || text.at(j) == QLatin1Char('.'))) {
                ++j;
            }
            setFormat(i, j - i, m_decoratorFormat);
            i = j;
            continue;
        }
        // 数字字面量
        if (ch.isDigit() || (ch == QLatin1Char('.') && i + 1 < n && text.at(i + 1).isDigit())) {
            const int len = highlightNumber(text, i);
            if (len > 0) {
                i += len;
                continue;
            }
        }
        // 标识符 / 关键字 / 内置名
        if (ch.isLetter() || ch == QLatin1Char('_')) {
            int j = i + 1;
            while (j < n && (text.at(j).isLetterOrNumber() || text.at(j) == QLatin1Char('_'))) {
                ++j;
            }
            const QString word = text.mid(i, j - i);
            if (expectDefinitionName) {
                setFormat(i, j - i, m_definitionFormat);
                expectDefinitionName = false;
            } else {
                highlightWord(i, j - i, word);
                if (word == QLatin1String("def") || word == QLatin1String("class")) {
                    expectDefinitionName = true;
                }
            }
            i = j;
            continue;
        }
        ++i;
    }
}

/**
 * @brief 关键字 / 内置名着色
 * @param start 起始列
 * @param length 长度
 * @param word 词文本
 */
void DAPythonHighlighter::highlightWord(int start, int length, const QString& word)
{
    if (DAPythonSyntax::isKeyword(word)) {
        setFormat(start, length, m_keywordFormat);
    } else if (DAPythonSyntax::builtins().contains(word)) {
        setFormat(start, length, m_builtinFormat);
    }
}

/**
 * @brief 数字字面量着色
 * @param text 当前行
 * @param start 数字起始列
 * @return 匹配长度（0 = 不是数字字面量）
 *
 * 匹配十六/二/八进制、带下划线的整数、浮点、科学计数与虚数后缀。
 */
int DAPythonHighlighter::highlightNumber(const QString& text, int start)
{
    static const QRegularExpression numRx(
        QStringLiteral("(?:0[xX][0-9a-fA-F_]+|0[bB][01_]+|0[oO][0-7_]+|(?:\\d[\\d_]*\\.?[\\d_]*|\\.\\d[\\d_]*)(?:[eE][+-]?\\d+)?[jJ]?)"));
    const QRegularExpressionMatch m =
        numRx.match(text, start, QRegularExpression::NormalMatch, QRegularExpression::AnchoredMatchOption);
    if (!m.hasMatch()) {
        return 0;
    }
    setFormat(start, m.capturedLength(), m_numberFormat);
    return m.capturedLength();
}
}  // namespace DA
