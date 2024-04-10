#include "DAChartAddCurveWidget.h"
#include "ui_DAChartAddCurveWidget.h"
#include <QButtonGroup>
#include "qwt_plot_curve.h"
namespace DA
{

class DAChartAddCurveWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartAddCurveWidget)
public:
    PrivateData(DAChartAddCurveWidget* p);

public:
    QButtonGroup* mBtnGroup { nullptr };
};

DAChartAddCurveWidget::PrivateData::PrivateData(DAChartAddCurveWidget* p) : q_ptr(p)
{
    mBtnGroup = new QButtonGroup(p);
}

//----------------------------------------------------
// DAChartAddCurveWidget
//----------------------------------------------------

DAChartAddCurveWidget::DAChartAddCurveWidget(QWidget* parent)
    : DAAbstractChartAddItemWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartAddCurveWidget)
{
    ui->setupUi(this);
    init();
}

DAChartAddCurveWidget::~DAChartAddCurveWidget()
{
    delete ui;
}

void DAChartAddCurveWidget::init()
{
    d_ptr->mBtnGroup->addButton(ui->toolButtonStepData);
    d_ptr->mBtnGroup->addButton(ui->toolButtonStepPlot);
    d_ptr->mBtnGroup->setExclusive(true);
    connect(d_ptr->mBtnGroup,
            QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
            this,
            &DAChartAddCurveWidget::onNavButtonClicked);
}

/**
 * @brief 按照设定创建曲线
 * @return
 */
QwtPlotItem* DAChartAddCurveWidget::createPlotItem()
{
    QVector< QPointF > xy = ui->pageData->getSeries();
    if (xy.empty()) {
        return nullptr;
    }
    QwtPlotCurve* item = new QwtPlotCurve();
    item->setSamples(xy);
    ui->pagePlot->updatePlotItem(item);
    return item;
}

void DAChartAddCurveWidget::setCurrentData(const DAData& d)
{
    ui->pageData->setCurrentData(d);
}

void DAChartAddCurveWidget::setDataManager(DADataManager* dmgr)
{
    ui->pageData->setDataManager(dmgr);
}

void DAChartAddCurveWidget::setScatterMode(bool on)
{
    if (on) {
        ui->pagePlot->setCurveStyle(QwtPlotCurve::NoCurve);
    } else {
        ui->pagePlot->setCurveStyle(QwtPlotCurve::Lines);
    }
    ui->pagePlot->enableMarkerEdit(on);
}

void DAChartAddCurveWidget::next()
{
    auto i = ui->stackedWidget->currentIndex();
    auto c = ui->stackedWidget->count();
    if (i < c) {
        ++i;
        ui->stackedWidget->setCurrentIndex(i);
    }
    updateNavButtonState();
}

void DAChartAddCurveWidget::previous()
{
    auto i = ui->stackedWidget->currentIndex();
    --i;
    if (i >= 0) {
        ui->stackedWidget->setCurrentIndex(i);
    }
    updateNavButtonState();
}

/**
 * @brief 获取步骤总数
 * @return 如果为0或者1，下一步按钮将没有直接就是完成
 */
int DAChartAddCurveWidget::getStepCount() const
{
    return ui->stackedWidget->count();
}

int DAChartAddCurveWidget::getCurrentStep() const
{
    return ui->stackedWidget->currentIndex();
}

void DAChartAddCurveWidget::updateNavButtonState()
{
    auto i = ui->stackedWidget->currentIndex();
    QSignalBlocker b(d_ptr->mBtnGroup);
    if (0 == i) {
        ui->toolButtonStepData->setChecked(true);
    } else {
        ui->toolButtonStepPlot->setChecked(true);
    }
}

void DAChartAddCurveWidget::onNavButtonClicked(QAbstractButton* button)
{
    if (button == ui->toolButtonStepData) {
        ui->stackedWidget->setCurrentWidget(ui->pageData);
    } else if (button == ui->toolButtonStepPlot) {
        ui->stackedWidget->setCurrentWidget(ui->pagePlot);
    }
}

}
