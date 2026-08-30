#ifndef DAAGENTSUBAGENTEDITDIALOG_H
#define DAAGENTSUBAGENTEDITDIALOG_H
#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

class QLineEdit;
class QPlainTextEdit;
class QListWidget;

namespace DA
{
class DAMarkdownHighlighter;

/**
 * @brief 子 Agent 定义结构化编辑对话框（subagent-phase1 C，Q13/Q15）
 *
 * 结构化表单：name（新增可编辑、编辑态只读——重命名由 manager 的
 * saveSubagent(oldName) 承担）、description（进父派发工具描述）、
 * 工具白名单（QListWidget 复选框，数据源 registeredToolNames()）、
 * 正文（QPlainTextEdit + DAMarkdownHighlighter，md 系统提示词）。
 * frontmatter 不暴露给用户，由 DAAgentSubagentDef 序列化处理。
 * 保存仅校验并收集表单，实际落盘由调用方经 saveSubagent 完成。
 */
class DAAgentSubagentEditDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造
     * @param def 编辑态预填的定义 JSON（{name,description,tools,system_prompt}），新增传空对象
     * @param existingDefs 当前全部定义（重名检测数据源）
     * @param registeredTools 已注册工具名列表（白名单复选框数据源）
     * @param parent 父窗口
     */
    DAAgentSubagentEditDialog(const QJsonObject& def,
                              const QJsonArray& existingDefs,
                              const QStringList& registeredTools,
                              QWidget* parent = nullptr);

    /// 收集表单为定义 JSON（{name,description,tools,system_prompt}，供 saveSubagent）
    QJsonObject getDefinition() const;
    /// 表单中的 name（保存成功后 manager 据此选中列表项）
    QString getName() const;

private Q_SLOTS:
    void onSaveClicked();

private:
    void buildUi();
    bool validate();

    QLineEdit* m_nameEdit { nullptr };
    QLineEdit* m_descEdit { nullptr };
    QListWidget* m_toolsList { nullptr };
    QPlainTextEdit* m_promptEdit { nullptr };
    DAMarkdownHighlighter* m_highlighter { nullptr };
    QStringList m_existingNames;  ///< 已存在的子 agent 名（重名检测）
    QStringList m_registeredTools;  ///< 已注册工具名（复选框顺序基准）
};
} // namespace DA

#endif // DAAGENTSUBAGENTEDITDIALOG_H
