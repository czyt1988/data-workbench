#include "DAChartAddBoxWidget.h"
#include "ui_DAChartAddBoxWidget.h"
#include "DAGlobalColorTheme.h"
#include <QButtonGroup>
#include "qwt_samples.h"
#include <QPen>
#include "DAChartUtil.h"
namespace DA
{

class DAChartAddBoxWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAChartAddBoxWidget)
public:
	PrivateData(DAChartAddBoxWidget* p);

public:
	QButtonGroup* mBtnGroup{ nullptr };
};

DAChartAddBoxWidget::PrivateData::PrivateData(DAChartAddBoxWidget* p) : q_ptr(p)
{
	mBtnGroup = new QButtonGroup(p);
}

//----------------------------------------------------
// DAChartAddBoxWidget
//----------------------------------------------------

DAChartAddBoxWidget::DAChartAddBoxWidget(QWidget* parent)
	: DAAbstractChartAddItemWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartAddBoxWidget)
{
	ui->setupUi(this);
	init();
}

DAChartAddBoxWidget::~DAChartAddBoxWidget()
{
	delete ui;
}

void DAChartAddBoxWidget::init()
{
	d_ptr->mBtnGroup->addButton(ui->toolButtonStepData);
	d_ptr->mBtnGroup->addButton(ui->toolButtonStepPlot);
	d_ptr->mBtnGroup->setExclusive(true);
	connect(d_ptr->mBtnGroup,
			QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
			this,
			&DAChartAddBoxWidget::onNavButtonClicked);
	connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &DAChartAddBoxWidget::onStackWidgetCurrentChanged);
}

/**
 * @brief 按照设定创建曲线
 * @return
 */
QwtPlotItem* DAChartAddBoxWidget::createPlotItem()
{
	QVector< QwtOHLCSample > ohlc = ui->pageData->getSeries();
	if (ohlc.empty()) {
		return nullptr;
	}
	QwtPlotTradingCurve* item = new QwtPlotTradingCurve();
	item->setSamples(ohlc);
	ui->pageBox->updatePlotItem(item);
	return item;
}

void DAChartAddBoxWidget::setCurrentData(const DAData& d)
{
	ui->pageData->setCurrentData(d);
}

void DAChartAddBoxWidget::setDataManager(DADataManager* dmgr)
{
	ui->pageData->setDataManager(dmgr);
}

void DAChartAddBoxWidget::next()
{
	auto i = ui->stackedWidget->currentIndex();
	auto c = ui->stackedWidget->count();
	if (i < c) {
		++i;
		ui->stackedWidget->setCurrentIndex(i);
	}
	updateNavButtonState();
}

void DAChartAddBoxWidget::previous()
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
int DAChartAddBoxWidget::getStepCount() const
{
    return ui->stackedWidget->count();
}

int DAChartAddBoxWidget::getCurrentStep() const
{
    return ui->stackedWidget->currentIndex();
}

void DAChartAddBoxWidget::updateNavButtonState()
{
	auto i = ui->stackedWidget->currentIndex();
	if (0 == i) {
		ui->toolButtonStepData->setChecked(true);
	} else {
		ui->toolButtonStepPlot->setChecked(true);
	}
}

void DAChartAddBoxWidget::onNavButtonClicked(QAbstractButton* button)
{
	if (button == ui->toolButtonStepData) {
		ui->stackedWidget->setCurrentWidget(ui->pageData);
	} else if (button == ui->toolButtonStepPlot) {
		ui->stackedWidget->setCurrentWidget(ui->pagePlotScrollWidget);
	}
}

void DAChartAddBoxWidget::onStackWidgetCurrentChanged(int i)
{
	if (0 == i) {
		ui->toolButtonStepData->setChecked(true);
	} else {
		ui->toolButtonStepPlot->setChecked(true);
	}
}
}
