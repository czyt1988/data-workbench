#ifndef DACHARTSETTINGWIDGET_H
#define DACHARTSETTINGWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "qwt_plot_item.h"

class QScrollArea;
namespace Ui
{
class DAChartSettingWidget;
}

namespace DA
{
class DAChartOperateWidget;
/**
 * @brief 绘图设置窗口
 */
class DAGUI_API DAChartSettingWidget : public QWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAChartSettingWidget)
public:
	enum SettingType
	{
		SettingFigure = 0,
		SettingChart,
		SettingItem
	};

public:
	explicit DAChartSettingWidget(QWidget* parent = nullptr);
	~DAChartSettingWidget();
	void setChartOprateWidget(DAChartOperateWidget* opt);
	void setFigure(DAFigureWidget* fig);
	void setFigure(DAFigureWidget* fig, DAChartWidget* chart);
	void setFigure(DAFigureWidget* fig, DAChartWidget* chart, QwtPlotItem* item);
	DAFigureWidget* getFigure() const;
	DAChartWidget* getChart() const;
	QwtPlotItem* getItem() const;
	void bindFigure(DAFigureWidget* fig);
	void unbindFigure(DAFigureWidget* fig);
	void bindChart(DAChartWidget* chart);
	void unbindChart(DAChartWidget* chart);
	// 获取chart在combobox的索引
	int indexOfChart(const DAChartWidget* chart);
	int indexOfItem(const QwtPlotItem* item);
	// 设置当前的chart
	void setCurrentChart(DAChartWidget* chart);
	DAChartWidget* getCurrentChart() const;
	// 设置当前的item
	void setCurrentItem(QwtPlotItem* item);
	QwtPlotItem* getCurrentItem() const;
	// 显示设置
	void showFigureSettingWidget();
	void showPlotSettingWidget();
	void showItemSettingWidget();
	// 把chart列表从combobox中移除
	void removeChartFromComboBox(DAChartWidget* chart);

protected:
	void changeEvent(QEvent* e);
	// 设置charts
	void resetChartsComboBox(DAFigureWidget* fig);
	// 设置items
	void setItemsComboBox(const QList< QwtPlotItem* >& its);
	void resetItemsComboBox(DAChartWidget* chart);
protected slots:
	void onFigureCloseing(DA::DAFigureWidget* f);
	void onFigureCreated(DA::DAFigureWidget* f);
	void onCurrentFigureChanged(DA::DAFigureWidget* f, int index);
	// 添加了chart
	void onChartAdded(DA::DAChartWidget* c);
	// 绘图即将移除
	void onChartRemoved(DA::DAChartWidget* c);
	// 当前的绘图发生了变更
	void onCurrentChartChanged(DA::DAChartWidget* c);
	// chart的item发生了变换信号
	void onItemAttached(QwtPlotItem* plotItem, bool on);
	// onItemAttached的特化，把chart传入
	void onChartItemAttached(DA::DAChartWidget* c, QwtPlotItem* plotItem, bool on);
	// current chart触发的改变
	void onComboBoxChartActivated(int i);
	// current item触发的改变
	void onComboBoxItemActived(int i);
	// 按钮组点击
	void onButtonGroupTypeButtonClicked(int id);
	// 绘图的属性发生变化，刷新设置界面
	void onChartPropertyHasChanged(DAChartWidget* chart);

private:
	// chart的ui显示设置
	void setChartUI(DAChartWidget* chart);
	void updateChartUI();
	// item的ui显示设置
	void setItemUI(QwtPlotItem* item);
	void updateItemUI();
	// 通过索引获取chart
	DAChartWidget* getChartByIndex(int i) const;

private:
	Ui::DAChartSettingWidget* ui;
};
}
#endif  // DACHARTSETTINGWIDGET_H
