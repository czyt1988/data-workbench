#ifndef DACHARTAXISSETTINGPANEL_H
#define DACHARTAXISSETTINGPANEL_H

#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
#include "qwt_axis.h"
class QwtPlot;
class QButtonGroup;

namespace DA
{
class DAPropertyPanelWidget;

/**
 * @brief QwtScaleWidget级别的属性设置面板
 *
 * 用于设置坐标轴的标签、刻度、边距等属性。
 * 不继承DAAbstractChartItemSettingWidget，因为目标不是QwtPlotItem。
 * 构造函数需要接收axisId参数以标识当前操作的坐标轴。
 *
 * 属性列表：
 * - PID_EnableAxis: 坐标轴启用开关
 * - PID_LabelText: 坐标轴标签文本
 * - PID_LabelFont: 标签字体
 * - PID_LabelFontColor: 标签字体颜色
 * - PID_LabelAlignment: 标签对齐方式
 * - PID_LabelRotation: 标签旋转角度
 * - PID_Margin: 边距
 * - PID_MinScale: 最小刻度值
 * - PID_MaxScale: 最大刻度值
 * - PID_ScaleStyle: 刻度样式（普通/日期时间）
 *
 * @see DAPropertyPanelWidget
 * @see QwtScaleWidget
 */
class DAGUI_API DAChartAxisSettingPanel : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 属性ID枚举
     */
    enum PropertyId {
        PID_EnableAxis = 1,
        PID_LabelText,
        PID_LabelFont,
        PID_LabelFontColor,
        PID_LabelAlignment,
        PID_LabelRotation,
        PID_Margin,
        PID_MinScale,
        PID_MaxScale,
        PID_ScaleStyle
    };

    /**
     * @brief 刻度样式枚举
     */
    enum ScaleStyle {
        NormalScale,     ///< 普通刻度
        DateTimeScale    ///< 日期时间刻度
    };

    /**
     * @brief 构造函数
     * @param axisId 坐标轴ID（QwtAxis::YLeft等）
     * @param parent 父控件
     */
    explicit DAChartAxisSettingPanel(QwtAxis::Position axisId, QWidget* parent = nullptr);
    ~DAChartAxisSettingPanel() override;

    // 获取通用的属性面板指针
    DAPropertyPanelWidget* propertyPanel() const;

    // 设置/获取目标QwtPlot（需要配合axisId）
    void setTarget(QwtPlot* plot);
    QwtPlot* target() const;

    // 获取当前坐标轴ID
    QwtAxis::Position axisId() const;

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
    // 设置刻度样式值
    void setScaleStyleValue(int style);

private:
    DAPropertyPanelWidget* mPanel;
    QPointer< QwtPlot > mPlot;
    QwtAxis::Position mAxisId;
    QButtonGroup* mScaleStyleButtonGroup;
};

}  // end namespace DA

#endif  // DACHARTAXISSETTINGPANEL_H
