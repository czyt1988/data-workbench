#ifndef DAFIGURESETTINGWIDGET_H
#define DAFIGURESETTINGWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "qwt_plot_item.h"
namespace Ui
{
class DAFigureSettingWidget;
}

namespace DA
{
/**
 * @brief 绘图设置窗口
 */
class DAGUI_API DAFigureSettingWidget : public QWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAFigureSettingWidget)
public:
	explicit DAFigureSettingWidget(QWidget* parent = nullptr);
	~DAFigureSettingWidget();
	void setFigure(DAFigureWidget* fig);
	void setFigure(DAFigureWidget* fig, DAChartWidget* chart);
	void setFigure(DAFigureWidget* fig, DAChartWidget* chart, QwtPlotItem* item);
	DAFigureWidget* getFigure() const;
	DAChartWidget* getChart() const;
	QwtPlotItem* getItem() const;
	void bindFigure(DAFigureWidget* fig);
	void unbindFigure(DAFigureWidget* fig);
	// 获取chart在combobox的索引
	int indexOfChart(const DAChartWidget* chart);
	// 设置当前的chart
	void setCurrentChart(DAChartWidget* chart);

protected:
	void changeEvent(QEvent* e);
protected slots:
	// 添加了chart
	void onChartAdded(DA::DAChartWidget* c);
	// 绘图即将移除
	void onChartWillRemove(DA::DAChartWidget* c);
	// 当前的绘图发生了变更
	void onCurrentChartChanged(DA::DAChartWidget* c);
	// chart的item发生了变换信号
	void onItemAttached(QwtPlotItem* plotItem, bool on);
	// onItemAttached的特化，把chart传入
	void onChartItemAttached(DA::DAChartWidget* c, QwtPlotItem* plotItem, bool on);

private:
	Ui::DAFigureSettingWidget* ui;
};
}
#endif  // DAFIGURESETTINGWIDGET_H
