#include "DAChartSettingWidget.h"
#include "ui_DAChartSettingWidget.h"
namespace DA
{
DAChartSettingWidget::DAChartSettingWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DAChartSettingWidget)
{
    ui->setupUi(this);
}

DAChartSettingWidget::~DAChartSettingWidget()
{
    delete ui;
}

void DAChartSettingWidget::setChartWidget(DAChartWidget* w)
{
}
}  // end da
