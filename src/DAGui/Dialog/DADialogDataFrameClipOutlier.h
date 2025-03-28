#ifndef DADIALOGDATAFRAMECLIPOUTLIER_H
#define DADIALOGDATAFRAMECLIPOUTLIER_H

#include <QDialog>
#include "DAGuiAPI.h"

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

	// lower参数
	void setLowerValue(double d);
	double getLowerValue() const;

	// upper参数
	void setUpperValue(double d);
	double getUpperValue() const;

private:
	Ui::DADialogDataFrameClipOutlier* ui;
};
}

#endif  // DADIALOGDATAFRAMECLIPOUTLIER_H
