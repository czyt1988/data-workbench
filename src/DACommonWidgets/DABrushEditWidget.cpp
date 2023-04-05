#include "DABrushEditWidget.h"
#include "ui_DABrushEditWidget.h"
namespace DA
{
DABrushEditWidget::DABrushEditWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DABrushEditWidget)
{
    ui->setupUi(this);
    connect(ui->pushButtonColor, &DAColorPickerButton::colorChanged, this, &DABrushEditWidget::onColorChanged);
    connect(ui->comboBox, &DABrushStyleComboBox::currentBrushStyleChanged, this, &DABrushEditWidget::onBrushStyleChanged);
}

DABrushEditWidget::~DABrushEditWidget()
{
    delete ui;
}

QBrush DABrushEditWidget::getCurrentBrush() const
{
    return _brush;
}

void DABrushEditWidget::setCurrentBrush(const QBrush& b)
{
    QSignalBlocker b1(ui->pushButtonColor), b2(ui->comboBox);
    _brush = b;
    ui->pushButtonColor->setColor(b.color());
    ui->comboBox->setCurrentBrushStyle(b.style());
    emit brushChanged(b);
}

void DABrushEditWidget::onColorChanged(const QColor& c)
{
    _brush.setColor(c);
    emit brushChanged(_brush);
}

void DABrushEditWidget::onBrushStyleChanged(Qt::BrushStyle s)
{
    _brush.setStyle(s);
    emit brushChanged(_brush);
}
}
