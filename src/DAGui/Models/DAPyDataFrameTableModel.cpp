#include "DAPyDataFrameTableModel.h"
#include "Commands/DACommandsDataFrame.h"
#include <QUndoStack>
#include <algorithm>
#include <QCache>
#ifndef DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT
#define DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT 1
#endif
#ifndef DAPYDATAFRAMETABLEMODULE_PROFILE_REALTIME_PRINT
#define DAPYDATAFRAMETABLEMODULE_PROFILE_REALTIME_PRINT 1
#endif
#if DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT
#include <QElapsedTimer>
#if DAPYDATAFRAMETABLEMODULE_PROFILE_REALTIME_PRINT
#define DAPYDATAFRAMETABLEMODEL_CALL_ADD(d_valuename)                                                                  \
    do {                                                                                                               \
        ++(d_ptr->d_valuename);                                                                                        \
        if (d_ptr->d_valuename % 10000 == 0) {                                                                         \
            qDebug() << #d_valuename "=" << d_ptr->d_valuename;                                                        \
        }                                                                                                              \
    } while (0)
#else
#define DAPYDATAFRAMETABLEMODEL_CALL_ADD(d_valuename) ++(d_ptr->d_valuename)
#endif
#else
// do nothing
#define DAPYDATAFRAMETABLEMODEL_CALL_ADD
#endif
namespace DA
{
class DAPyDataFrameTableModel::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyDataFrameTableModel)
public:
    PrivateData(DAPyDataFrameTableModel* p);
    // 根据是否使用缓存来获取对应数据
    bool isNoneDataframe() const;
    int getDataframeRowCount() const;
    int getDataframeColumnCount() const;
    QString getDataframeColumnName(int i) const;
    QVariant getDataframeIndexName(int i) const;
    void clearCacheData();

public:
    DAPyDataFrame dataframe;
    QUndoStack* undoStack { nullptr };
    int extraColumn { 1 };  ///< 扩展的列数，也就是会多显示出externColumn个空白的列，一般多显示出来的是为了用户添加数据用的
    int extraRow { 1 };  ///< 扩展的行数，也就是会多显示出externRow个空白的行，一般多显示出来的是为了用户添加数据用的
    int minShowRow { 20 };    ///< 最小显示的行数
    int minShowColumn { 4 };  ///< 最小显示的列数
#if DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT
    mutable std::size_t cntheaderData { 0 };
    mutable std::size_t cntcolumnCount { 0 };
    mutable std::size_t cntdata { 0 };
#endif
    int dataframeRow { 0 };        ///< dataframe的行
    int dataframeColumn { 0 };     ///< dataframe的列
    QList< QString > columnsName;  ///< 列名
                                   // 虚拟视图 -- 分页需要的参数
    int pageSize { 10000 };        // 每页行数
    int currentPage { 0 };         // 当前页码
                                   // 滑动窗需要的参数
    bool useCacheMode { false };  ///< 是否使用缓存，使用缓存模式，在设置dataframe时，会把dataframe的关键数据直接缓存到内存
};

//===================================================
// DAPyDataFrameTableModulePrivate
//===================================================

DAPyDataFrameTableModel::PrivateData::PrivateData(DAPyDataFrameTableModel* p) : q_ptr(p), undoStack(nullptr)
{
}

bool DAPyDataFrameTableModel::PrivateData::isNoneDataframe() const
{
    return dataframe.isNone();
}

int DAPyDataFrameTableModel::PrivateData::getDataframeRowCount() const
{
    if (useCacheMode) {
        return dataframeRow;
    }
    return static_cast< int >(dataframe.shape().first);
}

int DAPyDataFrameTableModel::PrivateData::getDataframeColumnCount() const
{
    if (useCacheMode) {
        return dataframeColumn;
    }
    return static_cast< int >(dataframe.shape().second);
}

QString DAPyDataFrameTableModel::PrivateData::getDataframeColumnName(int i) const
{
    if (useCacheMode) {
        return columnsName[ i ];
    } else {
        return dataframe.columnName(i);
    }
}

QVariant DAPyDataFrameTableModel::PrivateData::getDataframeIndexName(int i) const
{
    QVariant res;
    try {
        if (dataframe.isNone()) {
            // 如果索引为空，就显示序号
            return i;
        }
        res = dataframe.index().value(i);
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return res;
    }
    if (res.canConvert(QMetaType::QVariantList)) {
        // 说明是复合表头
        QVariantList ss = res.toList();
        QString str;
        for (int i = 0; i < ss.size(); ++i) {
            if (i == 0) {
                str += ss[ i ].toString();
            } else {
                str += " | " + ss[ i ].toString();
            }
        }
        return str;
    }
    return res;
}

void DAPyDataFrameTableModel::PrivateData::clearCacheData()
{
    dataframeRow    = 0;
    dataframeColumn = 0;
    columnsName.clear();
}

//===================================================
// DAPyDataFrameTableModule
//===================================================
DAPyDataFrameTableModel::DAPyDataFrameTableModel(QUndoStack* stack, QObject* parent)
    : DAAbstractCacheWindowTableModel(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->undoStack = stack;
}

DAPyDataFrameTableModel::~DAPyDataFrameTableModel()
{
#if DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT
    qDebug() << "cntheaderData=" << d_ptr->cntheaderData;
    qDebug() << "cntcolumnCount=" << d_ptr->cntcolumnCount;
    qDebug() << "cntdata=" << d_ptr->cntdata;
#endif
}

QVariant DAPyDataFrameTableModel::actualHeaderData(int actualSection, Qt::Orientation orientation, int role) const
{
    DA_DC(d);
    if ((role != Qt::DisplayRole && role != Qt::ToolTipRole) || d->isNoneDataframe()) {
        return QAbstractTableModel::headerData(actualSection, orientation, role);
    }
    // tooltips 和 display

    if (Qt::Horizontal == orientation) {  // 说明是水平表头
        if (actualSection >= d->getDataframeColumnCount()) {
            return QVariant();
        }
        return d->getDataframeColumnName(actualSection);
    } else {
        if (actualSection >= d->getDataframeRowCount()) {
            return QVariant();
        }
        return d->getDataframeIndexName(actualSection);
    }
    return QVariant();
}

int DAPyDataFrameTableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    DA_DC(d);
    if (d->isNoneDataframe()) {
        return d->minShowColumn;
    }
    return std::max(d->getDataframeColumnCount() + d->extraColumn, d->minShowColumn);
}

int DAPyDataFrameTableModel::actualRowCount() const
{
    DA_DC(d);
    if (d->isNoneDataframe()) {
        return d->minShowRow;
    }
    return d->getDataframeRowCount();
}

QVariant DAPyDataFrameTableModel::actualData(int actualRow, int actualColumn, int role) const
{
    DA_DC(d);
    if (d->isNoneDataframe()) {
        return QVariant();
    }
    if (actualRow >= d->getDataframeRowCount() || actualColumn >= d->getDataframeColumnCount()) {
        return QVariant();
    }
    switch (role) {
    case Qt::TextAlignmentRole:
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::BackgroundRole:
        return QVariant();
    case Qt::DisplayRole: {
        return d->dataframe.iat(actualRow, actualColumn);
    }
    default:
        break;
    }

    return QVariant();
}

bool DAPyDataFrameTableModel::setActualData(int actualRow, int actualColumn, const QVariant& value, int role)
{
    if (Qt::EditRole != role) {
        return false;
    }
    DA_D(d);
    if (d->isNoneDataframe()) {
        return false;
    }
    // 如果启用虚拟化，要计算实际的行号
    if (actualRow >= d->getDataframeRowCount()) {
        // todo:这里实现一个dataframe追加行
        return false;
    }
    if (actualColumn >= d->getDataframeColumnCount()) {
        // todo:这里实现一个dataframe追加列
        return false;
    }

    QVariant olddata = d->dataframe.iat(actualRow, actualColumn);
    if (value.isNull() == olddata.isNull()) {
        // 两次都为空就跳过
        return false;
    }
    if (!(d->undoStack)) {
        // 如果d->_undoStack设置为nullptr，将不使用redo/undo
        return d->dataframe.iat(actualRow, actualColumn, value);
    }
    std::unique_ptr< DACommandDataFrame_iat > cmd_iat(
        new DACommandDataFrame_iat(d->dataframe, actualRow, actualColumn, olddata, value));
    DAPyDataFrameTableModel* modle = this;
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
    // 这里说明设置成功了
    return true;
}

DAPyDataFrame& DAPyDataFrameTableModel::dataFrame()
{
    return d_ptr->dataframe;
}

const DAPyDataFrame& DAPyDataFrameTableModel::dataFrame() const
{
    return d_ptr->dataframe;
}

void DAPyDataFrameTableModel::setDAData(const DAData& d)
{
    if (!d.isDataFrame()) {
        d_ptr->dataframe = DAPyDataFrame();
        refreshData();
        return;
    }
    setDataFrame(d.toDataFrame());
}

void DAPyDataFrameTableModel::setDataFrame(const DAPyDataFrame& d)
{
    // 缓存好必要的信息
#if DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT
    QElapsedTimer __elasper;
    __elasper.start();
    qDebug() << "setDAData begin";
#endif
    d_ptr->dataframe = d;
    refreshData();
#if DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT
    qDebug() << "setDAData after refresh,cost:" << __elasper.elapsed() << " ms";
#endif
}

void DAPyDataFrameTableModel::setUseCacheMode(bool on)
{
    d_ptr->useCacheMode = on;
    refreshData();
}

/**
 * @brief 设置滑动窗模式的起始行
 * @param startRow
 */
void DAPyDataFrameTableModel::setCacheWindowStartRow(int startRow)
{
    DA_D(d);

    // startRow限制在指定的最小值和最大值之间。它能够确保startRow不会超出给定的范围
    const int dr        = d->getDataframeRowCount();
    const int cacheSize = getCacheWindowSize();
    startRow            = qBound(0, startRow, dr - cacheSize + d->extraRow);
    if (startRow >= dr) {
        startRow = dr - 1;
    }
    DAAbstractCacheWindowTableModel::setCacheWindowStartRow(startRow);
}

/**
 * @brief 全部刷新
 */
void DAPyDataFrameTableModel::refreshData()
{
    beginResetModel();
    if (d_ptr->useCacheMode) {
        cacheShape();
    } else {
        setCacheWindowStartRow(0);  // 滑动窗口到第一行
    }
    endResetModel();
}

/**
 * @brief 缓存尺寸相关的信息
 */
void DAPyDataFrameTableModel::cacheShape()
{
    DA_D(d);
    auto shape         = d->dataframe.shape();
    d->dataframeRow    = static_cast< int >(shape.first);
    d->dataframeColumn = static_cast< int >(shape.second);
    d->columnsName     = d->dataframe.columns();
#if DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT

#endif
}

void DAPyDataFrameTableModel::cacheRowShape()
{
    DA_D(d);
    auto shape      = d->dataframe.shape();
    d->dataframeRow = static_cast< int >(shape.first);
}

void DAPyDataFrameTableModel::cacheColumnShape()
{
    DA_D(d);
    auto shape         = d->dataframe.shape();
    d->dataframeColumn = static_cast< int >(shape.second);
    d->columnsName     = d->dataframe.columns();
}

/**
 * @brief 设置超出模型实际数据行数的额外空行数量。
 *
 * 该函数用于指定在模型中显示的额外空行数量（n），这些行不包含实际数据，
 * 主要用于提供空间给新的插入操作。例如，如果模型有100行实际数据，并调用
 * setExtraRowCount(5)，则视图将显示105行，其中最后5行为预留的空行。
 *
 * @param n 要添加到现有行数上的额外空行数量。n 应为非负整数。
 *
 * @note
 * - 如果 n 设为0，则仅显示模型中的实际数据行。
 * - 该函数不会影响模型的实际数据内容，只影响视图中显示的行数。
 * - 这些额外的行可以用来方便用户直接在表格末尾进行插入操作。
 *
 * 示例:
 * @code
 * // 假设模型中有100行实际数据
 * model->setExtraRowCount(5); // 视图现在会显示105行，其中最后5行为空行
 * @endcode
 *
 * @see getExtraRowCount
 */
void DAPyDataFrameTableModel::setExtraRowCount(int v)
{
    d_ptr->extraRow = v;
}

/**
 * @brief 超出模型实际数据行数的额外空行数量
 * @return 超出模型实际数据行数的额外空行数量
 * @see setExtraRowCount
 */
int DAPyDataFrameTableModel::getExtraRowCount() const
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
void DAPyDataFrameTableModel::setExtraColumnCount(int v)
{
    d_ptr->extraColumn = v;
}

/**
 * @brief 超出模型实际数据列数的额外空列数量
 * @return 超出模型实际数据列数的额外空列数量
 * @see setExtraColumnCount
 */
int DAPyDataFrameTableModel::getExtraColumnCount() const
{
    return d_ptr->extraColumn;
}

void DAPyDataFrameTableModel::setMinShowRowCount(int v)
{
    d_ptr->minShowRow = v;
}

int DAPyDataFrameTableModel::getMinShowRowCount() const
{
    return d_ptr->minShowRow;
}

void DAPyDataFrameTableModel::setMinShowColumnCount(int v)
{
    d_ptr->minShowColumn = v;
}

int DAPyDataFrameTableModel::getMinShowColumnCount() const
{
    return d_ptr->minShowColumn;
}

}  // end of namespace DA
