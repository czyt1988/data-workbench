#include "DAChartCanvasSettingWidget.h"
#include "ui_DAChartCanvasSettingWidget.h"
namespace DA
{
DAChartCanvasSettingWidget::DAChartCanvasSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartCanvasSettingWidget)
{
    ui->setupUi(this);
}

DAChartCanvasSettingWidget::~DAChartCanvasSettingWidget()
{
    delete ui;
}

void DAChartCanvasSettingWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void DAChartCanvasSettingWidget::setPlot(QwtPlot* plot)
{
    mPlot = plot;
}

QwtPlot* DAChartCanvasSettingWidget::getPlot() const
{
    return mPlot.data();
}

}
