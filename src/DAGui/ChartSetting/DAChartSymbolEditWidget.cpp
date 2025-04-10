﻿#include "DAChartSymbolEditWidget.h"
#include "ui_DAChartSymbolEditWidget.h"
#include <QSpinBox>
namespace DA
{
DAChartSymbolEditWidget::DAChartSymbolEditWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DAChartSymbolEditWidget)
{
	ui->setupUi(this);
	init();
}

DAChartSymbolEditWidget::~DAChartSymbolEditWidget()
{
    delete ui;
}

/**
 * @brief 设置Symbol Style
 * @param v
 */
void DAChartSymbolEditWidget::setSymbolStyle(QwtSymbol::Style v)
{
    ui->comboBoxSymbolStyle->setSymbolStyle(v);
}

/**
 * @brief 获取Symbol Style
 * @return
 */
QwtSymbol::Style DAChartSymbolEditWidget::getSymbolStyle() const
{
    return ui->comboBoxSymbolStyle->getSymbolStyle();
}

/**
 * @brief 符号大小
 * @param v
 */
void DAChartSymbolEditWidget::setSymbolSize(int v)
{
    ui->spinBoxSize->setValue(v);
}

/**
 * @brief 符号大小
 * @return
 */
int DAChartSymbolEditWidget::getSymbolSize() const
{
    return ui->spinBoxSize->value();
}

/**
 * @brief 设置符号颜色
 * @param v
 */
void DAChartSymbolEditWidget::setSymbolColor(const QColor& v)
{
    ui->pushButtonColor->setColor(v);
}

/**
 * @brief 符号颜色
 * @return
 */
QColor DAChartSymbolEditWidget::getSymbolColor() const
{
    return ui->pushButtonColor->color();
}

/**
 * @brief Outline Pen
 * @param v
 */
void DAChartSymbolEditWidget::setSymbolOutlinePen(const QPen& v)
{
    ui->penEditWidget->setCurrentPen(v);
}

/**
 * @brief Outline Pen
 * @return
 */
QPen DAChartSymbolEditWidget::getSymbolOutlinePen() const
{
    return ui->penEditWidget->getCurrentPen();
}

/**
 * @brief 根据ui，创建一个symbol
 * @note 注意，这是个工程函数，创建完成后如果不使用，调用者要对symbol进行释放
 * @return
 */
QwtSymbol* DAChartSymbolEditWidget::createSymbol() const
{
	int size     = getSymbolSize();
	QwtSymbol* s = new QwtSymbol(getSymbolStyle());
	s->setSize(QSize(size, size));
	s->setBrush(getSymbolColor());
	s->setPen(getSymbolOutlinePen());
	return s;
}

QwtIntervalSymbol* DAChartSymbolEditWidget::createIntervalSymbol() const
{
	// 1. 创建基础 QwtIntervalSymbol 对象
	QwtIntervalSymbol* intervalSymbol = new QwtIntervalSymbol();

	// 2. 从原函数获取样式参数（复用逻辑）
	QPen outlinePen  = getSymbolOutlinePen();  // 获取原符号边框笔
	QBrush fillBrush = getSymbolColor();       // 获取原符号填充画刷
	int size         = getSymbolSize();        // 获取原符号大小

	// 3. 设置区间符号样式
	intervalSymbol->setPen(outlinePen);   // 设置线条样式（颜色/宽度）
	intervalSymbol->setBrush(fillBrush);  // 设置填充（仅对某些样式有效）

	// 4. 选择区间符号类型（需根据需求调整）
	intervalSymbol->setStyle(QwtIntervalSymbol::Bar);  // 可选：Bar, Box, NoSymbol等

	// 5. 调整大小（QwtIntervalSymbol 的 size 影响标记的视觉宽度）
	intervalSymbol->setWidth(size);  // 设置线条/标记的宽度

	return intervalSymbol;
}

void DAChartSymbolEditWidget::init()
{
	ui->penEditWidget->setCurrentPen(Qt::NoPen);
	connect(ui->comboBoxSymbolStyle,
			&DAChartSymbolComboBox::symbolStyleChanged,
			this,
			&DAChartSymbolEditWidget::symbolStyleChanged);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	connect(ui->spinBoxSize, QOverload< int >::of(&QSpinBox::valueChanged), this, &DAChartSymbolEditWidget::symbolSizeChanged);
#else
	connect(ui->spinBoxSize, &QSpinBox::valueChanged, this, &DAChartSymbolEditWidget::symbolSizeChanged);
#endif
	connect(ui->pushButtonColor, &DAColorPickerButton::colorChanged, this, &DAChartSymbolEditWidget::symbolColorChanged);
	connect(ui->penEditWidget, &DAPenEditWidget::penChanged, this, &DAChartSymbolEditWidget::symbolOutlinePenChanged);
}

}  // end DA
