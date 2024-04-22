#ifndef DACHARTLEGENDITEMSETTINGWIDGET_H
#define DACHARTLEGENDITEMSETTINGWIDGET_H

#include "DAGuiAPI.h"
#include "DAAbstractChartItemSettingWidget.h"
class QwtPlotLegendItem;
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
	// setPlotItem之后调用的虚函数
	virtual void plotItemSet(QwtPlotItem* item);
	// 更新界面
	void updateUI(const QwtPlotLegendItem* item);

protected:
	void changeEvent(QEvent* e);
private slots:
	void onAligmentPositionChanged(Qt::Alignment al);
	void onSpinBoxHorizontalOffsetValueChanged(int v);
	void onSpinBoxVerticalOffsetValueChanged(int v);

private:
	Ui::DAChartLegendItemSettingWidget* ui;
};
}  // end DA
#endif  // DACHARTLEGENDITEMSETTINGWIDGET_H
