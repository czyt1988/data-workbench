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
 *
 * 绘图设置窗口主要由两个部分组成
 * 1. 绘图对象选择框：一个combobox，把所有涉及的可设置的绘图对象列举出来
 * 2. stackwidget，对应combobox的显示的窗口
 *
 * 由于每个设置窗口管理的对象都不一样，因此此窗口统一设置一个qwtplot指针
 *
 * @note 对于寄生绘图，此窗口不会做过多的复杂操作，仅仅把它当做一个普通的qwtplot来处理，但寄生绘图有些参数是不可设置，
 * 因此寄生绘图的设置需要外部窗口让用户选中寄生绘图的绘图对象来进行设置
 *
 * @note 此窗口不处理QwtFigure相关参数，最大单元是QwtPlot
 *
 */
class DAGUI_API DAChartSettingWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartSettingWidget)
public:
    /**
     * @brief 固定选择区域，固定选择区域是combobox比较固定的选中区域，用于识别combobox选中后能知道选中了什么内容
     */
    enum FixSelectionArea
    {
        PlotArea = 0,
        CanvasArea,
        YLeftScaleArea,
        YRightScaleArea,
        XBottomScaleArea,
        XTopScaleArea,
        PlotItemsArea
    };

public:
    explicit DAChartSettingWidget(QWidget* parent = nullptr);
    ~DAChartSettingWidget();
    // 设置绘图
    void setPlot(QwtPlot* plot);
    QwtPlot* getPlot() const;
    // 更新界面
    void updateUI();
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

    // 绘图删除的信号
    void onPlotDestroyed(QObject* obj);
    // plot的item发生了变换信号
    void onItemAttached(QwtPlotItem* plotItem, bool on);

private:
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
