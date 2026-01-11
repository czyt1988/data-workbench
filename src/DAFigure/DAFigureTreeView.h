#ifndef DAFIGURETREEVIEW_H
#define DAFIGURETREEVIEW_H
#include "DAFigureAPI.h"
#include <QTreeView>
#include "qwt_axis_id.h"
#include "DAFigureElementSelection.h"
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
    // 设置自动适应内容
    bool isAutoResizeColumnToContents() const;
    void setAutoResizeColumnToContents(bool on);
public Q_SLOTS:
    // 让树形控件的水平头自适应内容
    void resizeHeaderToContents();

Q_SIGNALS:
    // 请求改变图元颜色的信号
    void requestItemChangeColor(QwtPlotItem* item, QStandardItem* treeItem);
    void itemCliecked(const DAFigureElementSelection& ele);
    void itemDbCliecked(const DAFigureElementSelection& ele);
private Q_SLOTS:
    void onClicked(const QModelIndex& index);
    void onDoubleClicked(const QModelIndex& index);
    // chartitem attacted
    void onChartItemAttacted(QwtPlotItem* item, bool on);

private:
    void handleClicked(const QModelIndex& index, bool doubleClicked);
};
}

#endif  // DAFIGURETREEVIEW_H
