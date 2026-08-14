#ifndef DAAGENTEDITORDIALOG_H
#define DAAGENTEDITORDIALOG_H
#include <QDialog>
#include <QStringList>

class QLineEdit;
class QPlainTextEdit;

namespace DA
{
class DAMarkdownHighlighter;

/**
 * @brief Agent 提示词编辑器对话框
 *
 * 顶部 QLineEdit 编辑标题，主体 QPlainTextEdit（带 markdown 语法高亮）编辑提示词内容，
 * 底部 保存 / 取消 按钮。新增时 title/content 为空，修改时预填。
 */
class DAAgentEditorDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造
     * @param title 初始标题（新增时传空串）
     * @param content 初始内容（新增时传空串）
     * @param existingTitles 已存在的标题列表，用于保存时检测重名（自身旧标题除外）
     * @param oldTitle 修改模式下自身的旧标题（用于排除重名检测），新增时为空
     * @param parent 父窗口
     */
    DAAgentEditorDialog(const QString& title,
                        const QString& content,
                        const QStringList& existingTitles,
                        const QString& oldTitle = QString(),
                        QWidget* parent = nullptr);

    QString getTitle() const;
    QString getContent() const;

private Q_SLOTS:
    void onSaveClicked();
    void onTextChanged();

private:
    void buildUi();
    bool validate();

    QLineEdit* m_titleEdit { nullptr };
    QPlainTextEdit* m_contentEdit { nullptr };
    DAMarkdownHighlighter* m_highlighter { nullptr };
    QStringList m_existingTitles;
    QString m_oldTitle;
};
} // namespace DA

#endif // DAAGENTEDITORDIALOG_H
