#ifndef DATABLENOTESTORE_H
#define DATABLENOTESTORE_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QHash>
#include <QPair>
#include <QList>
#include <QString>
#include <QDateTime>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
/**
 * @brief 单元格批注子存储（元数据束成员，005 T1.26）
 *
 * 键 = (row, col) **位置键**（数据源绝对行/列号，装饰类 R20）；值 = {文本, 创建/修改
 * 时间}——时间戳字段为将来团队协作（T6.4 作者/状态/时间线）预留位，v1 只记录。
 *
 * 「批注跟着行走」（设计稿 §4.10 关键设计）：批注与静态样式同源，排序（002 的
 * onRowsReordered）、插入/删除行列都走元数据束层共享的 DATableKeyRemap 重映射工具，
 * 由页面 widget 在对应命令的 directional 回调里调用（与 DATableStyleManager 同点位）。
 *
 * **不进 DataFrame**（红线3）：批注是元数据，不污染导出与工作流。
 * XML 段：<note> 进 DATableMetadata 束聚合（table-metadata.xml），随工程保存重开恢复。
 * 增删改的撤销由 DACommandsTableStyle 同族命令记录旧新文本（D4），本类不入撤销栈。
 */
class DAGUI_API DATableNoteStore : public QObject
{
    Q_OBJECT
public:
    // 单元格键（数据源绝对行, 绝对列）
    using CellKey = QPair< int, int >;
    // 批注条目（时间戳为团队协作 T6.4 预留）
    struct Note
    {
        QString text;        ///< 批注文本（纯文本，v1 不做富文本/回复线程）
        QDateTime createdAt; ///< 创建时间
        QDateTime updatedAt; ///< 最近修改时间
        bool operator==(const Note& other) const
        {
            return text == other.text && createdAt == other.createdAt && updatedAt == other.updatedAt;
        }
    };

    explicit DATableNoteStore(QObject* parent = nullptr);

    // ===== 查 =====
    bool isEmpty() const;
    int count() const;
    bool hasNote(int row, int col) const;
    // 取条目（无批注返回默认条目，text 为空）
    Note note(int row, int col) const;
    // 取文本（无批注返回空串；delegate tooltip / 角标查询用）
    QString noteText(int row, int col) const;
    // 全部键（按行、列升序，round-trip 与遍历稳定）
    QList< CellKey > keys() const;

    // ===== 增删改（命令层调用；发 notesChanged）=====
    // 写入/更新文本：新建盖 createdAt，修改刷 updatedAt（当前时间）
    void setNote(int row, int col, const QString& text);
    // 直接写整条目（命令 undo/redo 还原时间戳用，不刷新时间）
    void setNoteEntry(int row, int col, const Note& note);
    void removeNote(int row, int col);
    void clearAll();

    // ===== 位置键重映射（DATableKeyRemap 共享工具，「批注跟着行走」）=====
    // 行重排（表头排序，perm 语义同 DATableKeyRemap：perm[i]=新表第 i 行的旧行号）
    void onRowsReordered(const QVector< int >& perm);
    // 行/列插入删除（签名与 DATableStyleManager 一致，一批索引）
    void onRowsInserted(const QList< int >& actualRows);
    void onRowsRemoved(const QList< int >& actualRows);
    void onColumnsInserted(const QList< int >& actualCols);
    void onColumnsRemoved(const QList< int >& actualCols);

    // ===== 整店快照/恢复（就地形状变换等命令的 undo 联动由元数据束统一处理）=====
    QHash< CellKey, Note > snapshot() const;
    void restore(const QHash< CellKey, Note >& notes);

    // ===== 序列化（<note> 段，子元素 <cell r c created updated>文本</cell>）=====
    void toXml(QDomDocument& doc, QDomElement& parent) const;
    bool fromXml(const QDomElement& e);

Q_SIGNALS:
    // 任意变更（增删改/重映射/清空），视图据此重绘角标
    void notesChanged();

private:
    QHash< CellKey, Note > mNotes;  ///< (绝对行, 绝对列) → 批注条目
};
}  // end of namespace DA
#endif  // DATABLENOTESTORE_H
