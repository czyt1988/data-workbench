#ifndef DACHARTSETTINGWIDGET_H
#define DACHARTSETTINGWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "qwt_plot_item.h"
#include "DAFigureElementSelection.h"
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
    DAChartOperateWidget* getChartOprateWidget() const;

    DAFigureWidget* getFigure() const;
    DAChartWidget* getChart() const;
    QwtPlotItem* getPlotItem() const;

    // 获取chart在combobox的索引
    int indexOfChart(const DAChartWidget* chart);
    int indexOfItem(const QwtPlotItem* item);

    DAChartWidget* getCurrentSelectChart() const;

    QwtPlotItem* getCurrentItem() const;
    // 显示设置
    void showFigureSettingWidget();
    void showPlotSettingWidget();
    void showItemSettingWidget();
    // 把chart列表从combobox中移除
    void removeChartFromComboBox(DAChartWidget* chart);
    // 设置选中内容，设置窗口会根据选中内容进行配置和显示
    void setSelection(const DAFigureElementSelection& sel);

protected:
    void changeEvent(QEvent* e);
    void setFigure(DAFigureWidget* fig);
    void setChart(DAChartWidget* chart);
    void setPlotItem(QwtPlotItem* item);

    // 重置Charts复选框
    void resetChartsComboBox();
    // 重置item复选框
    void resetItemsComboBox(DAChartWidget* chart);
    // 删除当前item复选框的一个QwtPlotItem,成功删除返回true
    bool removeItemFromItemComboBox(QwtPlotItem* item);
    // 更新chartui
    void updateChartUI();
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
    void setChartItemAttached(DA::DAChartWidget* c, QwtPlotItem* plotItem, bool on);
    // current chart触发的改变
    void onComboBoxChartIndexChanged(int i);
    // current item触发的改变
    void onComboBoxItemIndexChanged(int i);
    // 按钮组点击
    void onButtonGroupTypeButtonClicked(int id);
    // 绘图的属性发生变化，刷新设置界面
    void onChartPropertyHasChanged(DAChartWidget* chart);

private:
    // ChartOperateWidget相关的绑定
    void bindChartOprateWidget(DAChartOperateWidget* opt);
    void unbindChartOprateWidget(DAChartOperateWidget* opt);
    // figure相关的绑定
    void bindFigure(DAFigureWidget* fig);
    void unbindFigure(DAFigureWidget* fig);
    // chart相关的绑定
    void bindChart(DAChartWidget* chart);
    void unbindChart(DAChartWidget* chart);

    void updateItemUI();
    // 通过索引获取chart
    DAChartWidget* getChartByIndex(int i) const;
    QwtPlotItem* getPlotItemByIndex(int i) const;

private:
    Ui::DAChartSettingWidget* ui;
};
}
#endif  // DACHARTSETTINGWIDGET_H
