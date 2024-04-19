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

/**
 * @brief 设置对其
 * @param al
 */
void DAAligmentEditWidget::setCurrentAlignment(Qt::Alignment al)
{
    if (al.testFlag(Qt::AlignLeft)) {
        ui->toolButtonAligmentLeft->setChecked(true);
    } else if (al.testFlag(Qt::AlignRight)) {
        ui->toolButtonAligmentRight->setChecked(true);
    } else if (al.testFlag(Qt::AlignTop)) {
        ui->toolButtonAligmentTop->setChecked(true);
    } else if (al.testFlag(Qt::AlignBottom)) {
        ui->toolButtonAligmentBottom->setChecked(true);
    } else if (al.testFlag(Qt::AlignCenter) || al.testFlag(Qt::AlignHCenter) || al.testFlag(Qt::AlignVCenter)) {
        ui->toolButtonAligmentCenter->setChecked(true);
    }
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
