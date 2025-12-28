#include "DADataManagerTreeModel.h"
// Qt
#include <QHash>
#include <QDebug>
#include <QMimeData>
// DAData
#include "DADataManager.h"
#include "DAData.h"
#include "DADataManagerTableModel.h"
#include "DACommandsDataManager.h"
#include "MimeData/DAMimeDataForData.h"
#include "MimeData/DAMimeDataFormats.h"
#if DA_ENABLE_PYTHON
// Py
#include "pandas/DAPyDataFrame.h"
#include "DAPybind11QtTypeCast.h"
#endif
//

namespace DA
{

//===============================================================
// DAStandardItemDataDataframe
//===============================================================
DAStandardItemDataDataframe::DAStandardItemDataDataframe(const DAData& data) : QStandardItem(), m_dataframe(data)
{
    setEditable(true);
    setDragEnabled(true);
    setDropEnabled(false);
}

DAStandardItemDataDataframe::~DAStandardItemDataDataframe()
{
}

QVariant DAStandardItemDataDataframe::data(int role) const
{
    static QIcon s_dataframe_icon = QIcon(":/DAGui/DataType/icon/data-type/dataframe.svg");
    if (!m_dataframe.isDataFrame()) {
        return QStandardItem::data(role);
    }
    switch (role) {
    case Qt::DisplayRole: {
        return m_dataframe.getName();
    } break;
    case Qt::ToolTipRole: {
        return m_dataframe.getDescribe();
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        return s_dataframe_icon;
    } break;
    default:
        break;
    }

    return QStandardItem::data(role);
}

void DAStandardItemDataDataframe::setData(const QVariant& value, int role)
{
    if (role != Qt::EditRole) {
        return;
    }
    QString newName = value.toString().trimmed();
    if (newName.isEmpty()) {
        return;
    }
    DACommandDataManagerRenameData* cmd = new DACommandDataManagerRenameData(m_dataframe, newName);
    m_dataframe.getDataManager()->getUndoStack()->push(cmd);
}

DAData DAStandardItemDataDataframe::getDataframe() const
{
    return m_dataframe;
}

bool DAStandardItemDataDataframe::isDataframeItem(QStandardItem* item)
{
    if (!item) {
        return false;
    }
    return (item->type() == DAStandardItemDataDataframe::Type);
}
//===============================================================
// DAStandardItemDataDataframeSeries
//===============================================================

DAStandardItemDataDataframeSeries::DAStandardItemDataDataframeSeries(const DAData& data, const QString& seriesName)
    : QStandardItem(), m_dataframe(data), m_name(seriesName)
{
    setEditable(true);
    setDragEnabled(true);
    setDropEnabled(false);
}

DAStandardItemDataDataframeSeries::~DAStandardItemDataDataframeSeries()
{
}

QVariant DAStandardItemDataDataframeSeries::data(int role) const
{
    static QIcon s_dataframe_icon = QIcon(":/DAGui/DataType/icon/data-type/dataframe.svg");
    if (!m_dataframe.isDataFrame()) {
        return QStandardItem::data(role);
    }
    switch (role) {
    case Qt::DisplayRole: {
        return m_name;
    } break;
    case Qt::ToolTipRole: {
        return makeDescribeText(m_dataframe, m_name);
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        return seriesTypeToIcon(m_dataframe, m_name);
    } break;
    default:
        break;
    }

    return QStandardItem::data(role);
}

DAData DAStandardItemDataDataframeSeries::getDataframe() const
{
    return m_dataframe;
}

void DAStandardItemDataDataframeSeries::setSeriesName(const QString& name)
{
    m_name = name;
}

QString DAStandardItemDataDataframeSeries::getSeriesName() const
{
    return m_name;
}

bool DAStandardItemDataDataframeSeries::isDataframeSeriesItem(QStandardItem* item)
{
    if (!item) {
        return false;
    }
    return (item->type() == DAStandardItemDataDataframeSeries::Type);
}

QString DAStandardItemDataDataframeSeries::makeDescribeText(const DAData& data, const QString& seriesName)
{
    QString t;
    try {
        DAPyDataFrame df  = data.toDataFrame();
        DAPySeries series = df[ seriesName ];
        std::size_t size  = series.size();

        t = QObject::tr("%1.%2,size:%3").arg(data.getName(), seriesName).arg(size);  // cn:%1.%2,长度:%3
    } catch (...) {
    }
    return t;
}

/**
 * @brief 根据series的类型返回图标
 * @param data
 * @param seriesName
 * @return
 */
QIcon DAStandardItemDataDataframeSeries::seriesTypeToIcon(const DAData& data, const QString& seriesName)
{
    static QIcon s_int_type      = QIcon(":/DAGui/DataType/icon/data-type/int.svg");
    static QIcon s_float_type    = QIcon(":/DAGui/DataType/icon/data-type/float.svg");
    static QIcon s_datetime_type = QIcon(":/DAGui/DataType/icon/data-type/datetime.svg");
    static QIcon s_str_type      = QIcon(":/DAGui/DataType/icon/data-type/str.svg");
    static QIcon s_obj_type      = QIcon(":/DAGui/DataType/icon/data-type/obj.svg");
    try {
        DAPyDataFrame df  = data.toDataFrame();
        DAPySeries series = df[ seriesName ];
        auto dtype        = series.dtype();
        char c            = dtype.char_();
        switch (c) {
        case 'd':
        case 'f':
        case 'e': {
            return s_float_type;
        } break;
        case 'q':
        case 'Q':
        case 'l':
        case 'L':
        case 'h':
        case 'H':
        case 'b':
        case 'B': {
            return s_int_type;
        } break;
        case 'U': {
            return s_str_type;
        } break;
        case 'M': {
            return s_datetime_type;
        } break;
        case 'O': {
            return s_obj_type;
        } break;
        default:
            break;
        }
    } catch (...) {
    }
    return QIcon();
}
//===============================================================
// DADataManagerTreeModel::PrivateData
//===============================================================

class DADataManagerTreeModel::PrivateData
{
    DA_DECLARE_PUBLIC(DADataManagerTreeModel)
public:
    PrivateData(DADataManagerTreeModel* p);
    DADataManager* dataMgr                          = nullptr;
    bool expandDataframeToSeries                    = false;
    DADataManagerTreeModel::ColumnStyle columnStyle = DADataManagerTreeModel::ColumnWithNameOnly;
    bool enableEdit { false };
};

DADataManagerTreeModel::PrivateData::PrivateData(DADataManagerTreeModel* p) : q_ptr(p)
{
}

//===================================================
// DADataManagerTreeModel
//===================================================

DADataManagerTreeModel::DADataManagerTreeModel(QObject* parent) : QStandardItemModel(parent), DA_PIMPL_CONSTRUCT
{
    initialize();
}

DADataManagerTreeModel::DADataManagerTreeModel(DADataManager* dataMgr, QObject* parent)
    : QStandardItemModel(parent), DA_PIMPL_CONSTRUCT
{
    initialize();
    setDataManager(dataMgr);
}

DADataManagerTreeModel::DADataManagerTreeModel(DADataManager* dataMgr, ColumnStyle style, QObject* parent)
    : QStandardItemModel(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->columnStyle = style;
    initialize();
    setDataManager(dataMgr);
}

DADataManagerTreeModel::~DADataManagerTreeModel()
{
}

void DADataManagerTreeModel::initialize()
{
    // 设置表头
    if (d_ptr->columnStyle == ColumnWithNameOnly) {
        setHorizontalHeaderLabels({ tr("Name") });
    } else {
        setHorizontalHeaderLabels({ tr("Name"), tr("Properties") });
    }
}

void DADataManagerTreeModel::setupConnections()
{
    DA_D(d);
    if (!d->dataMgr) {
        return;
    }

    connect(d->dataMgr, &DADataManager::dataAdded, this, &DADataManagerTreeModel::onDataAdded);
    connect(d->dataMgr, &DADataManager::dataBeginRemove, this, &DADataManagerTreeModel::onDataBeginRemoved);
    connect(d->dataMgr, &DADataManager::dataChanged, this, &DADataManagerTreeModel::onDataChanged);
    connect(d->dataMgr, &DADataManager::datasCleared, this, &DADataManagerTreeModel::onDatasCleared);
}


QStandardItem* DADataManagerTreeModel::createDataItem(const DAData& data)
{
    if (data.isNull()) {
        return nullptr;
    }
    QStandardItem* item = new DAStandardItemDataDataframe(data);
    return item;
}

QStandardItem* DADataManagerTreeModel::createDataFrameSeriesItem(const QString& seriesName, const DAData& dataframeData)
{
    if (dataframeData.isNull()) {
        return nullptr;
    }
    QStandardItem* item = new DAStandardItemDataDataframeSeries(dataframeData, seriesName);
    return item;
}

void DADataManagerTreeModel::setDataManager(DADataManager* dataMgr)
{
    // 断开旧连接
    if (d_ptr->dataMgr) {
        d_ptr->dataMgr->disconnect(this);
    }

    // 清除现有数据
    clear();

    // 设置新管理器
    d_ptr->dataMgr = dataMgr;

    if (!dataMgr) {
        return;
    }

    // 加载现有数据
    int count = dataMgr->getDataCount();
    for (int i = 0; i < count; ++i) {
        addDataItem(dataMgr->getData(i));
    }

    // 设置连接
    setupConnections();
}

DADataManager* DADataManagerTreeModel::getDataManager() const
{
    return d_ptr->dataMgr;
}

void DADataManagerTreeModel::setExpandDataframeToSeries(bool on)
{
    if (d_ptr->expandDataframeToSeries != on) {
        d_ptr->expandDataframeToSeries = on;
        updateDataFrameExpansion();
    }
}

bool DADataManagerTreeModel::isExpandDataframeToSeries() const
{
    return d_ptr->expandDataframeToSeries;
}

void DADataManagerTreeModel::setColumnStyle(ColumnStyle style)
{
    if (d_ptr->columnStyle != style) {
        d_ptr->columnStyle = style;

        if (style == ColumnWithNameOnly) {
            setHorizontalHeaderLabels({ tr("Name") });
            setColumnCount(1);
        } else {
            setHorizontalHeaderLabels({ tr("Name"), tr("Properties") });
            setColumnCount(2);
        }

        emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
    }
}

DADataManagerTreeModel::ColumnStyle DADataManagerTreeModel::getColumnStyle() const
{
    return d_ptr->columnStyle;
}

QStandardItem* DADataManagerTreeModel::findFirstDataframeItemByData(const DAData& data) const
{
    if (data.isNull()) {
        return nullptr;
    }
    const int rowcnt = rowCount();
    for (int i = 0; i < rowcnt; ++i) {
        QStandardItem* it = item(i);
        if (!DAStandardItemDataDataframe::isDataframeItem(it)) {
            continue;
        }
        DAStandardItemDataDataframe* dfit = static_cast< DAStandardItemDataDataframe* >(it);
        if (dfit->getDataframe() == data) {
            return it;
        }
    }
    return nullptr;
}

/**
 * @brief 从item获取数据
 *
 * 无论dataframe还是dataframe下的series，通过此函数都能获取数据
 * @param item
 * @return
 */
DAData DADataManagerTreeModel::itemToData(QStandardItem* item)
{
    if (isDataframeItem(item)) {
        DAStandardItemDataDataframe* dfitem = static_cast< DAStandardItemDataDataframe* >(item);
        return dfitem->getDataframe();
    } else if (isDataframeSeriesItem(item)) {
        DAStandardItemDataDataframeSeries* seritem = static_cast< DAStandardItemDataDataframeSeries* >(item);
        return seritem->getDataframe();
    }
    return DAData();
}

/**
 * @brief 是否是dataframe
 * @param item
 * @return
 */
bool DADataManagerTreeModel::isDataframeItem(QStandardItem* item)
{
    return DAStandardItemDataDataframe::isDataframeItem(item);
}

/**
 * @brief 是否是series
 * @param item
 * @return
 */
bool DADataManagerTreeModel::isDataframeSeriesItem(QStandardItem* item)
{
    return DAStandardItemDataDataframeSeries::isDataframeSeriesItem(item);
}

/**
 * @brief 获取所有dataframe的名称
 * @return
 */
QStringList DADataManagerTreeModel::getAllDataframeNames() const
{
    QStringList names;

    for (int row = 0; row < rowCount(); ++row) {
        QStandardItem* item = this->item(row, 0);
        if (isDataframeItem(item)) {
            names.append(item->text());
        }
    }
    return names;
}

/**
 * @brief 清空
 */
void DADataManagerTreeModel::clear()
{
    QStandardItemModel::clear();

    // 重置表头
    if (d_ptr->columnStyle == ColumnWithNameOnly) {
        setHorizontalHeaderLabels({ tr("Name") });
    } else {
        setHorizontalHeaderLabels({ tr("Name"), tr("Properties") });
    }
}

void DADataManagerTreeModel::setEnableEdit(bool on)
{
    d_ptr->enableEdit = on;
}

bool DADataManagerTreeModel::isEnableEdit() const
{
    return d_ptr->enableEdit;
}
/**
Qt::ItemFlags DADataManagerTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = QStandardItemModel::flags(index);

    if (!d_ptr->enableEdit) {
        flags &= ~Qt::ItemIsEditable;
    }
    // 只有第一列可编辑（用于重命名）
    if (index.column() != 0) {
        flags &= ~Qt::ItemIsEditable;
        flags |= Qt::ItemIsDragEnabled;
        flags &= ~Qt::ItemIsDropEnabled;
    }

    // DataFrame的Series项不可编辑但可拖曳
    if (index.isValid()) {
        QStandardItem* item = itemFromIndex(index);
        if (item && isDataframeSeriesItem(item)) {
            flags &= ~Qt::ItemIsEditable;
            flags |= Qt::ItemIsDragEnabled;  // 启用拖曳
            flags &= ~Qt::ItemIsDropEnabled;
        }
    }

    return flags;
}

QVariant DADataManagerTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    // 第一列使用默认行为
    if (index.column() == 0) {
        return QStandardItemModel::data(index, role);
    }

    // 第二列显示属性（只有显示角色）
    if (index.column() == 1 && role == Qt::DisplayRole) {
        QStandardItem* item = itemFromIndex(index.siblingAtColumn(0));
        if (!item) {
            return QVariant();
        }

        DAData data = itemToData(item);
        if (data.isNull()) {
            return QVariant();
        }

#if DA_ENABLE_PYTHON
        if (data.isDataFrame()) {
            DAPyDataFrame df = data.toDataFrame();
            if (!df.isNone()) {
                auto shape = df.shape();
                return QString("[%1 × %2]").arg(shape.first).arg(shape.second);
            }
        } else if (data.isSeries()) {
            DAPySeries series = data.toSeries();
            return PY::toString(series.dtype());
        }
#endif
    }

    return QVariant();
}
**/
bool DADataManagerTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!d_ptr->enableEdit) {
        return false;
    }
    if (role != Qt::EditRole || index.column() != 0) {
        return false;
    }
    QStandardItem* item = itemFromIndex(index);
    if (!item) {
        return false;
    }
    // 重命名数据项目
    if (isDataframeItem(item)) {
        DAStandardItemDataDataframe* dfitem = static_cast< DAStandardItemDataDataframe* >(item);

        QString newName = value.toString().trimmed();
        if (newName.isEmpty()) {
            return false;
        }

        DAData data = dfitem->getDataframe();
        if (data.isNull()) {
            return false;
        }

        // 检查名称是否已存在
        QStringList existingNames = getAllDataframeNames();
        if (existingNames.contains(newName) && newName != data.getName()) {
            return false;
        }
        dfitem->setData(value, role);
        return true;
    }
    return false;
}

Qt::DropActions DADataManagerTreeModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

QStringList DADataManagerTreeModel::mimeTypes() const
{
    QStringList types;
    types << DAMIMEDATA_FORMAT_DADATAS;
    return types;
}

QMimeData* DADataManagerTreeModel::mimeData(const QModelIndexList& indexs) const
{
    if (indexs.isEmpty()) {
        return nullptr;
    }
    DAMimeDataForData* mime = new DAMimeDataForData();
    for (const QModelIndex& index : indexs) {
        QStandardItem* item = itemFromIndex(index);
        if (!item) {
            continue;
        }
        if (isDataframeSeriesItem(item)) {
            // 选中的是dataframe
            DAData data = itemToData(item);
            if (data.isNull() || !data.isDataFrame()) {
                continue;
            }
            mime->appendDataSeries(data, item->text());
        } else if (isDataframeItem(item)) {
            // 选中的是整个dataframe，则添加该dataframe的所有列
            DAData data = itemToData(item);
            if (data.isNull() || !data.isDataFrame()) {
                continue;
            }
            mime->appendDataframe(data);
        }
    }
    return mime;
}

void DADataManagerTreeModel::addDataItem(const DAData& data)
{
    if (data.isNull()) {
        return;
    }

    QStandardItem* item = createDataItem(data);
    if (!item) {
        return;
    }

    appendRow(item);

    // 如果是DataFrame且需要展开，添加Series子项
    if (data.isDataFrame() && d_ptr->expandDataframeToSeries) {
        updateDataFrameItemExpansion(item, true);
    }
}

void DADataManagerTreeModel::removeDataItem(const DAData& data)
{
    if (data.isNull()) {
        return;
    }
    QStandardItem* item = findFirstDataframeItemByData(data);
    while (item != nullptr) {
        removeRow(item->row());
        item = findFirstDataframeItemByData(data);
    }
}

void DADataManagerTreeModel::updateDataItem(const DAData& data, DADataManager::ChangeType changeType)
{
    QStandardItem* item = findFirstDataframeItemByData(data);
    if (!item) {
        return;
    }

    switch (changeType) {
    case DADataManager::ChangeName:
        Q_EMIT dataChanged(item->index(), item->index(), { Qt::DisplayRole });
        break;
    case DADataManager::ChangeDataframeColumnName:
        // 如果是DataFrame，更新展开状态
        if (data.isDataFrame()) {
            updateDataFrameItemExpansion(item, d_ptr->expandDataframeToSeries);
        }
        break;
    case DADataManager::ChangeDescribe:
        // 更新工具提示
        Q_EMIT dataChanged(item->index(), item->index(), { Qt::ToolTipRole });
        break;

    default:
        break;
    }
}

void DADataManagerTreeModel::updateDataFrameExpansion()
{
    bool expand = d_ptr->expandDataframeToSeries;

    for (int row = 0; row < rowCount(); ++row) {
        QStandardItem* item = this->item(row, 0);
        if (item && isDataframeItem(item)) {
            updateDataFrameItemExpansion(item, expand);
        }
    }
}

void DADataManagerTreeModel::updateDataFrameItemExpansion(QStandardItem* dataframeItem, bool expanded)
{
    if (!dataframeItem || !isDataframeItem(dataframeItem)) {
        return;
    }

    // 移除现有子项
    while (dataframeItem->rowCount() > 0) {
        dataframeItem->removeRow(0);
    }

    if (!expanded) {
        return;
    }

#if DA_ENABLE_PYTHON
    // 添加Series子项
    DAData data = itemToData(dataframeItem);
    if (data.isNull() || !data.isDataFrame()) {
        return;
    }

    DAPyDataFrame df = data.toDataFrame();
    if (df.isNone()) {
        return;
    }

    QStringList columns = df.columns();
    for (const QString& column : columns) {
        QStandardItem* seriesItem = createDataFrameSeriesItem(column, data);
        if (seriesItem) {
            dataframeItem->appendRow(seriesItem);
        }
    }
#endif
}


void DADataManagerTreeModel::onDataAdded(const DAData& data)
{
    addDataItem(data);
}

void DADataManagerTreeModel::onDataBeginRemoved(const DAData& data, int index)
{
    Q_UNUSED(index);
    removeDataItem(data);
}

void DADataManagerTreeModel::onDataChanged(const DAData& data, DADataManager::ChangeType changeType)
{
    updateDataItem(data, changeType);
}

void DADataManagerTreeModel::onDatasCleared()
{
    clear();
}


}  // end da
