#ifndef DAPYDATAFRAMETABLEMODULE_H
#define DAPYDATAFRAMETABLEMODULE_H
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
class DAGUI_API DAPyDataFrameTableModule : public QAbstractTableModel
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAPyDataFrameTableModule)
public:
	DAPyDataFrameTableModule(QUndoStack* stack, QObject* parent = nullptr);
	~DAPyDataFrameTableModule();
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
	void rowBeginRemove(const QList< int >& r);
	void rowEndRemove();
	// 行将插入
	void rowBeginInsert(const QList< int >& r);
	void rowEndInsert();
	// 行将移除
	void columnBeginRemove(const QList< int >& r);
	void columnEndRemove();
	// 行将插入
	void columnBeginInsert(const QList< int >& r);
	void columnEndInsert();
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
	void beginFunCall(const QList< int >& listlike, beginFun fun);
};
}  // end of namespace DA
#endif  // DAPYDATAFRAMETABLEMODULE_H
