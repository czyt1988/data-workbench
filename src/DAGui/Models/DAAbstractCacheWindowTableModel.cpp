#include "DAAbstractCacheWindowTableModel.h"
namespace DA
{
DAAbstractCacheWindowTableModel::DAAbstractCacheWindowTableModel(QObject* parent) : QAbstractTableModel(parent)
{
}

DAAbstractCacheWindowTableModel::~DAAbstractCacheWindowTableModel()
{
}

/**
 * @brief 设置窗口起始行
 *
 * 起始行统一在基类夹取到 [0, maxCacheWindowStartRow()]，派生类不需要再各自夹取；
 * 传入当前值时也会重新夹取一次（数据变少后能把窗口拉回合法范围）。
 * @param startRow 期望的窗口起始行（数据源绝对行）
 */
void DAAbstractCacheWindowTableModel::setCacheWindowStartRow(int startRow)
{
    const int newStart = qBound(0, startRow, maxCacheWindowStartRow());
    if (newStart == mWindowStartRow) {
        return;
    }
    const int oldStart = mWindowStartRow;
    mWindowStartRow    = newStart;

    // 窗口滑动后，所有可视行的数据映射都发生了变化
    // （visual row i 从 oldStart+i 变为 startRow+i），
    // 需要对整个可视窗口发射 dataChanged 和 headerDataChanged。
    // Qt 只会重绘 viewport 中实际可见的区域，所以发射大范围信号不会造成额外开销。
    refreshWindow();
    Q_EMIT cacheWindowChanged(oldStart, newStart);
}

/**
 * @brief 窗口起始行的合法上界
 *
 * 窗口需要覆盖「全部数据行 + 尾部额外空行」；整表装得下时窗口只能停在 0，
 * 否则起始行大于 0 会让首行永远滚不回来。
 * @return 最大合法起始行
 */
int DAAbstractCacheWindowTableModel::maxCacheWindowStartRow() const
{
    const int total = actualRowCount() + getExtraRowCount();
    if (total <= getCacheWindowSize()) {
        return 0;
    }
    return total - getCacheWindowSize();
}

/**
 * @brief 窗口尾部额外空行数量（基类默认无额外空行）
 * @return 额外空行数
 */
int DAAbstractCacheWindowTableModel::getExtraRowCount() const
{
    return 0;
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

/**
 * @brief 整窗刷新：对窗口内所有行列发 dataChanged，并刷新纵向表头
 */
void DAAbstractCacheWindowTableModel::refreshWindow()
{
    const int rows = rowCount();
    const int cols = columnCount();
    if (rows <= 0 || cols <= 0) {
        return;
    }
    Q_EMIT dataChanged(index(0, 0), index(rows - 1, cols - 1));
    Q_EMIT headerDataChanged(Qt::Vertical, 0, rows - 1);
}

/**
 * @brief 单行数据变更通知
 * @param row 变更行的绝对行号
 */
void DAAbstractCacheWindowTableModel::notifyRowChanged(int row)
{
    cacheShape();
    notifyDataChanged(row, 0, row, columnCount() - 1);
}

/**
 * @brief 单列数据变更通知
 * @param col 变更列号
 */
void DAAbstractCacheWindowTableModel::notifyColumnChanged(int col)
{
    if (col < 0 || col >= columnCount()) {
        return;
    }
    cacheShape();
    const int r = qMax(0, rowCount() - 1);
    Q_EMIT dataChanged(createIndex(0, col), createIndex(r, col));
}

/**
 * @brief 单元格数据变更通知
 * @param row 变更行的绝对行号
 * @param col 变更列号
 */
void DAAbstractCacheWindowTableModel::notifyDataChanged(int row, int col)
{
    notifyDataChanged(row, col, row, col);
}

/**
 * @brief 区间数据变更通知
 *
 * 形参为绝对行号，落在窗口外的部分会被裁掉；完全在窗口外时静默返回。
 * @param rowStart 起始绝对行号
 * @param colStart 起始列号
 * @param rowEnd 结束绝对行号
 * @param colEnd 结束列号
 */
void DAAbstractCacheWindowTableModel::notifyDataChanged(int rowStart, int colStart, int rowEnd, int colEnd)
{
    const int offset = getCacheWindowStartRow();
    int r0           = qMin(rowStart, rowEnd) - offset;
    int r1           = qMax(rowStart, rowEnd) - offset;
    const int rows   = rowCount();
    if (r1 < 0 || r0 >= rows) {
        return;  // 完全落在窗口外
    }
    r0               = qMax(0, r0);
    r1               = qMin(r1, rows - 1);
    const int cols   = columnCount();
    int c0           = qMax(0, qMin(colStart, colEnd));
    int c1           = qMin(cols - 1, qMax(colStart, colEnd));
    if (c1 < c0 || c1 < 0) {
        return;
    }
    Q_EMIT dataChanged(createIndex(r0, c0), createIndex(r1, c1));
}

/**
 * @brief 行移除通知
 *
 * 结构变化会让窗口内所有行的映射整体位移，因此按整窗刷新。
 * @param r 被移除行的绝对行号列表
 */
void DAAbstractCacheWindowTableModel::notifyRowsRemoved(const QList< int >& r)
{
    if (r.isEmpty()) {
        return;
    }
    cacheShape();
    refreshWindow();
    Q_EMIT tableStructureChanged();
}

/**
 * @brief 行插入通知
 * @param r 被插入行的绝对行号列表
 */
void DAAbstractCacheWindowTableModel::notifyRowsInserted(const QList< int >& r)
{
    if (r.isEmpty()) {
        return;
    }
    cacheShape();
    refreshWindow();
    Q_EMIT tableStructureChanged();
}

/**
 * @brief 列移除通知
 * @param c 被移除列的列号列表
 */
void DAAbstractCacheWindowTableModel::notifyColumnsRemoved(const QList< int >& c)
{
    if (c.isEmpty()) {
        return;
    }
    cacheShape();
    refreshWindow();
    Q_EMIT tableStructureChanged();
}

/**
 * @brief 列插入通知
 * @param c 被插入列的列号列表
 */
void DAAbstractCacheWindowTableModel::notifyColumnsInserted(const QList< int >& c)
{
    if (c.isEmpty()) {
        return;
    }
    cacheShape();
    refreshWindow();
    Q_EMIT tableStructureChanged();
}

void DAAbstractCacheWindowTableModel::cacheShape()
{
}
}
