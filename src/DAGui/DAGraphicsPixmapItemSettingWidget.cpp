#include "DAGraphicsPixmapItemSettingWidget.h"
#include "ui_DAGraphicsPixmapItemSettingWidget.h"
#include <QSignalBlocker>
#include <QPointer>
#include "DAGraphicsPixmapItem.h"
namespace DA
{
class DAGraphicsPixmapItemSettingWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAGraphicsPixmapItemSettingWidget)
public:
    PrivateData(DAGraphicsPixmapItemSettingWidget* p);

public:
    QPointer< DAGraphicsPixmapItem > mItem;
};

DAGraphicsPixmapItemSettingWidget::PrivateData::PrivateData(DAGraphicsPixmapItemSettingWidget* p)
{
}

//----------------------------------------------------
// DAGraphicsPixmapItemSettingWidget
//----------------------------------------------------

DAGraphicsPixmapItemSettingWidget::DAGraphicsPixmapItemSettingWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAGraphicsPixmapItemSettingWidget)
{
    ui->setupUi(this);
    connect(ui->horizontalSliderAlpha,
            &QSlider::valueChanged,
            this,
            &DAGraphicsPixmapItemSettingWidget::onHorizontalSliderAlphaValueChanged);
    connect(ui->spinBoxAlpha,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAGraphicsPixmapItemSettingWidget::onSpinBoxValueChanged);
}

DAGraphicsPixmapItemSettingWidget::~DAGraphicsPixmapItemSettingWidget()
{
    delete ui;
}

void DAGraphicsPixmapItemSettingWidget::setItem(DAGraphicsPixmapItem* item)
{
    d_ptr->mItem = item;
    updateData();
}

DAGraphicsPixmapItem* DAGraphicsPixmapItemSettingWidget::getItem() const
{
    return d_ptr->mItem;
}

void DAGraphicsPixmapItemSettingWidget::updateData()
{
    if (nullptr == d_ptr->mItem) {
        return;
    }
    QSignalBlocker b1(ui->spinBoxAlpha);
    QSignalBlocker b2(ui->horizontalSliderAlpha);
    Q_UNUSED(b1);
    Q_UNUSED(b2);
    auto alpha = d_ptr->mItem->getAlpha();
    ui->spinBoxAlpha->setValue(alpha);
    ui->horizontalSliderAlpha->setValue(alpha);
}

/**
 * @brief 设置当前透明度的值
 * @param v
 */
void DAGraphicsPixmapItemSettingWidget::setCurrentAlphaValue(int v)
{
    QSignalBlocker b1(ui->spinBoxAlpha);
    QSignalBlocker b2(ui->horizontalSliderAlpha);
    Q_UNUSED(b1);
    Q_UNUSED(b2);
    ui->spinBoxAlpha->setValue(v);
    ui->horizontalSliderAlpha->setValue(v);
    if (nullptr == d_ptr->mItem) {
        return;
    }
    d_ptr->mItem->setAlpha(v);
}

int DAGraphicsPixmapItemSettingWidget::getCurrentAlphaValue() const
{
    return ui->spinBoxAlpha->value();
}

void DAGraphicsPixmapItemSettingWidget::onHorizontalSliderAlphaValueChanged(int v)
{
    QSignalBlocker b(ui->spinBoxAlpha);
    ui->spinBoxAlpha->setValue(v);
    if (d_ptr->mItem) {
        d_ptr->mItem->setAlpha(v);
    }
    emit pixmapAlphaValueChanged(v);
}

void DAGraphicsPixmapItemSettingWidget::onSpinBoxValueChanged(int v)
{
    QSignalBlocker b(ui->horizontalSliderAlpha);
    ui->horizontalSliderAlpha->setValue(v);
    if (d_ptr->mItem) {
        d_ptr->mItem->setAlpha(v);
    }
    emit pixmapAlphaValueChanged(v);
}

}
