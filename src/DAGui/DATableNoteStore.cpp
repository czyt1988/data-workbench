#include "DATableNoteStore.h"
#include "DATableKeyRemap.h"
#include <algorithm>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父对象（一般为元数据束）
 */
DATableNoteStore::DATableNoteStore(QObject* parent) : QObject(parent)
{
}

bool DATableNoteStore::isEmpty() const
{
    return mNotes.isEmpty();
}

int DATableNoteStore::count() const
{
    return mNotes.size();
}

bool DATableNoteStore::hasNote(int row, int col) const
{
    return mNotes.contains(qMakePair(row, col));
}

DATableNoteStore::Note DATableNoteStore::note(int row, int col) const
{
    return mNotes.value(qMakePair(row, col));
}

QString DATableNoteStore::noteText(int row, int col) const
{
    auto it = mNotes.constFind(qMakePair(row, col));
    return it == mNotes.constEnd() ? QString() : it->text;
}

/**
 * @brief 全部键（按行、列升序——XML round-trip 与遍历稳定）
 */
QList< DATableNoteStore::CellKey > DATableNoteStore::keys() const
{
    QList< CellKey > res = mNotes.keys();
    std::sort(res.begin(), res.end(), [](const CellKey& a, const CellKey& b) {
        return (a.first != b.first) ? (a.first < b.first) : (a.second < b.second);
    });
    return res;
}

/**
 * @brief 写入/更新批注文本（新建盖 createdAt，修改刷 updatedAt）
 *
 * 空文本等价删除（批注没有"空内容"状态）。命令层记录旧新条目后经 setNoteEntry
 * 还原时间戳；本入口给"用户新写"路径，时间戳取当前时间。
 */
void DATableNoteStore::setNote(int row, int col, const QString& text)
{
    if (text.isEmpty()) {
        removeNote(row, col);
        return;
    }
    const CellKey key  = qMakePair(row, col);
    const QDateTime now = QDateTime::currentDateTime();
    auto it            = mNotes.find(key);
    if (it == mNotes.end()) {
        Note n;
        n.text      = text;
        n.createdAt = now;
        n.updatedAt = now;
        mNotes.insert(key, n);
    } else {
        if (it->text == text) {
            return;  // 文本未变不动时间戳不发信号
        }
        it->text      = text;
        it->updatedAt = now;
    }
    Q_EMIT notesChanged();
}

/**
 * @brief 直接写整条目（命令 undo/redo 还原时间戳用，不刷新时间）
 */
void DATableNoteStore::setNoteEntry(int row, int col, const Note& note)
{
    if (note.text.isEmpty()) {
        removeNote(row, col);
        return;
    }
    mNotes.insert(qMakePair(row, col), note);
    Q_EMIT notesChanged();
}

void DATableNoteStore::removeNote(int row, int col)
{
    if (mNotes.remove(qMakePair(row, col)) > 0) {
        Q_EMIT notesChanged();
    }
}

void DATableNoteStore::clearAll()
{
    if (mNotes.isEmpty()) {
        return;
    }
    mNotes.clear();
    Q_EMIT notesChanged();
}

/**
 * @brief 行重排同步（表头排序 D1）：批注跟着行走（设计稿 §4.10）
 *
 * perm 语义同 DATableKeyRemap：perm[i] = 新表第 i 行在旧表中的行号（新→旧）。
 * @param perm 正置换；空视为无操作
 */
void DATableNoteStore::onRowsReordered(const QVector< int >& perm)
{
    if (perm.isEmpty() || mNotes.isEmpty()) {
        return;
    }
    DATableKeyRemap::remapKeys(mNotes, perm, DATableKeyRemap::Dimension::Row);
    Q_EMIT notesChanged();
}

/**
 * @brief 行插入同步：row >= 插入行的键 +1（降序逐个处理避免连锁覆盖）
 * @param actualRows 插入的真实行号列表
 */
void DATableNoteStore::onRowsInserted(const QList< int >& actualRows)
{
    if (actualRows.isEmpty() || mNotes.isEmpty()) {
        return;
    }
    DATableKeyRemap::shiftKeysOnInserted(mNotes, actualRows, DATableKeyRemap::Dimension::Row);
    Q_EMIT notesChanged();
}

/**
 * @brief 行删除同步：row == 删除行的批注删除，row > 删除行的键 -1
 * @param actualRows 删除的真实行号列表
 */
void DATableNoteStore::onRowsRemoved(const QList< int >& actualRows)
{
    if (actualRows.isEmpty() || mNotes.isEmpty()) {
        return;
    }
    DATableKeyRemap::shiftKeysOnRemoved(mNotes, actualRows, DATableKeyRemap::Dimension::Row);
    Q_EMIT notesChanged();
}

/**
 * @brief 列插入同步：col >= 插入列的键 +1
 * @param actualCols 插入的真实列号列表
 */
void DATableNoteStore::onColumnsInserted(const QList< int >& actualCols)
{
    if (actualCols.isEmpty() || mNotes.isEmpty()) {
        return;
    }
    DATableKeyRemap::shiftKeysOnInserted(mNotes, actualCols, DATableKeyRemap::Dimension::Column);
    Q_EMIT notesChanged();
}

/**
 * @brief 列删除同步：col == 删除列的批注删除，col > 删除列的键 -1
 * @param actualCols 删除的真实列号列表
 */
void DATableNoteStore::onColumnsRemoved(const QList< int >& actualCols)
{
    if (actualCols.isEmpty() || mNotes.isEmpty()) {
        return;
    }
    DATableKeyRemap::shiftKeysOnRemoved(mNotes, actualCols, DATableKeyRemap::Dimension::Column);
    Q_EMIT notesChanged();
}

QHash< DATableNoteStore::CellKey, DATableNoteStore::Note > DATableNoteStore::snapshot() const
{
    return mNotes;
}

void DATableNoteStore::restore(const QHash< CellKey, Note >& notes)
{
    mNotes = notes;
    Q_EMIT notesChanged();
}

/**
 * @brief 序列化 <note> 段（子元素 <cell r c created updated>文本</cell>，键升序稳定）
 */
void DATableNoteStore::toXml(QDomDocument& doc, QDomElement& parent) const
{
    QDomElement e = doc.createElement(QStringLiteral("note"));
    const QList< CellKey > sortedKeys = keys();
    for (const CellKey& k : sortedKeys) {
        const Note& n  = mNotes[ k ];
        QDomElement ce = doc.createElement(QStringLiteral("cell"));
        ce.setAttribute(QStringLiteral("r"), k.first);
        ce.setAttribute(QStringLiteral("c"), k.second);
        if (n.createdAt.isValid()) {
            ce.setAttribute(QStringLiteral("created"), n.createdAt.toString(Qt::ISODateWithMs));
        }
        if (n.updatedAt.isValid()) {
            ce.setAttribute(QStringLiteral("updated"), n.updatedAt.toString(Qt::ISODateWithMs));
        }
        ce.appendChild(doc.createTextNode(n.text));
        e.appendChild(ce);
    }
    parent.appendChild(e);
}

/**
 * @brief 从 <note> 段反序列化（先自清空；未知属性忽略，前向兼容）
 * @return 始终返回 true
 */
bool DATableNoteStore::fromXml(const QDomElement& e)
{
    mNotes.clear();
    QDomElement ce = e.firstChildElement(QStringLiteral("cell"));
    while (!ce.isNull()) {
        Note n;
        n.text      = ce.text();
        n.createdAt = QDateTime::fromString(ce.attribute(QStringLiteral("created")), Qt::ISODateWithMs);
        n.updatedAt = QDateTime::fromString(ce.attribute(QStringLiteral("updated")), Qt::ISODateWithMs);
        const int r = ce.attribute(QStringLiteral("r"), QStringLiteral("-1")).toInt();
        const int c = ce.attribute(QStringLiteral("c"), QStringLiteral("-1")).toInt();
        if (!n.text.isEmpty() && r >= 0 && c >= 0) {
            mNotes.insert(qMakePair(r, c), n);
        }
        ce = ce.nextSiblingElement(QStringLiteral("cell"));
    }
    Q_EMIT notesChanged();
    return true;
}

}  // end of namespace DA
