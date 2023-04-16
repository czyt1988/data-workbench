#include "DABrushEditWidget.h"
#include "ui_DABrushEditWidget.h"
#include "colorWidgets/SAColorMenu.h"
namespace DA
{
DABrushEditWidget::DABrushEditWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DABrushEditWidget)
{
    ui->setupUi(this);
    ui->colorButton->setPopupMode(QToolButton::MenuButtonPopup);
    mColorMenu = new SAColorMenu(this);
    mColorMenu->bindToColorToolButton(ui->colorButton);
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

void DABrushEditWidget::setCurrentBrush(const QBrush& b)
{
    QSignalBlocker b1(ui->colorButton), b2(ui->comboBox);
    mBrush = b;
    ui->colorButton->setColor(b.color());
    ui->comboBox->setCurrentBrushStyle(b.style());
    ui->colorButton->setEnabled(b.style() != Qt::NoBrush);
    emit brushChanged(b);
}

void DABrushEditWidget::onColorChanged(const QColor& c)
{
    mBrush.setColor(c);
    emit brushChanged(mBrush);
}

void DABrushEditWidget::onBrushStyleChanged(Qt::BrushStyle s)
{
    mBrush.setStyle(s);
    ui->colorButton->setEnabled(s != Qt::NoBrush);
    emit brushChanged(mBrush);
}
}
