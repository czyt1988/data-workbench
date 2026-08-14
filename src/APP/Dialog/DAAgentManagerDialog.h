#ifndef DAAGENTMANAGERDIALOG_H
#define DAAGENTMANAGERDIALOG_H
#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace DA
{
class DAAgentPromptOps;

/**
 * @brief Agent 提示词管理对话框
 *
 * 左侧 QListWidget 列出当前所有 agent 标题，右侧 添加 / 修改 / 删除 三个按钮，
 * 底部 关闭。所有文件操作通过 DAAgentPromptOps 接口完成（由 DAAgentManager 实现，
 * 经 DAAgentInterface::agentPromptOps() 注入），本对话框不依赖 DAAgent 模块。
 */
class DAAgentManagerDialog : public QDialog
{
    Q_OBJECT
public:
    DAAgentManagerDialog(DAAgentPromptOps* ops, QWidget* parent = nullptr);

private Q_SLOTS:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onSelectionChanged();

private:
    void buildUi();
    void refreshList();
    void updateButtonStates();
    QString currentSelectedTitle() const;
    QStringList existingTitles() const;

    DAAgentPromptOps* m_ops;
    QListWidget* m_list { nullptr };
    QPushButton* m_addBtn { nullptr };
    QPushButton* m_editBtn { nullptr };
    QPushButton* m_deleteBtn { nullptr };
};
} // namespace DA

#endif // DAAGENTMANAGERDIALOG_H
