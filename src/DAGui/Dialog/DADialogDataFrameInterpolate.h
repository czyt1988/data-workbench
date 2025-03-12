#ifndef DADIALOGDATAFRAMEINTERPOLATE_H
#define DADIALOGDATAFRAMEINTERPOLATE_H

#include <QDialog>
#include "DAGuiAPI.h"

namespace Ui
{
class DADialogDataFrameInterpolate;
}

namespace DA
{
/**
 * @brief interpolate参数设置
 */
class DAGUI_API DADialogDataFrameInterpolate : public QDialog
{
	Q_OBJECT

public:
	explicit DADialogDataFrameInterpolate(QWidget* parent = nullptr);
	~DADialogDataFrameInterpolate();

	//初始化界面
	void initDialogDataFrameInterpolate();
	// method参数
	QString getInterpolateMethod() const;

	// order参数
	bool isEnableInterpolateOrder(double d);
	void setEnableInterpolateOrder();

	void setInterpolateOrder(double d);
	double getInterpolateOrder() const;
	// limit参数
	bool isEnableLimitCount() const;
	void setEnableLimit(bool on);

	int getLimitCount() const;
	void setLimitCount(int d);

private:
	Ui::DADialogDataFrameInterpolate* ui;
};
}
#endif  // DADIALOGDATAFRAMEINTERPOLATE_H
