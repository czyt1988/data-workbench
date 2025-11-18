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

    enum ComboBoxRole
    {
        RolePlotItemPtr = Qt::UserRole + 1  ///< 存放plotitem指针的角色
    };

public:
    explicit DAChartSettingWidget(QWidget* parent = nullptr);
    ~DAChartSettingWidget();
    // 设置绘图
    void setPlot(QwtPlot* plot);
    QwtPlot* getPlot() const;
    // 更新界面
    void updateUI();
    // 设置plotItem
    void setCurrentPlotItem(QwtPlotItem* item);
    QwtPlotItem* getCurrentPlotItem() const;

    // 设置选中内容，设置窗口会根据选中内容进行配置和显示
    void setSelection(const DAFigureElementSelection& sel);

    // 显示绘图设置
    void showPlotSetting();
    void showCanvasSetting();
    void showScaleYLeftSetting();
    void showScaleYRightSetting();
    void showScaleXBottomSetting();
    void showScaleXTopSetting();
    void showPlotItemSetting(QwtPlotItem* item);

protected:
    void changeEvent(QEvent* e);
    // 通过plotitem，从combobox找到对应的索引
    int findComboBoxIndexFromPlotItem(const QwtPlotItem* item) const;

protected slots:

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

    void resetComboBox();

private:
    Ui::DAChartSettingWidget* ui;
};
}
#endif  // DACHARTSETTINGWIDGET_H
