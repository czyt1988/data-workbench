#ifndef DADIALOGDATAFRAMEFILLNA_H
#define DADIALOGDATAFRAMEFILLNA_H

#include <QDialog>
#include "DAGuiAPI.h"
namespace Ui
{
class DADialogDataFrameFillna;
}
namespace DA
{

/**
 * @brief fillna参数设置
 */
class DAGUI_API DADialogDataFrameFillna : public QDialog
{
	Q_OBJECT

public:
	explicit DADialogDataFrameFillna(QWidget* parent = nullptr);
	~DADialogDataFrameFillna();
	// value参数
	void setFillNanValue(double d);
	double getFillNanValue() const;
	// limit参数
	bool isEnableLimitCount() const;
	void setEnableLimit(bool on);

	int getLimitCount() const;
	void setLimitCount(int d);

private:
	Ui::DADialogDataFrameFillna* ui;
};
}

#endif  // DADIALOGDATAFRAMEFILLNA_H
