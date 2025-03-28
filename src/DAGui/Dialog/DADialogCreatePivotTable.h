#ifndef DADIALOGCREATEPIVOTTABLE_H
#define DADIALOGCREATEPIVOTTABLE_H

#include <QDialog>
#include "DAGuiAPI.h"
#include "pandas/DAPyDataFrame.h"
#include "DADataManager.h"

namespace Ui
{
class DADialogCreatePivotTable;
}

namespace DA
{
/**
 * @brief PivotTable参数设置
 */
class DAGUI_API DADialogCreatePivotTable : public QDialog
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DADialogCreatePivotTable)

public:
	explicit DADialogCreatePivotTable(QWidget* parent = nullptr);
	~DADialogCreatePivotTable();

	// 获取选中的dataframe
	DAPyDataFrame getDataFrame() const;
	void setDataframe(const DAPyDataFrame& d);
	// value参数
	QStringList getPivotTableValue() const;

	// index参数
	QStringList getPivotTableIndex() const;

	// column参数
	QStringList getPivotTableColumn() const;

	// Aggfunc参数
	QString getPivotTableAggfunc() const;

	// Margins参数
	bool isEnableMarginsName() const;
	void setEnableMargins(bool on);

	QString getMarginsName() const;
	void setMarginsName(QString& s);

	// Sort参数
	bool isEnableSort() const;
	void setEnableSort(bool on);

private:
	// Aggfunc参数
	void initPivotTableAggfunc();

private slots:
	void onTableItemClicked(const QModelIndex& index);

private:
	Ui::DADialogCreatePivotTable* ui;
};
}
#endif  // DADIALOGCREATEPIVOTTABLE_H
