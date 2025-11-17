#ifndef DAFIGURETREEVIEW_H
#define DAFIGURETREEVIEW_H
#include "DAFigureAPI.h"
#include <QTreeView>
#include "qwt_axis_id.h"

class QwtPlot;
class QwtPlotItem;
class QStandardItem;
namespace DA
{
class DAFigureWidget;
class DAFigureTreeModel;

/**
 * @brief 绘图树
 */
class DAFIGURE_API DAFigureTreeView : public QTreeView
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigureTreeView)
public:
    DAFigureTreeView(QWidget* parent = nullptr);
    ~DAFigureTreeView();
    // 设置figure
    void setFigureWidget(DAFigureWidget* fig);
    DAFigureWidget* getFigureWidget() const;
    // 刷新
    void refresh();
    // 获取内部模型
    DAFigureTreeModel* getFigureTreeModel() const;
Q_SIGNALS:
    // 请求改变图元颜色的信号
    void requestItemChangeColor(QwtPlotItem* item, QStandardItem* treeItem);

    // 单击信号
    void plotClicked(QwtPlot* plot, QStandardItem* treeItem);
    void axisClicked(QwtAxisId axisId, QwtPlot* plot);
    void plotItemClicked(QwtPlotItem* item, QwtPlot* plot, QStandardItem* treeItem);

    // 双击信号
    void plotDoubleClicked(QwtPlot* plot, QStandardItem* treeItem);
    void axisDoubleClicked(QwtAxisId axisId, QwtPlot* plot);
    void plotItemDoubleClicked(QwtPlotItem* item, QwtPlot* plot, QStandardItem* treeItem);

private Q_SLOTS:
    void onClicked(const QModelIndex& index);
    void onDoubleClicked(const QModelIndex& index);

private:
    void handleClicked(const QModelIndex& index, bool doubleClicked);
    void emitPlotClick(QwtPlot* plot, QStandardItem* item, bool db);
    void emitAxisClick(int axisId, QwtPlot* plot, bool db);
    void emitPlotItemClick(QwtPlotItem* item, QwtPlot* plot, QStandardItem* treeItem, bool db);
};
}

#endif  // DAFIGURETREEVIEW_H
