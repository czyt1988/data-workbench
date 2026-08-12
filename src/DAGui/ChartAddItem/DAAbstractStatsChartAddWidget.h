#ifndef DAABSTRACTSTATSCHARTADDWIDGET_H
#define DAABSTRACTSTATSCHARTADDWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartAddItemWidget.h"
#include <QJsonObject>

namespace DA
{
class DAFigureWidget;
class DAChartWidget;

/**
 * @brief 统计绘图设置窗口基类
 *
 * 统计绘图不走 DAAbstractChartAddItemWidget::createPlotItem() 契约（不返回 QwtPlotItem*），
 * 而是收集参数为 QJsonObject，通过 plotRequested 信号发射给 Controller，
 * 由 Controller 调用 Python 统计绘图函数完成计算和绘图。
 */
class DAGUI_API DAAbstractStatsChartAddWidget : public DAAbstractChartAddItemWidget
{
    Q_OBJECT
public:
    explicit DAAbstractStatsChartAddWidget(QWidget* parent = nullptr);
    virtual ~DAAbstractStatsChartAddWidget();

    /**
     * @brief 统计绘图不实现 createPlotItem()，此函数始终返回 nullptr
     * @note 统计绘图走 Python 路线，子类应实现 buildPlotParams() 并发射 plotRequested 信号
     */
    virtual QwtPlotItem* createPlotItem() override;

    /**
     * @brief 收集当前窗口中的参数为 JSON 对象
     * @return 参数 JSON
     */
    virtual QJsonObject buildPlotParams() const = 0;

    /**
     * @brief 用户确认时的处理逻辑（验证数据、设置属性、发射信号）
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
     * @param fig 目标 Figure 窗口
     * @param chart 目标 Chart 窗口
     */
    void plotRequested(const QJsonObject& params, DA::DAFigureWidget* fig, DA::DAChartWidget* chart);

protected:
    DAFigureWidget* mFigureWidget { nullptr };
    DAChartWidget* mChartWidget { nullptr };
};

}  // namespace DA

#endif  // DAABSTRACTSTATSCHARTADDWIDGET_H
