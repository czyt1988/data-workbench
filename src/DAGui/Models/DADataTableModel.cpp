#include "DADataTableModel.h"
#include "Commands/DACommandsDataFrame.h"
#include "DATableStyleManager.h"
#include "DATableDataSource.h"
#include "DATableDataBlock.h"
#include <QPointer>
namespace DA
{

// 复合表头（QVariantList，如MultiIndex）转为"a | b"文本
static QVariant joinCompositeIndex(const QVariant& res)
{
    if (res.canConvert< QVariantList >()) {
        QVariantList ss = res.toList();
        QString str;
        for (int j = 0; j < ss.size(); ++j) {
            if (j == 0) {
                str += ss[ j ].toString();
            } else {
                str += " | " + ss[ j ].toString();
            }
        }
        return str;
    }
    return res;
}

class DADataTableModel::PrivateData
{
    DA_DECLARE_PUBLIC(DADataTableModel)
public:
    PrivateData(DADataTableModel* p);
    // 根据是否使用缓存来获取对应数据
    bool isNone() const;
    int getDataRowCount() const;
    int getDataColumnCount() const;
    QString getDataframeColumnName(int i) const;
    QVariant getDataframeIndexName(int i) const;
    void clearCacheData();
    // 是否走表格数据源路径（schema走缓存、cell走块缓存）
    bool useTableSourcePath() const;
    // 重新解析tableSource并清空块缓存（数据可能被替换）
    void updateTableSource();
    // 块缓存未命中时按对齐块取数（惰性数据源在此触发分页查询）
    void ensureBlockCached(int actualRow) const;
    // 取单元格：块缓存优先，失败回退逐cell路径
    QVariant getTableCell(int actualRow, int actualColumn) const;
    // 取行头：块缓存优先，失败回退逐行路径
    QVariant getRowHeader(int actualRow) const;

public:
    DAData data;
    DATableStyleManager* styleManager { nullptr };  ///< 用于列显示格式查询，非拥有
    QUndoStack* undoStack { nullptr };
    int extraColumn { 1 };  ///< 扩展的列数，也就是会多显示出externColumn个空白的列，一般多显示出来的是为了用户添加数据用的
    int extraRow { 1 };  ///< 扩展的行数，也就是会多显示出externRow个空白的行，一般多显示出来的是为了用户添加数据用的
    int minShowRow { 20 };         ///< 最小显示的行数
    int minShowColumn { 4 };       ///< 最小显示的列数
    int dataframeRow { 0 };        ///< dataframe的行
    int dataframeColumn { 0 };     ///< dataframe的列
    QList< QString > columnsName;  ///< 列名
    int pageSize { 10000 };        // 每页行数
    int currentPage { 0 };         // 当前页码
                                   // 滑动窗需要的参数
    bool useCacheMode { false };  ///< 是否使用缓存，使用缓存模式，在设置dataframe时，会把dataframe的关键数据直接缓存到内存
    // 表格数据源路径：mutable允许const的data()/headerData()回调中懒加载块
    mutable DATableDataSource* tableSource { nullptr };  ///< 非拥有，setData/refreshData时重新解析
    mutable DATableDataBlock blockCache;                 ///< 当前窗口块缓存
    int blockFetchRowCount { 512 };                      ///< fetchBlock单次取数行数
};

DADataTableModel::PrivateData::PrivateData(DADataTableModel* p) : q_ptr(p)
{
}

bool DADataTableModel::PrivateData::isNone() const
{
    return data.isNull();
}

bool DADataTableModel::PrivateData::useTableSourcePath() const
{
    return tableSource != nullptr;
}

void DADataTableModel::PrivateData::updateTableSource()
{
    tableSource = data.tableSource();
    blockCache.clear();
}

void DADataTableModel::PrivateData::ensureBlockCached(int actualRow) const
{
    if (!tableSource || actualRow < 0) {
        return;
    }
    std::size_t row = static_cast< std::size_t >(actualRow);
    if (blockCache.containsRow(row)) {
        return;
    }
    std::size_t fetchSize = static_cast< std::size_t >(qMax(1, blockFetchRowCount));
    // 对齐到块边界，滚动时缓存命中可预期（每跨过一块边界触发一次取数）
    std::size_t start = (row / fetchSize) * fetchSize;
    blockCache = tableSource->fetchBlock(start, fetchSize);
}

QVariant DADataTableModel::PrivateData::getTableCell(int actualRow, int actualColumn) const
{
    if (tableSource) {
        ensureBlockCached(actualRow);
        if (blockCache.containsRow(static_cast< std::size_t >(actualRow))) {
            return blockCache.cell(static_cast< std::size_t >(actualRow), static_cast< std::size_t >(actualColumn));
        }
        // 块取数失败，回退逐cell路径
    }
    if (data.isDataFrame()) {
        return data.toDataFrame().iat(actualRow, actualColumn);
    } else if (data.isSeries() && actualColumn == 0) {
        return data.toSeries().value(actualRow);
    }
    return QVariant();
}

QVariant DADataTableModel::PrivateData::getRowHeader(int actualRow) const
{
    if (tableSource) {
        ensureBlockCached(actualRow);
        if (blockCache.containsRow(static_cast< std::size_t >(actualRow))) {
            QVariant header = blockCache.rowHeader(static_cast< std::size_t >(actualRow));
            if (header.isValid()) {
                return joinCompositeIndex(header);
            }
            // 数据源未提供行头（如数据库惰性表），显示序号
            return actualRow;
        }
        // 块取数失败，回退逐行路径
    }
    return getDataframeIndexName(actualRow);
}

int DADataTableModel::PrivateData::getDataRowCount() const
{
    if (useCacheMode || useTableSourcePath()) {
        return dataframeRow;
    }
    return static_cast< int >(data.shape().first);
}

int DADataTableModel::PrivateData::getDataColumnCount() const
{
    if (useCacheMode || useTableSourcePath()) {
        return dataframeColumn;
    }
    return static_cast< int >(data.shape().second);
}

QString DADataTableModel::PrivateData::getDataframeColumnName(int i) const
{
    if (useCacheMode || useTableSourcePath()) {
        if (i >= 0 && i < columnsName.size()) {
            return columnsName[ i ];
        }
        return QString();
    } else {
        if (data.isDataFrame()) {
            return data.toDataFrame().columnName(i);
        } else if (data.isSeries() && 0 == i) {
            return data.toSeries().name();
        }
    }
    return QString();
}

QVariant DADataTableModel::PrivateData::getDataframeIndexName(int i) const
{
    QVariant res;
    try {
        if (data.isNull()) {
            // 如果索引为空，就显示序号
            return i;
        }
        if (data.isDataFrame()) {
            res = data.toDataFrame().index().value(i);
        } else if (data.isSeries() && 0 == i) {
            res = data.toSeries().index().value(i);
        }
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return res;
    }
    return joinCompositeIndex(res);
}

void DADataTableModel::PrivateData::clearCacheData()
{
    dataframeRow    = 0;
    dataframeColumn = 0;
    columnsName.clear();
    blockCache.clear();
}

//----------------------------------------------------
// DADataTableModel
//----------------------------------------------------
DADataTableModel::DADataTableModel(QUndoStack* stack, QObject* parent)
    : DAAbstractCacheWindowTableModel(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->undoStack = stack;
}

DADataTableModel::~DADataTableModel()
{
}

int DADataTableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    DA_DC(d);
    if (d->isNone()) {
        return d->minShowColumn;
    }
    return std::max(d->getDataColumnCount() + d->extraColumn, d->minShowColumn);
}

QVariant DADataTableModel::actualHeaderData(int actualSection, Qt::Orientation orientation, int role) const
{
    DA_DC(d);
    if ((role != Qt::DisplayRole && role != Qt::ToolTipRole) || d->isNone()) {
        return QAbstractTableModel::headerData(actualSection, orientation, role);
    }
    // tooltips 和 display

    if (Qt::Horizontal == orientation) {  // 说明是水平表头
        if (actualSection >= d->getDataColumnCount()) {
            return QVariant();
        }
        return d->getDataframeColumnName(actualSection);
    } else {
        if (actualSection >= d->getDataRowCount()) {
            return QVariant();
        }
        // 表格数据源路径行头来自块缓存，避免逐行调python取index
        return d->getRowHeader(actualSection);
    }
    return QVariant();
}

int DADataTableModel::actualRowCount() const
{
    DA_DC(d);
    if (d->isNone()) {
        return d->minShowRow;
    }
    return d->getDataRowCount();
}

Qt::ItemFlags DADataTableModel::actualFlags(int actualRow, int actualColumn) const
{
    Qt::ItemFlags flags = DAAbstractCacheWindowTableModel::actualFlags(actualRow, actualColumn);
    DA_DC(d);
    // 不可编辑的表格数据源（如数据库惰性表）去掉编辑标志，视图不会弹出编辑器
    if (d->tableSource && !d->tableSource->isTableEditable()) {
        flags &= ~Qt::ItemIsEditable;
    }
    return flags;
}

QVariant DADataTableModel::actualData(int actualRow, int actualColumn, int role) const
{
    DA_DC(d);
    if (d->isNone()) {
        return QVariant();
    }
    if (actualRow >= d->getDataRowCount() || actualColumn >= d->getDataColumnCount()) {
        return QVariant();
    }
    switch (role) {
    case Qt::TextAlignmentRole:
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::BackgroundRole:
        return QVariant();
    case Qt::DisplayRole:
    case Qt::EditRole: {
        // 取原始值（EditRole 必须返回原始值，使编辑器拿到原始类型而非格式化字符串）
        // 表格数据源走块缓存批量取数，其余类型保留逐cell路径
        QVariant raw = d->getTableCell(actualRow, actualColumn);
        if (role == Qt::EditRole) {
            return raw;
        }
        // DisplayRole：若有列级显示格式则格式化（同时供 Ctrl+C 复制读取）
        if (d->styleManager && d->styleManager->hasColumnFormat(actualColumn)) {
            return d->styleManager->getColumnFormat(actualColumn).formatValue(raw);
        }
        return raw;
    }
    default:
        break;
    }

    return QVariant();
}

bool DADataTableModel::setActualData(int actualRow, int actualColumn, const QVariant& value, int role)
{
    if (Qt::EditRole != role) {
        return false;
    }
    DA_D(d);
    if (d->isNone()) {
        return false;
    }
    // 表格数据源声明不可编辑（如数据库惰性表）时拒绝编辑
    if (d->tableSource && !d->tableSource->isTableEditable()) {
        return false;
    }
    // 如果启用虚拟化，要计算实际的行号
    if (actualRow >= d->getDataRowCount()) {
        // todo:这里实现一个dataframe追加行
        return false;
    }
    if (actualColumn >= d->getDataColumnCount()) {
        // todo:这里实现一个dataframe追加列
        return false;
    }
    if (d->data.isDataFrame()) {
        DAPyDataFrame df = d->data.toDataFrame();
        QVariant olddata = df.iat(actualRow, actualColumn);
        if (value.isNull() && olddata.isNull()) {
            // 两次都为空就跳过
            // 注意：历史上此处误写为 isNull()==isNull()，caster修复numpy标量转换后
            // olddata不再为空，导致非空->非空的正常单元格编辑被静默拒绝
            return false;
        }
        if (!(d->undoStack)) {
            // 如果d->_undoStack设置为nullptr，将不使用redo/undo
            // 无undo路径不会触发notify回调，需手动失效块缓存
            bool r = df.iat(actualRow, actualColumn, value);
            if (r) {
                d->blockCache.clear();
            }
            return r;
        }
        std::unique_ptr< DACommandDataFrame_iat > cmd_iat(
            new DACommandDataFrame_iat(df, actualRow, actualColumn, olddata, value));
        QPointer< DADataTableModel > modle = this;
        cmd_iat->setCallBack([ modle, actualRow, actualColumn ]() {
            if (modle) {
                modle->notifyDataChanged(actualRow, actualColumn);
            }
        });
        if (!cmd_iat->exec()) {
            // 没设置成功，退出
            return false;
        }
        d->undoStack->push(cmd_iat.release());  // push后会自动调用redo，第二次调用redo会被忽略
        d->undoStack->setActive(true);
    } else {
        // Series 或其他类型暂不支持编辑，返回 false
        return false;
    }
    return true;
}

void DADataTableModel::setData(const DAData& data)
{
    d_ptr->data = data;
    refreshData();
}

void DADataTableModel::setStyleManager(DATableStyleManager* mgr)
{
    d_ptr->styleManager = mgr;
}

DAData DADataTableModel::getData() const
{
    return d_ptr->data;
}

void DADataTableModel::setUseCacheMode(bool on)
{
    d_ptr->useCacheMode = on;
    refreshData();
}

void DADataTableModel::setCacheWindowStartRow(int startRow)
{
    DA_D(d);

    // startRow限制在指定的最小值和最大值之间。它能够确保startRow不会超出给定的范围
    const int dr        = d->getDataRowCount();
    const int cacheSize = getCacheWindowSize();
    if (dr <= cacheSize) {
        startRow = 0;
    } else {
        startRow = qBound(0, startRow, dr - cacheSize + d->extraRow);
        if (startRow >= dr) {
            startRow = dr - 1;
        }
    }
    DAAbstractCacheWindowTableModel::setCacheWindowStartRow(startRow);
    // 表格数据源路径：预取新窗口起始块，避免重绘时逐cell触发取数
    if (d->tableSource && dr > 0) {
        d->ensureBlockCached(startRow);
    }
}

void DADataTableModel::refreshData()
{
    DA_D(d);
    // 数据可能被替换（setPyObject等），重新解析表格数据源并失效块缓存
    d->updateTableSource();
    beginResetModel();
    if (d->useCacheMode) {
        cacheShape();
    } else {
        if (d->tableSource) {
            // 表格数据源路径：缓存schema，避免rowCount/columnCount高频回调反复进python
            cacheShape();
        }
        setCacheWindowStartRow(0);  // 滑动窗口到第一行
    }
    endResetModel();
}

/**
 * @brief cell级变更通知
 *
 * 基类此函数不调用cacheShape()，块缓存不会随之失效，
 * 这里先清空块缓存再走基类通知，保证重绘读到新值
 * @param row 变更的绝对行号
 * @param col 变更的列号
 */
void DADataTableModel::notifyDataChanged(int row, int col)
{
    d_ptr->blockCache.clear();
    DAAbstractCacheWindowTableModel::notifyDataChanged(row, col);
}

/**
 * @brief 区间变更通知
 *
 * 同单cell版本，先失效块缓存再走基类通知
 * @param rowStart 起始绝对行号
 * @param colStart 起始列号
 * @param rowEnd 结束绝对行号
 * @param colEnd 结束列号
 */
void DADataTableModel::notifyDataChanged(int rowStart, int colStart, int rowEnd, int colEnd)
{
    d_ptr->blockCache.clear();
    DAAbstractCacheWindowTableModel::notifyDataChanged(rowStart, colStart, rowEnd, colEnd);
}

/**
 * @brief 超出模型实际数据行数的额外空行数量
 * @return 超出模型实际数据行数的额外空行数量
 * @see setExtraRowCount
 */
int DADataTableModel::getExtraRowCount() const
{
    return d_ptr->extraRow;
}

/**
 * @brief 设置超出模型实际数据列数的额外空列数量。
 *
 * 该函数用于指定在模型中显示的额外空列数量（n），这些列不包含实际数据，
 * 主要用于提供空间给新的插入操作。例如，如果模型有10列实际数据，并调用
 * setExtraColumnCount(5)，则视图将显示15列，其中最后5列为预留的空列。
 *
 * @param n 要添加到现有列数上的额外空列数量。n 应为非负整数。
 *
 * @note
 * - 如果 n 设为0，则仅显示模型中的实际数据列。
 * - 该函数不会影响模型的实际数据内容，只影响视图中显示的列数。
 * - 这些额外的列可以用来方便用户直接在表格末尾进列插入操作。
 *
 * 示例:
 * @code
 * // 假设模型中有10列实际数据
 * model->setExtraRowCount(5); // 视图现在会显示15列，其中最后5列为空列
 * @endcode
 *
 * @see getExtraRowCount()
 */
void DADataTableModel::setExtraColumnCount(int v)
{
    d_ptr->extraColumn = v;
}

/**
 * @brief 超出模型实际数据列数的额外空列数量
 * @return 超出模型实际数据列数的额外空列数量
 * @see setExtraColumnCount
 */
int DADataTableModel::getExtraColumnCount() const
{
    return d_ptr->extraColumn;
}

void DADataTableModel::setMinShowRowCount(int v)
{
    d_ptr->minShowRow = v;
}

int DADataTableModel::getMinShowRowCount() const
{
    return d_ptr->minShowRow;
}

void DADataTableModel::setMinShowColumnCount(int v)
{
    d_ptr->minShowColumn = v;
}

int DADataTableModel::getMinShowColumnCount() const
{
    return d_ptr->minShowColumn;
}

/**
 * @brief 设置块级取数每次获取的行数
 *
 * 仅对DATableDataSource数据生效：滚动/重绘触发的fetchBlock单次取数行数，
 * 越大取数次数越少但单次开销越大，惰性数据源（数据库分页）可按查询延迟调优
 * @param n 行数，最小1
 */
void DADataTableModel::setBlockFetchRowCount(int n)
{
    d_ptr->blockFetchRowCount = qMax(1, n);
}

/**
 * @brief 获取块级取数每次获取的行数
 * @return 行数
 */
int DADataTableModel::getBlockFetchRowCount() const
{
    return d_ptr->blockFetchRowCount;
}

void DADataTableModel::cacheShape()
{
    DA_D(d);
    // notify*系列可能由基类直接调用，先刷新tableSource并失效块缓存，
    // 保证数据内容变化（undo回调、python侧inplace修改通知）后不读到旧块
    d->updateTableSource();
    if (d->tableSource) {
        d->dataframeRow    = static_cast< int >(d->tableSource->tableRowCount());
        d->dataframeColumn = static_cast< int >(d->tableSource->tableColumnCount());
        d->columnsName.clear();
        d->columnsName.reserve(d->dataframeColumn);
        for (int i = 0; i < d->dataframeColumn; ++i) {
            d->columnsName.append(d->tableSource->tableColumnName(i));
        }
        return;
    }
    auto shape         = d->data.shape();
    d->dataframeRow    = static_cast< int >(shape.first);
    d->dataframeColumn = static_cast< int >(shape.second);
    if (d->data.isDataFrame()) {
        d->columnsName = d->data.toDataFrame().columns();
    } else if (d->data.isSeries()) {
        d->columnsName = { d->data.toSeries().name() };
    }
}

}  // end DA
