#ifndef DACHARTADDINTERVALCURVEWIDGET_H
#define DACHARTADDINTERVALCURVEWIDGET_H
#include "DAChartAddXYESeriesWidget.h"
namespace DA
{
class DAGUI_API DAChartAddIntervalCurveWidget : public DAChartAddXYESeriesWidget
{
public:
	explicit DAChartAddIntervalCurveWidget(QWidget* parent = nullptr);
	~DAChartAddIntervalCurveWidget();
	// 创建item
	virtual QwtPlotItem* createPlotItem() override;
};

}

#endif  // DACHARTADDINTERVALCURVEWIDGET_H
