#include "DAAppChartOperateWidget.h"
#include "Dialog/DADialogChartGuide.h"
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
 * @brief 生成绘图引导窗口
 * @param data
 * @return
 */
QwtPlotItem* DAAppChartOperateWidget::plotWithGuideDialog(const DAData& data)
{
}

}  // end DA
