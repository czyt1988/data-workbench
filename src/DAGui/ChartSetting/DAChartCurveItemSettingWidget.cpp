#include "DAChartCurveItemSettingWidget.h"
#include "ui_DAChartCurveItemSettingWidget.h"
namespace DA
{
DAChartCurveItemSettingWidget::DAChartCurveItemSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartCurveItemSettingWidget)
{
    ui->setupUi(this);
}

DAChartCurveItemSettingWidget::~DAChartCurveItemSettingWidget()
{
    delete ui;
}

void DAChartCurveItemSettingWidget::setTitle(const QString& t)
{
    ui->lineEditTitle->setText(t);
}

QString DAChartCurveItemSettingWidget::getTitle() const
{
    return ui->lineEditTitle->text();
}
}
