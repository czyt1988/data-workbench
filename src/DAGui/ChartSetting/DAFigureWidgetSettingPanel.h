#ifndef DAFIGUREWIDGETSETTINGPANEL_H
#define DAFIGUREWIDGETSETTINGPANEL_H

#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>

namespace DA
{
class DAFigureWidget;
class DAPropertyPanelWidget;

/**
 * @brief DAFigureWidget级别的属性设置面板
 *
 * 用于设置DAFigureWidget的尺寸限制和背景等属性。
 * 不继承DAAbstractChartItemSettingWidget，因为目标不是QwtPlotItem。
 *
 * 属性列表：
 * - PID_MinWidth: 最小宽度
 * - PID_MinHeight: 最小高度
 * - PID_MaxWidth: 最大宽度
 * - PID_MaxHeight: 最大高度
 * - PID_BackgroundBrush: 背景画刷
 *
 * @see DAPropertyPanelWidget
 * @see DAFigureWidget
 */
class DAGUI_API DAFigureWidgetSettingPanel : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 属性ID枚举
     */
    enum PropertyId {
        PID_MinWidth = 1,
        PID_MinHeight,
        PID_MaxWidth,
        PID_MaxHeight,
        PID_BackgroundBrush
    };

    explicit DAFigureWidgetSettingPanel(QWidget* parent = nullptr);
    ~DAFigureWidgetSettingPanel() override;

    // 获取通用的属性面板指针
    DAPropertyPanelWidget* propertyPanel() const;

    // 设置/获取目标DAFigureWidget
    void setTarget(DAFigureWidget* fig);
    DAFigureWidget* target() const;

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
    QPointer< DAFigureWidget > mFigure;
};

}  // end namespace DA

#endif  // DAFIGUREWIDGETSETTINGPANEL_H
