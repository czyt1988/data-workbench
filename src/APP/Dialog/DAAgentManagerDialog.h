#ifndef DAAGENTMANAGERDIALOG_H
#define DAAGENTMANAGERDIALOG_H
#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTabWidget;

namespace DA
{
class DAAgentPromptOps;
class DAAgentInterface;

/**
 * @brief Agent 管理对话框（双 Tab）
 *
 * Tab1「提示词库」：左侧 QListWidget 列出所有 agent 标题，右侧 添加/修改/删除；
 * Tab2「子 Agent」：子 agent 定义列表 + 增删改（数据源 subagentDefinitions()）。
 * 提示词操作经 DAAgentPromptOps（DAAgentManager 实现，agentPromptOps() 注入）；
 * 子 agent 操作经 DAAgentInterface 的 subagentDefinitions/saveSubagent/deleteSubagent/
 * registeredToolNames；本对话框不依赖 DAAgent 模块。
 */
class DAAgentManagerDialog : public QDialog
{
    Q_OBJECT
public:
    DAAgentManagerDialog(DAAgentPromptOps* ops, DAAgentInterface* agent, QWidget* parent = nullptr);

private Q_SLOTS:
    // ---- Tab1 提示词库 ----
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onSelectionChanged();
    // ---- Tab2 子 Agent ----
    void onSubagentAddClicked();
    void onSubagentEditClicked();
    void onSubagentDeleteClicked();
    void onSubagentItemDoubleClicked(QListWidgetItem* item);
    void onSubagentSelectionChanged();

private:
    void buildUi();
    void refreshList();
    void updateButtonStates();
    QString currentSelectedTitle() const;
    QStringList existingTitles() const;
    // ---- Tab2 子 Agent 辅助 ----
    void buildSubagentTab(QTabWidget* tabs);
    void refreshSubagentList();
    void updateSubagentButtonStates();
    QString currentSelectedSubagent() const;

    DAAgentPromptOps* m_ops;
    DAAgentInterface* m_agent;  ///< 接口（子 agent CRUD 数据源；提示词库仅用 m_ops）
    QListWidget* m_list { nullptr };
    QPushButton* m_addBtn { nullptr };
    QPushButton* m_editBtn { nullptr };
    QPushButton* m_deleteBtn { nullptr };
    // ---- Tab2 子 Agent ----
    QListWidget* m_subagentList { nullptr };
    QPushButton* m_subagentAddBtn { nullptr };
    QPushButton* m_subagentEditBtn { nullptr };
    QPushButton* m_subagentDeleteBtn { nullptr };
};
} // namespace DA

#endif // DAAGENTMANAGERDIALOG_H
