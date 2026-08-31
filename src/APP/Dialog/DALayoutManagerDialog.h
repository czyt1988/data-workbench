#ifndef DALAYOUTMANAGERDIALOG_H
#define DALAYOUTMANAGERDIALOG_H
#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace DA
{
class DAAppLayoutManager;

/**
 * @brief 布局管理对话框
 *
 * 左侧列表展示所有布局方案（预置在前，预置方案显示翻译名且不可删除），
 * 右侧按钮：保存当前布局 / 应用选中布局 / 删除选中布局 / 恢复默认布局。
 * 所有操作直接经 DAAppLayoutManager 完成，应用布局立即生效（对话框不关闭，
 * 便于连续尝试不同方案）；恢复默认布局会弹出确认框。
 */
class DALayoutManagerDialog : public QDialog
{
    Q_OBJECT
public:
    DALayoutManagerDialog(DAAppLayoutManager* mgr, QWidget* parent = nullptr);

private Q_SLOTS:
    void onSaveCurrentClicked();
    void onApplyClicked();
    void onDeleteClicked();
    void onResetDefaultClicked();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onSelectionChanged();

private:
    void buildUi();
    // 重新从布局管理器读取方案列表填充
    void refreshList();
    // 按选中状态刷新按钮可用性（预置方案不可删除）
    void updateButtonStates();
    // 当前选中方案的原始名（预置为 "Default"/"Focus Analysis"，自定义为用户命名）
    QString currentSelectedName() const;
    // 应用指定方案（供双击/按钮复用）
    void applyLayout(const QString& name);

    DAAppLayoutManager* m_mgr;
    QListWidget* m_list { nullptr };
    QPushButton* m_saveCurrentBtn { nullptr };
    QPushButton* m_applyBtn { nullptr };
    QPushButton* m_deleteBtn { nullptr };
    QPushButton* m_resetDefaultBtn { nullptr };
};

}  // namespace DA

#endif  // DALAYOUTMANAGERDIALOG_H
