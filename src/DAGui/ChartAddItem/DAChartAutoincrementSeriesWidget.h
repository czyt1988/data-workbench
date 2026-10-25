#ifndef DACHARTAUTOINCREMENTSERIESWIDGET_H
#define DACHARTAUTOINCREMENTSERIESWIDGET_H
#include "DAGuiAPI.h"
#include <QGroupBox>

namespace DA
{

/**
 * @brief 图表添加窗口的“自增序列”参数组
 *
 * 保留原 QGroupBox 的可勾选 + flat 视觉：勾选 = 使用自增序列；未勾选时
 * QGroupBox 自动禁用内部输入。初值/步长用 QDoubleSpinBox，非法输入从源头消失
 * （调用方不再需要解析字符串并弹“必须为浮点数”的警告）。
 *
 * 只服务绘图添加窗口（依赖 DAShared 的 DAAutoincrementSeries 语义），
 * 不满足通用控件的四问，因此留在 DAGui 而不是 DAWidgets。
 */
class DAGUI_API DAChartAutoincrementSeriesWidget : public QGroupBox
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartAutoincrementSeriesWidget)
public:
    explicit DAChartAutoincrementSeriesWidget(QWidget* parent = nullptr);
    ~DAChartAutoincrementSeriesWidget();

    // 是否使用自增序列（等价于 QGroupBox 的勾选态）
    void setSeriesEnabled(bool on);
    bool isSeriesEnabled() const;

    // 初值
    void setStartValue(double v);
    double startValue() const;

    // 步长
    void setStepValue(double v);
    double stepValue() const;

    // 初值/步长的取值范围
    void setRange(double min, double max);
    // 初值/步长的小数位数
    void setDecimals(int n);

Q_SIGNALS:
    /**
     * @brief 初值或步长变化
     */
    void seriesConfigChanged();
    /**
     * @brief 勾选态变化（true = 使用自增序列）
     * @param on 是否使用自增序列
     */
    void seriesEnabledChanged(bool on);

private:
    Q_DISABLE_COPY(DAChartAutoincrementSeriesWidget)
};
}  // namespace DA

#endif  // DACHARTAUTOINCREMENTSERIESWIDGET_H
