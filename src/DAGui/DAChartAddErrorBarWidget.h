#ifndef DACHARTADDERRORBARWIDGET_H
#define DACHARTADDERRORBARWIDGET_H
#include "DAGuiAPI.h"
#include "DAChartAddXYESeriesWidget.h"
namespace DA
{
/**
 * @brief 添加误差棒图
 *
 * 基于 QwtPlotIntervalCurve 实现 error bar 语义，复用 DAChartAddXYESeriesWidget
 * 的 x/y/error 三列数据提取逻辑，区间表示 y±error。
 */
class DAGUI_API DAChartAddErrorBarWidget : public DAChartAddXYESeriesWidget
{
	Q_OBJECT
public:
	explicit DAChartAddErrorBarWidget(QWidget* parent = nullptr);
	~DAChartAddErrorBarWidget();
	virtual QwtPlotItem* createPlotItem() override;
};
}  // namespace DA
#endif  // DACHARTADDERRORBARWIDGET_H
