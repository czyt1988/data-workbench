#include "DAGraphicsPixmapItemSettingWidget.h"
#include "ui_DAGraphicsPixmapItemSettingWidget.h"
#include <QSignalBlocker>
namespace DA
{
DAGraphicsPixmapItemSettingWidget::DAGraphicsPixmapItemSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAGraphicsPixmapItemSettingWidget)
{
    ui->setupUi(this);
    connect(ui->horizontalSliderAlpha, &QSlider::valueChanged, this, &DAGraphicsPixmapItemSettingWidget::onHorizontalSliderAlphaValueChanged);
    connect(ui->spinBoxAlpha, QOverload< int >::of(&QSpinBox::valueChanged), this, &DAGraphicsPixmapItemSettingWidget::onSpinBoxValueChanged);
}

DAGraphicsPixmapItemSettingWidget::~DAGraphicsPixmapItemSettingWidget()
{
    delete ui;
}

void DAGraphicsPixmapItemSettingWidget::onHorizontalSliderAlphaValueChanged(int v)
{
    QSignalBlocker b(ui->spinBoxAlpha);
    ui->spinBoxAlpha->setValue(v);
    emit pixmapAlphaValueChanged(v);
}

void DAGraphicsPixmapItemSettingWidget::onSpinBoxValueChanged(int v)
{
    QSignalBlocker b(ui->horizontalSliderAlpha);
    ui->horizontalSliderAlpha->setValue(v);
    emit pixmapAlphaValueChanged(v);
}

}
