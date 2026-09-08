#ifndef DADIALOGAGENTSESSIONMANAGER_H
#define DADIALOGAGENTSESSIONMANAGER_H

#include <QDialog>
#include <QVariantList>
#include <QString>
#include <QColor>
#include "DAGuiAPI.h"

class QTableWidget;
class QPushButton;
class QLabel;
class QStackedWidget;

namespace DA
{
/**
 * @brief Agent 会话管理对话框
 *
 * 左侧表格列举当前工程下的所有会话（标题 / 状态 / 消息数 / 更新时间，
 * 支持表头点击排序），右侧详情面板展示选中会话的完整信息
 * （token 输入/输出/合计、创建/更新时间、状态等），表格下方汇总栏
 * 显示会话总数与累计 token 消耗。提供会话切换、重命名、删除功能，
 * 操作通过信号上抛，由 DAAgentDockWidget signal→signal 直连转发到 DAAgentInterface。
 *
 * 状态列（concurrent-sessions）：显示后台会话运行态角标——
 * 启动中/运行中/等待输入/出错（数据来自 listSessionsForUI 的 state 字段，
 * 空闲会话表格列留空，详情面板显示「空闲」）。
 *
 * 操作语义：
 * - 切换（双击行、Switch 按钮或右键菜单）：emit switchRequested + 关闭对话框
 * - 重命名：QInputDialog 输入新标题 → emit renameRequested，本地乐观更新该行
 * - 删除（Delete 按钮或右键菜单）：二次确认 → emit deleteRequested，本地移除该行
 *
 * @note 列表数据为构造时的快照；对话框关闭后由 sessionListChanged 信号回灌权威状态。
 */
class DAGUI_API DADialogAgentSessionManager : public QDialog
{
    Q_OBJECT
public:
    // 构造函数
    DADialogAgentSessionManager(const QVariantList& sessions, const QString& currentSessionId, QWidget* parent = nullptr);

private Q_SLOTS:
    void onSwitchClicked();
    void onRenameClicked();
    void onDeleteClicked();
    void onItemDoubleClicked(int row);
    void onSelectionChanged();
    void onTableContextMenu(const QPoint& pos);

Q_SIGNALS:
    /// 请求切换到指定会话
    void switchRequested(const QString& sessionId);
    /// 请求重命名会话
    void renameRequested(const QString& sessionId, const QString& newTitle);
    /// 请求删除会话
    void deleteRequested(const QString& sessionId);
    /// 请求停止指定会话的后台运行（审计 L14：失控后台会话不必先切换再 Stop）
    void stopRequested(const QString& sessionId);

private:
    // 填充表格（末尾刷新汇总栏与详情面板）
    void populateSessions(const QVariantList& sessions);
    // 根据当前选中刷新按钮可用态
    void updateButtonStates();
    // 根据当前选中刷新右侧详情面板（无选中显示占位页）
    void updateDetailPanel();
    // 刷新底部汇总栏（会话数 + 累计 tokens）
    void updateSummaryLabel();
    // 取指定行的会话 ID（无选中返回空串）
    QString sessionIdAt(int row) const;
    // 当前选中行（无选中返回 -1）
    int currentSelectedRow() const;
    // 把 ISO8601 时间串格式化为本地显示串
    static QString formatTimestamp(const QString& iso);
    // 状态键 → 显示文本与语义色（空闲返回 false，text/color 不置值）
    static bool stateDisplay(const QString& stateKey, QString& text, QColor& color);

private:
    QTableWidget* mTable;
    QPushButton* mSwitchBtn;
    QPushButton* mRenameBtn;
    QPushButton* mDeleteBtn;
    QPushButton* mCloseBtn;
    QString mCurrentSessionId;
    // ---- 右侧详情面板 ----
    QStackedWidget* mDetailStack;     ///< 0=占位页，1=详情页
    QLabel* mDetailTitleLabel;        ///< 会话标题（粗体、可换行）
    QLabel* mDetailStateValue;        ///< 状态值（语义色）
    QLabel* mDetailMsgValue;          ///< 消息数
    QLabel* mDetailInValue;           ///< 输入 tokens
    QLabel* mDetailOutValue;          ///< 输出 tokens
    QLabel* mDetailTotalValue;        ///< 合计 tokens
    QLabel* mDetailCreatedValue;      ///< 创建时间
    QLabel* mDetailUpdatedValue;      ///< 更新时间
    QColor mDetailStateDefaultColor;  ///< 状态标签默认文字色（空闲时恢复）
    // ---- 汇总栏 ----
    QLabel* mSummaryLabel;            ///< 共 N 个会话 · 累计 tokens X
};
}  // namespace DA
#endif  // DADIALOGAGENTSESSIONMANAGER_H
