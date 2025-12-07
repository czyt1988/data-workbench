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
#if DA_ENABLE_PYTHON
// Py
#include "pandas/DAPyDataFrame.h"
#include "DAPybind11QtTypeCast.h"
#endif
//

namespace DA
{
class DADataManagerTreeModel::PrivateData
{
    DA_DECLARE_PUBLIC(DADataManagerTreeModel)
public:
    PrivateData(DADataManagerTreeModel* p);
    DADataManager* dataMgr                          = nullptr;
    bool expandDataframeToSeries                    = false;
    DADataManagerTreeModel::ColumnStyle columnStyle = DADataManagerTreeModel::ColumnWithNameOnly;

    // 数据ID到项的映射，提高查找效率
    QHash< DAData::IdType, QStandardItem* > dataIdToItemMap;
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

void DADataManagerTreeModel::rebuildDataMap()
{
    clearDataMap();

    for (int row = 0; row < rowCount(); ++row) {
        QStandardItem* item = this->item(row, 0);
        if (!item) {
            continue;
        }
        QVariant idVar = item->data(RoleDataId);
        if (idVar.isValid()) {
            bool ok           = false;
            DAData::IdType id = idVar.toULongLong(&ok);
            if (ok) {
                d_ptr->dataIdToItemMap[ id ] = item;
            }
        }
    }
}

void DADataManagerTreeModel::clearDataMap()
{
    d_ptr->dataIdToItemMap.clear();
}

QStandardItem* DADataManagerTreeModel::createDataItem(const DAData& data)
{
    if (data.isNull()) {
        return nullptr;
    }

    QStandardItem* item = new QStandardItem(data.getName());

    // 设置图标
    item->setIcon(DADataManagerTableModel::dataToIcon(data));

    // 设置角色数据
    item->setData(static_cast< int >(DataFrameItem), RoleItemType);
    item->setData(data.id(), RoleDataId);

    // 设置工具提示
    item->setToolTip(makeDataToolTip(data));

    // 设置为可编辑（只允许重命名）
    item->setEditable(true);
    item->setDragEnabled(true);
    item->setDropEnabled(false);

    return item;
}

QStandardItem* DADataManagerTreeModel::createDataFrameSeriesItem(const QString& seriesName, DAData::IdType dataframeId)
{
    QStandardItem* item = new QStandardItem(seriesName);

    // 设置角色数据
    item->setData(static_cast< int >(SeriesInnerDataframe), RoleItemType);
    item->setData(dataframeId, RoleDataId);

    // 设置工具提示
    item->setToolTip(QString("Series: %1").arg(seriesName));

    // Series项不可编辑
    item->setEditable(false);
    item->setDragEnabled(false);
    item->setDropEnabled(false);

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
    clearDataMap();

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

    // 构建数据映射
    rebuildDataMap();
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

QStandardItem* DADataManagerTreeModel::findItemByData(const DAData& data) const
{
    if (data.isNull()) {
        return nullptr;
    }
    return d_ptr->dataIdToItemMap.value(data.id(), nullptr);
}

QStandardItem* DADataManagerTreeModel::findItemByDataId(DAData::IdType id) const
{
    return d_ptr->dataIdToItemMap.value(id, nullptr);
}

DAData DADataManagerTreeModel::itemToData(QStandardItem* item) const
{
    if (!item || !d_ptr->dataMgr) {
        return DAData();
    }

    QVariant idVar = item->data(RoleDataId);
    if (!idVar.isValid()) {
        return DAData();
    }

    bool ok           = false;
    DAData::IdType id = idVar.toULongLong(&ok);
    if (!ok) {
        return DAData();
    }

    return d_ptr->dataMgr->getDataById(id);
}

bool DADataManagerTreeModel::isDataframeItem(QStandardItem* item) const
{
    if (!item) {
        return false;
    }

    QVariant typeVar = item->data(RoleItemType);
    if (!typeVar.isValid() || typeVar.toInt() != DataFrameItem) {
        return false;
    }

#if 1
    return true;
#else  // 这是最严格的筛查
    DAData data = itemToData(item);
    return data.isDataFrame();
#endif
}

bool DADataManagerTreeModel::isDataframeSeriesItem(QStandardItem* item) const
{
    if (!item) {
        return false;
    }

    QVariant typeVar = item->data(RoleItemType);
    return (typeVar.isValid() && typeVar.toInt() == SeriesInnerDataframe);
}

QStringList DADataManagerTreeModel::getAllDataNames() const
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

void DADataManagerTreeModel::clear()
{
    QStandardItemModel::clear();
    clearDataMap();

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

Qt::ItemFlags DADataManagerTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = QStandardItemModel::flags(index);

    if (!d_ptr->enableEdit) {
        flags &= ~Qt::ItemIsEditable;
    }
    // 只有第一列可编辑（用于重命名）
    if (index.column() != 0) {
        flags &= ~Qt::ItemIsEditable;
        flags &= ~Qt::ItemIsDragEnabled;
        flags &= ~Qt::ItemIsDropEnabled;
    }

    // DataFrame的Series项不可编辑
    if (index.isValid()) {
        QStandardItem* item = itemFromIndex(index);
        if (item && isDataframeSeriesItem(item)) {
            flags &= ~Qt::ItemIsEditable;
            flags &= ~Qt::ItemIsDragEnabled;
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
        QString newName = value.toString().trimmed();
        if (newName.isEmpty()) {
            return false;
        }

        DAData data = itemToData(item);
        if (data.isNull()) {
            return false;
        }

        // 检查名称是否已存在
        QStringList existingNames = getAllDataNames();
        if (existingNames.contains(newName) && newName != data.getName()) {
            return false;
        }
        DACommandDataManagerRenameData* cmd = new DACommandDataManagerRenameData(data, newName);
        d_ptr->dataMgr->getUndoStack()->push(cmd);

        // 无需执行更新项文本，因为设置名字会触发onDataChanged槽，在此槽里会更新文本
        // item->setText(newName);

        return true;
    }
    return false;
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
    d_ptr->dataIdToItemMap[ data.id() ] = item;

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

    QStandardItem* item = findItemByData(data);
    if (!item) {
        qDebug() << "Cannot find item for data:" << data.getName();
        return;
    }

    // 从映射中移除
    d_ptr->dataIdToItemMap.remove(data.id());

    // 从模型中移除
    removeRow(item->row());
}

void DADataManagerTreeModel::updateDataItem(const DAData& data, DADataManager::ChangeType changeType)
{
    QStandardItem* item = findItemByData(data);
    if (!item) {
        return;
    }

    switch (changeType) {
    case DADataManager::ChangeName:
        item->setText(data.getName());
        break;

    case DADataManager::ChangeValue:
    case DADataManager::ChangeDataframeColumnName:
        // 如果是DataFrame，更新展开状态
        if (data.isDataFrame()) {
            updateDataFrameItemExpansion(item, d_ptr->expandDataframeToSeries);
        }
        break;

    case DADataManager::ChangeDescribe:
        // 更新工具提示
        item->setToolTip(data.getDescribe());
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
        QStandardItem* seriesItem = createDataFrameSeriesItem(column, data.id());
        if (seriesItem) {
            dataframeItem->appendRow(seriesItem);
        }
    }
#endif
}

QString DADataManagerTreeModel::makeDataToolTip(const DAData& data)
{
    QString toolTip = data.getName();
    toolTip += tr("\nType: %1").arg(data.typeToString());
    const QString describe = data.getDescribe();
    if (!describe.isEmpty()) {
        toolTip += tr("\nDescription: %1").arg(data.getDescribe());
    }
    return toolTip;
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
