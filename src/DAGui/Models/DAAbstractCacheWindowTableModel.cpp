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
        } else {
            // 部分刷新
            if (oldStart < startRow) {
                Q_EMIT dataChanged(index(0, 0), index(startRow - oldStart - 1, columnCount() - 1));
            }
            if (oldStart + cacheSize > startRow + cacheSize) {
                const int diff = oldStart + cacheSize - (startRow + cacheSize);
                Q_EMIT dataChanged(index(rowCount() - diff, 0), index(rowCount() - 1, columnCount() - 1));
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
}
