#include "DAChartCurveItemSettingWidget.h"
#include "ui_DAChartCurveItemSettingWidget.h"

namespace DA
{

DAChartCurveItemSettingWidget::DAChartCurveItemSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartCurveItemSettingWidget)
{
    ui->setupUi(this);
    initUI();
}

DAChartCurveItemSettingWidget::~DAChartCurveItemSettingWidget()
{
    delete ui;
}
void DAChartCurveItemSettingWidget::initUI()
{
    ui->comboBoxCurveStyle->addItem(tr("Lines"), (int)QwtPlotCurve::Lines);
    ui->comboBoxCurveStyle->addItem(tr("Steps"), (int)QwtPlotCurve::Steps);
    ui->comboBoxCurveStyle->addItem(tr("Dots"), (int)QwtPlotCurve::Dots);
    connect(ui->comboBoxCurveStyle,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAChartCurveItemSettingWidget::onCurveStyleComboBoxCurrentIndexChanged);
}
void DAChartCurveItemSettingWidget::setChartCurveItem(QwtPlotCurve* item)
{
    _item = item;
}

QwtPlotCurve* DAChartCurveItemSettingWidget::getChartCurveItem() const
{
    return _item;
}

void DAChartCurveItemSettingWidget::onCurveStyleComboBoxCurrentIndexChanged(int i)
{
    QwtPlotCurve::CurveStyle st = static_cast< QwtPlotCurve::CurveStyle >(ui->comboBoxCurveStyle->itemData(i).toInt());
    emit curveStyleChanged(st);
}

}
