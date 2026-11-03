#include "Chart/AddItem/DAAbstractStatsChartAddWidget.h"
#include "Chart/DAStatsPlotRequest.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DALogCategory.h"

#include <memory>

namespace DA
{
DAAbstractStatsChartAddWidget::DAAbstractStatsChartAddWidget(QWidget* parent) : DAAbstractChartAddWidgetBase(parent)
{
}

DAAbstractStatsChartAddWidget::~DAAbstractStatsChartAddWidget()
{
}

/**
 * @brief 用户确认的模板方法：收集请求 → 校验 → 发射信号
 *
 * 请求对象由 createPlotRequest() 创建、unique_ptr 接管，
 * 指针不逃出本函数；plotRequested 以 const 引用同步发射，出作用域自动析构。
 */
void DAAbstractStatsChartAddWidget::onButtonBoxAccepted()
{
    std::unique_ptr< DAStatsPlotRequest > req(createPlotRequest());
    if (!req->isValid()) {
        daWarning << req->errorString();
        return;
    }
    Q_EMIT plotRequested(*req);
}

void DAAbstractStatsChartAddWidget::setFigureWidget(DAFigureWidget* fig)
{
    mFigureWidget = fig;
}

DAFigureWidget* DAAbstractStatsChartAddWidget::getFigureWidget() const
{
    return mFigureWidget;
}

void DAAbstractStatsChartAddWidget::setChartWidget(DAChartWidget* chart)
{
    mChartWidget = chart;
}

DAChartWidget* DAAbstractStatsChartAddWidget::getChartWidget() const
{
    return mChartWidget;
}

}  // namespace DA
