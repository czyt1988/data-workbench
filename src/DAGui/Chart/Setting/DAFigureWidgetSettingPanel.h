#ifndef DAFIGUREWIDGETSETTINGPANEL_H
#define DAFIGUREWIDGETSETTINGPANEL_H

#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>

class QPushButton;

namespace DA
{
class DAFigureWidget;
class DAPropertyPanelContainerWidget;

/**
 * @brief DAFigureWidget级别的属性设置面板
 *
 * 用于设置DAFigureWidget的尺寸限制、背景以及保留式网格布局等属性。
 * 不继承DAAbstractChartItemSettingWidget，因为目标不是QwtPlotItem。

 * 属性列表：
 * - PID_MinWidth: 最小宽度
 * - PID_MinHeight: 最小高度
 * - PID_MaxWidth: 最大宽度
 * - PID_MaxHeight: 最大高度
 * - PID_BackgroundBrush: 背景画刷
 * - PID_GridRows / PID_GridColumns: 网格行列数（只在未启用网格时可改）
 * - PID_GridToggle: 启用/退出网格布局按钮
 * - PID_GridEqual: 把网格轨道恢复为等分

 * @see DAPropertyPanelContainerWidget
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
        PID_BackgroundBrush,
        PID_GridToggle,
        PID_GridRows,
        PID_GridColumns,
        PID_GridEqual
    };

    explicit DAFigureWidgetSettingPanel(QWidget* parent = nullptr);
    ~DAFigureWidgetSettingPanel() override;

    // 获取通用的属性面板指针
    DAPropertyPanelContainerWidget* propertyPanel() const;

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

    // 启用/退出网格布局
    void onGridToggleClicked();

    // 把网格轨道恢复为等分
    void onGridEqualTracksClicked();

private:
    // 把 figure 里已有的子图按当前视觉顺序登记进网格（超过单元格数量的保持自由布局）
    void placeExistingChartsInGrid(DAFigureWidget* fig, int rows, int cols);

    DAPropertyPanelContainerWidget* mPanel;
    QPointer< DAFigureWidget > mFigure;
    QPushButton* mGridToggleButton { nullptr };
    QPushButton* mGridEqualButton { nullptr };
};

}  // end namespace DA

#endif  // DAFIGUREWIDGETSETTINGPANEL_H
