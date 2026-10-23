#ifndef DACHARTADDCURVEWIDGET_H
#define DACHARTADDCURVEWIDGET_H
#include "DAGuiAPI.h"
#include "DAChartAddXYSeriesWidget.h"

namespace DA
{

/**
 * @brief 添加xy series，适用二维数据绘图的系列获取
 */
class DAGUI_API DAChartAddCurveWidget : public DAChartAddXYSeriesWidget
{
    Q_OBJECT
public:
    explicit DAChartAddCurveWidget(QWidget* parent = nullptr);
    ~DAChartAddCurveWidget();

    virtual QwtPlotItem* createPlotItem() override;
    // 多条曲线（1 个 x + 多个 y），每条曲线按 y 序列名命名
    virtual QList< QwtPlotItem* > createPlotItems() override;
};
}
#endif  // DACHARTADDCURVEWIDGET_H
