#ifndef DADIALOGDATAFRAMECLIPOUTLIER_H
#define DADIALOGDATAFRAMECLIPOUTLIER_H

#include <QDialog>
#include "DAGuiAPI.h"
#include "pandas/DAPyDataFrame.h"

namespace Ui
{
class DADialogDataFrameClipOutlier;
}

namespace DA
{
/**
 * @brief clipoutlier参数设置
 */
class DAGUI_API DADialogDataFrameClipOutlier : public QDialog
{
    Q_OBJECT

public:
	explicit DADialogDataFrameClipOutlier(QWidget* parent = nullptr);
	~DADialogDataFrameClipOutlier();

	// 初始化界面
	void initDADialogDataFrameClipOutlier();

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
	Ui::DADialogDataFrameClipOutlier* ui;
};
}

#endif  // DADIALOGDATAFRAMECLIPOUTLIER_H
