#include "DAPyDataFrameTableModule.h"
#include "Commands/DACommandsDataFrame.h"
#include <QUndoStack>
#include <algorithm>
namespace DA
{
class DAPyDataFrameTableModulePrivate
{
	DA_IMPL_PUBLIC(DAPyDataFrameTableModule)
public:
	DAPyDataFrameTableModulePrivate(DAPyDataFrameTableModule* p);

public:
	DAPyDataFrame _dataframe;
	QUndoStack* _undoStack;
	int extraColumn { 1 };  ///< 扩展的列数，也就是会多显示出externColumn个空白的列，一般多显示出来的是为了用户添加数据用的
	int extraRow { 1 };  ///< 扩展的行数，也就是会多显示出externRow个空白的行，一般多显示出来的是为了用户添加数据用的
	int minShowRow { 20 };    ///< 最小显示的行数
	int minShowColumn { 4 };  ///< 最小显示的列数
};
}  // end of namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DAPyDataFrameTableModulePrivate
//===================================================

DAPyDataFrameTableModulePrivate::DAPyDataFrameTableModulePrivate(DAPyDataFrameTableModule* p)
    : q_ptr(p), _undoStack(nullptr)
{
}
//===================================================
// DAPyDataFrameTableModule
//===================================================
DAPyDataFrameTableModule::DAPyDataFrameTableModule(QUndoStack* stack, QObject* parent)
    : QAbstractTableModel(parent), d_ptr(new DAPyDataFrameTableModulePrivate(this))
{
    d_ptr->_undoStack = stack;
}

DAPyDataFrameTableModule::~DAPyDataFrameTableModule()
{
}

QVariant DAPyDataFrameTableModule::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || d_ptr->_dataframe.isNone()) {
		return QVariant();
	}
	std::pair< std::size_t, std::size_t > shape = d_ptr->_dataframe.shape();
	if (Qt::Horizontal == orientation) {  // 说明是水平表头
		if (section >= (int)shape.second) {
			return QVariant();
		}
		return d_ptr->_dataframe.columns()[ section ];
	} else {
		if (section >= (int)shape.first) {
			return QVariant();
		}
		QVariant h = d_ptr->_dataframe.index()[ section ];
		// 查看是否是符合index
		if (h.canConvert(QMetaType::QVariantList)) {
			// 说明是复合表头
			QVariantList ss = h.toList();
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
		return h;
	}
	return QVariant();
}

int DAPyDataFrameTableModule::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	if (d_ptr->_dataframe.isNone()) {
		return d_ptr->minShowColumn;
	}
	std::pair< std::size_t, std::size_t > shape = d_ptr->_dataframe.shape();
	if ((static_cast< int >(shape.second) + d_ptr->extraColumn) < d_ptr->minShowColumn) {
		return d_ptr->minShowColumn;
	}
	return (static_cast< int >(shape.second) + d_ptr->extraColumn);
}

int DAPyDataFrameTableModule::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	if (d_ptr->_dataframe.isNone()) {
		return d_ptr->minShowRow;
	}
	std::pair< std::size_t, std::size_t > shape = d_ptr->_dataframe.shape();
	if ((static_cast< int >(shape.first) + d_ptr->extraRow) < d_ptr->minShowRow) {
		return d_ptr->minShowRow;
	}
	return (static_cast< int >(shape.first) + d_ptr->extraRow);
}

QVariant DAPyDataFrameTableModule::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || d_ptr->_dataframe.isNone()) {
		return QVariant();
	}
	std::pair< std::size_t, std::size_t > shape = d_ptr->_dataframe.shape();
	if (index.row() >= (int)shape.first || index.column() >= (int)shape.second) {
		return QVariant();
	}
	if (role == Qt::TextAlignmentRole) {
		// 返回的是对其方式
		return int(Qt::AlignLeft | Qt::AlignVCenter);
	} else if (role == Qt::DisplayRole) {
		// 返回的是内容
		return d_ptr->_dataframe.iat(index.row(), index.column());
	} else if (role == Qt::BackgroundRole) {
		// 背景颜色
		return QVariant();
	}
	return QVariant();
}

bool DAPyDataFrameTableModule::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (Qt::EditRole != role) {
		return false;
	}
	if (!index.isValid() || d_ptr->_dataframe.isNone()) {
		return false;
	}
	std::pair< std::size_t, std::size_t > shape = d_ptr->_dataframe.shape();
	if (index.row() >= static_cast< int >(shape.first)) {
		// todo:这里实现一个dataframe追加行
		return false;
	}
	if (index.column() >= static_cast< int >(shape.second)) {
		// todo:这里实现一个dataframe追加列
		return false;
	}
	QVariant olddata = d_ptr->_dataframe.iat(index.row(), index.column());
	if (value.isNull() == olddata.isNull()) {
		// 两次都为空就跳过
		return false;
	}
	if (!(d_ptr->_undoStack)) {
		// 如果d_ptr->_undoStack设置为nullptr，将不使用redo/undo
		return d_ptr->_dataframe.iat(index.row(), index.column(), value);
	}
	std::unique_ptr< DACommandDataFrame_iat > cmd_iat(
		new DACommandDataFrame_iat(d_ptr->_dataframe, index.row(), index.column(), olddata, value, this));
	if (!cmd_iat->exec()) {
		// 没设置成功，退出
		return false;
	}
	d_ptr->_undoStack->push(cmd_iat.release());  // push后会自动调用redo，第二次调用redo会被忽略
	d_ptr->_undoStack->setActive(true);
	// 这里说明设置成功了
	return true;
}

Qt::ItemFlags DAPyDataFrameTableModule::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

DAPyDataFrame& DAPyDataFrameTableModule::dataFrame()
{
	return d_ptr->_dataframe;
}

const DAPyDataFrame& DAPyDataFrameTableModule::dataFrame() const
{
	return d_ptr->_dataframe;
}

void DAPyDataFrameTableModule::setDAData(const DAData& d)
{
	if (!d.isDataFrame()) {
		d_ptr->_dataframe = DAPyDataFrame();
		return;
	}
	beginResetModel();
	d_ptr->_dataframe = d.toDataFrame();
	endResetModel();
}

void DAPyDataFrameTableModule::setDataFrame(const DAPyDataFrame& d)
{
	beginResetModel();
	d_ptr->_dataframe = d;
	endResetModel();
}

void DAPyDataFrameTableModule::refreshRow(int row)
{
	if (row >= rowCount()) {
		return;
	}
	int c = columnCount() - 1;
	if (c < 0) {
		c = 0;
	}
	emit dataChanged(createIndex(row, 0), createIndex(row, c));
}

void DAPyDataFrameTableModule::refreshColumn(int col)
{
	if (col >= columnCount()) {
		return;
	}
	int r = rowCount() - 1;
	if (r < 0) {
		r = 0;
	}
	emit dataChanged(createIndex(0, col), createIndex(r, col));
}

/**
 * @brief 刷新
 * @param row
 * @param col
 */
void DAPyDataFrameTableModule::refresh(int row, int col)
{
	if (row >= rowCount() || col >= columnCount()) {
		return;
	}
	emit dataChanged(createIndex(row, col), createIndex(row, col));
}

void DAPyDataFrameTableModule::refresh(int rowStart, int colStart, int rowEnd, int colEnd)
{
	if (rowEnd >= rowCount() || colEnd >= columnCount()) {
		return;
	}
	emit dataChanged(createIndex(rowStart, colStart), createIndex(rowEnd, colEnd));
}

/**
 * @brief 全部刷新
 */
void DAPyDataFrameTableModule::refresh()
{
	beginResetModel();
	endResetModel();
}

#define DAPyDataFrameTableModule_beginXX(funname, list)

/**
 * @brief 通知model行被移除了
 * @param r
 */
void DAPyDataFrameTableModule::rowBeginRemove(const QList< int >& r)
{
    beginFunCall(r, [ this ](const QModelIndex& p, int f, int l) { this->beginRemoveRows(p, f, l); });
}

void DAPyDataFrameTableModule::rowEndRemove()
{
    endRemoveRows();
}

void DAPyDataFrameTableModule::rowBeginInsert(const QList< int >& r)
{
    beginFunCall(r, [ this ](const QModelIndex& p, int f, int l) { this->beginInsertRows(p, f, l); });
}

void DAPyDataFrameTableModule::rowEndInsert()
{
    endInsertRows();
}

void DAPyDataFrameTableModule::columnBeginRemove(const QList< int >& r)
{
    beginFunCall(r, [ this ](const QModelIndex& p, int f, int l) { this->beginRemoveColumns(p, f, l); });
}

void DAPyDataFrameTableModule::columnEndRemove()
{
    endRemoveColumns();
}

void DAPyDataFrameTableModule::columnBeginInsert(const QList< int >& r)
{
    beginFunCall(r, [ this ](const QModelIndex& p, int f, int l) { this->beginInsertColumns(p, f, l); });
}

void DAPyDataFrameTableModule::columnEndInsert()
{
    endInsertColumns();
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
void DAPyDataFrameTableModule::setExtraRowCount(int v)
{
    d_ptr->extraRow = v;
}

/**
 * @brief 超出模型实际数据行数的额外空行数量
 * @return 超出模型实际数据行数的额外空行数量
 * @see setExtraRowCount
 */
int DAPyDataFrameTableModule::getExtraRowCount() const
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
void DAPyDataFrameTableModule::setExtraColumnCount(int v)
{
    d_ptr->extraColumn = v;
}

/**
 * @brief 超出模型实际数据列数的额外空列数量
 * @return 超出模型实际数据列数的额外空列数量
 * @see setExtraColumnCount
 */
int DAPyDataFrameTableModule::getExtraColumnCount() const
{
    return d_ptr->extraColumn;
}

void DAPyDataFrameTableModule::setMinShowRowCount(int v)
{
    d_ptr->minShowRow = v;
}

int DAPyDataFrameTableModule::getMinShowRowCount() const
{
    return d_ptr->minShowRow;
}

void DAPyDataFrameTableModule::setMinShowColumnCount(int v)
{
    d_ptr->minShowColumn = v;
}

int DAPyDataFrameTableModule::getMinShowColumnCount() const
{
    return d_ptr->minShowColumn;
}

void DAPyDataFrameTableModule::beginFunCall(const QList< int >& listlike, DAPyDataFrameTableModule::beginFun fun)
{
	if (listlike.size() == 1) {
		fun(QModelIndex(), listlike[ 0 ], listlike[ 0 ]);
	} else {
		// 排序加去重
		QList< int > orderindex = listlike;
		std::sort(orderindex.begin(), orderindex.end());
		auto last = std::unique(orderindex.begin(), orderindex.end());
		orderindex.erase(last, orderindex.end());
		int first = orderindex[ 0 ];
		for (auto i = orderindex.begin() + 1; i != orderindex.end(); ++i) {
			auto pre = i - 1;
			if ((*i) - (*pre) != 1) {
				// 后一个减去前一个不为1，说明跨越
				// 这时候就要执行beginRemoveRows，同时记录删除的数量
				fun(QModelIndex(), first, *pre);
				first = *i;
			}
		}
		// 对于一直连续，这个直接一次连续，对于不连续，最后一段跨度也是此
		fun(QModelIndex(), first, orderindex.back());
	}
}
