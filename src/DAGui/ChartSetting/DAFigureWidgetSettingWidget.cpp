#include "DAFigureWidgetSettingWidget.h"
#include "ui_DAFigureWidgetSettingWidget.h"

DAFigureWidgetSettingWidget::DAFigureWidgetSettingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DAFigureWidgetSettingWidget)
{
	ui->setupUi(this);
}

DAFigureWidgetSettingWidget::~DAFigureWidgetSettingWidget()
{
	delete ui;
}

void DAFigureWidgetSettingWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}
