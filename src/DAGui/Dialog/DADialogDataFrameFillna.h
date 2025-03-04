#ifndef DADIALOGDATAFRAMEFILLNA_H
#define DADIALOGDATAFRAMEFILLNA_H

#include <QDialog>

namespace Ui
{
class DADialogDataFrameFillna;
}

class DADialogDataFrameFillna : public QDialog
{
	Q_OBJECT

public:
	explicit DADialogDataFrameFillna(QWidget* parent = nullptr);
	~DADialogDataFrameFillna();

	void initCheckBoxGroup();

	int getCheckBoxStatus();

	float getFilledValue();

	QString getFillMethod(int filltype);

private:
	Ui::DADialogDataFrameFillna* ui;
};

#endif  // DADIALOGDATAFRAMEFILLNA_H
