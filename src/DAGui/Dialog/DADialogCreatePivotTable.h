#ifndef DADIALOGCREATEPIVOTTABLE_H
#define DADIALOGCREATEPIVOTTABLE_H

#include <QDialog>
#include "DAGuiAPI.h"

namespace Ui
{
class DADialogCreatePivotTable;
}

namespace DA
{
class DADataOperateOfDataFrameWidget;
class DAAppController;
/**
 * @brief PivotTable参数设置
 */
class DAGUI_API DADialogCreatePivotTable : public QDialog
{
    Q_OBJECT

public:
	explicit DADialogCreatePivotTable(QWidget* parent = nullptr);
	~DADialogCreatePivotTable();

	// 初始化界面
	void initCreatePivotTable();

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
	Ui::DADialogCreatePivotTable* ui;
	DAAppController* mController { nullptr };
};
}
#endif  // DADIALOGCREATEPIVOTTABLE_H
