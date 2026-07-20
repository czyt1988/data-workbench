#ifndef DACHARTDATAPICKERSETTINGPANEL_H
#define DACHARTDATAPICKERSETTINGPANEL_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
class QwtPlot;
class QwtPlotSeriesDataPicker;

namespace DA
{
class DAPropertyPanelContainerWidget;
class DAFigureWidget;

/**
 * @brief DataPicker级别的属性设置面板
 *
 * 用于设置QwtPlotSeriesDataPicker的完整状态，包括拾取模式、显示X值、
 * 文字位置、插值模式、特征点绘制、文字样式等。
 * 同时包含Figure级别的拾取器联动开关。
 *
 * 属性列表：
 * - PID_PickerMode: 拾取模式（关闭/Y值/XY值）
 * - PID_ShowXValue: 是否显示X值
 * - PID_TextPlacement: 文字位置
 * - PID_InterpolationMode: 插值模式
 * - PID_DrawFeaturePoint: 是否绘制特征点
 * - PID_FeaturePointSize: 特征点大小
 * - PID_NearestSearchWindowSize: 最近点搜索窗口大小
 * - PID_TextBackgroundBrush: 文字背景画刷
 * - PID_TextAlignment: 文字对齐
 * - PID_TextOffsetX: 文字偏移X
 * - PID_TextOffsetY: 文字偏移Y
 * - PID_PickerGroupEnabled: 拾取器联动开关（Figure级别）
 *
 * @see DAPropertyPanelContainerWidget
 * @see QwtPlotSeriesDataPicker
 * @see DAChartCanvasSettingPanel
 */
class DAGUI_API DAChartDataPickerSettingPanel : public QWidget
{
    Q_OBJECT
public:
    enum PropertyId {
        PID_PickerMode = 1,
        PID_ShowXValue,
        PID_TextPlacement,
        PID_InterpolationMode,
        PID_DrawFeaturePoint,
        PID_FeaturePointSize,
        PID_NearestSearchWindowSize,
        PID_TextBackgroundBrush,
        PID_TextAlignment,
        PID_TextOffsetX,
        PID_TextOffsetY,
        PID_PickerGroupEnabled
    };

    explicit DAChartDataPickerSettingPanel(QWidget* parent = nullptr);
    ~DAChartDataPickerSettingPanel() override;

    DAPropertyPanelContainerWidget* propertyPanel() const;

    void setTarget(QwtPlot* plot);
    QwtPlot* target() const;

    void updateUI();
    void replot();

Q_SIGNALS:
    void propertyValueChanged(int propertyId);

protected Q_SLOTS:
    void buildPropertyPanel();
    void onPanelPropertyValueChanged(int propertyId);
    void onPropertyValueChanged(int propertyId);

private:
    DAPropertyPanelContainerWidget* mPanel;
    QPointer< QwtPlot > mPlot;
};

}  // namespace DA

#endif  // DACHARTDATAPICKERSETTINGPANEL_H
