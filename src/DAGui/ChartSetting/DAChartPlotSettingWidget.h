#ifndef DACHARTPLOTSETTINGWIDGET_H
#define DACHARTPLOTSETTINGWIDGET_H

#include <QWidget>
#include <QPointer>
#include "DAGuiAPI.h"
#include "DAChartWidget.h"
namespace Ui
{
class DAChartPlotSettingWidget;
}
namespace DA
{
/**
 * @brief 图表设置窗口
 */
class DAGUI_API DAChartPlotSettingWidget : public QWidget
{
    Q_OBJECT

public:
	explicit DAChartPlotSettingWidget(QWidget* parent = nullptr);
	~DAChartPlotSettingWidget();
	// 设置chart
	void setChartWidget(DAChartWidget* w);

private:
	Ui::DAChartPlotSettingWidget* ui;
	QPointer< DAChartWidget > mChartPlot;
};
}  // end DA
#endif  // DACHARTPLOTSETTINGWIDGET_H
