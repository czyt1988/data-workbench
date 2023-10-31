#include "DAVariantTableModel.h"
namespace DA
{
DAVariantTableModel::DAVariantTableModel(QObject* p) : QAbstractTableModel(p)
{
}

DAVariantTableModel::DAVariantTableModel(DATable< QVariant >* d, QObject* p) : QAbstractTableModel(p), mData(d)
{
}

DAVariantTableModel::~DAVariantTableModel()
{
}

QVariant DAVariantTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    return section + 1;
}

int DAVariantTableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (mData) {
        return mData->columnCount();
    }
    return 0;
}

int DAVariantTableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (mData) {
        return mData->rowCount();
    }
    return 0;
}

QVariant DAVariantTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || nullptr == mData) {
        return QVariant();
    }
    if (index.row() >= mData->rowCount()) {
        return QVariant();
    }
    if (index.column() >= mData->columnCount()) {
        return QVariant();
    }
    switch (role) {
    case Qt::TextAlignmentRole:
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::DisplayRole:
        return mData->at(index.row(), index.column());
    default:
        break;
    }

    return QVariant();
}

Qt::ItemFlags DAVariantTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}
}
