#ifndef DACHARTPLOTSETTINGPANEL_H
#define DACHARTPLOTSETTINGPANEL_H

#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
#include "qwt_plot.h"

namespace DA
{
class DAPropertyPanelWidget;

/**
 * @brief QwtPlot级别的属性设置面板
 *
 * 用于设置QwtPlot的标题、脚注等全局属性。
 * 不继承DAAbstractChartItemSettingWidget，因为目标不是QwtPlotItem。
 *
 * 属性列表：
 * - PID_TitleText: 标题文本
 * - PID_TitleFont: 标题字体
 * - PID_TitleColor: 标题颜色
 * - PID_FooterText: 脚注文本
 * - PID_FooterFont: 脚注字体
 * - PID_FooterColor: 脚注颜色
 *
 * @see DAPropertyPanelWidget
 * @see QwtPlot
 */
class DAGUI_API DAChartPlotSettingPanel : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 属性ID枚举
     */
    enum PropertyId {
        PID_TitleText = 1,
        PID_TitleFont,
        PID_TitleColor,
        PID_FooterText,
        PID_FooterFont,
        PID_FooterColor
    };

    explicit DAChartPlotSettingPanel(QWidget* parent = nullptr);
    ~DAChartPlotSettingPanel() override;

    // 获取通用的属性面板指针
    DAPropertyPanelWidget* propertyPanel() const;

    // 设置/获取目标QwtPlot
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

#endif  // DACHARTPLOTSETTINGPANEL_H
