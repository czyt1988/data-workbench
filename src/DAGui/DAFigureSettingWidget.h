#ifndef DAFIGURESETTINGWIDGET_H
#define DAFIGURESETTINGWIDGET_H

#include <QWidget>

namespace Ui {
class DAFigureSettingWidget;
}

class DAFigureSettingWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DAFigureSettingWidget(QWidget *parent = nullptr);
	~DAFigureSettingWidget();

protected:
	void changeEvent(QEvent *e);

private:
	Ui::DAFigureSettingWidget *ui;
};

#endif // DAFIGURESETTINGWIDGET_H
