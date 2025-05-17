#include "DAPyDataFrameTableModel.h"
#include "Commands/DACommandsDataFrame.h"
#include <QUndoStack>
#include <algorithm>
#ifndef DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT
#define DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT 1
#endif
namespace DA
{
class DAPyDataFrameTableModel::PrivateData
{
	DA_DECLARE_PUBLIC(DAPyDataFrameTableModel)
public:
	PrivateData(DAPyDataFrameTableModel* p);

public:
	DAPyDataFrame dataframe;
	QUndoStack* undoStack { nullptr };
	int extraColumn { 1 };  ///< 扩展的列数，也就是会多显示出externColumn个空白的列，一般多显示出来的是为了用户添加数据用的
	int extraRow { 1 };  ///< 扩展的行数，也就是会多显示出externRow个空白的行，一般多显示出来的是为了用户添加数据用的
	int minShowRow { 20 };    ///< 最小显示的行数
	int minShowColumn { 4 };  ///< 最小显示的列数
};

//===================================================
// DAPyDataFrameTableModulePrivate
//===================================================

DAPyDataFrameTableModel::PrivateData::PrivateData(DAPyDataFrameTableModel* p) : q_ptr(p), undoStack(nullptr)
{
}
//===================================================
// DAPyDataFrameTableModule
//===================================================
DAPyDataFrameTableModel::DAPyDataFrameTableModel(QUndoStack* stack, QObject* parent)
	: QAbstractTableModel(parent), DA_PIMPL_CONSTRUCT
{
	d_ptr->undoStack = stack;
}

DAPyDataFrameTableModel::~DAPyDataFrameTableModel()
{
}

QVariant DAPyDataFrameTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	DA_DC(d);
	if (role != Qt::DisplayRole || d->dataframe.isNone()) {
		return QVariant();
	}
	std::pair< std::size_t, std::size_t > shape = d->dataframe.shape();
	if (Qt::Horizontal == orientation) {  // 说明是水平表头
		if (section >= (int)shape.second) {
			return QVariant();
		}
		return d->dataframe.columns()[ section ];
	} else {
		if (section >= (int)shape.first) {
			return QVariant();
		}
		QVariant h = d->dataframe.index()[ section ];
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

int DAPyDataFrameTableModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	DA_DC(d);
	if (d->dataframe.isNone()) {
		return d->minShowColumn;
	}
	std::pair< std::size_t, std::size_t > shape = d->dataframe.shape();
	return std::max(static_cast< int >(shape.second) + d->extraColumn, d->minShowColumn);
}

int DAPyDataFrameTableModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	DA_DC(d);
	if (d->dataframe.isNone()) {
		return d->minShowRow;
	}
	std::pair< std::size_t, std::size_t > shape = d->dataframe.shape();
	return std::max(static_cast< int >(shape.first) + d->extraRow, d->minShowRow);
}

QVariant DAPyDataFrameTableModel::data(const QModelIndex& index, int role) const
{
	DA_DC(d);
	if (!index.isValid() || d->dataframe.isNone()) {
		return QVariant();
	}
	std::pair< std::size_t, std::size_t > shape = d->dataframe.shape();
	if (index.row() >= (int)shape.first || index.column() >= (int)shape.second) {
		return QVariant();
	}
	if (role == Qt::TextAlignmentRole) {
		// 返回的是对其方式
		return int(Qt::AlignLeft | Qt::AlignVCenter);
	} else if (role == Qt::DisplayRole) {
		// 返回的是内容
		return d->dataframe.iat(index.row(), index.column());
	} else if (role == Qt::BackgroundRole) {
		// 背景颜色
		return QVariant();
	}
	return QVariant();
}

bool DAPyDataFrameTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (Qt::EditRole != role) {
		return false;
	}
	DA_D(d);
	if (!index.isValid() || d->dataframe.isNone()) {
		return false;
	}
	std::pair< std::size_t, std::size_t > shape = d->dataframe.shape();
	if (index.row() >= static_cast< int >(shape.first)) {
		// todo:这里实现一个dataframe追加行
		return false;
	}
	if (index.column() >= static_cast< int >(shape.second)) {
		// todo:这里实现一个dataframe追加列
		return false;
	}
	QVariant olddata = d->dataframe.iat(index.row(), index.column());
	if (value.isNull() == olddata.isNull()) {
		// 两次都为空就跳过
		return false;
	}
	if (!(d->undoStack)) {
		// 如果d->_undoStack设置为nullptr，将不使用redo/undo
		return d->dataframe.iat(index.row(), index.column(), value);
	}
	std::unique_ptr< DACommandDataFrame_iat > cmd_iat(
		new DACommandDataFrame_iat(d->dataframe, index.row(), index.column(), olddata, value, this));
	if (!cmd_iat->exec()) {
		// 没设置成功，退出
		return false;
	}
	d->undoStack->push(cmd_iat.release());  // push后会自动调用redo，第二次调用redo会被忽略
	d->undoStack->setActive(true);
	// 这里说明设置成功了
	return true;
}

Qt::ItemFlags DAPyDataFrameTableModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
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
		return;
	}
	beginResetModel();
	d_ptr->dataframe = d.toDataFrame();
	endResetModel();
}

void DAPyDataFrameTableModel::setDataFrame(const DAPyDataFrame& d)
{
	beginResetModel();
	d_ptr->dataframe = d;
	endResetModel();
}

void DAPyDataFrameTableModel::refreshRow(int row)
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

void DAPyDataFrameTableModel::refreshColumn(int col)
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
void DAPyDataFrameTableModel::refresh(int row, int col)
{
	if (row >= rowCount() || col >= columnCount()) {
		return;
	}
	emit dataChanged(createIndex(row, col), createIndex(row, col));
}

void DAPyDataFrameTableModel::refresh(int rowStart, int colStart, int rowEnd, int colEnd)
{
	if (rowEnd >= rowCount() || colEnd >= columnCount()) {
		return;
	}
	emit dataChanged(createIndex(rowStart, colStart), createIndex(rowEnd, colEnd));
}

/**
 * @brief 全部刷新
 */
void DAPyDataFrameTableModel::refresh()
{
	beginResetModel();
	endResetModel();
}

/**
 * @brief 通知模型，行被移除了
 * @param r
 */
void DAPyDataFrameTableModel::notifyRowsRemoved(const QList< int >& r)
{
	beginFunCall(r, [ this ](const QModelIndex& p, int f, int l) { this->beginRemoveRows(p, f, l); });
	endRemoveRows();
}

/**
 * @brief 通知模型，行被插入了
 * @param r
 */
void DAPyDataFrameTableModel::notifyRowsInserted(const QList< int >& r)
{
	beginFunCall(r, [ this ](const QModelIndex& p, int f, int l) { this->beginInsertRows(p, f, l); });
	endInsertRows();
}

/**
 * @brief 通知模型，列被移除了
 * @param c
 */
void DAPyDataFrameTableModel::notifyColumnsRemoved(const QList< int >& c)
{
	beginFunCall(c, [ this ](const QModelIndex& p, int f, int l) { this->beginRemoveColumns(p, f, l); });
	endRemoveColumns();
}

/**
 * @brief 通知模型，列被插入了
 * @param r
 */
void DAPyDataFrameTableModel::notifyColumnsInserted(const QList< int >& c)
{
	beginFunCall(c, [ this ](const QModelIndex& p, int f, int l) { this->beginInsertColumns(p, f, l); });
	endInsertColumns();
}

#define DAPyDataFrameTableModule_beginXX(funname, list)

/**
 * @brief 通知model行被移除了
 * @param r
 */
void DAPyDataFrameTableModel::rowsBeginRemove(const QList< int >& r)
{
    beginFunCall(r, [ this ](const QModelIndex& p, int f, int l) { this->beginRemoveRows(p, f, l); });
}

void DAPyDataFrameTableModel::rowsBeginInsert(const QList< int >& r)
{
    beginFunCall(r, [ this ](const QModelIndex& p, int f, int l) { this->beginInsertRows(p, f, l); });
}

void DAPyDataFrameTableModel::columnBeginRemove(const QList< int >& r)
{
    beginFunCall(r, [ this ](const QModelIndex& p, int f, int l) { this->beginRemoveColumns(p, f, l); });
}

void DAPyDataFrameTableModel::columnsBeginInsert(const QList< int >& r)
{
    beginFunCall(r, [ this ](const QModelIndex& p, int f, int l) { this->beginInsertColumns(p, f, l); });
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

void DAPyDataFrameTableModel::beginFunCall(const QList< int >& listlike, DAPyDataFrameTableModel::beginFun fun)
{
	// 如果输入列表为空，直接返回，无需进行后续处理
	if (listlike.isEmpty()) {
		return;
	}
#if 1  // deepseek 优化
	// 将输入列表复制到一个新列表中，以便进行排序和去重操作
	QList< int > sorted = listlike;
	// 对列表进行升序排序
	std::sort(sorted.begin(), sorted.end());
	// 使用 std::unique 去除相邻的重复元素，返回指向去重后最后一个元素的下一个位置的迭代
	auto last = std::unique(sorted.begin(), sorted.end());
	// 将列表末尾的多余元素（即重复元素）擦除
	sorted.erase(last, sorted.end());
	// 获取排序后第一个元素的值，作为初始的起始索引
	int first = sorted.first();
	// 遍历排序后的列表，处理连续和非连续的索引范围
	for (int i = 1; i < sorted.size(); ++i) {
		// 检查当前元素是否与前一个元素连续（即是否相差 1）
		if (sorted[ i ] != sorted[ i - 1 ] + 1) {
			// 如果不连续，调用传入的函数，处理从 first 到 sorted[i-1] 的连续索引范围
			fun(QModelIndex(), first, sorted[ i - 1 ]);
			// 更新 first 为当前元素的值，作为下一个区间的起始索引
			first = sorted[ i ];
		}
	}
	// 循环结束后，处理最后一个连续的索引范围（从 first 到 sorted.last()）
	fun(QModelIndex(), first, sorted.last());

#else
	if (listlike.empty()) {
		return;
	}
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
#endif
}
}  // end of namespace DA
