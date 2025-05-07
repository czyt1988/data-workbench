#ifndef DACHARTADDSPECTROGRAMWIDGET_H
#define DACHARTADDSPECTROGRAMWIDGET_H
#include "DAChartAddtGridRasterDataWidget.h"
namespace DA
{
class DAGUI_API DAChartAddSpectrogramWidget : public DAChartAddtGridRasterDataWidget
{
public:
	explicit DAChartAddSpectrogramWidget(QWidget* parent = nullptr);
	~DAChartAddSpectrogramWidget();
	// 创建item
	virtual QwtPlotItem* createPlotItem() override;
};
}
#endif  // DACHARTADDSPECTROGRAMWIDGET_H
