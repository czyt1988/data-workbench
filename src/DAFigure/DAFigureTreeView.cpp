#include "DAFigureTreeView.h"
#include <QPointer>
#include <QMouseEvent>
#include "Models/DAFigureTreeModel.h"
#include "DAFigureWidget.h"
#include "qwt_figure.h"
namespace DA
{
//==============================================================
// DAFigureTreeViewPrivate
//==============================================================

class DAFigureTreeView::PrivateData
{
public:
    DA_DECLARE_PUBLIC(DAFigureTreeView)
    PrivateData(DAFigureTreeView* p);
    DAFigureTreeModel* figureModel() const;
    QPointer< DAFigureWidget > mFigureWidget;
    bool isAutoResizeColumnToContents { true };  ///< 是否自动刷新内容
};

DAFigureTreeView::PrivateData::PrivateData(DAFigureTreeView* p) : q_ptr(p)
{
}

DAFigureTreeModel* DAFigureTreeView::PrivateData::figureModel() const
{
    return qobject_cast< DAFigureTreeModel* >(q_ptr->model());
}
//==============================================================
// DAFigureTreeView
//==============================================================

DAFigureTreeView::DAFigureTreeView(QWidget* parent) : QTreeView(parent), DA_PIMPL_CONSTRUCT
{
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    // 连接点击/双击
    connect(this, &QTreeView::clicked, this, &DAFigureTreeView::onClicked);
    connect(this, &QTreeView::doubleClicked, this, &DAFigureTreeView::onDoubleClicked);
    DAFigureTreeModel* m = new DAFigureTreeModel(this);
    connect(m, &DAFigureTreeModel::chartItemAttached, this, &DAFigureTreeView::onChartItemAttacted);
    setModel(m);
    // 列宽自适应
    setColumnWidth(0, 200);
    setColumnWidth(1, 40);
    setColumnWidth(2, 40);
}

DAFigureTreeView::~DAFigureTreeView()
{
}

/**
 * @brief 设置fig
 * @param fig
 */
void DAFigureTreeView::setFigureWidget(DA::DAFigureWidget* fig)
{
    if (fig == d_ptr->mFigureWidget) {
        return;
    }
    DAFigureTreeModel* m = d_ptr->figureModel();
    if (!m) {
        return;
    }
    m->setFigure(fig->figure());
    d_ptr->mFigureWidget = fig;
    expandAll();
}

/**
 * @brief 获取管理的窗口
 * @return
 */
DAFigureWidget* DAFigureTreeView::getFigureWidget() const
{
    return d_ptr->mFigureWidget.data();
}

void DAFigureTreeView::refresh()
{
    DAFigureTreeModel* m = d_ptr->figureModel();
    if (!m) {
        return;
    }
    m->refresh();
    expandAll();
}

DAFigureTreeModel* DAFigureTreeView::getFigureTreeModel() const
{
    return d_ptr->figureModel();
}

/**
 * @brief 是否自动适应内容
 * @return
 */
bool DAFigureTreeView::isAutoResizeColumnToContents() const
{
    return d_ptr->isAutoResizeColumnToContents;
}

/**
 * @brief 设置自动适应内容
 * @return
 */
void DAFigureTreeView::setAutoResizeColumnToContents(bool on)
{
    d_ptr->isAutoResizeColumnToContents = on;
}

/**
 * @brief 让树形控件的水平头自适应内容
 */
void DAFigureTreeView::resizeHeaderToContents()
{
    auto m = model();
    if (!m) {
        return;
    }
    for (int i = 0, n = m->columnCount(); i < n; ++i) {
        resizeColumnToContents(i);
    }
}

void DAFigureTreeView::onChartItemAttacted(QwtPlotItem* item, bool on)
{
    if (on && isAutoResizeColumnToContents()) {
        // 注意，只有on的时候才触发，否则删除的时候也触发会导致standarditem操作item或者plot导致崩溃
        resizeHeaderToContents();
    }
}

void DAFigureTreeView::onClicked(const QModelIndex& index)
{
    handleClicked(index, false);
}

void DAFigureTreeView::onDoubleClicked(const QModelIndex& index)
{
    handleClicked(index, true);
}

void DAFigureTreeView::handleClicked(const QModelIndex& index, bool doubleClicked)
{
    if (!index.isValid()) {
        return;
    }

    DAFigureTreeModel* model = getFigureTreeModel();
    if (!model) {
        return;
    }
    QStandardItem* item = model->itemFromIndex(index);

    const int nodeType  = item->data(DAFigureTreeModel::RoleNodeType).toInt();
    DAFigureWidget* fig = getFigureWidget();
    if (!fig) {
        return;
    }
    DAFigureElementSelection::SelectionColumns col { DAFigureElementSelection::ColumnName };
    switch (index.column()) {
    case 0:
        col = DAFigureElementSelection::ColumnName;
        break;
    case 1:
        col = DAFigureElementSelection::ColumnVisible;
        break;
    case 2:
        col = DAFigureElementSelection::ColumnProperty;
        break;
    default:
        break;
    }
    switch (nodeType) {
    case DAFigureTreeModel::NodeTypePlotFolder: {
        QwtPlot* plot = model->plotFromItem(item);
        DAFigureElementSelection sel(fig, plot, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        break;
    }
    case DAFigureTreeModel::NodeTypePlot: {
        QwtPlot* plot = model->plotFromItem(item);
        DAFigureElementSelection sel(fig, plot, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        break;
    }
    case DAFigureTreeModel::NodeTypeAxis: {
        QwtPlot* plot = model->plotFromItem(item);
        int axisId    = model->axisIdFromItem(item);
        DAFigureElementSelection sel(fig, plot, plot->axisWidget(axisId), axisId, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        // emitAxisClick(axisId, plot, item, doubleClicked);
        break;
    }
    case DAFigureTreeModel::NodeTypePlotItem: {
        QwtPlot* plot         = model->plotFromItem(item);
        QwtPlotItem* plotItem = model->plotItemFromItem(item);
        DAFigureElementSelection sel(fig, plot, plotItem, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        // emitPlotItemClick(plotItem, plot, item, doubleClicked);

        /* 颜色列点击 -> 发信号 */
        if (index.column() == 2 && plotItem) {
            Q_EMIT requestItemChangeColor(plotItem, item);
        }
        break;
    }
    default:
        break;
    }
}

}
