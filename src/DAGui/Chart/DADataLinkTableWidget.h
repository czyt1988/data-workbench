#ifndef DADATALINKTABLEWIDGET_H
#define DADATALINKTABLEWIDGET_H
#include <QWidget>
#include <QPointer>
#include <QHash>
#include "DAGuiAPI.h"

class QTabWidget;
class QwtPlotItem;
namespace DA
{
class DADataLinkTableTab;
class DAFigureWidget;
class DAChartOperateWidget;
class DADataProbeMarker;
}

/**
 * @brief 数据联动表容器（每个 DAFigureWidget 一个 tab）
 *
 * 纯视图架构：监听各 figure 的探针创建信号，自动维护 tab 与表格内容；
 * 不持有独立的持久化状态，工程加载后探针恢复时表格随之自动重建。
 * 需要 APP 层调用 setChartOperateWidget 接入绘图生命周期。
 */
namespace DA
{
class DAGUI_API DADataLinkTableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DADataLinkTableWidget(QWidget* parent = nullptr);
    ~DADataLinkTableWidget() override;

    // 接入绘图操作窗口，自动跟踪 figure 的创建与销毁
    void setChartOperateWidget(DA::DAChartOperateWidget* chartOpt);

    // figure 加载完成后由上层调用（工程加载/图表恢复完成时机），重建对应 tab 内容
    void refreshFigure(DA::DAFigureWidget* figWidget);

signals:
    // 有新的探针列产生（探针模式点击），提示上层把本窗口的 dock 提到前台
    void requestRaise();

private slots:
    void onFigureCreated(DA::DAFigureWidget* figWidget);
    void onFigureRemoving(DA::DAFigureWidget* figWidget);
    void onProbeCreated(DA::DAFigureWidget* figWidget, DA::DADataProbeMarker* probe);

private:
    DADataLinkTableTab* ensureTab(DA::DAFigureWidget* figWidget);
    int findTabIndex(DA::DAFigureWidget* figWidget) const;

private:
    QTabWidget* m_tabWidget { nullptr };
    QPointer< DA::DAChartOperateWidget > m_chartOpt;
    QHash< DA::DAFigureWidget*, DADataLinkTableTab* > m_tabs;
};
}  // namespace DA

#endif  // DADATALINKTABLEWIDGET_H
