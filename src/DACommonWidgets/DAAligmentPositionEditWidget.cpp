#include "DAAligmentPositionEditWidget.h"
#include "ui_DAAligmentPositionEditWidget.h"
namespace DA
{
DAAligmentPositionEditWidget::DAAligmentPositionEditWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::DAAligmentPositionEditWidget)
{
	ui->setupUi(this);
	ui->buttonGroup->setId(ui->toolButtonTopLeft, static_cast< int >(Qt::AlignLeft | Qt::AlignTop));
	ui->buttonGroup->setId(ui->toolButtonTop, static_cast< int >(Qt::AlignTop | Qt::AlignHCenter));
	ui->buttonGroup->setId(ui->toolButtonTopRight, static_cast< int >(Qt::AlignTop | Qt::AlignRight));
	ui->buttonGroup->setId(ui->toolButtonLeft, static_cast< int >(Qt::AlignLeft | Qt::AlignVCenter));
	ui->buttonGroup->setId(ui->toolButtonCenter, static_cast< int >(Qt::AlignCenter));
	ui->buttonGroup->setId(ui->toolButtonRight, static_cast< int >(Qt::AlignRight | Qt::AlignVCenter));
	ui->buttonGroup->setId(ui->toolButtonBottomLeft, static_cast< int >(Qt::AlignLeft | Qt::AlignBottom));
	ui->buttonGroup->setId(ui->toolButtonBottom, static_cast< int >(Qt::AlignBottom | Qt::AlignHCenter));
	ui->buttonGroup->setId(ui->toolButtonBottomRight, static_cast< int >(Qt::AlignBottom | Qt::AlignRight));
}

DAAligmentPositionEditWidget::~DAAligmentPositionEditWidget()
{
	delete ui;
}

void DAAligmentPositionEditWidget::setAligmentPosition(Qt::Alignment al)
{
	if (al == getAligmentPosition()) {
		return;
	}
	Qt::Alignment realal;
	if (al.testFlag(Qt::AlignLeft) && al.testFlag(Qt::AlignTop)) {
		realal = Qt::AlignLeft | Qt::AlignTop;
	} else if (al.testFlag(Qt::AlignTop) && al.testFlag(Qt::AlignHCenter)) {
		realal = Qt::AlignTop | Qt::AlignHCenter;
	} else if (al.testFlag(Qt::AlignTop) && al.testFlag(Qt::AlignRight)) {
		realal = Qt::AlignTop | Qt::AlignRight;
	} else if (al.testFlag(Qt::AlignLeft) && al.testFlag(Qt::AlignVCenter)) {
		realal = Qt::AlignLeft | Qt::AlignVCenter;
	} else if (al.testFlag(Qt::AlignCenter)) {
		realal = Qt::AlignCenter;
	} else if (al.testFlag(Qt::AlignRight) && al.testFlag(Qt::AlignVCenter)) {
		realal = Qt::AlignRight | Qt::AlignVCenter;
	} else if (al.testFlag(Qt::AlignLeft) && al.testFlag(Qt::AlignBottom)) {
		realal = Qt::AlignLeft | Qt::AlignBottom;
	} else if (al.testFlag(Qt::AlignBottom) && al.testFlag(Qt::AlignHCenter)) {
		realal = Qt::AlignBottom | Qt::AlignHCenter;
	} else if (al.testFlag(Qt::AlignRight) && al.testFlag(Qt::AlignBottom)) {
		realal = Qt::AlignRight | Qt::AlignBottom;
	}
	auto button = ui->buttonGroup->button(static_cast< int >(realal));
	if (button) {
		if (!button->isChecked()) {
			button->setChecked(true);
			emit aligmentPositionChanged(realal);
		}
	}
}

Qt::Alignment DAAligmentPositionEditWidget::getAligmentPosition() const
{
	return static_cast< Qt::Alignment >(ui->buttonGroup->checkedId());
}

void DAAligmentPositionEditWidget::changeEvent(QEvent* e)
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
}
