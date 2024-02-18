#include "DAChartAddCurveWidget.h"
#include "ui_DAChartAddCurveWidget.h"
#include <QButtonGroup>
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
    connect(d_ptr->mBtnGroup, QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked), this, &DAChartAddCurveWidget::onNavButtonClicked);
}

QwtPlotItem* DAChartAddCurveWidget::createPlotItem()
{
    return nullptr;
}

void DAChartAddCurveWidget::setCurrentData(const DAData& d)
{
    ui->pageData->setCurrentData(d);
}

DAData DAChartAddCurveWidget::getCurrentData() const
{
    return ui->pageData->getCurrentData();
}

void DAChartAddCurveWidget::setDataManager(DADataManager* dmgr)
{
    ui->pageData->setDataManager(dmgr);
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
