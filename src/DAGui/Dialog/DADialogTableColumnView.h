#ifndef DADIALOGTABLECOLUMNVIEW_H
#define DADIALOGTABLECOLUMNVIEW_H
#include "DAGuiAPI.h"
#include <QDialog>
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>

namespace Ui
{
class DADialogTableColumnView;
}

namespace DA
{
/**
 * @brief 列视图对话框（006d）：勾选可见列 + 冻结列数
 *
 * 列表列出全部实际列（按 store 的全局列序；隐藏列留在原位、只是未勾选；
 * 重名显示“列名 (2)”），勾选 = 可见；冻结列数范围 0 … 当前勾选数，起始值 =
 * 生效冻结数；确定后由页面按差异构造一个列视图父命令，取消不改动；
 * v1 不做实时预览。
 */
class DAGUI_API DADialogTableColumnView : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogTableColumnView(QWidget* parent = nullptr);
    ~DADialogTableColumnView() override;

    // (re)初始化（堆建一次后复用，范式照 DADialogTableDisplayFormat::setup）：
    // orderedColumns = 全局列序（含隐藏列）；hiddenColumns = 当前隐藏集合；
    // effectiveFrozenColumnCount = 生效冻结数（SpinBox 起始值）
    void setup(const QList< QPair< QString, int > >& orderedColumns,
               const QSet< QPair< QString, int > >& hiddenColumns,
               int effectiveFrozenColumnCount);

    // 确定后的新隐藏集合（= 列全集 − 勾选集合；调用方只在 Accepted 后取）
    QSet< QPair< QString, int > > hiddenColumns() const;
    // 确定后的新冻结列数
    int frozenColumnCount() const;

private:
    // 勾选变化：SpinBox 上限 = 勾选数（Qt 自动夹紧当前值）
    void updateFreezeRange();

private:
    Ui::DADialogTableColumnView* ui { nullptr };
    QList< QPair< QString, int > > mOrderedColumns;
};
}  // end of namespace DA
#endif  // DADIALOGTABLECOLUMNVIEW_H
