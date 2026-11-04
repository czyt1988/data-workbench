#ifndef DAPYTHONCODEEDIT_H
#define DAPYTHONCODEEDIT_H
#include <QHash>
#include <QList>
#include <QPlainTextEdit>
#include <QString>
#include <QStringList>
#include "DAGuiAPI.h"

class QCompleter;
class QAbstractItemView;
class QStringListModel;

namespace DA
{
class DAPythonHighlighter;

/**
 * @brief 补全候选条目（显示文本 + 插入文本）
 *
 * 用于列名这类“显示与实际插入不同”的上下文词：例如列名 `my col` 显示为
 * my col，点选后插入 `my col`（反引号包裹）。
 */
struct DAGUI_API DAPythonCompletionEntry
{
    QString text;        ///< 显示与匹配文本
    QString insertText;  ///< 实际插入文本（空 = 与 text 相同）
};

/**
 * @brief 可复用的 Python 代码编辑器（语法高亮 + 自动补全）
 *
 * 组合 DAUtils 的 DAPythonHighlighter 与 DAPyBindQt 的 DAPyPythonCompletion：
 * - 高亮：关键字/内置/字符串/注释/数字/def/class/装饰器/反引号列名；
 * - 补全：Python 关键字、内置名、pd/np/DataFrame/Series/.str/.dt 成员、
 *   文档内已有标识符，以及调用方注入的上下文词（如列名）；
 * - 触发：输入标识符/“.”自动弹出，Ctrl+Space 手动弹出；Enter/Tab 接受，
 *   Esc 关闭，上下键选择；
 * - Ctrl+Enter 发出 submitRequested()（对话框可接为“确定”）。
 *
 * 本控件不认识表格/列等业务概念，列名由 setExtraEntries() 注入。
 */
class DAGUI_API DAPythonCodeEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit DAPythonCodeEdit(QWidget* parent = nullptr);
    ~DAPythonCodeEdit();

    // 注入上下文词（如列名）；显示 text、插入 insertText
    void setExtraEntries(const QList< DAPythonCompletionEntry >& entries);
    void clearExtraEntries();
    // 在光标处插入文本（调用方负责空格/反引号等格式）
    void insertCodeAtCursor(const QString& text);
    // 补全开关
    void setCompletionEnabled(bool on);
    bool isCompletionEnabled() const;
    // 补全弹窗（测试/调试用；未创建时返回 nullptr）
    QAbstractItemView* completionPopup() const;

Q_SIGNALS:
    // Ctrl+Enter 提交
    void submitRequested();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;

private Q_SLOTS:
    // 手动弹出补全（Ctrl+Space）
    void showCompletion();
    // 文本变化 → 自动补全
    void onTextChanged();

private:
    void updateCompletion(bool autoTrigger);
    QString currentPrefix(int* startAbs) const;
    QString memberBaseKey() const;
    bool cursorInsideBacktick(QString* prefix) const;
    QStringList buildCandidates(bool inBacktick, const QString& baseKey) const;
    void insertCompletion(const QString& displayText);
    bool isWordChar(const QChar& c) const;
    bool isColumnWord(const QString& word) const;

    DAPythonHighlighter* m_highlighter { nullptr };
    QCompleter* m_completer { nullptr };
    QStringListModel* m_model { nullptr };
    QList< DAPythonCompletionEntry > m_extraEntries;
    QHash< QString, QString > m_insertTexts;  ///< 显示文本 → 插入文本
    int m_completionStart { -1 };             ///< 本次补全前缀的起始文档位置
    bool m_inBacktick { false };              ///< 本次补全是否在反引号列名内
    bool m_completionEnabled { true };
    bool m_updating { false };  ///< 程序化插入时抑制自动补全
};
}  // namespace DA
#endif  // DAPYTHONCODEEDIT_H
