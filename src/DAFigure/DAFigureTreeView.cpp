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
    setModel(new DAFigureTreeModel(this));
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

    const int nodeType = item->data(DAFigureTreeModel::RoleNodeType).toInt();

    switch (nodeType) {
    case DAFigureTreeModel::NodeTypePlot: {
        QwtPlot* plot = model->plotFromItem(item);
        emitPlotClick(plot, item, doubleClicked);
        break;
    }
    case DAFigureTreeModel::NodeTypeAxis: {
        QwtPlot* plot = model->plotFromItem(item);
        int axisId    = model->axisIdFromItem(item);
        emitAxisClick(axisId, plot, doubleClicked);
        break;
    }
    case DAFigureTreeModel::NodeTypePlotItem: {
        QwtPlot* plot         = model->plotFromItem(item);
        QwtPlotItem* plotItem = model->plotItemFromItem(item);
        emitPlotItemClick(plotItem, plot, item, doubleClicked);

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

void DAFigureTreeView::emitPlotClick(QwtPlot* plot, QStandardItem* item, bool db)
{
    db ? Q_EMIT plotDoubleClicked(plot, item) : Q_EMIT plotClicked(plot, item);
}

void DAFigureTreeView::emitAxisClick(int axisId, QwtPlot* plot, bool db)
{
    db ? Q_EMIT axisDoubleClicked(axisId, plot) : Q_EMIT axisClicked(axisId, plot);
}

void DAFigureTreeView::emitPlotItemClick(QwtPlotItem* item, QwtPlot* plot, QStandardItem* treeItem, bool db)
{
    db ? Q_EMIT plotItemDoubleClicked(item, plot, treeItem) : Q_EMIT plotItemClicked(item, plot, treeItem);
}

}
