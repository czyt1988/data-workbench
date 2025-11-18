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
    enum SettingWidgetType
    {
        SettingPlot = 0,
        SettingCanvas,
        SettingYLeftScale,
        SettingYRightScale,
        SettingXBottomScale,
        SettingXTopScale,
        SettingPlotItems
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
    // 设置plotItem，注意，如果item的plot不是当前的plot，会调用setPlot把item的plot设置为当前plot
    void setCurrentPlotItem(QwtPlotItem* item);
    QwtPlotItem* getCurrentPlotItem() const;

    // 设置选中内容，设置窗口会根据选中内容进行配置和显示
    void setSelection(const DAFigureElementSelection& sel);

    // 显示绘图设置
    void showSettingWidget(SettingWidgetType widType);
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
    // 把plotitem添加到combobox
    void appendPlotItemToComboBox(const QwtPlotItem* item);
    // 从combobox获取plotitem
    QwtPlotItem* getPlotItemFromComboBox(int index) const;
    // 把plotitem从combobox移除
    void removePlotItemFromComboBox(const QwtPlotItem* item);
protected slots:
    void onComboBoxItemIndexChanged(int index);
    // plot的item发生了变换信号
    void onItemAttached(QwtPlotItem* plotItem, bool on);
    // 重置combobox
    void resetComboBox();

private:
    Ui::DAChartSettingWidget* ui;
};
}
#endif  // DACHARTSETTINGWIDGET_H
