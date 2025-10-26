#include "DAFigureSettingWidget.h"
#include "ui_DAFigureSettingWidget.h"

DAFigureSettingWidget::DAFigureSettingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DAFigureSettingWidget)
{
	ui->setupUi(this);
}

DAFigureSettingWidget::~DAFigureSettingWidget()
{
	delete ui;
}

void DAFigureSettingWidget::changeEvent(QEvent *e)
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
