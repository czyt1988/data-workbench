#include "DAChartAddErrorBarWidget.h"
#include "ui_DAChartAddErrorBarWidget.h"
#include "DAGlobalColorTheme.h"
#include <QButtonGroup>
#include "qwt_plot_curve.h"
#include <QPen>
#include "DAChartUtil.h"
namespace DA
{

class DAChartAddErrorBarWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAChartAddErrorBarWidget)
public:
	PrivateData(DAChartAddErrorBarWidget* p);

public:
	QButtonGroup* mBtnGroup{ nullptr };
};

DAChartAddErrorBarWidget::PrivateData::PrivateData(DAChartAddErrorBarWidget* p) : q_ptr(p)
{
	mBtnGroup = new QButtonGroup(p);
}

//----------------------------------------------------
// DAChartAddErrorBarWidget
//----------------------------------------------------

DAChartAddErrorBarWidget::DAChartAddErrorBarWidget(QWidget* parent)
	: DAAbstractChartAddItemWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartAddErrorBarWidget)
{
	ui->setupUi(this);
	init();
}

DAChartAddErrorBarWidget::~DAChartAddErrorBarWidget()
{
	delete ui;
}

void DAChartAddErrorBarWidget::init()
{
	d_ptr->mBtnGroup->addButton(ui->toolButtonStepData);
	d_ptr->mBtnGroup->addButton(ui->toolButtonStepPlot);
	d_ptr->mBtnGroup->setExclusive(true);
	connect(d_ptr->mBtnGroup,
			QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
			this,
			&DAChartAddErrorBarWidget::onNavButtonClicked);
	connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &DAChartAddErrorBarWidget::onStackWidgetCurrentChanged);
	QColor c = DAGlobalColorTheme::getInstance().color();
	ui->pageErrorBar->setCurvePen(QPen(c, 1.0));
}

/**
 * @brief 按照设定创建曲线
 * @return
 */
QwtPlotItem* DAChartAddErrorBarWidget::createPlotItem()
{
	QVector< QwtIntervalSample > xye = ui->pageData->getSeries();
	if (xye.empty()) {
		return nullptr;
	}
	QwtPlotIntervalCurve* item = new QwtPlotIntervalCurve();
	item->setSamples(xye);
	ui->pageErrorBar->updatePlotItem(item);
	return item;
}

void DAChartAddErrorBarWidget::setCurrentData(const DAData& d)
{
	ui->pageData->setCurrentData(d);
}

void DAChartAddErrorBarWidget::setDataManager(DADataManager* dmgr)
{
	ui->pageData->setDataManager(dmgr);
}

void DAChartAddErrorBarWidget::next()
{
	auto i = ui->stackedWidget->currentIndex();
	auto c = ui->stackedWidget->count();
	if (i < c) {
		++i;
		ui->stackedWidget->setCurrentIndex(i);
	}
	updateNavButtonState();
}

void DAChartAddErrorBarWidget::previous()
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
int DAChartAddErrorBarWidget::getStepCount() const
{
    return ui->stackedWidget->count();
}

int DAChartAddErrorBarWidget::getCurrentStep() const
{
    return ui->stackedWidget->currentIndex();
}

void DAChartAddErrorBarWidget::updateNavButtonState()
{
	auto i = ui->stackedWidget->currentIndex();
	if (0 == i) {
		ui->toolButtonStepData->setChecked(true);
	} else {
		ui->toolButtonStepPlot->setChecked(true);
	}
}

void DAChartAddErrorBarWidget::onNavButtonClicked(QAbstractButton* button)
{
	if (button == ui->toolButtonStepData) {
		ui->stackedWidget->setCurrentWidget(ui->pageData);
	} else if (button == ui->toolButtonStepPlot) {
		ui->stackedWidget->setCurrentWidget(ui->pagePlotScrollWidget);
	}
}

void DAChartAddErrorBarWidget::onStackWidgetCurrentChanged(int i)
{
	if (0 == i) {
		ui->toolButtonStepData->setChecked(true);
	} else {
		ui->toolButtonStepPlot->setChecked(true);
	}
}
}
