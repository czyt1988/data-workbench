#include "DAChartAddBarWidget.h"
#include "ui_DAChartAddBarWidget.h"
#include "DAGlobalColorTheme.h"
#include <QButtonGroup>
#include "qwt_plot_curve.h"
#include <QPen>
#include "DAChartUtil.h"
namespace DA
{

class DAChartAddBarWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAChartAddBarWidget)
public:
	PrivateData(DAChartAddBarWidget* p);

public:
	QButtonGroup* mBtnGroup{ nullptr };
};

DAChartAddBarWidget::PrivateData::PrivateData(DAChartAddBarWidget* p) : q_ptr(p)
{
	mBtnGroup = new QButtonGroup(p);
}

//----------------------------------------------------
// DAChartAddBarWidget
//----------------------------------------------------

DAChartAddBarWidget::DAChartAddBarWidget(QWidget* parent)
	: DAAbstractChartAddItemWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartAddBarWidget)
{
	ui->setupUi(this);
	init();
}

DAChartAddBarWidget::~DAChartAddBarWidget()
{
	delete ui;
}

void DAChartAddBarWidget::init()
{
	d_ptr->mBtnGroup->addButton(ui->toolButtonStepData);
	d_ptr->mBtnGroup->addButton(ui->toolButtonStepPlot);
	d_ptr->mBtnGroup->setExclusive(true);
	connect(d_ptr->mBtnGroup,
			QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
			this,
			&DAChartAddBarWidget::onNavButtonClicked);
	connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &DAChartAddBarWidget::onStackWidgetCurrentChanged);
}

/**
 * @brief 按照设定创建曲线
 * @return
 */
QwtPlotItem* DAChartAddBarWidget::createPlotItem()
{
	QVector< QPointF > xy = ui->pageData->getSeries();
	if (xy.empty()) {
		return nullptr;
	}
	QwtPlotBarChart* item = new QwtPlotBarChart();
	item->setSamples(xy);
	ui->pageBar->updatePlotItem(item);
	return item;
}

void DAChartAddBarWidget::setCurrentData(const DAData& d)
{
	ui->pageData->setCurrentData(d);
}

void DAChartAddBarWidget::setDataManager(DADataManager* dmgr)
{
	ui->pageData->setDataManager(dmgr);
}

void DAChartAddBarWidget::setBarMode(bool on)
{
	// 设置柱子是否可见
	if (on) {
		ui->pageBar->setBarStyle(QwtColumnSymbol::Style::Box);  // 显示柱子
	} else {
		ui->pageBar->setBarStyle(QwtColumnSymbol::Style::NoStyle);  // 隐藏柱子
	}
}

void DAChartAddBarWidget::next()
{
	auto i = ui->stackedWidget->currentIndex();
	auto c = ui->stackedWidget->count();
	if (i < c) {
		++i;
		ui->stackedWidget->setCurrentIndex(i);
	}
	updateNavButtonState();
}

void DAChartAddBarWidget::previous()
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
int DAChartAddBarWidget::getStepCount() const
{
    return ui->stackedWidget->count();
}

int DAChartAddBarWidget::getCurrentStep() const
{
    return ui->stackedWidget->currentIndex();
}

void DAChartAddBarWidget::updateNavButtonState()
{
	auto i = ui->stackedWidget->currentIndex();
	if (0 == i) {
		ui->toolButtonStepData->setChecked(true);
	} else {
		ui->toolButtonStepPlot->setChecked(true);
	}
}

void DAChartAddBarWidget::onNavButtonClicked(QAbstractButton* button)
{
	if (button == ui->toolButtonStepData) {
		ui->stackedWidget->setCurrentWidget(ui->pageData);
	} else if (button == ui->toolButtonStepPlot) {
		ui->stackedWidget->setCurrentWidget(ui->pagePlotScrollWidget);
	}
}

void DAChartAddBarWidget::onStackWidgetCurrentChanged(int i)
{
	if (0 == i) {
		ui->toolButtonStepData->setChecked(true);
	} else {
		ui->toolButtonStepPlot->setChecked(true);
	}
}
}
