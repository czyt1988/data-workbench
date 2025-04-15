#include "DAChartAddSpectrogramWidget.h"
#include "ui_DAChartAddSpectrogramWidget.h"
#include "DAGlobalColorTheme.h"
#include <QButtonGroup>
#include "qwt_plot_spectrogram.h"
#include <QPen>
#include "DAChartUtil.h"
namespace DA
{

class DAChartAddSpectrogramWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAChartAddSpectrogramWidget)
public:
	PrivateData(DAChartAddSpectrogramWidget* p);

public:
	QButtonGroup* mBtnGroup{ nullptr };
};

DAChartAddSpectrogramWidget::PrivateData::PrivateData(DAChartAddSpectrogramWidget* p) : q_ptr(p)
{
	mBtnGroup = new QButtonGroup(p);
}

//----------------------------------------------------
// DAChartAddSpectrogramWidget
//----------------------------------------------------

DAChartAddSpectrogramWidget::DAChartAddSpectrogramWidget(QWidget* parent)
	: DAAbstractChartAddItemWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartAddSpectrogramWidget)
{
	ui->setupUi(this);
	init();
}

DAChartAddSpectrogramWidget::~DAChartAddSpectrogramWidget()
{
	delete ui;
}

void DAChartAddSpectrogramWidget::init()
{
	d_ptr->mBtnGroup->addButton(ui->toolButtonStepData);
	d_ptr->mBtnGroup->addButton(ui->toolButtonStepPlot);
	d_ptr->mBtnGroup->setExclusive(true);
	connect(d_ptr->mBtnGroup,
			QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked),
			this,
			&DAChartAddSpectrogramWidget::onNavButtonClicked);
	connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &DAChartAddSpectrogramWidget::onStackWidgetCurrentChanged);
	QColor c = DAGlobalColorTheme::getInstance().color();
	ui->pageSpectrogram->setCurvePen(QPen(c, 1.0));
}

/**
 * @brief 按照设定创建曲线
 * @return
 */
QwtPlotItem* DAChartAddSpectrogramWidget::createPlotItem()
{
	QVector< QPointF > xy = ui->pageData->getSeries();
	if (xy.empty()) {
		return nullptr;
	}
	QwtPlotSpectrogram* item = new QwtPlotSpectrogram();
	//	item->setSamples(xy);
	ui->pageSpectrogram->updatePlotItem(item);
	return item;
}

void DAChartAddSpectrogramWidget::setCurrentData(const DAData& d)
{
	ui->pageData->setCurrentData(d);
}

void DAChartAddSpectrogramWidget::setDataManager(DADataManager* dmgr)
{
	ui->pageData->setDataManager(dmgr);
}

void DAChartAddSpectrogramWidget::next()
{
	auto i = ui->stackedWidget->currentIndex();
	auto c = ui->stackedWidget->count();
	if (i < c) {
		++i;
		ui->stackedWidget->setCurrentIndex(i);
	}
	updateNavButtonState();
}

void DAChartAddSpectrogramWidget::previous()
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
int DAChartAddSpectrogramWidget::getStepCount() const
{
    return ui->stackedWidget->count();
}

int DAChartAddSpectrogramWidget::getCurrentStep() const
{
    return ui->stackedWidget->currentIndex();
}

void DAChartAddSpectrogramWidget::updateNavButtonState()
{
	auto i = ui->stackedWidget->currentIndex();
	if (0 == i) {
		ui->toolButtonStepData->setChecked(true);
	} else {
		ui->toolButtonStepPlot->setChecked(true);
	}
}

void DAChartAddSpectrogramWidget::onNavButtonClicked(QAbstractButton* button)
{
	if (button == ui->toolButtonStepData) {
		ui->stackedWidget->setCurrentWidget(ui->pageData);
	} else if (button == ui->toolButtonStepPlot) {
		ui->stackedWidget->setCurrentWidget(ui->pagePlotScrollWidget);
	}
}

void DAChartAddSpectrogramWidget::onStackWidgetCurrentChanged(int i)
{
	if (0 == i) {
		ui->toolButtonStepData->setChecked(true);
	} else {
		ui->toolButtonStepPlot->setChecked(true);
	}
}
}
