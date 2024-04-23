#include "DABrushEditWidget.h"
#include "ui_DABrushEditWidget.h"
namespace DA
{
DABrushEditWidget::DABrushEditWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DABrushEditWidget)
{
	ui->setupUi(this);
	ui->colorButton->setColor(QColor());
	connect(ui->colorButton, &DAColorPickerButton::colorChanged, this, &DABrushEditWidget::onColorChanged);
	connect(ui->comboBox, &DABrushStyleComboBox::currentBrushStyleChanged, this, &DABrushEditWidget::onBrushStyleChanged);
}

DABrushEditWidget::~DABrushEditWidget()
{
	delete ui;
}

QBrush DABrushEditWidget::getCurrentBrush() const
{
	return mBrush;
}

void DABrushEditWidget::setStyleTextVisible(bool on)
{
	ui->comboBox->setStyleTextVisible(on);
}

bool DABrushEditWidget::isStyleTextVisible() const
{
    return ui->comboBox->isStyleTextVisible();
}

/**
 * @brief 获取当前的画刷类型
 * @return
 */
Qt::BrushStyle DABrushEditWidget::getCurrentBrushStyle() const
{
    return ui->comboBox->getCurrentBrushStyle();
}

/**
 * @brief 获取当前画刷的颜色
 * @return
 */
QColor DABrushEditWidget::getCurrentBrushColor() const
{
    return ui->colorButton->color();
}

void DABrushEditWidget::setCurrentBrush(const QBrush& b)
{
	QSignalBlocker b1(ui->colorButton), b2(ui->comboBox);
	mBrush = b;
	ui->colorButton->setColor(b.color());
	ui->comboBox->setCurrentBrushStyle(b.style());
	//    ui->colorButton->setEnabled(b.style() != Qt::NoBrush);
	emit brushChanged(b);
}

/**
   @brief 设置当前画刷样式
   @note 此函数会发射信号@ref brushChanged
   @param s
 */
void DABrushEditWidget::setCurrentBrushStyle(Qt::BrushStyle s)
{
    ui->comboBox->setCurrentBrushStyle(s);
}

void DABrushEditWidget::onColorChanged(const QColor& c)
{
	Qt::BrushStyle s = getCurrentBrushStyle();
	if (c.isValid()) {
		// 如果颜色有效，而当前的画刷类型是无，则自动设置为solid
		if (s == Qt::NoBrush) {
			QSignalBlocker b1(ui->comboBox);
			ui->comboBox->setCurrentBrushStyle(Qt::SolidPattern);
			mBrush.setStyle(Qt::SolidPattern);
		}
	} else {
		// 如果颜色无效，那么画刷也设置为无效
		if (s != Qt::NoBrush) {
			QSignalBlocker b1(ui->comboBox);
			ui->comboBox->setCurrentBrushStyle(Qt::NoBrush);
			mBrush.setStyle(Qt::NoBrush);
		}
	}
	mBrush.setColor(c);

	emit brushChanged(mBrush);
}

void DABrushEditWidget::onBrushStyleChanged(Qt::BrushStyle s)
{
	if (getCurrentBrushStyle() == s) {
		return;
	}
	mBrush.setStyle(s);
	//    ui->colorButton->setEnabled(s != Qt::NoBrush);
	emit brushChanged(mBrush);
}
}
