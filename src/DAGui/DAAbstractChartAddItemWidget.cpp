#include "DAAbstractChartAddItemWidget.h"
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
}
