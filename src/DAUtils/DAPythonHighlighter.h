#ifndef DAPYTHONHIGHLIGHTER_H
#define DAPYTHONHIGHLIGHTER_H
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include "DAUtilsAPI.h"

namespace DA
{
/**
 * @brief Python 语法高亮器（通用，纯 Qt）
 *
 * 与 DAMarkdownHighlighter 同层，供任意模块的 Python 代码编辑器复用。
 * 覆盖：关键字、内置函数、字符串（含三引号跨行）、注释、数字、def/class 名、
 * 装饰器与 pandas 反引号列名。颜色全部取自 DESIGN.md 官方色板。
 */
class DAUTILS_API DAPythonHighlighter : public QSyntaxHighlighter
{
public:
    explicit DAPythonHighlighter(QTextDocument* parent = nullptr);
    ~DAPythonHighlighter();

protected:
    // 逐行高亮（三引号字符串跨行状态经 blockState 传递）
    void highlightBlock(const QString& text) override;

private:
    // 按词性着色关键字/内置名
    void highlightWord(int start, int length, const QString& word);
    // 数字字面量（十六进制/二进制/八进制/浮点/虚数），返回匹配长度（0=不匹配）
    int highlightNumber(const QString& text, int start);

    QTextCharFormat m_keywordFormat;     ///< 关键字（主蓝加粗）
    QTextCharFormat m_builtinFormat;     ///< 内置函数（深蓝）
    QTextCharFormat m_stringFormat;      ///< 字符串（绿）
    QTextCharFormat m_commentFormat;     ///< 注释（中灰斜体）
    QTextCharFormat m_numberFormat;      ///< 数字（紫）
    QTextCharFormat m_definitionFormat;  ///< def/class 名（深蓝灰加粗）
    QTextCharFormat m_decoratorFormat;   ///< 装饰器（紫）
    QTextCharFormat m_columnFormat;      ///< 反引号列名（青蓝加粗）
};
}  // namespace DA
#endif  // DAPYTHONHIGHLIGHTER_H
