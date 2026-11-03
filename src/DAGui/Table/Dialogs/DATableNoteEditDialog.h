#ifndef DATABLENOTEEDITDIALOG_H
#define DATABLENOTEEDITDIALOG_H
#include <QDialog>
#include <QString>
#include <QPoint>
#include "DAGuiAPI.h"

class QPushButton;

namespace DA
{
class DATextEditWidget;
/**
 * @brief 单元格批注编辑弹层（005 T1.26，轻量对话框）
 *
 * 纯文本编辑（v1 不做富文本/回复线程）+ 保存 / 删除（仅已有批注时显示）/ 取消。
 * 弹层位置跟随单元格：调用方以 popupNear(单元格右下角全局坐标) 定位后 exec()。
 * 时间戳字段由存储层维护（团队协作 T6.4 预留），本弹层只编辑文本。
 */
class DAGUI_API DATableNoteEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DATableNoteEditDialog(QWidget* parent = nullptr);
    ~DATableNoteEditDialog();

    // 批注文本（编辑前装载 / 确定后取用）
    void setNoteText(const QString& text);
    QString noteText() const;
    // 是否已有批注（true 显示「删除」按钮——编辑/删除同一弹层）
    void setExistingNote(bool existing);
    // 用户是否点了「删除」（accepted 后查询）
    bool isDeleteRequested() const;
    // 跟随单元格定位：把弹层挪到给定全局坐标（单元格右下角）附近，出屏时回夹
    void popupNear(const QPoint& globalCellCorner);

private Q_SLOTS:
    void onDeleteClicked();

private:
    void setupUI();

    DATextEditWidget* mEdit { nullptr };
    QPushButton* mBtnDelete { nullptr };
    bool mDeleteRequested { false };
};
}  // end of namespace DA
#endif  // DATABLENOTEEDITDIALOG_H
