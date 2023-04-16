#ifndef DAAPPCHARTOPERATEWIDGET_H
#define DAAPPCHARTOPERATEWIDGET_H
#include "DAChartOperateWidget.h"
namespace DA
{
/**
 * @brief DAChartOperateWidget的app特化
 */
class DAAppChartOperateWidget : public DAChartOperateWidget
{
public:
    DAAppChartOperateWidget(QWidget* parent = nullptr);
    ~DAAppChartOperateWidget();
};
}

#endif  // DAAPPCHARTOPERATEWIDGET_H
