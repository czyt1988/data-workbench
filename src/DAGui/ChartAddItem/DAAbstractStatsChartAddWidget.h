#ifndef DAABSTRACTSTATSCHARTADDWIDGET_H
#define DAABSTRACTSTATSCHARTADDWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddWidgetBase.h"
#include "DAData.h"
#include <QJsonObject>

namespace DA
{
class DAFigureWidget;
class DAChartWidget;

/**
 * @brief 统计绘图设置窗口基类
 *
 * 统计绘图不返回 QwtPlotItem，而是把参数收集为 JSON、连同数据源一起
 * 通过 plotRequested 信号交给 Controller，由 Controller 调用 Python
 * 统计绘图函数完成计算和绘制。
 *
 * 它与 DAAbstractChartAddItemWidget 是并列的两种输出契约，
 * 不继承「必须返回 QwtPlotItem*」的接口。
 */
class DAGUI_API DAAbstractStatsChartAddWidget : public DAAbstractChartAddWidgetBase
{
    Q_OBJECT
public:
    explicit DAAbstractStatsChartAddWidget(QWidget* parent = nullptr);
    virtual ~DAAbstractStatsChartAddWidget();

    /**
     * @brief 收集当前窗口中的参数为 JSON 对象
     * @return 参数 JSON
     */
    virtual QJsonObject buildPlotParams() const = 0;

    /**
     * @brief 用户确认时的处理逻辑（校验数据、发射绘图请求）
     * @note 子类必须实现此方法，由引导对话框在 accept 时调用
     */
    virtual void onButtonBoxAccepted() = 0;

    // 设置/获取 Figure 窗口
    void setFigureWidget(DAFigureWidget* fig);
    DAFigureWidget* getFigureWidget() const;

    // 设置/获取 Chart 窗口
    void setChartWidget(DAChartWidget* chart);
    DAChartWidget* getChartWidget() const;

Q_SIGNALS:
    /**
     * @brief 绘图请求信号，子类在用户确认参数后发射此信号
     * @param params 绘图参数 JSON
     * @param data 数据源，直接随信号传递，避免用动态属性做隐式通道
     * @param fig 目标 Figure 窗口
     * @param chart 目标 Chart 窗口
     */
    void plotRequested(const QJsonObject& params, const DA::DAData& data, DA::DAFigureWidget* fig, DA::DAChartWidget* chart);

protected:
    DAFigureWidget* mFigureWidget { nullptr };
    DAChartWidget* mChartWidget { nullptr };
};

}  // namespace DA

#endif  // DAABSTRACTSTATSCHARTADDWIDGET_H
