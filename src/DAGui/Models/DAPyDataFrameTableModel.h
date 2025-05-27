#ifndef DAPYDATAFRAMETABLEMODEL_H
#define DAPYDATAFRAMETABLEMODEL_H
#include "DAGuiAPI.h"
#include <QtCore/qglobal.h>
#include <QAbstractTableModel>
#include "DAAbstractCacheWindowTableModel.h"
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
class DAGUI_API DAPyDataFrameTableModel : public DAAbstractCacheWindowTableModel
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAPyDataFrameTableModel)
public:
	DAPyDataFrameTableModel(QUndoStack* stack, QObject* parent = nullptr);
	~DAPyDataFrameTableModel();

public:
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual QVariant actualHeaderData(int actualSection, Qt::Orientation orientation, int role) const override;
    virtual int actualRowCount() const override;
    virtual QVariant actualData(int actualRow, int actualColumn, int role) const override;
    // 设置数据
    virtual bool setActualData(int actualRow, int actualColumn, const QVariant& value, int role = Qt::EditRole) override;

public:
	DAPyDataFrame& dataFrame();
	const DAPyDataFrame& dataFrame() const;
	// 缓存模式
	// 设置数据
	void setDAData(const DAData& d);
	void setDataFrame(const DAPyDataFrame& d);
    // 设置使用缓存模式，缓存模式不会频繁调用dataframe，在setdataframe时把常用的参数缓存
    void setUseCacheMode(bool on = true);
	/// @group 滑动窗模式
	/// @{
	// 设置滑动窗模式的起始行
    virtual void setCacheWindowStartRow(int startRow) override;
	/// @}

	// 刷新
    void refreshData();
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
Q_SIGNALS:
	void currentPageChanged(int newPage);
};
}  // end of namespace DA
#endif  // DAPYDATAFRAMETABLEMODEL_H
