#include "DATableStyleManager.h"
#include <algorithm>

namespace DA
{

namespace
{
// 位移 QHash<int, V> 的键：key >= threshold 的键 +offset
template < typename V >
void shiftHashKeys(QHash< int, V >& hash, int threshold, int offset)
{
    QHash< int, V > shifted;
    for (auto it = hash.begin(); it != hash.end();) {
        if (it.key() >= threshold) {
            shifted.insert(it.key() + offset, it.value());
            it = hash.erase(it);
        } else {
            ++it;
        }
    }
    hash.unite(shifted);
}

// 位移 QHash<QPair<int,int>, V> 中第一维的键
template < typename V >
void shiftHashKeysFirst(QHash< QPair< int, int >, V >& hash, int threshold, int offset)
{
    QHash< QPair< int, int >, V > shifted;
    for (auto it = hash.begin(); it != hash.end();) {
        if (it.key().first >= threshold) {
            shifted.insert(qMakePair(it.key().first + offset, it.key().second), it.value());
            it = hash.erase(it);
        } else {
            ++it;
        }
    }
    hash.unite(shifted);
}

// 位移 QHash<QPair<int,int>, V> 中第二维的键
template < typename V >
void shiftHashKeysSecond(QHash< QPair< int, int >, V >& hash, int threshold, int offset)
{
    QHash< QPair< int, int >, V > shifted;
    for (auto it = hash.begin(); it != hash.end();) {
        if (it.key().second >= threshold) {
            shifted.insert(qMakePair(it.key().first, it.key().second + offset), it.value());
            it = hash.erase(it);
        } else {
            ++it;
        }
    }
    hash.unite(shifted);
}

// 删除 QHash<QPair<int,int>, V> 中第一维等于 val 的条目
template < typename V >
void removeHashKeysFirst(QHash< QPair< int, int >, V >& hash, int val)
{
    for (auto it = hash.begin(); it != hash.end();) {
        if (it.key().first == val) {
            it = hash.erase(it);
        } else {
            ++it;
        }
    }
}

// 删除 QHash<QPair<int,int>, V> 中第二维等于 val 的条目
template < typename V >
void removeHashKeysSecond(QHash< QPair< int, int >, V >& hash, int val)
{
    for (auto it = hash.begin(); it != hash.end();) {
        if (it.key().second == val) {
            it = hash.erase(it);
        } else {
            ++it;
        }
    }
}
}  // namespace

/**
 * @brief 构造函数
 * @param parent 父对象
 */
DATableStyleManager::DATableStyleManager(QObject* parent) : QObject(parent)
{
}

/**
 * @brief 渲染查询：返回该单元格最终叠加样式
 *
 * 叠加优先级：列 → 行 → 单元格（后者覆盖前者已设置的属性）。
 * @param actualRow 真实行号
 * @param actualCol 真实列号
 * @return 叠加后的最终样式
 */
DATableCellStyle DATableStyleManager::resolveCellStyle(int actualRow, int actualCol) const
{
    DATableCellStyle result;
    auto colIt = mColumnStyles.find(actualCol);
    if (colIt != mColumnStyles.end()) {
        result.mergeFrom(colIt.value());
    }
    auto rowIt = mRowStyles.find(actualRow);
    if (rowIt != mRowStyles.end()) {
        result.mergeFrom(rowIt.value());
    }
    auto cellIt = mCellStyles.find(qMakePair(actualRow, actualCol));
    if (cellIt != mCellStyles.end()) {
        result.mergeFrom(cellIt.value());
    }
    return result;
}

/**
 * @brief 是否存在单元格级样式
 * @param actualRow 真实行号
 * @param actualCol 真实列号
 * @return 存在返回 true
 */
bool DATableStyleManager::hasCellStyle(int actualRow, int actualCol) const
{
    return mCellStyles.contains(qMakePair(actualRow, actualCol));
}

/**
 * @brief 获取单元格级样式
 * @param actualRow 真实行号
 * @param actualCol 真实列号
 * @return 样式对象，不存在时返回空的 DATableCellStyle
 */
DATableCellStyle DATableStyleManager::getCellStyle(int actualRow, int actualCol) const
{
    auto it = mCellStyles.find(qMakePair(actualRow, actualCol));
    return (it != mCellStyles.end()) ? it.value() : DATableCellStyle();
}

/**
 * @brief 设置单元格级样式
 * @param actualRow 真实行号
 * @param actualCol 真实列号
 * @param s 样式
 * @param merge true 时合并到已有样式，false 时整体替换
 */
void DATableStyleManager::setCellStyle(int actualRow, int actualCol, const DATableCellStyle& s, bool merge)
{
    QPair< int, int > key = qMakePair(actualRow, actualCol);
    if (merge && mCellStyles.contains(key)) {
        mCellStyles[ key ].mergeFrom(s);
    } else {
        mCellStyles[ key ] = s;
    }
    Q_EMIT styleChanged(actualRow, actualCol);
}

/**
 * @brief 清除单元格级样式
 * @param actualRow 真实行号
 * @param actualCol 真实列号
 */
void DATableStyleManager::clearCell(int actualRow, int actualCol)
{
    mCellStyles.remove(qMakePair(actualRow, actualCol));
    Q_EMIT styleChanged(actualRow, actualCol);
}

/**
 * @brief 是否存在列级样式
 * @param actualCol 真实列号
 * @return 存在返回 true
 */
bool DATableStyleManager::hasColumnStyle(int actualCol) const
{
    return mColumnStyles.contains(actualCol);
}

/**
 * @brief 获取列级样式
 * @param actualCol 真实列号
 * @return 样式对象，不存在时返回空的 DATableCellStyle
 */
DATableCellStyle DATableStyleManager::getColumnStyle(int actualCol) const
{
    auto it = mColumnStyles.find(actualCol);
    return (it != mColumnStyles.end()) ? it.value() : DATableCellStyle();
}

/**
 * @brief 设置列级样式
 * @param actualCol 真实列号
 * @param s 样式
 * @param merge true 时合并，false 时替换
 */
void DATableStyleManager::setColumnStyle(int actualCol, const DATableCellStyle& s, bool merge)
{
    if (merge && mColumnStyles.contains(actualCol)) {
        mColumnStyles[ actualCol ].mergeFrom(s);
    } else {
        mColumnStyles[ actualCol ] = s;
    }
    Q_EMIT styleRangeChanged(0, actualCol, -1, actualCol);
}

/**
 * @brief 清除列级样式
 * @param actualCol 真实列号
 */
void DATableStyleManager::clearColumn(int actualCol)
{
    mColumnStyles.remove(actualCol);
    Q_EMIT styleRangeChanged(0, actualCol, -1, actualCol);
}

/**
 * @brief 获取所有有样式的列号
 * @return 列号列表
 */
QList< int > DATableStyleManager::styledColumns() const
{
    return mColumnStyles.keys();
}

/**
 * @brief 是否存在行级样式
 * @param actualRow 真实行号
 * @return 存在返回 true
 */
bool DATableStyleManager::hasRowStyle(int actualRow) const
{
    return mRowStyles.contains(actualRow);
}

/**
 * @brief 获取行级样式
 * @param actualRow 真实行号
 * @return 样式对象，不存在时返回空的 DATableCellStyle
 */
DATableCellStyle DATableStyleManager::getRowStyle(int actualRow) const
{
    auto it = mRowStyles.find(actualRow);
    return (it != mRowStyles.end()) ? it.value() : DATableCellStyle();
}

/**
 * @brief 设置行级样式
 * @param actualRow 真实行号
 * @param s 样式
 * @param merge true 时合并，false 时替换
 */
void DATableStyleManager::setRowStyle(int actualRow, const DATableCellStyle& s, bool merge)
{
    if (merge && mRowStyles.contains(actualRow)) {
        mRowStyles[ actualRow ].mergeFrom(s);
    } else {
        mRowStyles[ actualRow ] = s;
    }
    Q_EMIT styleRangeChanged(actualRow, 0, actualRow, -1);
}

/**
 * @brief 清除行级样式
 * @param actualRow 真实行号
 */
void DATableStyleManager::clearRow(int actualRow)
{
    mRowStyles.remove(actualRow);
    Q_EMIT styleRangeChanged(actualRow, 0, actualRow, -1);
}

/**
 * @brief 获取所有有样式的行号
 * @return 行号列表
 */
QList< int > DATableStyleManager::styledRows() const
{
    return mRowStyles.keys();
}

/**
 * @brief 获取所有有样式的单元格键列表
 * @return (actualRow, actualCol) 键列表
 */
QList< QPair< int, int > > DATableStyleManager::styledCells() const
{
    return mCellStyles.keys();
}

/**
 * @brief 清除范围内的单元格级样式
 *
 * 仅清除单元格级（mCellStyles）样式，不影响行级/列级。
 * 如需清除整行/整列样式，使用 clearRow/clearColumn。
 * @param rowStart 起始行
 * @param colStart 起始列
 * @param rowEnd 结束行
 * @param colEnd 结束列
 */
void DATableStyleManager::clearRange(int rowStart, int colStart, int rowEnd, int colEnd)
{
    // 规范化范围，确保 start <= end
    if (rowStart > rowEnd) std::swap(rowStart, rowEnd);
    if (colStart > colEnd) std::swap(colStart, colEnd);

    QList< QPair< int, int > > toRemove;
    for (auto it = mCellStyles.begin(); it != mCellStyles.end(); ++it) {
        int r = it.key().first;
        int c = it.key().second;
        if (r >= rowStart && r <= rowEnd && c >= colStart && c <= colEnd) {
            toRemove.append(it.key());
        }
    }
    for (const auto& k : std::as_const(toRemove)) {
        mCellStyles.remove(k);
    }
    Q_EMIT styleRangeChanged(rowStart, colStart, rowEnd, colEnd);
}

/**
 * @brief 清除三个维度的全部样式
 */
void DATableStyleManager::clearAll()
{
    mCellStyles.clear();
    mColumnStyles.clear();
    mRowStyles.clear();
    Q_EMIT styleReset();
}

/**
 * @brief 行插入同步：三个维度联动
 *
 * 单元格级和行级中 row >= insertedRow 的键 +1（降序处理避免连锁覆盖）。
 * 列级不受影响。
 * @param actualRows 插入的真实行号列表
 */
void DATableStyleManager::onRowsInserted(const QList< int >& actualRows)
{
    if (actualRows.isEmpty()) {
        return;
    }
    QList< int > sortedRows = actualRows;
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater< int >());
    for (int insertedRow : std::as_const(sortedRows)) {
        shiftHashKeysFirst(mCellStyles, insertedRow, +1);
        shiftHashKeys(mRowStyles, insertedRow, +1);
    }
    Q_EMIT styleReset();
}

/**
 * @brief 行删除同步：三个维度联动
 *
 * 单元格级和行级中删除 row == removedRow 的键，row > removedRow 的键 -1。
 * 列级不受影响。
 * @param actualRows 删除的真实行号列表
 */
void DATableStyleManager::onRowsRemoved(const QList< int >& actualRows)
{
    if (actualRows.isEmpty()) {
        return;
    }
    QList< int > sortedRows = actualRows;
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater< int >());
    for (int removedRow : std::as_const(sortedRows)) {
        // 先删除 row == removedRow 的条目，再位移 row > removedRow 的条目
        removeHashKeysFirst(mCellStyles, removedRow);
        mRowStyles.remove(removedRow);
        shiftHashKeysFirst(mCellStyles, removedRow + 1, -1);
        shiftHashKeys(mRowStyles, removedRow + 1, -1);
    }
    Q_EMIT styleReset();
}

/**
 * @brief 列插入同步：三个维度联动
 *
 * 单元格级和列级中 col >= insertedCol 的键 +1（降序处理避免连锁覆盖）。
 * 行级不受影响。
 * @param actualCols 插入的真实列号列表
 */
void DATableStyleManager::onColumnsInserted(const QList< int >& actualCols)
{
    if (actualCols.isEmpty()) {
        return;
    }
    QList< int > sortedCols = actualCols;
    std::sort(sortedCols.begin(), sortedCols.end(), std::greater< int >());
    for (int insertedCol : std::as_const(sortedCols)) {
        shiftHashKeysSecond(mCellStyles, insertedCol, +1);
        shiftHashKeys(mColumnStyles, insertedCol, +1);
    }
    Q_EMIT styleReset();
}

/**
 * @brief 列删除同步：三个维度联动
 *
 * 单元格级和列级中删除 col == removedCol 的键，col > removedCol 的键 -1。
 * 行级不受影响。
 * @param actualCols 删除的真实列号列表
 */
void DATableStyleManager::onColumnsRemoved(const QList< int >& actualCols)
{
    if (actualCols.isEmpty()) {
        return;
    }
    QList< int > sortedCols = actualCols;
    std::sort(sortedCols.begin(), sortedCols.end(), std::greater< int >());
    for (int removedCol : std::as_const(sortedCols)) {
        // 先删除 col == removedCol 的条目，再位移 col > removedCol 的条目
        removeHashKeysSecond(mCellStyles, removedCol);
        mColumnStyles.remove(removedCol);
        shiftHashKeysSecond(mCellStyles, removedCol + 1, -1);
        shiftHashKeys(mColumnStyles, removedCol + 1, -1);
    }
    Q_EMIT styleReset();
}

/**
 * @brief 序列化到 XML 元素
 *
 * 写入列级（column-style）、行级（row-style）、单元格级（cell）三种标签。
 * @param doc QDomDocument
 * @param e 目标父元素（table 元素）
 */
void DATableStyleManager::toXml(QDomDocument& doc, QDomElement& e) const
{
    // 列级
    for (auto it = mColumnStyles.begin(); it != mColumnStyles.end(); ++it) {
        QDomElement cs = doc.createElement(QStringLiteral("column-style"));
        cs.setAttribute(QStringLiteral("col"), it.key());
        it.value().toXml(doc, cs);
        e.appendChild(cs);
    }
    // 行级
    for (auto it = mRowStyles.begin(); it != mRowStyles.end(); ++it) {
        QDomElement rs = doc.createElement(QStringLiteral("row-style"));
        rs.setAttribute(QStringLiteral("row"), it.key());
        it.value().toXml(doc, rs);
        e.appendChild(rs);
    }
    // 单元格级
    for (auto it = mCellStyles.begin(); it != mCellStyles.end(); ++it) {
        QDomElement ce = doc.createElement(QStringLiteral("cell"));
        ce.setAttribute(QStringLiteral("row"), it.key().first);
        ce.setAttribute(QStringLiteral("col"), it.key().second);
        it.value().toXml(doc, ce);
        e.appendChild(ce);
    }
}

/**
 * @brief 从 XML 元素反序列化
 *
 * 解析 column-style/row-style/cell 三种标签，先 clearAll 再写入。
 * 未知标签忽略（前向兼容）。
 * @param e 源元素（table 元素）
 * @return 始终返回 true
 */
bool DATableStyleManager::fromXml(const QDomElement& e)
{
    clearAll();
    QDomNode n = e.firstChild();
    while (!n.isNull()) {
        QDomElement child = n.toElement();
        if (child.isNull()) {
            n = n.nextSibling();
            continue;
        }
        QString tag = child.tagName();
        if (tag == QLatin1String("column-style")) {
            int col = child.attribute(QStringLiteral("col")).toInt();
            DATableCellStyle s;
            s.fromXml(child);
            mColumnStyles[ col ] = s;
        } else if (tag == QLatin1String("row-style")) {
            int row = child.attribute(QStringLiteral("row")).toInt();
            DATableCellStyle s;
            s.fromXml(child);
            mRowStyles[ row ] = s;
        } else if (tag == QLatin1String("cell")) {
            int row = child.attribute(QStringLiteral("row")).toInt();
            int col = child.attribute(QStringLiteral("col")).toInt();
            DATableCellStyle s;
            s.fromXml(child);
            mCellStyles[ qMakePair(row, col) ] = s;
        }
        // 未知标签忽略
        n = n.nextSibling();
    }
    return true;
}

/**
 * @brief 三个维度是否全部为空
 * @return 全部为空返回 true
 */
bool DATableStyleManager::isEmpty() const
{
    return mCellStyles.isEmpty() && mColumnStyles.isEmpty() && mRowStyles.isEmpty();
}

}  // end of namespace DA
