#ifndef DAVARIANTTABLEMODEL_H
#define DAVARIANTTABLEMODEL_H
#include "DAGuiAPI.h"
#include "DATable.hpp"
#include <QAbstractTableModel>
#include <functional>
class QUndoStack;
namespace DA
{
/**
 * @brief 对DATable<QVariant>显示的的model
 *
 * 内置了redo/undo
 */
class DAGUI_API DAVariantTableModel : public QAbstractTableModel
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAVariantTableModel)
	friend class DAVariantTableModelSetDataCommand;

public:
	using FpToDisplayString = std::function< QString(const QVariant& v) >;

public:
	DAVariantTableModel(QObject* p = nullptr);
	DAVariantTableModel(DATable< QVariant >* d, QObject* p = nullptr);
	~DAVariantTableModel();
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	// 更新表格
	void update();
	// 设置是否可编辑
	void setEnableEdit(bool on = true);
	// 获取UndoStack
	QUndoStack* getUndoStack() const;
	// 设置table
	void setTable(DATable< QVariant >* t);
	DATable< QVariant >* getTable() const;
	// 清空表格
	void clearTable();
	// 注册显示函数，把QVariant转换为文本显示出来
	void registDisplayFun(FpToDisplayString fp);
	// 设置表头
	void setHeader(const QStringList& h);
public slots:
	void redo();
	void undo();

public:
	void setTableData(int row, int col, const QVariant& v);
	QVariant getTableData(int row, int col) const;
	void removeTableCell(int row, int col);
};
}

#endif  // DAVARIANTTABLEMODEL_H
