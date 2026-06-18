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
    const int oldStart = mWindowStartRow;
    if (startRow == oldStart) {
        return;
    }
    mWindowStartRow = startRow;

    // 窗口滑动后，所有可视行的数据映射都发生了变化
    // （visual row i 从 oldStart+i 变为 startRow+i），
    // 需要对整个可视窗口发射 dataChanged 和 headerDataChanged。
    // Qt 只会重绘 viewport 中实际可见的区域，所以发射大范围信号不会造成额外开销。
    const int rows = rowCount();
    const int cols = columnCount();
    if (rows > 0 && cols > 0) {
        Q_EMIT dataChanged(index(0, 0), index(rows - 1, cols - 1));
        Q_EMIT headerDataChanged(Qt::Vertical, 0, rows - 1);
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
