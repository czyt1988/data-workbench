#ifndef DATABLESORTDIALOG_H
#define DATABLESORTDIALOG_H
#include "DAGuiAPI.h"
#include <QDialog>
#include <QList>
#include <QPair>
#include <QStringList>
#include <QVector>

class QComboBox;
class QLabel;
class QToolButton;
class QVBoxLayout;

namespace DA
{
/**
 * @brief 自定义排序对话框（多条件排序，参考 Excel/WPS 的「自定义排序」窗口）
 *
 * 每个条件一行：列（含「索引」伪列，对应 DATableSortController::c_indexSortColumn）
 * + 排序依据（当前引擎只支持「数值/单元格值」一种依据，故为只读文字）+ 次序（升序/降序）；
 * 行序即优先级（第一行是主关键字）。行内按钮：上移 / 下移 / 复制条件 / 删除条件；
 * 顶部「添加条件」追加一行。
 *
 * 只产出规格，不执行排序：确定后由页面经 DATableSortController::applySpec 作为
 * 一条撤销命令入栈（与列头右键的升/降/追加次级共用同一执行链）。
 */
class DAGUI_API DATableSortDialog : public QDialog
{
    Q_OBJECT
public:
    // columnNames 为当前表列名（位置即列号）；initialSpec 为预置规格（列号 -1 = 索引，顺序即优先级）
    explicit DATableSortDialog(const QStringList& columnNames,
                               const QVector< QPair< int, Qt::SortOrder > >& initialSpec = {},
                               QWidget* parent = nullptr);
    ~DATableSortDialog() override;

    // 确定后的排序规格（列号 -1 = 索引；同列重复只保留首次出现；顺序即优先级）
    QVector< QPair< int, Qt::SortOrder > > sortSpec() const;

private Q_SLOTS:
    // 追加一行（列取第一个未被占用的列，次序升序）
    void addCondition();

private:
    // 单行控件集合
    struct RowWidgets
    {
        QWidget* container { nullptr };   ///< 行容器（拥有列/次序/按钮）
        QLabel* rank { nullptr };         ///< 优先级序号（1 起）
        QComboBox* column { nullptr };    ///< 列选择（itemData = 列号，-1 = 索引）
        QComboBox* order { nullptr };     ///< 次序（itemData = Qt::SortOrder）
        QToolButton* up { nullptr };      ///< 上移
        QToolButton* down { nullptr };    ///< 下移
        QToolButton* copy { nullptr };    ///< 复制条件
        QToolButton* remove { nullptr };  ///< 删除条件
    };

    void setupUI();
    // 按给定列号/次序追加一行（列号越界时按 0 处理）
    void appendRow(int column, Qt::SortOrder order);
    // 删除一行（至少保留一行）
    void removeRow(const RowWidgets& row);
    // 行整体上移/下移（delta = -1 上移 / +1 下移）
    void moveRow(const RowWidgets& row, int delta);
    // 复制条件：在本行下方插入同样的一行
    void duplicateRow(const RowWidgets& row);
    // 重建序号、刷新行按钮与确定按钮的可用状态
    void refreshRowState();
    // 该行当前选择的列号（-1 = 索引）
    static int rowColumn(const RowWidgets& row);
    // 该行当前选择的次序
    static Qt::SortOrder rowOrder(const RowWidgets& row);
    // 行下标（不在列表中返回 -1）
    int rowIndexOf(const RowWidgets& row) const;

private:
    QStringList mColumnNames;          ///< 列名（下标 = 列号）
    QVector< RowWidgets > mRows;       ///< 条件行（顺序即优先级）
    QVBoxLayout* mRowsLayout { nullptr };  ///< 条件行容器布局
    QToolButton* mButtonAdd { nullptr };   ///< 添加条件
    QLabel* mHintLabel { nullptr };        ///< 底部提示
};
}  // end of namespace DA
#endif  // DATABLESORTDIALOG_H
