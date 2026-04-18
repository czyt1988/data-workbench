#ifndef DACHARTCANVASSETTINGPANEL_H
#define DACHARTCANVASSETTINGPANEL_H

#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
class QwtPlot;

namespace DA
{
class DAPropertyPanelWidget;

/**
 * @brief QwtPlotCanvas级别的属性设置面板
 *
 * 用于设置QwtPlotCanvas的背景、边框等画布属性。
 * 不继承DAAbstractChartItemSettingWidget，因为目标不是QwtPlotItem。
 *
 * 属性列表：
 * - PID_BackgroundBrush: 背景画刷
 * - PID_BorderWidth: 边框宽度
 * - PID_BorderPen: 边框画笔
 * - PID_FrameStyle: 边框样式
 *
 * @see DAPropertyPanelWidget
 * @see QwtPlotCanvas
 */
class DAGUI_API DAChartCanvasSettingPanel : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 属性ID枚举
     */
    enum PropertyId {
        PID_BackgroundBrush = 1,
        PID_BorderWidth,
        PID_BorderPen,
        PID_FrameStyle
    };

    explicit DAChartCanvasSettingPanel(QWidget* parent = nullptr);
    ~DAChartCanvasSettingPanel() override;

    // 获取通用的属性面板指针
    DAPropertyPanelWidget* propertyPanel() const;

    // 设置/获取目标QwtPlot（通过QwtPlot获取canvas）
    void setTarget(QwtPlot* plot);
    QwtPlot* target() const;

    // 更新UI显示
    void updateUI();

    // 触发重绘
    void replot();

Q_SIGNALS:
    /**
     * @brief 属性值变化信号
     * @param propertyId 属性ID
     */
    void propertyValueChanged(int propertyId);

protected Q_SLOTS:
    // 构建属性面板
    void buildPropertyPanel();

    // 转发DAPropertyPanelWidget::propertyValueChanged到propertyValueChanged信号
    void onPanelPropertyValueChanged(int propertyId);

    // 属性值变化处理
    void onPropertyValueChanged(int propertyId);

private:
    DAPropertyPanelWidget* mPanel;
    QPointer< QwtPlot > mPlot;
};

}  // end namespace DA

#endif  // DACHARTCANVASSETTINGPANEL_H
