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
	auto cur = getCurrentAlignment();
	if (cur == al) {
		return;
	}
	if (al.testFlag(Qt::AlignLeft)) {
		ui->toolButtonAligmentLeft->setChecked(true);
	} else if (al.testFlag(Qt::AlignRight)) {
		ui->toolButtonAligmentRight->setChecked(true);
	} else if (al.testFlag(Qt::AlignTop)) {
		ui->toolButtonAligmentTop->setChecked(true);
	} else if (al.testFlag(Qt::AlignBottom)) {
		ui->toolButtonAligmentBottom->setChecked(true);
	} else if (al.testFlag(Qt::AlignCenter) || al.testFlag(Qt::AlignHCenter) || al.testFlag(Qt::AlignVCenter)) {
		if (cur.testFlag(Qt::AlignCenter) || cur.testFlag(Qt::AlignHCenter) || cur.testFlag(Qt::AlignVCenter)) {
			// 这个相当于cur == al
			return;
		}
		ui->toolButtonAligmentCenter->setChecked(true);
	}
	emit alignmentChanged(al);
}

/**
 * @brief DAAligmentEditWidget::getCurrentAlignment
 * @return
 */
Qt::Alignment DAAligmentEditWidget::getCurrentAlignment() const
{
	if (ui->toolButtonAligmentLeft->isChecked()) {
		return Qt::AlignLeft;
	} else if (ui->toolButtonAligmentRight->isChecked()) {
		return Qt::AlignRight;
	} else if (ui->toolButtonAligmentTop->isChecked()) {
		return Qt::AlignTop;
	} else if (ui->toolButtonAligmentBottom->isChecked()) {
		return Qt::AlignBottom;
	} else if (ui->toolButtonAligmentCenter->isChecked()) {
		return Qt::AlignCenter;
	}
	return Qt::AlignCenter;
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
