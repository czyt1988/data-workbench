#ifndef DACHARTMANAGEWIDGET_H
#define DACHARTMANAGEWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include "DAFigureElementSelection.h"
#include "qwt_axis_id.h"
namespace Ui
{
class DAChartManageWidget;
}

// qt
class QStandardItem;
// qwt
class QwtPlot;
class QwtPlotItem;

namespace DA
{
class DAChartOperateWidget;
class DAFigureWidget;
class DAChartWidget;
class DAChartItemStandardItem;
class DAChartWidgetStandardItem;
/**
 * @brief 绘图管理窗口
 */
class DAGUI_API DAChartManageWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartManageWidget)
public:
    DAChartManageWidget(QWidget* parent = nullptr);
    ~DAChartManageWidget();
    // 设置绘图炒作窗口，只有设置了绘图操作窗口，管理窗口才可以工作
    void setChartOperateWidget(DAChartOperateWidget* cow);
    // 设置item点击时如果不是当前chart，设置为当前chart
    void setCurrentChartOnItemClicked(bool on);
    bool isSetCurrentChartOnItemClicked() const;
    // 设置item双击时如果不是当前chart，设置为当前chart
    void setCurrentChartOnItemDoubleClicked(bool on);
    bool isSetCurrentChartOnItemDoubleClicked() const;
    // 通过plot获取figure
    DAFigureWidget* plotToFigureWidget(QwtPlot* plot) const;
Q_SIGNALS:
    /**
     * @brief 绘图元素选中信号
     * @param selection 选中的内容
     * @sa DAFigureElementSelection
     */
    void figureElementClicked(const DAFigureElementSelection& selection);
    void figureElementDbClicked(const DAFigureElementSelection& selection);

private:
    // 设置当前显示的fig对应的view
    void setCurrentDisplayView(DA::DAFigureWidget* fig);
private slots:
    void onFigureCreated(DA::DAFigureWidget* fig);
    void onFigureCloseing(DA::DAFigureWidget* fig);
    void onCurrentFigureChanged(DA::DAFigureWidget* fig, int index);
    // 点击了plotitem，这里要把信号转发出去
    void onPlotClicked(QwtPlot* plot, QStandardItem* treeItem);
    void onPlotItemClicked(QwtPlotItem* item, QwtPlot* plot, QStandardItem* treeItem);
    void onAxisClicked(QwtAxisId axisId, QwtPlot* plot, QStandardItem* treeItem);

private:
    Ui::DAChartManageWidget* ui;
};
}  // end of namespace DA
#endif  // DACHARTMANAGEWIDGET_H
