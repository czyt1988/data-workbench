#include "DAAligmentEditWidget.h"
#include "ui_DAAligmentEditWidget.h"
namespace DA
{

DAAligmentEditWidget::DAAligmentEditWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DAAligmentEditWidget)
{
	ui->setupUi(this);

	connect(ui->buttonGroup, QOverload< int >::of(&QButtonGroup::buttonClicked), this, &DAAligmentEditWidget::onButtonGroupClicked);
	ui->buttonGroup->setId(ui->toolButtonAligmentBottom, static_cast< int >(Qt::AlignBottom));
	ui->buttonGroup->setId(ui->toolButtonAligmentTop, static_cast< int >(Qt::AlignTop));
	ui->buttonGroup->setId(ui->toolButtonAligmentLeft, static_cast< int >(Qt::AlignLeft));
	ui->buttonGroup->setId(ui->toolButtonAligmentRight, static_cast< int >(Qt::AlignRight));
	ui->buttonGroup->setId(ui->toolButtonAligmentCenter, static_cast< int >(Qt::AlignCenter));
}

DAAligmentEditWidget::~DAAligmentEditWidget()
{
	delete ui;
}

void DAAligmentEditWidget::setCurrentAlignment(Qt::Alignment al)
{
}

void DAAligmentEditWidget::changeEvent(QEvent* e)
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

void DAAligmentEditWidget::onButtonGroupClicked(int id)
{
	Qt::Alignment al = static_cast< Qt::Alignment >(id);
	emit alignmentChanged(al);
}
}  // end da
