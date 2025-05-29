#ifndef DADialogDataFrameDataSelect_H
#define DADialogDataFrameDataSelect_H

#include <QDialog>
#include "DAGuiAPI.h"
#include "pandas/DAPyDataFrame.h"

namespace Ui
{
class DADialogDataFrameDataSelect;
}

namespace DA
{
/**
 * @brief dataselect参数设置
 */
class DAGUI_API DADialogDataFrameDataSelect : public QDialog
{
    Q_OBJECT

public:
	explicit DADialogDataFrameDataSelect(QWidget* parent = nullptr);
	~DADialogDataFrameDataSelect();

	// 初始化界面
	void initDADialogDataFrameDataSelect();

	// 获取选中的dataframe
	void setDataframe(const DAPyDataFrame& df);

	// Data设置
	void setFilterData(const int index);
	QString getFilterData() const;

	// lower参数
	void setLowerValue(const double d);
	double getLowerValue() const;

	// upper参数
	void setUpperValue(const double d);
	double getUpperValue() const;

private:
	Ui::DADialogDataFrameDataSelect* ui;
};
}

#endif  // DADialogDataFrameDataSelect_H
