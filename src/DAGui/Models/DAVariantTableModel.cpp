#include "DAVariantTableModel.h"
#include <QUndoStack>
namespace DA
{
class DAVariantTableModel::PrivateData
{
public:
    DA_DECLARE_PUBLIC(DAVariantTableModel)
public:
    PrivateData(DAVariantTableModel* p);

public:
    DATable< QVariant >* mData { nullptr };
    Qt::ItemFlags mItemFlags { Qt::ItemIsSelectable | Qt::ItemIsEnabled };
    QUndoStack mStack;
};

DAVariantTableModel::PrivateData::PrivateData(DAVariantTableModel* p) : q_ptr(p)
{
}

//----------------------------------------------------
// DAVariantTableModel
//----------------------------------------------------

class DAVariantTableModelSetDataCommand : public QUndoCommand
{
public:
    DAVariantTableModelSetDataCommand(DAVariantTableModel* model, int row, int col, const QVariant& value, QUndoCommand* par = nullptr);
    void redo() override;
    void undo() override;

public:
    DAVariantTableModel* mModel;
    int mRow;
    int mCol;
    QVariant mValue;
    QVariant mOldValue;
};

DAVariantTableModelSetDataCommand::DAVariantTableModelSetDataCommand(DAVariantTableModel* model,
                                                                     int row,
                                                                     int col,
                                                                     const QVariant& value,
                                                                     QUndoCommand* par)
    : QUndoCommand(par), mModel(model), mRow(row), mCol(col), mValue(value)
{
    mOldValue = mModel->getTableData(row, col);
}

void DAVariantTableModelSetDataCommand::redo()
{
    if (mValue.isNull()) {
        //说明没有值要移除
        mModel->removeTableCell(mRow, mCol);
    } else {
        mModel->setTableData(mRow, mCol, mValue);
    }
}

void DAVariantTableModelSetDataCommand::undo()
{
    if (mOldValue.isNull()) {
        //说明没有值要移除
        mModel->removeTableCell(mRow, mCol);
    } else {
        mModel->setTableData(mRow, mCol, mOldValue);
    }
}

//----------------------------------------------------
// DAVariantTableModel
//----------------------------------------------------
DAVariantTableModel::DAVariantTableModel(QObject* p) : QAbstractTableModel(p)
{
}

DAVariantTableModel::DAVariantTableModel(DATable< QVariant >* d, QObject* p)
    : QAbstractTableModel(p), DA_PIMPL_CONSTRUCT
{
    d_ptr->mData = d;
}

DAVariantTableModel::~DAVariantTableModel()
{
}

QVariant DAVariantTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    return section + 1;
}

int DAVariantTableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (d_ptr->mData) {
        return d_ptr->mData->columnCount();
    }
    return 0;
}

int DAVariantTableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (d_ptr->mData) {
        return d_ptr->mData->rowCount();
    }
    return 0;
}

QVariant DAVariantTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || nullptr == d_ptr->mData) {
        return QVariant();
    }
    if (index.row() >= d_ptr->mData->rowCount()) {
        return QVariant();
    }
    if (index.column() >= d_ptr->mData->columnCount()) {
        return QVariant();
    }
    switch (role) {
    case Qt::TextAlignmentRole:
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::DisplayRole:
        return getTableData(index.row(), index.column());
    default:
        break;
    }

    return QVariant();
}

bool DAVariantTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (Qt::EditRole != role) {
        return false;
    }
    if (!index.isValid() || nullptr == d_ptr->mData) {
        return false;
    }
    DAVariantTableModelSetDataCommand* cmd = new DAVariantTableModelSetDataCommand(this, index.row(), index.column(), value);
    d_ptr->mStack.push(cmd);
    return true;
}

Qt::ItemFlags DAVariantTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return d_ptr->mItemFlags;
}

void DAVariantTableModel::update()
{
    beginResetModel();
    endResetModel();
}

/**
 * @brief 设置是否可编辑
 * @param on
 */
void DAVariantTableModel::setEnableEdit(bool on)
{
    d_ptr->mItemFlags.setFlag(Qt::ItemIsEditable, on);
}

QUndoStack* DAVariantTableModel::getUndoStack() const
{
    return &(d_ptr->mStack);
}

void DAVariantTableModel::redo()
{
    d_ptr->mStack.redo();
}

void DAVariantTableModel::undo()
{
    d_ptr->mStack.undo();
}

void DAVariantTableModel::setTableData(int row, int col, const QVariant& v)
{
    d_ptr->mData->set(row, col, v);
    qDebug() << "setTableData(" << row << "," << col << "," << v << ")";
    qDebug() << "after set :" << getTableData(row, col);
    emit dataChanged(index(row, col), index(row, col));
}

/**
 * @brief 获取表格数据
 * @param row
 * @param col
 * @return
 */
QVariant DAVariantTableModel::getTableData(int row, int col) const
{
    auto i = d_ptr->mData->find(row, col);
    if (i == d_ptr->mData->end()) {
        return QVariant();
    }
    return i->second;
}

/**
 * @brief 移除单元格
 * @param row
 * @param col
 */
void DAVariantTableModel::removeTableCell(int row, int col)
{
    d_ptr->mData->removeCell(row, col);
}

}
