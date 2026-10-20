#ifndef DATABLEVIEWSTATESTORE_H
#define DATABLEVIEWSTATESTORE_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QHash>
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
/**
 * @brief 表格视图状态子存储（列宽 / 行头宽 / 列序 / 隐藏 / 冻结列数）
 *
 * 列宽、列序、隐藏按「列名 + 第几次出现（occurrence）」存储——语义类列引用约定（R20）：
 * pandas 允许重名列且 da_insert_column 不做重名检查，键必须是 (列名, 出现序号)
 * 才能保证两个同名列各自的宽度互不串。行头宽为单值。冻结列数为计数
 * （视图顺序前 m 个可见列，006）。
 *
 * 纪律（R6）：禁止用 QHeaderView::saveState() 持久化——滑动窗下段号=窗口行号，
 * 状态会随滚动互相污染；本存储只按列名与单值记录。
 *
 * 列改名由页面在命令 exec 后调用 renameColumn 迁移；列删除调用 removeColumn。
 * 加载后由页面调一次 normalizeToColumns 收敛（孤儿剪除 + 新列追加列序末尾）。
 * 视图状态随工程保存（DATableMetadata 聚合进 table-metadata.xml 的 <view-state> 段），
 * 不进撤销栈。
 */
class DAGUI_API DATableViewStateStore : public QObject
{
    Q_OBJECT
public:
    // 列宽条目（应用与遍历用）
    struct ColumnWidthEntry
    {
        QString name;      ///< 列名
        int occurrence;    ///< 第几次出现（重名列消歧位，从 0 计）
        int width;         ///< 宽度（像素）
    };

    explicit DATableViewStateStore(QObject* parent = nullptr);

    // 列宽查询：未记录返回 -1
    int columnWidth(const QString& name, int occurrence = 0) const;
    bool hasColumnWidth(const QString& name, int occurrence = 0) const;
    void setColumnWidth(const QString& name, int occurrence, int width);
    void removeColumnWidth(const QString& name, int occurrence);
    // 全部列宽条目（应用视图时遍历）
    QList< ColumnWidthEntry > columnWidthEntries() const;

    // 行头宽：未记录返回 -1
    int rowHeaderWidth() const;
    void setRowHeaderWidth(int width);
    // 统一行高（行边界拖拽语义；滑动窗下段号=窗口行号，逐行行高无稳定语义，故单值，R6）：
    // 未记录返回 -1
    int rowHeight() const;
    void setRowHeight(int height);
    // 水平表头高（表头底边拖拽语义）：未记录返回 -1
    int horizontalHeaderHeight() const;
    void setHorizontalHeaderHeight(int height);

    // 列序：列名的有序表（空 = 未记录 = 数据列序，006）
    QList< QPair< QString, int > > columnOrder() const;
    void setColumnOrder(const QList< QPair< QString, int > >& order);
    // 隐藏集合（列名 + occurrence，006）
    bool isColumnHidden(const QString& name, int occurrence = 0) const;
    void setColumnHidden(const QString& name, int occurrence, bool hidden);
    QList< QPair< QString, int > > hiddenColumns() const;
    // 冻结列数（视图顺序前 m 个可见列，006）
    int freezeColumnCount() const;
    void setFreezeColumnCount(int count);
    // 一次性加载收敛标志：fromXml 置 false（待收敛），normalizeToColumns 后置 true
    bool isColumnSetNormalized() const;
    // 加载收敛：按实际列身份表剪除孤儿条目、把新列追加到列序末尾；
    // 返回 true 表示有改动（调用方据此发一条用户可见提示）
    bool normalizeToColumns(const QList< QPair< QString, int > >& actualColumns);

    // 列改名迁移：把 oldName 的全部 occurrence 条目改键为 newName
    void renameColumn(const QString& oldName, const QString& newName);
    // 列删除：移除该列名的全部条目（同名后续列的 occurrence 由页面按新列序重算）
    void removeColumn(const QString& name);

    // 序列化（<view-state> 段）
    void toXml(QDomDocument& doc, QDomElement& parent) const;
    bool fromXml(const QDomElement& e);
    bool isEmpty() const;
    void clear();

private:
    // (列名, occurrence) -> 宽度
    QHash< QPair< QString, int >, int > mColumnWidths;
    int mRowHeaderWidth { -1 };  ///< 行头宽（像素），-1 表示未记录
    int mRowHeight { -1 };       ///< 统一行高（像素），-1 表示未记录
    int mHorizontalHeaderHeight { -1 };  ///< 水平表头高（像素），-1 表示未记录
    // 列序：列名的有序表（空 = 未记录 = 数据列序）
    QList< QPair< QString, int > > mColumnOrder;
    // 隐藏集合（列名 + occurrence）
    QSet< QPair< QString, int > > mHiddenColumns;
    // 冻结列数（视图顺序前 m 个可见列）
    int mFreezeColumnCount { 0 };
    // 一次性加载收敛标志：fromXml 置 false（待收敛），normalizeToColumns 后置 true
    bool mColumnSetNormalized { false };
};
}  // end of namespace DA
#endif  // DATABLEVIEWSTATESTORE_H
