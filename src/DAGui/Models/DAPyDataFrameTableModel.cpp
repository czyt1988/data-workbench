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
	void setCacheMode(DAPyDataFrameTableModel::CacheMode m);

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
	DAPyIndex index;               ///< 保存index
	bool isNone { true };          ///< 标记是否为none
	bool isIndexNone { true };     ///< index是否为空
	DAPyDataFrameTableModel::CacheMode cacheMode { DAPyDataFrameTableModel::DynamicSlidingCacheMode };
	// 虚拟视图 -- 分页需要的参数
    int pageSize { 10000 };        // 每页行数
    int currentPage { 0 };         // 当前页码
                                   // 滑动窗需要的参数
    int cacheWindowSize { 2000 };  // 默认窗口大小
    int windowStartRow { 0 };      // 当前窗口起始行
    bool isPageMode { false };     ///< 是否分页
};

//===================================================
// DAPyDataFrameTableModulePrivate
//===================================================

DAPyDataFrameTableModel::PrivateData::PrivateData(DAPyDataFrameTableModel* p) : q_ptr(p), undoStack(nullptr)
{
}

void DAPyDataFrameTableModel::PrivateData::setCacheMode(DAPyDataFrameTableModel::CacheMode m)
{
	if (cacheMode == m) {
		return;
	}
	cacheMode = m;
	// 清理
	if (m == DAPyDataFrameTableModel::DynamicSlidingCacheMode) {
		currentPage = 0;
		isPageMode  = false;
	} else {
		windowStartRow = 0;
		isPageMode     = true;
	}
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
#if DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT
	qDebug() << "cntheaderData=" << d_ptr->cntheaderData;
	qDebug() << "cntcolumnCount=" << d_ptr->cntcolumnCount;
	qDebug() << "cntdata=" << d_ptr->cntdata;
#endif
}

QVariant DAPyDataFrameTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	DA_DC(d);
	DAPYDATAFRAMETABLEMODEL_CALL_ADD(cntheaderData);

    if ((role != Qt::DisplayRole && role != Qt::ToolTipRole) || d->isNone) {
        return QAbstractTableModel::headerData(section, orientation, role);
	}
    // tooltips 和 display
	if (Qt::Horizontal == orientation) {  // 说明是水平表头
		if (section >= d->columnsName.size()) {
			return QVariant();
		}
		return d->columnsName[ section ];
	} else {
		// 行头处理：转换为实际数据行索引
		int actualSection(0);
		if (d->isPageMode) {
			actualSection = d->currentPage * d->pageSize + section;
		} else {
			actualSection = d->windowStartRow + section;
		}

		if (actualSection >= d->dataframeRow) {
			return QVariant();
		}
		if (d->isIndexNone) {
			// 如果索引为空，就显示序号
			return actualSection;
		}
		QVariant h = d->index[ actualSection ];
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
	DAPYDATAFRAMETABLEMODEL_CALL_ADD(cntcolumnCount);
	if (d->isNone) {
		return d->minShowColumn;
	}
	return std::max(d->dataframeColumn + d->extraColumn, d->minShowColumn);
}

int DAPyDataFrameTableModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	DA_DC(d);
	if (d->isNone) {
		return d->minShowRow;
	}
	if (d->isPageMode) {
		// 启用分页虚拟化时返回缓存行数
		int visibleRows = qMin(d->pageSize, d->dataframeRow);
		return std::max(visibleRows + d->extraRow, d->minShowRow);
	} else {
		// 动态窗口模式
		return qMin(d->cacheWindowSize, d->dataframeRow + d->extraRow);
	}
}

QVariant DAPyDataFrameTableModel::data(const QModelIndex& index, int role) const
{
	DA_DC(d);
	DAPYDATAFRAMETABLEMODEL_CALL_ADD(cntdata);
	if (!index.isValid() || d->isNone) {
		return QVariant();
	}
	int actualRow = 0;
	if (d->isPageMode) {
		// 分页模式
		// 行头处理：转换为实际数据行索引
		actualRow = d->currentPage * d->pageSize + index.row();
	} else {
		actualRow = d->windowStartRow + index.row();
	}
    if (actualRow >= d->dataframeRow || index.column() >= d->dataframeColumn) {
		return QVariant();
	}
	switch (role) {
	case Qt::TextAlignmentRole:
		return int(Qt::AlignLeft | Qt::AlignVCenter);
	case Qt::BackgroundRole:
		return QVariant();
    case Qt::DisplayRole: {
        return d->dataframe.iat(actualRow, index.column());
	}
	default:
		break;
	}

	return QVariant();
}

bool DAPyDataFrameTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (Qt::EditRole != role) {
		return false;
	}
	DA_D(d);
	if (!index.isValid() || d->isNone) {
		return false;
	}
	// 如果启用虚拟化，要计算实际的行号
	int actualRow(0);
	if (d->isPageMode) {
		actualRow = d->currentPage * d->pageSize + index.row();
	} else {
		actualRow = d->windowStartRow + index.row();
	}
	if (actualRow >= d->dataframeRow) {
		// todo:这里实现一个dataframe追加行
		return false;
	}
	if (index.column() >= d->dataframeColumn) {
		// todo:这里实现一个dataframe追加列
		return false;
	}
	QVariant olddata = d->dataframe.iat(actualRow, index.column());
	if (value.isNull() == olddata.isNull()) {
		// 两次都为空就跳过
		return false;
	}
	if (!(d->undoStack)) {
		// 如果d->_undoStack设置为nullptr，将不使用redo/undo
		return d->dataframe.iat(actualRow, index.column(), value);
	}
	std::unique_ptr< DACommandDataFrame_iat > cmd_iat(
        new DACommandDataFrame_iat(d->dataframe, actualRow, index.column(), olddata, value, this));
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

void DAPyDataFrameTableModel::setCacheMode(CacheMode mode)
{
	d_ptr->setCacheMode(mode);
}

DAPyDataFrameTableModel::CacheMode DAPyDataFrameTableModel::getCacheMode() const
{
	return d_ptr->cacheMode;
}

void DAPyDataFrameTableModel::setDAData(const DAData& d)
{
	if (!d.isDataFrame()) {
		d_ptr->dataframe = DAPyDataFrame();
		refresh();
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
	refresh();
#if DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT
	qDebug() << "setDAData after refresh,cost:" << __elasper.elapsed() << " ms";
#endif
}

void DAPyDataFrameTableModel::setPageSize(int size)
{
	DA_D(d);
	if (size <= 0) {
		size = 1;
	}
	d->pageSize = size;
	if (d->isPageMode) {
		beginResetModel();
		endResetModel();
	}
}

int DAPyDataFrameTableModel::getPageSize() const
{
	return d_ptr->pageSize;
}

void DAPyDataFrameTableModel::setCurrentPage(int page)
{
	DA_D(d);
	if (!d->isPageMode) {
		return;
	}
	int totalPages = qMax(1, d->dataframeRow / d->pageSize + (d->dataframeRow % d->pageSize ? 1 : 0));
	page           = qBound(0, page, totalPages - 1);
	if (d->currentPage == page) {
		return;
	}

	d->currentPage = page;
	emit currentPageChanged(page);
	emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));  // 刷新当前页数据
}

int DAPyDataFrameTableModel::getCurrentPage() const
{
    return d_ptr->currentPage;
}

/**
 * @brief 设置滑动窗模式的起始行
 * @param startRow
 */
void DAPyDataFrameTableModel::setCacheWindowStartRow(int startRow)
{
	DA_D(d);
	if (d->isPageMode) {
		return;
	}
	// startRow限制在指定的最小值和最大值之间。它能够确保startRow不会超出给定的范围
    startRow = qBound(0, startRow, d->dataframeRow - d->cacheWindowSize + d->extraRow);
    if (startRow >= d->dataframeRow) {
        startRow = d->dataframeRow - 1;
    }
	if (startRow != d->windowStartRow) {
		const int oldStart = d->windowStartRow;
		d->windowStartRow  = startRow;

		// 计算需要刷新的区域
		const int overlapStart = qMax(oldStart, startRow);
		const int overlapEnd   = qMin(oldStart + d->cacheWindowSize, startRow + d->cacheWindowSize);

		if (overlapStart >= overlapEnd) {
			// 完全无重叠，全量刷新
			Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
		} else {
			// 部分刷新
			if (oldStart < startRow) {
				Q_EMIT dataChanged(index(0, 0), index(startRow - oldStart - 1, columnCount() - 1));
			}
			if (oldStart + d->cacheWindowSize > startRow + d->cacheWindowSize) {
				const int diff = oldStart + d->cacheWindowSize - (startRow + d->cacheWindowSize);
				Q_EMIT dataChanged(index(rowCount() - diff, 0), index(rowCount() - 1, columnCount() - 1));
			}
		}
	}
}

int DAPyDataFrameTableModel::getCacheWindowStartRow() const
{
	return d_ptr->windowStartRow;
}

void DAPyDataFrameTableModel::setCacheWindowSize(int s)
{
	DA_D(d);
	d->cacheWindowSize = s;
	setCacheWindowStartRow(d->windowStartRow);
}

int DAPyDataFrameTableModel::getCacheWindowSize() const
{
    return d_ptr->cacheWindowSize;
}

/**
 * @brief 获取真实的行数
 * @return
 */
int DAPyDataFrameTableModel::getActualDataframeRowCount() const
{
    return d_ptr->dataframeRow;
}

void DAPyDataFrameTableModel::notifyRowChanged(int row)
{
	if (row >= rowCount()) {
		return;
	}
	int c = columnCount() - 1;
	if (c < 0) {
		c = 0;
	}
	cacheRowShape();
	emit dataChanged(createIndex(row, 0), createIndex(row, c));
}

void DAPyDataFrameTableModel::notifyColumnChanged(int col)
{
	if (col >= columnCount()) {
		return;
	}
	int r = rowCount() - 1;
	if (r < 0) {
		r = 0;
	}
	cacheColumnShape();
	emit dataChanged(createIndex(0, col), createIndex(r, col));
}

/**
 * @brief 刷新
 * @param row
 * @param col
 */
void DAPyDataFrameTableModel::notifyDataChanged(int row, int col)
{
	if (row >= rowCount() || col >= columnCount()) {
		return;
	}
	emit dataChanged(createIndex(row, col), createIndex(row, col));
}

void DAPyDataFrameTableModel::notifyDataChanged(int rowStart, int colStart, int rowEnd, int colEnd)
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
	DA_D(d);
	beginResetModel();
	d->isNone = d->dataframe.isNone();
	cacheShape();
    if (d->isPageMode) {
        setCurrentPage(0);  // 重置到第一页
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
	d->index           = d->dataframe.index();
#if DAPYDATAFRAMETABLEMODULE_PROFILE_PRINT

#endif
}

void DAPyDataFrameTableModel::cacheRowShape()
{
	DA_D(d);
	auto shape      = d->dataframe.shape();
	d->dataframeRow = static_cast< int >(shape.first);
	d->index        = d->dataframe.index();
	d->isIndexNone  = d->index.isNone();
}

void DAPyDataFrameTableModel::cacheColumnShape()
{
	DA_D(d);
	auto shape         = d->dataframe.shape();
	d->dataframeColumn = static_cast< int >(shape.second);
	d->columnsName     = d->dataframe.columns();
}

/**
 * @brief 通知模型，行被移除了
 * @param r
 */
void DAPyDataFrameTableModel::notifyRowsRemoved(const QList< int >& r)
{
    if (r.isEmpty()) {
        return;
    }
    // 由于使用了缓存表，删除只需要刷新数据即可
    cacheRowShape();
    // 获取最小和最大行号
    int minRow = *std::min_element(r.begin(), r.end());
    emit dataChanged(createIndex(minRow, 0), createIndex(rowCount() - 1, columnCount() - 1));
}

/**
 * @brief 通知模型，行被插入了
 * @param r
 */
void DAPyDataFrameTableModel::notifyRowsInserted(const QList< int >& r)
{
    if (r.isEmpty()) {
        return;
    }
    // 由于使用了缓存表，删除只需要刷新数据即可
    cacheRowShape();
    // 获取最小和最大行号
    int minRow = *std::min_element(r.begin(), r.end());
    emit dataChanged(createIndex(minRow, 0), createIndex(rowCount() - 1, columnCount() - 1));
}

/**
 * @brief 通知模型，列被移除了
 * @param c
 */
void DAPyDataFrameTableModel::notifyColumnsRemoved(const QList< int >& c)
{
    if (c.isEmpty()) {
        return;
    }
    // 由于使用了缓存表，删除只需要刷新数据即可
    cacheColumnShape();
    int minCol = *std::min_element(c.begin(), c.end());
    emit dataChanged(createIndex(0, minCol), createIndex(rowCount() - 1, columnCount() - 1));
}

/**
 * @brief 通知模型，列被插入了
 * @param r
 */
void DAPyDataFrameTableModel::notifyColumnsInserted(const QList< int >& c)
{
    if (c.isEmpty()) {
        return;
    }
    // 由于使用了缓存表，删除只需要刷新数据即可
    cacheColumnShape();
    int minCol = *std::min_element(c.begin(), c.end());
    emit dataChanged(createIndex(0, minCol), createIndex(rowCount() - 1, columnCount() - 1));
}

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
}

}  // end of namespace DA
