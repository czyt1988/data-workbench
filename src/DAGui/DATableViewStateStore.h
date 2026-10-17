#ifndef DATABLEVIEWSTATESTORE_H
#define DATABLEVIEWSTATESTORE_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
/**
 * @brief 表格视图状态子存储（列宽 / 行头宽）
 *
 * 列宽按「列名 + 第几次出现（occurrence）」存储——语义类列引用约定（R20）：
 * pandas 允许重名列且 da_insert_column 不做重名检查，键必须是 (列名, 出现序号)
 * 才能保证两个同名列各自的宽度互不串。行头宽为单值。
 *
 * 纪律（R6）：禁止用 QHeaderView::saveState() 持久化——滑动窗下段号=窗口行号，
 * 状态会随滚动互相污染；本存储只按列名与单值记录。
 *
 * 列改名由页面在命令 exec 后调用 renameColumn 迁移；列删除调用 removeColumn。
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
};
}  // end of namespace DA
#endif  // DATABLEVIEWSTATESTORE_H
