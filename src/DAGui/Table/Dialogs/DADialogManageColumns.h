#ifndef DADIALOGMANAGECOLUMNS_H
#define DADIALOGMANAGECOLUMNS_H
#include "DAGuiAPI.h"
#include <QDialog>
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>
#include <QVector>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace DA
{
/**
 * @brief 列管理对话框：列名 / 顺序 / 隐藏 / 冻结
 *
 * 表格三列：列名（双击编辑即改名）、隐藏（勾选 + 柔和灰底）、冻结（勾选 +
 * 柔和蓝底；既冻结又隐藏用蓝灰中间色）。行序即全局列序——拖动行或上移/下移
 * 按钮调整列顺序（搜索过滤期间只在可见行之间移动）。顶部搜索框按列名过滤行；
 * 底部快捷按钮：上移、下移、显示全部、取消全部冻结。
 *
 * 确定为「一个撤销单元」：改名走数据命令（DACommandDataFrame_renameColumns），
 * 列序/隐藏/冻结走视图状态命令（DATableViewStateStore 真源），由页面按差异
 * 组批；取消不改动任何东西。列序与表头拖动同一套语义（显示列序，不改
 * DataFrame 的列顺序）。
 */
class DAGUI_API DADialogManageColumns : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogManageColumns(QWidget* parent = nullptr);
    ~DADialogManageColumns() override;

public:
    // (re)初始化：orderedColumns = 全局列序（含隐藏列）
    void setup(const QList< QPair< QString, int > >& orderedColumns,
               const QSet< QPair< QString, int > >& hiddenColumns,
               const QSet< QPair< QString, int > >& frozenColumns);
    // 确定后的隐藏集合（含被搜索过滤掉的行）
    QSet< QPair< QString, int > > hiddenColumns() const;
    // 确定后的冻结列集合（顺序由调用方按全局列序合并）
    QSet< QPair< QString, int > > frozenColumns() const;
    // 确定后的改名对：((旧列名, occurrence) → 新列名)，仅变化的行
    QList< QPair< QPair< QString, int >, QString > > renamedColumns() const;
    // 确定后的列顺序（行序 = 新全局列序，含隐藏列）
    QList< QPair< QString, int > > columnOrder() const;

private:
    // 一行：列身份 + 当前编辑态
    struct Row
    {
        QPair< QString, int > ident;  ///< 原始列身份（列名 + 重名消歧位）
        QString text;                 ///< 当前列名文本（改名后为新名）
        bool hidden { false };        ///< 隐藏勾选
        bool frozen { false };        ///< 冻结勾选
    };
    // 重建表格内容（setup 时调用）
    void rebuildRows(const QList< QPair< QString, int > >& orderedColumns,
                     const QSet< QPair< QString, int > >& hiddenColumns,
                     const QSet< QPair< QString, int > >& frozenColumns);
    // 按 mRows 重建表格项（拖动/移动后也走这里）
    void refreshTable();
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
    // 列名编辑校验：非空、不与其他行重名；非法回滚（回旧值）并在底部内联提示
    bool validateColumnName(int row, const QString& newName);
    // 底部内联提示（空文本 = 隐藏）
    void setStatusText(const QString& text);
    // 上下移动选中行（delta=-1 上移，+1 下移；跳过搜索隐藏行）
    void moveSelectedRow(int delta);
    // 把 from 行移动到 to 位置（mRows 与表格同步重建）
    void moveRow(int from, int to);
    // 当前选中行（无选中返回 -1）
    int selectionRow() const;
    // 上移/下移按钮可用态（无选中或已到边界时禁用）
    void updateMoveButtons();

private:
    QVector< Row > mRows;  ///< 行数据（行序 = 列序）
    QLineEdit* mSearchEdit { nullptr };
    QTableWidget* mTable { nullptr };
    QPushButton* mBtnMoveUp { nullptr };
    QPushButton* mBtnMoveDown { nullptr };
    QPushButton* mBtnShowAll { nullptr };
    QPushButton* mBtnUnfreeze { nullptr };
    QLabel* mStatusLabel { nullptr };
    bool mUpdating { false };  ///< 程序化改动期间屏蔽 cellChanged 回环
};
}  // end of namespace DA
#endif  // DADIALOGMANAGECOLUMNS_H
