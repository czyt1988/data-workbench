#ifndef DADIALOGAGENTSESSIONMANAGER_H
#define DADIALOGAGENTSESSIONMANAGER_H

#include <QDialog>
#include <QVariantList>
#include <QString>
#include "DAGuiAPI.h"

class QTableWidget;
class QPushButton;

namespace DA
{
/**
 * @brief Agent 会话管理对话框
 *
 * 用一个表格列举当前工程下的所有会话（标题 / 消息数 / 更新时间），
 * 提供会话切换、重命名、删除功能。操作通过信号上抛，由 DAAgentDockWidget
 * signal→signal 直连转发到 DAAgentInterface。
 *
 * 操作语义：
 * - 切换（双击行或 Switch 按钮）：emit switchRequested + 关闭对话框
 * - 重命名：QInputDialog 输入新标题 → emit renameRequested，本地乐观更新该行
 * - 删除：二次确认 → emit deleteRequested，本地移除该行
 *
 * @note 列表数据为构造时的快照；对话框关闭后由 sessionListChanged 信号回灌权威状态。
 */
class DAGUI_API DADialogAgentSessionManager : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param sessions 会话列表 payload（每元素 QVariantMap{id,title,updatedAt,messageCount}）
     * @param currentSessionId 当前活跃会话 ID（用于高亮与默认选中）
     * @param parent 父窗口
     */
    DADialogAgentSessionManager(const QVariantList& sessions, const QString& currentSessionId, QWidget* parent = nullptr);

private Q_SLOTS:
    void onSwitchClicked();
    void onRenameClicked();
    void onDeleteClicked();
    void onItemDoubleClicked(int row);
    void onSelectionChanged();

Q_SIGNALS:
    /// 请求切换到指定会话
    void switchRequested(const QString& sessionId);
    /// 请求重命名会话
    void renameRequested(const QString& sessionId, const QString& newTitle);
    /// 请求删除会话
    void deleteRequested(const QString& sessionId);

private:
    // 填充表格
    void populateSessions(const QVariantList& sessions);
    // 根据当前选中刷新按钮可用态
    void updateButtonStates();
    // 取指定行的会话 ID（无选中返回空串）
    QString sessionIdAt(int row) const;
    // 当前选中行（无选中返回 -1）
    int currentSelectedRow() const;
    // 把 ISO8601 时间串格式化为本地显示串
    static QString formatTimestamp(const QString& iso);

private:
    QTableWidget* m_table;
    QPushButton* m_switchBtn;
    QPushButton* m_renameBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_closeBtn;
    QString m_currentSessionId;
};
}  // namespace DA
#endif  // DADIALOGAGENTSESSIONMANAGER_H
