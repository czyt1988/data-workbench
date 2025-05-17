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
 * TODO:Bug 这个类如果删除最后一列或行会出现问题ASSERT: "last < columnCount(parent)" in file itemmodels\qabstractitemmodel.cpp, line 3092
 */
class DAGUI_API DAPyDataFrameTableModel : public QAbstractTableModel
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAPyDataFrameTableModel)
public:
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
	void setDAData(const DAData& d);
	void setDataFrame(const DAPyDataFrame& d);
	// 刷新
	void refreshRow(int row);
	void refreshColumn(int col);
	void refresh(int row, int col);
	void refresh(int rowStart, int colStart, int rowEnd, int colEnd);
	void refresh();
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
	void rowsBeginRemove(const QList< int >& r);
	void rowsBeginInsert(const QList< int >& r);
	void columnBeginRemove(const QList< int >& r);
	void columnsBeginInsert(const QList< int >& r);
	void beginFunCall(const QList< int >& listlike, beginFun fun);
};
}  // end of namespace DA
#endif  // DAPYDATAFRAMETABLEMODEL_H
