#ifndef DACHART3DSETTINGWIDGET_H
#define DACHART3DSETTINGWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
#include "DAFigureElementSelection.h"
// qwt3d
#include "qwt3d_plot.h"
#include "qwt3d_plotitem.h"

class QScrollArea;
namespace Ui
{
class DAChart3DSettingWidget;
}

namespace DA
{
class DAChart3DCommonItemsSettingWidget;

/**
 * @brief 3D绘图设置窗口
 *
 * 结构与DAChartSettingWidget平行：
 * 1. 绘图对象选择框（QComboBox）
 * 2. QStackedWidget，对应combobox的显示内容
 *
 * 固定条目：3D Chart Area / Coordinate System / X Axis / Y Axis / Z Axis / Color Legend
 * 动态条目：3D plot items（通过DAChart3DCommonItemsSettingWidget）
 */
class DAGUI_API DAChart3DSettingWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChart3DSettingWidget)
public:
    /**
     * @brief 固定选择区域，用于识别combobox选中后切换到哪个设置页面
     */
    enum SettingWidgetType
    {
        Setting3DChart = 0,
        Setting3DCoordSys,
        Setting3DXAxis,
        Setting3DYAxis,
        Setting3DZAxis,
        Setting3DColorLegend,
        Setting3DPlotItems
    };

    enum ComboBoxRole
    {
        RolePlot3DItemPtr = Qt::UserRole + 1  ///< 存放Qwt3DPlotItem指针的角色
    };

public:
    explicit DAChart3DSettingWidget(QWidget* parent = nullptr);
    ~DAChart3DSettingWidget();
    // 设置3D绘图
    void setPlot3D(Qwt3DPlot* plot);
    Qwt3DPlot* getPlot3D() const;
    // 更新界面
    void updateUI();
    // 设置当前选中的3D plotitem
    void setCurrentPlot3DItem(Qwt3DPlotItem* item);
    Qwt3DPlotItem* getCurrentPlot3DItem() const;

    // 设置选中内容，根据选中类型路由到对应设置页面
    void setSelection(const DAFigureElementSelection& sel);

    // 显示对应设置窗口
    void showSettingWidget(SettingWidgetType widType);
    void show3DChartSetting();
    void show3DCoordSysSetting();
    void show3DXAxisSetting();
    void show3DYAxisSetting();
    void show3DZAxisSetting();
    void show3DColorLegendSetting();
    void showPlot3DItemSetting(Qwt3DPlotItem* item);

    // 获取通用items设置窗口
    DAChart3DCommonItemsSettingWidget* getChart3DCommonItemsSettingWidget() const;

protected:
    void changeEvent(QEvent* e) override;
    // 通过plotitem查找combobox索引
    int findComboBoxIndexFromPlot3DItem(const Qwt3DPlotItem* item) const;
    // 添加plotitem到combobox
    void appendPlot3DItemToComboBox(const Qwt3DPlotItem* item);
    // 从combobox获取plotitem
    Qwt3DPlotItem* getPlot3DItemFromComboBox(int index) const;
    // 从combobox移除plotitem
    void removePlot3DItemFromComboBox(const Qwt3DPlotItem* item);
protected Q_SLOTS:
    void onComboBoxItemIndexChanged(int index);
    // 3D plot的item挂载/卸载
    void onPlot3DItemAttached(Qwt3DPlotItem* item, bool on);
    // 重置combobox
    void resetComboBox();

private:
    Ui::DAChart3DSettingWidget* ui;
};
}  // namespace DA
#endif  // DACHART3DSETTINGWIDGET_H
