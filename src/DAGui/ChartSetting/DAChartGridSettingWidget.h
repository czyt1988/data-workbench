#ifndef DACHARTGRIDSETTINGWIDGET_H
#define DACHARTGRIDSETTINGWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
#include "DAAbstractChartItemSettingWidget.h"

namespace Ui
{
class DAChartGridSettingWidget;
}

namespace DA
{
/**
 * @brief grid设置窗口(QwtPlotItem::Rtti_PlotGrid)
 */
class DAGUI_API DAChartGridSettingWidget : public DAAbstractChartItemSettingWidget
{
	Q_OBJECT

public:
	explicit DAChartGridSettingWidget(QWidget* parent = nullptr);
	~DAChartGridSettingWidget();
protected slots:
	// major画笔
	void onMajorLinePenChanged(const QPen& p);
	// minor画笔
	void onMinorLinePenChanged(const QPen& p);
	// item设置了
	virtual void plotItemSet(QwtPlotItem* item) override;

protected:
	void changeEvent(QEvent* e) override;
	//
	void clearUI();

private:
	Ui::DAChartGridSettingWidget* ui;
};
}  // end DA

#endif  // DACHARTGRIDSETTINGWIDGET_H
