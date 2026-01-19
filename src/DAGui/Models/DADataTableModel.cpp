#include "DADataTableModel.h"
#include "Commands/DACommandsDataFrame.h"
namespace DA
{

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

public:
    DAData data;
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
};

DADataTableModel::PrivateData::PrivateData(DADataTableModel* p) : q_ptr(p)
{
}

bool DADataTableModel::PrivateData::isNone() const
{
    return data.isNull();
}

int DADataTableModel::PrivateData::getDataRowCount() const
{
    if (useCacheMode) {
        return dataframeRow;
    }
    return static_cast< int >(data.shape().first);
}

int DADataTableModel::PrivateData::getDataColumnCount() const
{
    if (useCacheMode) {
        return dataframeColumn;
    }
    return static_cast< int >(data.shape().second);
}

QString DADataTableModel::PrivateData::getDataframeColumnName(int i) const
{
    if (useCacheMode) {
        return columnsName[ i ];
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

void DADataTableModel::PrivateData::clearCacheData()
{
    dataframeRow    = 0;
    dataframeColumn = 0;
    columnsName.clear();
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
        return d->getDataframeIndexName(actualSection);
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
    case Qt::DisplayRole: {
        if (d->data.isDataFrame()) {
            return d->data.toDataFrame().iat(actualRow, actualColumn);
        } else if (d->data.isSeries() && actualColumn == 0) {
            return d->data.toSeries().value(actualRow);
        }
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
        if (value.isNull() == olddata.isNull()) {
            // 两次都为空就跳过
            return false;
        }
        if (!(d->undoStack)) {
            // 如果d->_undoStack设置为nullptr，将不使用redo/undo
            return df.iat(actualRow, actualColumn, value);
        }
        std::unique_ptr< DACommandDataFrame_iat > cmd_iat(
            new DACommandDataFrame_iat(df, actualRow, actualColumn, olddata, value));
        DADataTableModel* modle = this;
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
    }
    // 这里说明设置成功了
    return true;
}

void DADataTableModel::setData(const DAData& data)
{
    d_ptr->data = data;
    refreshData();
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
}

void DADataTableModel::refreshData()
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

void DADataTableModel::cacheShape()
{
    DA_D(d);
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
