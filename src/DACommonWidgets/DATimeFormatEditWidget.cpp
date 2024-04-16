#include "DATimeFormatEditWidget.h"
#include "ui_DATimeFormatEditWidget.h"
#include <QComboBox>
namespace DA
{

DATimeFormatEditWidget::DATimeFormatEditWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::DATimeFormatEditWidget), m_currentDateTime(QDateTime::currentDateTime())
{
	ui->setupUi(this);
	setComboxTimeFormat(ui->comboBox);
	connect(ui->comboBox, &QComboBox::currentTextChanged, this, &DATimeFormatEditWidget::onCurrentTextChanged);
	ui->comboBox->setCurrentIndex(ui->comboBox->count() - 1);
}

DATimeFormatEditWidget::~DATimeFormatEditWidget()
{
	delete ui;
}

QString DATimeFormatEditWidget::getTimeFormat() const
{
	return ui->comboBox->currentText();
}

void DATimeFormatEditWidget::setTimeFormatText(const QString& str)
{
	ui->comboBox->setCurrentText(str);
}

void DATimeFormatEditWidget::onCurrentTextChanged(const QString& text)
{
	ui->labelExample->setText(m_currentDateTime.toString(text));
}

void DATimeFormatEditWidget::setComboxTimeFormat(QComboBox* comb)
{
	comb->clear();
	comb->addItem(QString("h:m"), QVariant(QString("h:m")));
	comb->setItemData(comb->count() - 1, tr("example: 8:5"), Qt::ToolTipRole);

	comb->addItem(QString("hh:mm"), QVariant(QString("hh:mm")));
	comb->setItemData(comb->count() - 1, tr("example: 08:05"), Qt::ToolTipRole);

	comb->addItem(QString("h:m:s"), QVariant(QString("h:m:s")));
	comb->setItemData(comb->count() - 1, tr("example: 8:5:12"), Qt::ToolTipRole);

	comb->addItem(QString("hh:mm:ss"), QVariant(QString("hh:mm:ss")));
	comb->setItemData(comb->count() - 1, tr("example: 08:05:12"), Qt::ToolTipRole);

	comb->addItem(QString("yyyy-M-d"), QVariant(QString("yyyy-M-d")));
	comb->setItemData(comb->count() - 1, tr("example: 2017-8-1"), Qt::ToolTipRole);

	comb->addItem(QString("yyyy-M-d h:m"), QVariant(QString("yyyy-M-d h:m")));
	comb->setItemData(comb->count() - 1, tr("example: 2017-8-1 8:5"), Qt::ToolTipRole);

	comb->addItem(QString("yyyy-M-d h:m:s"), QVariant(QString("yyyy-M-d h:m:s")));
	comb->setItemData(comb->count() - 1, tr("example: 2017-8-1 8:5:12"), Qt::ToolTipRole);

	comb->addItem(QString("yyyy-MM-dd"), QVariant(QString("yyyy-MM-dd")));
	comb->setItemData(comb->count() - 1, tr("example: 2017-08-01"), Qt::ToolTipRole);

	comb->addItem(QString("yyyy-MM-dd hh:mm"), QVariant(QString("yyyy-MM-dd hh:mm")));
	comb->setItemData(comb->count() - 1, tr("example: 2017-08-01 08:05"), Qt::ToolTipRole);

	comb->addItem(QString("yyyy-MM-dd hh:mm:ss"), QVariant(QString("yyyy-MM-dd hh:mm:ss")));
	comb->setItemData(comb->count() - 1, tr("example: 2017-08-01 08:05:12"), Qt::ToolTipRole);
}

}  // end DA
