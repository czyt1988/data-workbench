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
class DAFigureTreeView;
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
    // 设置绘图操作窗口，只有设置了绘图操作窗口，管理窗口才可以工作
    void setChartOperateWidget(DAChartOperateWidget* cow);
    // 设置item点击时如果不是当前chart，设置为当前chart
    void setCurrentChartOnItemClicked(bool on);
    bool isSetCurrentChartOnItemClicked() const;
    // 设置item双击时如果不是当前chart，设置为当前chart
    void setCurrentChartOnItemDoubleClicked(bool on);
    bool isSetCurrentChartOnItemDoubleClicked() const;
    // 通过plot获取figure
    DAFigureWidget* plotToFigureWidget(QwtPlot* plot) const;
    // 获取当前的figure
    DAFigureWidget* getCurrentFigure() const;
    // 刷新指定plotItem的可见性列显示（作用于当前treeView）
    void refreshPlotItemVisibility(QwtPlotItem* item);
    // 刷新指定坐标轴的可见性列显示（作用于当前treeView）
    void refreshAxisVisibility(QwtPlot* plot, QwtAxisId axisId);
    // 刷新指定plotItem的文字列显示（用于重命名后）
    void refreshPlotItemText(QwtPlotItem* item);
    // 刷新指定坐标轴的文字列显示（用于重命名后）
    void refreshAxisText(QwtPlot* plot, QwtAxisId axisId);
    // 刷新指定chart节点的文字列显示（用于重命名后）
    void refreshPlotFolderText(QwtPlot* plot);
public Q_SLOTS:
    // 把管理树展开
    void expandCurrentTree();
    // 把管理树收起
    void collapseCurrentTree();
Q_SIGNALS:
    /**
     * @brief 绘图元素选中信号
     * @param selection 选中的内容
     * @sa DAFigureElementSelection
     */
    void figureElementClicked(const DAFigureElementSelection& selection);
    void figureElementDbClicked(const DAFigureElementSelection& selection);
    /**
     * @brief 请求绘图的设置
     * @param fig
     */
    void requestFigureSetting(DA::DAFigureWidget* fig);

    /**
     * @brief 当前选中的绘图发生了改变
     * @param fig
     */
    void selectFigureChanged(DA::DAFigureWidget* fig);

protected:
    // 获取当前的tree
    DAFigureTreeView* currentTreeView() const;

private:
    // 设置当前显示的fig对应的view
    void setCurrentDisplayView(DA::DAFigureWidget* fig);
    DAFigureWidget* getComboboxFigure(int index) const;
    void setStackCurrentFigure(DA::DAFigureWidget* fig);
    void setComboboxCurrentFigure(DA::DAFigureWidget* fig);
private Q_SLOTS:
    void onFigureCreated(DA::DAFigureWidget* fig);
    void onFigureCloseing(DA::DAFigureWidget* fig);
    void onCurrentFigureChanged(DA::DAFigureWidget* fig, int index);
    // 点击了plotitem，这里要把信号转发出去
    void onPlotClicked(QwtPlot* plot, QStandardItem* treeItem);
    void onPlotItemClicked(QwtPlotItem* item, QwtPlot* plot, QStandardItem* treeItem);
    void onAxisClicked(QwtAxisId axisId, QwtPlot* plot, QStandardItem* treeItem);
    // 绘图设置窗口点击
    void onToolButtonFigureSettingClicked();
    // combobox
    void onComboboxCurrentIndexChanged(int index);
    // 树形控件右键菜单请求
    void onTreeViewContextMenuRequested(DA::DAFigureTreeView* tree, const QPoint& pos);
    // 右键菜单动作触发
    void onContextMenuRenameTriggered();
    void onContextMenuVisibleTriggered(bool on);
    void onContextMenuDeleteTriggered();
    void onContextMenuSettingTriggered();

private:
    Ui::DAChartManageWidget* ui;
};
}  // end of namespace DA
#endif  // DACHARTMANAGEWIDGET_H
