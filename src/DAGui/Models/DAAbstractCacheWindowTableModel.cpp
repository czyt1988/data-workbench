#include "DAAbstractCacheWindowTableModel.h"
namespace DA
{
DAAbstractCacheWindowTableModel::DAAbstractCacheWindowTableModel(QObject* parent) : QAbstractTableModel(parent)
{
}

DAAbstractCacheWindowTableModel::~DAAbstractCacheWindowTableModel()
{
}

void DAAbstractCacheWindowTableModel::setCacheWindowStartRow(int startRow)
{
    const int oldStart  = mWindowStartRow;
    const int cacheSize = getCacheWindowSize();
    mWindowStartRow     = startRow;
    if (startRow != oldStart) {
        DAAbstractCacheWindowTableModel::setCacheWindowStartRow(startRow);

        // 计算需要刷新的区域
        const int overlapStart = qMax(oldStart, startRow);
        const int overlapEnd   = qMin(oldStart + cacheSize, startRow + cacheSize);

        if (overlapStart >= overlapEnd) {
            // 完全无重叠，全量刷新
            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
            Q_EMIT headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
        } else {
            // 部分刷新
            if (oldStart < startRow) {
                Q_EMIT dataChanged(index(0, 0), index(startRow - oldStart - 1, columnCount() - 1));
                Q_EMIT headerDataChanged(Qt::Vertical, startRow - oldStart - 1, rowCount() - 1);
            }
            if (oldStart + cacheSize > startRow + cacheSize) {
                const int diff = oldStart + cacheSize - (startRow + cacheSize);
                Q_EMIT dataChanged(index(rowCount() - diff, 0), index(rowCount() - 1, columnCount() - 1));
                Q_EMIT headerDataChanged(Qt::Vertical, rowCount() - diff, rowCount() - 1);
            }
        }
    }
}

int DAAbstractCacheWindowTableModel::getCacheWindowStartRow() const
{
    return mWindowStartRow;
}

void DAAbstractCacheWindowTableModel::setCacheWindowSize(int s)
{
    mCacheWindowSize = s;
    setCacheWindowStartRow(mWindowStartRow);
}

int DAAbstractCacheWindowTableModel::getCacheWindowSize() const
{
    return mCacheWindowSize;
}

Qt::ItemFlags DAAbstractCacheWindowTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return actualFlags(index.row() + getCacheWindowStartRow(), index.column());
}

int DAAbstractCacheWindowTableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return qMin(getCacheWindowSize(), actualRowCount());
}

QVariant DAAbstractCacheWindowTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (Qt::Horizontal == orientation) {
        return actualHeaderData(section, orientation, role);
    }
    // 说明是垂直section
    int actualSection = getCacheWindowStartRow() + section;
    return actualHeaderData(actualSection, orientation, role);
}

QVariant DAAbstractCacheWindowTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    int actualRow = getCacheWindowStartRow() + index.row();
    return actualData(actualRow, index.column(), role);
}

bool DAAbstractCacheWindowTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid()) {
        return false;
    }
    int actualRow = getCacheWindowStartRow() + index.row();
    return setActualData(actualRow, index.column(), value, role);
}

Qt::ItemFlags DAAbstractCacheWindowTableModel::actualFlags(int actualRow, int actualColumn) const
{
    Q_UNUSED(actualRow);
    Q_UNUSED(actualColumn);
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

bool DAAbstractCacheWindowTableModel::setActualData(int actualRow, int actualColumn, const QVariant& value, int role)
{
    Q_UNUSED(actualRow);
    Q_UNUSED(actualColumn);
    Q_UNUSED(value);
    Q_UNUSED(role);
    return false;
}

void DAAbstractCacheWindowTableModel::notifyRowChanged(int row)
{
    if (row >= rowCount()) {
        return;
    }
    int c = columnCount() - 1;
    if (c < 0) {
        c = 0;
    }
    cacheShape();
    Q_EMIT dataChanged(createIndex(row, 0), createIndex(row, c));
}

void DAAbstractCacheWindowTableModel::notifyColumnChanged(int col)
{
    if (col >= columnCount()) {
        return;
    }
    int r = rowCount() - 1;
    if (r < 0) {
        r = 0;
    }
    cacheShape();
    Q_EMIT dataChanged(createIndex(0, col), createIndex(r, col));
}

void DAAbstractCacheWindowTableModel::notifyDataChanged(int row, int col)
{
    if (row >= rowCount() || col >= columnCount()) {
        return;
    }
    Q_EMIT dataChanged(createIndex(row, col), createIndex(row, col));
}

void DAAbstractCacheWindowTableModel::notifyDataChanged(int rowStart, int colStart, int rowEnd, int colEnd)
{
    if (rowEnd >= rowCount() || colEnd >= columnCount()) {
        return;
    }
    Q_EMIT dataChanged(createIndex(rowStart, colStart), createIndex(rowEnd, colEnd));
}

void DAAbstractCacheWindowTableModel::notifyRowsRemoved(const QList< int >& r)
{
    if (r.isEmpty()) {
        return;
    }
    // 由于使用了缓存表，删除只需要刷新数据即可
    cacheShape();
    // 获取最小和最大行号
    int minRow = *std::min_element(r.begin(), r.end());
    Q_EMIT dataChanged(createIndex(minRow, 0), createIndex(rowCount() - 1, columnCount() - 1));
}

void DAAbstractCacheWindowTableModel::notifyRowsInserted(const QList< int >& r)
{
    if (r.isEmpty()) {
        return;
    }
    // 由于使用了缓存表，删除只需要刷新数据即可
    cacheShape();
    // 获取最小和最大行号
    int minRow = *std::min_element(r.begin(), r.end());
    Q_EMIT dataChanged(createIndex(minRow, 0), createIndex(rowCount() - 1, columnCount() - 1));
}

void DAAbstractCacheWindowTableModel::notifyColumnsRemoved(const QList< int >& c)
{
    if (c.isEmpty()) {
        return;
    }
    // 由于使用了缓存表，删除只需要刷新数据即可
    cacheShape();
    int minCol = *std::min_element(c.begin(), c.end());
    Q_EMIT dataChanged(createIndex(0, minCol), createIndex(rowCount() - 1, columnCount() - 1));
}

void DAAbstractCacheWindowTableModel::notifyColumnsInserted(const QList< int >& c)
{
    if (c.isEmpty()) {
        return;
    }
    // 由于使用了缓存表，删除只需要刷新数据即可
    cacheShape();
    int minCol = *std::min_element(c.begin(), c.end());
    Q_EMIT dataChanged(createIndex(0, minCol), createIndex(rowCount() - 1, columnCount() - 1));
}

void DAAbstractCacheWindowTableModel::cacheShape()
{
}
}
