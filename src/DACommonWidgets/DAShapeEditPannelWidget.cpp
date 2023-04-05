#include "DAShapeEditPannelWidget.h"
#include "ui_DAShapeEditPannelWidget.h"
namespace DA
{

DAShapeEditPannelWidget::DAShapeEditPannelWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DAShapeEditPannelWidget)
{
    ui->setupUi(this);
    connect(ui->widgetPenEdit, &DAPenEditWidget::penChanged, this, &DAShapeEditPannelWidget::borderPenChanged);
    connect(ui->widgetBrushEdit, &DABrushEditWidget::brushChanged, this, &DAShapeEditPannelWidget::backgroundBrushChanged);
}

DAShapeEditPannelWidget::~DAShapeEditPannelWidget()
{
    delete ui;
}

/**
 * @brief 获取背景画刷
 * @return
 */
QBrush DAShapeEditPannelWidget::getBackgroundBrush() const
{
    return ui->widgetBrushEdit->getCurrentBrush();
}

/**
 * @brief 获取边框画笔
 * @return
 */
QPen DAShapeEditPannelWidget::getBorderPen() const
{
    return ui->widgetPenEdit->getCurrentPen();
}

/**
 * @brief 设置背景画刷
 * @param b
 * 会发射backgroundColorChanged信号
 */
void DAShapeEditPannelWidget::setBackgroundBrush(const QBrush& b)
{
    //此函数会触发DABrushEditWidget::brushChanged信号，从而触发onBrushChanged
    ui->widgetBrushEdit->setCurrentBrush(b);
}

/**
 * @brief 设置边框画笔
 * @param v
 */
void DAShapeEditPannelWidget::setBorderPen(const QPen& v)
{
    ui->widgetPenEdit->setCurrentPen(v);
}

}  // end of da
