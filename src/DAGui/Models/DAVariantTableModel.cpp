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

	DAVariantTableModel::FpToDisplayString mToDisplayString { nullptr };  ///< 显示设置
	QStringList mHeader;                                                  ///< 表头
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
	DAVariantTableModelSetDataCommand(DAVariantTableModel* model,
	                                  int row,
	                                  int col,
	                                  const QVariant& value,
	                                  QUndoCommand* par = nullptr);
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
		// 说明没有值要移除
		mModel->removeTableCell(mRow, mCol);
	} else {
		mModel->setTableData(mRow, mCol, mValue);
	}
}

void DAVariantTableModelSetDataCommand::undo()
{
	if (mOldValue.isNull()) {
		// 说明没有值要移除
		mModel->removeTableCell(mRow, mCol);
	} else {
		mModel->setTableData(mRow, mCol, mOldValue);
	}
}

//----------------------------------------------------
// DAVariantTableModel
//----------------------------------------------------
DAVariantTableModel::DAVariantTableModel(QObject* p) : QAbstractTableModel(p), DA_PIMPL_CONSTRUCT
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
	if (Qt::Horizontal == orientation) {
		// 水平表头
		if (section < d_ptr->mHeader.size()) {
			return d_ptr->mHeader[ section ];
		}
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
	if (!index.isValid() || !d_ptr->mData) {
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
	if (!index.isValid() || !d_ptr->mData) {
		return false;
	}
	DAVariantTableModelSetDataCommand* cmd = new DAVariantTableModelSetDataCommand(this, index.row(), index.column(), value);
	d_ptr->mStack.push(cmd);
	return true;
}

Qt::ItemFlags DAVariantTableModel::flags(const QModelIndex& index) const
{
	if (!index.isValid() || !d_ptr->mData) {
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

/**
   @brief 设置表格

   原来的redo/undo将清空
   @param t
 */
void DAVariantTableModel::setTable(DATable< QVariant >* t)
{
	d_ptr->mStack.clear();
	beginResetModel();
	d_ptr->mData = t;
	endResetModel();
}

/**
   @brief 返回表格指针

   @note 注意，这是一个非常危险的操作，因为DAVariantTableModel是带redo/undo的，如果用户对表进行写操作，
   将和当前的redo/undo冲突

   @return
 */
DATable< QVariant >* DAVariantTableModel::getTable() const
{
	return d_ptr->mData;
}

/**
   @brief 清空表格
 */
void DAVariantTableModel::clearTable()
{
	if (!d_ptr->mData) {
		return;
	}
	beginResetModel();
	d_ptr->mData->clear();
	endResetModel();
}

/**
   @brief 注册显示函数，把QVariant转换为文本显示出来
   @example 示例：
   @code
   model->registDisplayFun([](const QVariant& v) -> QString {
        return QString::number(v.toDouble(), 'f');
    });
   @endcode
   @param fp
 */
void DAVariantTableModel::registDisplayFun(DAVariantTableModel::FpToDisplayString fp)
{
	d_ptr->mToDisplayString = fp;
}

void DAVariantTableModel::setHeader(const QStringList& h)
{
	d_ptr->mHeader = h;
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
	if (!d_ptr->mData) {
		return;
	}
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
	if (!(d_ptr->mData)) {
		return QVariant();
	}
	auto i = d_ptr->mData->find(row, col);
	if (i == d_ptr->mData->end()) {
		return QVariant();
	}
	if (d_ptr->mToDisplayString) {
		// 如果注册了显示函数指针，先调用显示函数指针
		return d_ptr->mToDisplayString(i->second);
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
	if (!d_ptr->mData) {
		return;
	}
	d_ptr->mData->removeCell(row, col);
}

}
