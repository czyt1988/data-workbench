#include "DAAbstractChartAddItemWidget.h"
#include <QDebug>
namespace DA
{
DAAbstractChartAddItemWidget::DAAbstractChartAddItemWidget(QWidget* par) : QWidget(par)
{
}

DAAbstractChartAddItemWidget::~DAAbstractChartAddItemWidget()
{
}

void DAAbstractChartAddItemWidget::updateData()
{
}

void DAAbstractChartAddItemWidget::next()
{
}

void DAAbstractChartAddItemWidget::previous()
{
}

/**
 * @brief 获取步骤总数
 *
 * 一个例子：
 *
 * @code
 * void DAChartAddCurveWidget::next()
 * {
 *     auto i = ui->stackedWidget->currentIndex();
 *     auto c = ui->stackedWidget->count();
 *     if (i < c) {
 *         ++i;
 *         ui->stackedWidget->setCurrentIndex(i);
 *     }
 * }
 *
 * void DAChartAddCurveWidget::previous()
 * {
 *     auto i = ui->stackedWidget->currentIndex();
 *     --i;
 *     if (i >= 0) {
 *         ui->stackedWidget->setCurrentIndex(i);
 *     }
 * }
 *
 * int DAChartAddCurveWidget::getStepCount() const
 * {
 *     return ui->stackedWidget->count();
 * }
 *
 * int DAChartAddCurveWidget::getCurrentStep() const
 * {
 *     return ui->stackedWidget->currentIndex();
 * }
 * @endcode
 * @return
 */
int DAAbstractChartAddItemWidget::getStepCount() const
{
    return 0;
}

int DAAbstractChartAddItemWidget::getCurrentStep() const
{
    return -1;
}

/**
 * @brief 设置到第一步
 */
void DAAbstractChartAddItemWidget::toFirst()
{
	int c = getStepCount();
	// 防止异常的继承导致程序奔溃
	int max = 99;
	if (c > 0) {
		int step = 0;
		while (getCurrentStep() != 0 || step < max) {
			previous();
			++step;
		}
		if (step == max) {
			qDebug() << "Critual widget toFirstStep have get max step";
		}
	}
}

/**
 * @brief 跳转到最后一步
 */
void DAAbstractChartAddItemWidget::toLast()
{
	int c = getStepCount();
	c -= 1;
	int max = 99;
	if (c > 0) {
		int step = 0;
		while (getCurrentStep() != c || step < max) {
			next();
			++step;
		}
		if (step == max) {
			qDebug() << "Critual widget toLastStep have get max step";
		}
	}
}
}
