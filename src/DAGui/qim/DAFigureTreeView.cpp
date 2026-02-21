#include "DAFigureTreeView.h"
#include <QPointer>
#include <QMouseEvent>
#include "DAFigureScrollArea.h"
#include "DAPlotTreeModel.h"
#include "plot/QImPlotItemNode.h"
#include "plot/QImPlotNode.h"
#include "plot/QImPlotAxisInfo.h"
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
    DAPlotTreeModel* figureModel() const;
    QPointer< DAFigureScrollArea > mFigureWidget;
    bool isAutoResizeColumnToContents { true };  ///< 是否自动刷新内容
};

DAFigureTreeView::PrivateData::PrivateData(DAFigureTreeView* p) : q_ptr(p)
{
}

DAPlotTreeModel* DAFigureTreeView::PrivateData::figureModel() const
{
    return qobject_cast< DAPlotTreeModel* >(q_ptr->model());
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
    DAPlotTreeModel* m = new DAPlotTreeModel(this);
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
void DAFigureTreeView::setFigureWidget(DAFigureScrollArea* fig)
{
    if (fig == d_ptr->mFigureWidget) {
        return;
    }
    DAPlotTreeModel* m = d_ptr->figureModel();
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
DAFigureScrollArea* DAFigureTreeView::getFigureWidget() const
{
    return d_ptr->mFigureWidget.data();
}

void DAFigureTreeView::refresh()
{
    DAPlotTreeModel* m = d_ptr->figureModel();
    if (!m) {
        return;
    }
    m->refresh();
    expandAll();
}

DAPlotTreeModel* DAFigureTreeView::getFigureTreeModel() const
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

    DAPlotTreeModel* model = getFigureTreeModel();
    if (!model) {
        return;
    }
    QStandardItem* item               = model->itemFromIndex(index);
    const DAPlotTreeItemType nodeType = DAPlotTreeModel::itemType(item);

    DAFigureScrollArea* fig = getFigureWidget();
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
    case DAPlotTreeItemType::Plot: {
        QIM::QImPlotNode* plot = DAPlotTreeModel::pointerFromItem< QIM::QImPlotNode* >(item);
        DAFigureElementSelection sel(fig, plot, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        break;
    }
    case DAPlotTreeItemType::Axis: {
        QIM::QImPlotAxisInfo* axis = DAPlotTreeModel::pointerFromItem< QIM::QImPlotAxisInfo* >(item);
        QIM::QImPlotNode* plot     = axis ? axis->plotNode() : nullptr;
        DAFigureElementSelection sel(fig, plot, axis, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        break;
    }
    case DAPlotTreeItemType::PlotItem: {
        QIM::QImPlotItemNode* plotitem = DAPlotTreeModel::pointerFromItem< QIM::QImPlotItemNode* >(item);
        QIM::QImPlotNode* plot         = plotitem ? plotitem->plotNode() : nullptr;
        DAFigureElementSelection sel(fig, plot, plotitem, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        // emitPlotItemClick(plotItem, plot, item, doubleClicked);

        /* 颜色列点击 -> 发信号 */
        if (index.column() == 2 && plotitem) {
            Q_EMIT requestItemChangeColor(plotitem, item);
        }
        break;
    }
    default:
        break;
    }
}

}
