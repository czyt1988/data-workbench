#include "DADataManagerTableModel.h"
#include <QIcon>

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DADataManagerTableModel
//===================================================
DADataManagerTableModel::DADataManagerTableModel(QObject* p) : QAbstractTableModel(p), _dataManager(nullptr)
{
}

DADataManagerTableModel::DADataManagerTableModel(DADataManager* dm, QObject* p) : QAbstractTableModel(p)
{
    setDataManager(dm);
}

DADataManagerTableModel::~DADataManagerTableModel()
{
}

QVariant DADataManagerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (Qt::Horizontal == orientation) {  //说明是水平表头
        switch (section) {
        case 0:
            return tr("name");
        case 1:
            return tr("type");
        default:
            return QVariant();
        }
    } else {
        return QVariant();
    }
    return QVariant();
}

int DADataManagerTableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 2;
}

int DADataManagerTableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (nullptr == _dataManager) {
        return 0;
    }
    return _dataManager->getDataCount();
}

QVariant DADataManagerTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || nullptr == _dataManager) {
        return QVariant();
    }
    if (index.row() >= _dataManager->getDataCount()) {
        return QVariant();
    }
    DAData data = _dataManager->getData(index.row());

    switch (role) {
    case Qt::TextAlignmentRole:
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::DisplayRole:
        return dataDisplay(data, index);
    case Qt::DecorationRole:
        return dataDecoration(data, index);
    case Qt::BackgroundRole:
        return dataBackground(data, index);
    case Qt::ToolTipRole:
        return dataToolTip(data, index);
    case DA_ROLE_DADATAMANAGERTABLEMODEL_DATA:
        return QVariant::fromValue< DA::DAData >(data);
    default:
        break;
    }

    return QVariant();
}

Qt::ItemFlags DADataManagerTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void DADataManagerTableModel::setDataManager(DADataManager* dm)
{
    beginResetModel();
    _dataManager = dm;
    connect(dm, &DADataManager::dataAdded, this, &DADataManagerTableModel::onDataAdded);
    connect(dm, &DADataManager::dataBeginRemove, this, &DADataManagerTableModel::onDataBeginRemoved);
    connect(dm, &DADataManager::dataRemoved, this, &DADataManagerTableModel::onDataRemoved);
    endResetModel();
}

QVariant DADataManagerTableModel::dataDisplay(const DAData& d, const QModelIndex& index) const
{
    if (0 == index.column()) {
        return d.getName();
    } else if (1 == index.column()) {
        return d.typeToString();
    }
    return QVariant();
}

QVariant DADataManagerTableModel::dataDecoration(const DAData& d, const QModelIndex& index) const
{
    if (index.column() != 0) {
        return QVariant();
    }
    return dataToIcon(d);
}

QVariant DADataManagerTableModel::dataBackground(const DAData& d, const QModelIndex& index) const
{
    Q_UNUSED(d);
    return QVariant();
}

QVariant DADataManagerTableModel::dataToolTip(const DAData& d, const QModelIndex& index) const
{
    if (index.column() == 0) {
        return d.getDescribe();
    }
    return QVariant();
}
/**
 * @brief 变量对应的图标
 * @param d
 * @return
 */
QIcon DADataManagerTableModel::dataToIcon(const DAData& d)
{
    static QIcon s_iconNone      = QIcon(":/dataType/Icon/typeNone.svg");
    static QIcon s_iconDataframe = QIcon(":/dataType/Icon/typeDataframe.svg");
    static QIcon s_iconSeries    = QIcon(":/dataType/Icon/typeDataSeries.svg");
    static QIcon s_iconObject    = QIcon(":/dataType/Icon/typeObject.svg");
    switch (d.getDataType()) {
    case DAAbstractData::TypeNone:
        return s_iconNone;
    case DAAbstractData::TypePythonDataFrame:
        return s_iconDataframe;
    case DAAbstractData::TypePythonSeries:
        return s_iconSeries;
    case DAAbstractData::TypePythonObject:
        return s_iconObject;
    default:
        break;
    }
    return s_iconObject;
}

/**
 * @brief 刷新
 * @param row 行
 * @param col 列
 */
void DADataManagerTableModel::refresh(int row, int col)
{
    emit dataChanged(createIndex(row, col), createIndex(row, col));
}

void DADataManagerTableModel::onDataAdded(const DA::DAData& d)
{
    Q_UNUSED(d);
    int dataIndex = _dataManager->getDataIndex(d);
    beginInsertRows(QModelIndex(), dataIndex, dataIndex);
    endInsertRows();
}

void DADataManagerTableModel::onDataBeginRemoved(const DAData& d, int dataIndex)
{
    Q_UNUSED(d);
    beginRemoveRows(QModelIndex(), dataIndex, dataIndex);
}

void DADataManagerTableModel::onDataRemoved(const DA::DAData& d, int dataIndex)
{
    Q_UNUSED(d);
    Q_UNUSED(dataIndex);
    endRemoveRows();
}
