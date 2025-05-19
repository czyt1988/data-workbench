#ifndef DAPYDATAFRAMETABLEMODEL_H
#define DAPYDATAFRAMETABLEMODEL_H
#include "DAGuiAPI.h"
#include <QtCore/qglobal.h>
#include <QAbstractTableModel>
#include "pandas/DAPyDataFrame.h"
#include "DAData.h"
#include <functional>
class QUndoStack;
namespace DA
{

/**
 * @brief 针对DAPyDataFrame的table model
 *
 * @note QTableView有个bug，在面对超大规模的数据时，会出现遍历所有行的headerData情况，导致非常耗时，同时QHeaderView也有这个问题，在选中一列时，
 * 要遍历这一列所有行的headerData，调试发现会大量调用columnCount，并不能实现真正的虚拟显示，因此，TableModel的实现，将数据进行缓存，
 * 让数据在一个固定的区间里面刷新，从而解决这个问题。
 */
class DAGUI_API DAPyDataFrameTableModel : public QAbstractTableModel
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAPyDataFrameTableModel)
public:
    enum CacheMode
    {
        /**
         * @brief 滑动窗缓存模式
         *
         * 滑动窗模式下，会有一个固定尺寸的窗，数据缓存到这个窗里面进行显示，滑动窗适合一个view展示所有超大规模的数据
         *
         * 默认为滑动窗模式
         */
        DynamicSlidingCacheMode,
        /**
         * @brief 分页缓存模式
         *
         * 分页模式下，可以给model设置分页，定位到不同页下进行偏移输出，这个模式适合有分页按钮的显示
         *
         * @sa setPageSize getPageSize setCurrentPage getCurrentPage
         * @sa currentPageChanged
         */
        PageCacheMode

    };
	DAPyDataFrameTableModel(QUndoStack* stack, QObject* parent = nullptr);
	~DAPyDataFrameTableModel();
	using beginFun = std::function< void(const QModelIndex&, int, int) >;

public:
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

public:
	DAPyDataFrame& dataFrame();
	const DAPyDataFrame& dataFrame() const;
    // 缓存模式
    void setCacheMode(CacheMode mode);
    CacheMode getCacheMode() const;
    // 设置数据
	void setDAData(const DAData& d);
	void setDataFrame(const DAPyDataFrame& d);

    /// @group 分页模式
    /// @{
	void setPageSize(int size);
	int getPageSize() const;
	void setCurrentPage(int page);
	int getCurrentPage() const;
    /// @}

    /// @group 滑动窗模式
    /// @{
    // 设置滑动窗模式的起始行
    void setCacheWindowStartRow(int startRow);
    /// @}

	// 获取真实的dataframe行数
	int getActualDataframeRowCount() const;
	// 刷新
	void refresh();
	void notifyRowChanged(int row);
	void notifyColumnChanged(int col);
	void notifyDataChanged(int row, int col);
	void notifyDataChanged(int rowStart, int colStart, int rowEnd, int colEnd);
	// 行将移除
	void notifyRowsRemoved(const QList< int >& r);

	// 行将插入
	void notifyRowsInserted(const QList< int >& r);

	// 行将移除
	void notifyColumnsRemoved(const QList< int >& c);

	// 行将插入
	void notifyColumnsInserted(const QList< int >& c);

	// 超出模型实际数据行数的额外空行数量
	void setExtraRowCount(int v);
	int getExtraRowCount() const;
	// 超出模型实际数据列数的额外空列数量
	void setExtraColumnCount(int v);
	int getExtraColumnCount() const;
	// 最小显示的行数量
	void setMinShowRowCount(int v);
	int getMinShowRowCount() const;
	// 最小显示的列数量
	void setMinShowColumnCount(int v);
	int getMinShowColumnCount() const;

protected:
	// 缓存
	void cacheShape();
	void cacheRowShape();
	void cacheColumnShape();
	void rowsBeginRemove(const QList< int >& r);
	void rowsBeginInsert(const QList< int >& r);
	void columnBeginRemove(const QList< int >& r);
	void columnsBeginInsert(const QList< int >& r);
	void beginFunCall(const QList< int >& listlike, beginFun fun);
    // 滑动窗模式有用
    void prefetchData(int start, int end) const;
Q_SIGNALS:
    void currentPageChanged(int newPage);
};
}  // end of namespace DA
#endif  // DAPYDATAFRAMETABLEMODEL_H
