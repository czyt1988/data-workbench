#include "DAAppChartOperateWidget.h"
#include "Dialog/DADialogChartGuide.h"
#include "DAAppFigureWidget.h"
namespace DA
{

DAAppChartOperateWidget::DAAppChartOperateWidget(QWidget* parent) : DAChartOperateWidget(parent)
{
}

DAAppChartOperateWidget::~DAAppChartOperateWidget()
{
}

/**
 * @brief 设置dmg
 * @param mgr
 */
void DAAppChartOperateWidget::setDataManager(DADataManager* mgr)
{
    mDataMgr = mgr;
}

/**
 * @brief 创建figure
 * @return
 */
DAFigureWidget* DAAppChartOperateWidget::createFigure()
{
    DAFigureWidget* fig = DAChartOperateWidget::createFigure();
    if (DAAppFigureWidget* appFig = qobject_cast< DAAppFigureWidget* >(fig)) {
        appFig->setDataManager(mDataMgr);
    }
    return fig;
}

/**
 * @brief 生成绘图引导窗口
 * @param data
 * @return
 */
QwtPlotItem* DAAppChartOperateWidget::plotWithGuideDialog(const DAData& data)
{
}

}  // end DA
