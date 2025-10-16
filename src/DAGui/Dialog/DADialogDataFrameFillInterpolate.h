#ifndef DADIALOGDATAFRAMEFILLINTERPOLATE_H
#define DADIALOGDATAFRAMEFILLINTERPOLATE_H

#include <QDialog>
#include "DAGuiAPI.h"

namespace Ui
{
class DADialogDataFrameFillInterpolate;
}

namespace DA
{
/**
 * @brief interpolate参数设置
 */
class DAGUI_API DADialogDataFrameFillInterpolate : public QDialog
{
	Q_OBJECT

public:
	explicit DADialogDataFrameFillInterpolate(QWidget* parent = nullptr);
	~DADialogDataFrameFillInterpolate();

	// 初始化界面
	void initDialogDataFrameInterpolate();
	// method参数
	QString getInterpolateMethod() const;

	// order参数
	bool isEnableInterpolateOrder(double d);
	void setEnableInterpolateOrder();

	void setInterpolateOrder(double d);
	double getInterpolateOrder() const;

	// axis参数
	bool getInterPolateAxis() const;

	// limit参数
	bool isEnableLimitCount() const;
	void setEnableLimit(bool on);

	int getLimitCount() const;
	void setLimitCount(int d);
private Q_SLOTS:
    void onComboboxCurrentIndexChanged(int index);

private:
	Ui::DADialogDataFrameFillInterpolate* ui;
};
}
#endif  // DADIALOGDATAFRAMEFILLINTERPOLATE_H
