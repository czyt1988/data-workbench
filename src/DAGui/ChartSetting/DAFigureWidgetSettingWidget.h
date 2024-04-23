#ifndef DAFIGUREWIDGETSETTINGWIDGET_H
#define DAFIGUREWIDGETSETTINGWIDGET_H

#include <QWidget>

namespace Ui {
class DAFigureWidgetSettingWidget;
}

class DAFigureWidgetSettingWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DAFigureWidgetSettingWidget(QWidget *parent = nullptr);
	~DAFigureWidgetSettingWidget();

protected:
	void changeEvent(QEvent *e);

private:
	Ui::DAFigureWidgetSettingWidget *ui;
};

#endif // DAFIGUREWIDGETSETTINGWIDGET_H
