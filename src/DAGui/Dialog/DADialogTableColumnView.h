#ifndef DADIALOGTABLECOLUMNVIEW_H
#define DADIALOGTABLECOLUMNVIEW_H
#include "DAGuiAPI.h"
#include <QDialog>
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>

class QTableWidget;
class QLineEdit;
class QPushButton;

namespace DA
{
/**
 * @brief 列视图对话框（006d 改版）：表格化管理列名/隐藏/冻结
 *
 * 表格三列：列名（双击编辑，支持重命名）、隐藏（勾选 + 柔和灰底）、
 * 冻结（勾选 + 柔和蓝底；既冻结又隐藏用蓝灰中间色）。顶部保留搜索框
 * （按列名过滤行）；底部两个快捷按钮：显示全部（取消所有隐藏）、
 * 取消全部冻结。确定后由页面按差异构造一个列视图批量父命令
 * （改名对走 DACommandDataFrame_renameColumns，同样支持 undo/redo），
 * 取消不改动。
 *
 * 冻结勾选语义：冻结的是表格前缀——用户勾选某行的冻结时，该行之前的
 * 所有行自动补勾（Excel 冻结语义），取消某行冻结时其后所有行的冻结
 * 一并取消（冻结名额必须连续）。
 */
class DAGUI_API DADialogTableColumnView : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogTableColumnView(QWidget* parent = nullptr);
    ~DADialogTableColumnView() override;

    // (re)初始化（堆建一次后复用）：
    // orderedColumns = 全局列序（含隐藏列）；hiddenColumns = 当前隐藏集合；
    // frozenCount = 冻结列数（列序前 N 个，隐藏列占名额）
    void setup(const QList< QPair< QString, int > >& orderedColumns,
               const QSet< QPair< QString, int > >& hiddenColumns,
               int frozenCount);

    // 确定后的新隐藏集合（调用方只在 Accepted 后取）
    QSet< QPair< QString, int > > hiddenColumns() const;
    // 确定后的新冻结列数（= 冻结勾选的行数）
    int frozenColumnCount() const;
    // 确定后的改名对：((旧列名, occurrence) → 新列名)，仅变化的行；
    // occurrence 是旧身份的消歧位，新列名由用户输入
    QList< QPair< QPair< QString, int >, QString > > renamedColumns() const;

private:
    // 重建表格内容（setup 时调用）
    void rebuildRows(const QList< QPair< QString, int > >& orderedColumns,
                     const QSet< QPair< QString, int > >& hiddenColumns,
                     int frozenCount);
    // 冻结勾选变化：把冻结名额规整为表格前缀（勾选行 → 前面全补；取消 → 后面全清）
    void normalizeFrozenPrefix(int changedRow);
    // 应用行背景色（隐藏=浅灰、冻结=浅蓝、双勾=蓝灰中间色；DESIGN.md 官方色板）
    void refreshRowBackgrounds();
    // 搜索文本变化：按列名过滤行（被过滤行的勾选状态保留）
    void applySearchFilter(const QString& text);
    // 显示全部按钮：清掉所有行的隐藏勾选
    void showAllColumns();
    // 取消全部冻结按钮：清掉所有行的冻结勾选
    void unfreezeAllColumns();
    // 单元格内容变化（列名编辑提交 / 勾选变化统一入口）
    void onCellChanged(int row, int column);
    // 列名编辑校验：非空、不与其他行重名；非法回滚（回旧值）
    bool validateColumnName(int row, const QString& newName);

private:
    QList< QPair< QString, int > > mOrderedColumns;  ///< 表格行对应的列身份（含隐藏列）
    QLineEdit* mSearchEdit { nullptr };
    QTableWidget* mTable { nullptr };
    QPushButton* mBtnShowAll { nullptr };
    QPushButton* mBtnUnfreeze { nullptr };
    bool mUpdating { false };  ///< 程序化改动期间屏蔽 cellChanged 回环
};
}  // end of namespace DA
#endif  // DADIALOGTABLECOLUMNVIEW_H
