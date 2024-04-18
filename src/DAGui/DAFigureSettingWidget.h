#ifndef DAFIGURESETTINGWIDGET_H
#define DAFIGURESETTINGWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "qwt_plot_item.h"

class QScrollArea;
namespace Ui
{
class DAFigureSettingWidget;
}

namespace DA
{
class DAChartOperateWidget;
/**
 * @brief 绘图设置窗口
 */
class DAGUI_API DAFigureSettingWidget : public QWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAFigureSettingWidget)
public:
    enum SettingType
    {
        SettingFigure = 0,
        SettingChart,
        SettingItem
    };

public:
    explicit DAFigureSettingWidget(QWidget* parent = nullptr);
	~DAFigureSettingWidget();
    void setChartOprateWidget(DAChartOperateWidget* opt);
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
    // 设置当前的item
    void setCurrentItem(QwtPlotItem* item);

protected:
	void changeEvent(QEvent* e);
    // 设置charts
    void setChartsComboBox(DAFigureWidget* fig);
    // 设置items
    void setItemsComboBox(const QList< QwtPlotItem* >& its);
    void setItemsComboBox(DAChartWidget* chart);
protected slots:
    void onFigureCloseing(DA::DAFigureWidget* f);
    void onFigureCreated(DA::DAFigureWidget* f);
    void onCurrentFigureChanged(DA::DAFigureWidget* f, int index);
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
    // current chart触发的改变
    void onComboBoxChartIndexChanged(int i);
    // current item触发的改变
    void onComboBoxItemIndexChanged(int i);
    // 按钮组点击
    void onButtonGroupTypeButtonClicked(int id);

private:
	Ui::DAFigureSettingWidget* ui;
};
}
#endif  // DAFIGURESETTINGWIDGET_H
