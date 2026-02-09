#ifndef DACHARTADDBARWIDGET_H
#define DACHARTADDBARWIDGET_H
#include "DAChartAddXYSeriesWidget.h"
namespace DA
{

/**
 * @brief 添加曲线和添加bar都是在xy的基础上添加
 */
class DAChartAddBarWidget : public DAChartAddXYSeriesWidget
{
	Q_OBJECT
public:
	DAChartAddBarWidget(QWidget* parent = nullptr);
	~DAChartAddBarWidget();
	// 创建item
	virtual QwtPlotItem* createPlotItem() override;
};
}

#endif  // DACHARTADDBARWIDGET_H
