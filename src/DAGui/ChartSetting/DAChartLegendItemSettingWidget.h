#ifndef DACHARTLEGENDITEMSETTINGWIDGET_H
#define DACHARTLEGENDITEMSETTINGWIDGET_H

#include "DAGuiAPI.h"
#include "DAAbstractChartItemSettingWidget.h"
namespace Ui
{
class DAChartLegendItemSettingWidget;
}
namespace DA
{
class DAGUI_API DAChartLegendItemSettingWidget : public DAAbstractChartItemSettingWidget
{
	Q_OBJECT

public:
	explicit DAChartLegendItemSettingWidget(QWidget* parent = nullptr);
	~DAChartLegendItemSettingWidget();

protected:
	void changeEvent(QEvent* e);

private:
	Ui::DAChartLegendItemSettingWidget* ui;
};
}  // end DA
#endif  // DACHARTLEGENDITEMSETTINGWIDGET_H
