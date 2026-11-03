#ifndef DAABSTRACTSTATSCHARTADDWIDGET_H
#define DAABSTRACTSTATSCHARTADDWIDGET_H
#include "DAGuiAPI.h"
#include "Chart/AddItem/DAAbstractChartAddWidgetBase.h"
#include "DAData.h"

namespace DA
{
class DAFigureWidget;
class DAChartWidget;
class DAStatsPlotRequest;

/**
 * @brief 统计绘图设置窗口基类
 *
 * 统计绘图不返回 QwtPlotItem，而是把参数收集为类型化的 DAStatsPlotRequest、
 * 连同数据源一起通过 plotRequested 信号交给 Controller，由 Controller 调用
 * Python 统计绘图函数完成计算和绘制。
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
     * @brief 从当前 UI 收集类型化绘图请求（工厂方法）
     * @return 新建的请求对象，所有权移交基类 onButtonBoxAccepted 的 unique_ptr
     */
    virtual DAStatsPlotRequest* createPlotRequest() const = 0;

    /**
     * @brief 用户确认时的处理逻辑（非虚模板方法：校验请求并发射绘图信号）
     * @note 由引导对话框在 accept 时调用，子类不得重写
     */
    void onButtonBoxAccepted();

    // 设置/获取 Figure 窗口
    void setFigureWidget(DAFigureWidget* fig);
    DAFigureWidget* getFigureWidget() const;

    // 设置/获取 Chart 窗口
    void setChartWidget(DAChartWidget* chart);
    DAChartWidget* getChartWidget() const;

Q_SIGNALS:
    /**
     * @brief 绘图请求信号，基类在用户确认且请求有效后发射
     * @param req 类型化绘图请求（携带参数、数据源与目标 figure/chart）
     * @note 同步直连；请求对象生命周期仅限信号链内，不得在槽中保存引用
     */
    void plotRequested(const DA::DAStatsPlotRequest& req);

protected:
    DAFigureWidget* mFigureWidget { nullptr };
    DAChartWidget* mChartWidget { nullptr };
};

}  // namespace DA

#endif  // DAABSTRACTSTATSCHARTADDWIDGET_H
