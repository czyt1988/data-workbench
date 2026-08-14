#ifndef DAFIGUREDOCKWIDGET_H
#define DAFIGUREDOCKWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "DAGlobals.h"
namespace DA
{
class DAFigureWidget;
DA_IMPL_FORWARD_DECL(DAFigureDockWidget)
/**
 * @brief 绘图停靠窗口部件
 *
 * DAFigureWidget 的停靠封装层，继承 QWidget（非 QDockWidget / ads::CDockWidget）。
 * 本项目使用 Qt-Advanced-Docking-System (ADS)，DAChartOperateWidget 内部持有一个
 * 嵌套的 ads::CDockManager，把本 QWidget 包装进 ads::CDockWidget 后加入停靠区。
 * 若继承 QDockWidget / ads::CDockWidget 再交给停靠管理器，会出现双标题栏、
 * float/拖拽冲突。
 *
 * @note 本类继承 QWidget，作为 figure 在停靠区的容器层
 */
class DAGUI_API DAFigureDockWidget : public QWidget
{
    Q_OBJECT
    DA_IMPL(DAFigureDockWidget)
public:
    // 构造，fig 为被封装的绘图窗口，构造后由本对象接管其布局父级（reparent）
    explicit DAFigureDockWidget(DAFigureWidget* fig, QWidget* parent = nullptr);
    ~DAFigureDockWidget();
    // 获取封装的绘图窗口
    DAFigureWidget* getFigureWidget() const;
};
}  // namespace DA
#endif  // DAFIGUREDOCKWIDGET_H
