#include "DAFigureTreeView.h"
#include <QPointer>
#include <QMouseEvent>
#include <QDropEvent>
#include "Models/DAFigureTreeModel.h"
#include "DAFigureWidget.h"
#include "DAFigureWidgetCommands.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_plot_item.h"
#include "qwt3d_plotitem.h"
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
    // 启用拖拽：PlotItem可在不同绘图节点间拖动
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    // 连接点击/双击
    connect(this, &QTreeView::clicked, this, &DAFigureTreeView::onClicked);
    connect(this, &QTreeView::doubleClicked, this, &DAFigureTreeView::onDoubleClicked);
    DAFigureTreeModel* m = new DAFigureTreeModel(this);
    connect(m, &DAFigureTreeModel::chartItemAttached, this, &DAFigureTreeView::onChartItemAttacted);
    connect(m, &DAFigureTreeModel::chart3DItemAttached, this, [this](Qwt3DPlotItem* item, bool on) {
        if (on && isAutoResizeColumnToContents()) {
            resizeHeaderToContents();
        }
    });
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
    m->setFigureWidget(fig);
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
 * @brief 刷新指定plotItem的可见性列显示
 * @param item 需要刷新的plotItem
 */
void DAFigureTreeView::refreshPlotItemVisibility(QwtPlotItem* item)
{
    DAFigureTreeModel* m = d_ptr->figureModel();
    if (m) {
        m->notifyPlotItemVisibilityChanged(item);
    }
}

/**
 * @brief 刷新指定坐标轴的可见性列显示
 * @param plot 坐标轴所在的plot
 * @param axisId 坐标轴ID
 */
void DAFigureTreeView::refreshAxisVisibility(QwtPlot* plot, QwtAxisId axisId)
{
    DAFigureTreeModel* m = d_ptr->figureModel();
    if (m) {
        m->notifyAxisVisibilityChanged(plot, axisId);
    }
}

/**
 * @brief 刷新指定plotItem的文字列显示（用于重命名后）
 * @param item 需要刷新的plotItem
 */
void DAFigureTreeView::refreshPlotItemText(QwtPlotItem* item)
{
    DAFigureTreeModel* m = d_ptr->figureModel();
    if (m) {
        m->notifyPlotItemTextChanged(item);
    }
}

/**
 * @brief 刷新指定坐标轴的文字列显示（用于重命名后）
 * @param plot 坐标轴所在的plot
 * @param axisId 坐标轴ID
 */
void DAFigureTreeView::refreshAxisText(QwtPlot* plot, QwtAxisId axisId)
{
    DAFigureTreeModel* m = d_ptr->figureModel();
    if (m) {
        m->notifyAxisTextChanged(plot, axisId);
    }
}

/**
 * @brief 刷新指定chart节点的文字列显示（用于重命名后）
 * @param plot chart对应的plot
 */
void DAFigureTreeView::refreshPlotFolderText(QwtPlot* plot)
{
    DAFigureTreeModel* m = d_ptr->figureModel();
    if (m) {
        m->notifyPlotFolderTextChanged(plot);
    }
}

void DAFigureTreeView::refresh3DPlotItemVisibility(Qwt3DPlotItem* item)
{
    DAFigureTreeModel* m = d_ptr->figureModel();
    if (m) {
        m->notify3DPlotItemVisibilityChanged(item);
    }
}

void DAFigureTreeView::refresh3DPlotItemText(Qwt3DPlotItem* item)
{
    DAFigureTreeModel* m = d_ptr->figureModel();
    if (m) {
        m->notify3DPlotItemTextChanged(item);
    }
}

void DAFigureTreeView::refresh3DPlot3DText(DAChart3DWidget* chart)
{
    DAFigureTreeModel* m = d_ptr->figureModel();
    if (m) {
        m->notify3DPlot3DTextChanged(chart);
    }
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
    // === 3D 节点处理 ===
    case DAFigureTreeModel::NodeTypePlot3DFolder: {
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        DAFigureElementSelection sel(fig, chart3D, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        break;
    }
    case DAFigureTreeModel::NodeTypePlot3D: {
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        DAFigureElementSelection sel(fig, chart3D, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        break;
    }
    case DAFigureTreeModel::NodeTypePlot3DAxis: {
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        int axis3DId = item->data(DAFigureTreeModel::RoleAxis3DId).toInt();
        DAFigureElementSelection sel(fig, chart3D, axis3DId, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        break;
    }
    case DAFigureTreeModel::NodeTypePlot3DItem: {
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        Qwt3DPlotItem* plot3DItem = model->plot3DItemFromItem(item);
        DAFigureElementSelection sel(fig, chart3D, plot3DItem, col);
        doubleClicked ? Q_EMIT itemDbCliecked(sel) : Q_EMIT itemCliecked(sel);
        break;
    }
    default:
        break;
    }
}

void DAFigureTreeView::dropEvent(QDropEvent* event)
{
    if (event->source() != this) {
        QTreeView::dropEvent(event);
        return;
    }

    DAFigureTreeModel* model = getFigureTreeModel();
    DAFigureWidget* fig      = getFigureWidget();
    if (!model || !fig) {
        event->ignore();
        return;
    }

    // 获取拖拽源——选中的PlotItem（column 0）
    QModelIndexList selected = selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        event->ignore();
        return;
    }
    QModelIndex sourceCol0 = model->index(selected.first().row(), 0, selected.first().parent());
    if (model->itemType(model->itemFromIndex(sourceCol0)) != DAFigureTreeModel::NodeTypePlotItem) {
        event->ignore();
        return;
    }
    QwtPlotItem* plotItem = model->plotItemFromIndex(sourceCol0);
    QwtPlot* sourcePlot   = model->plotFromIndex(sourceCol0);
    if (!plotItem || !sourcePlot) {
        event->ignore();
        return;
    }

    // 获取放置目标
    QModelIndex dropIndex = indexAt(event->pos());
    if (!dropIndex.isValid()) {
        event->ignore();
        return;
    }
    QModelIndex targetCol0  = model->index(dropIndex.row(), 0, dropIndex.parent());
    QStandardItem* targetItem = model->itemFromIndex(targetCol0);
    if (!targetItem) {
        event->ignore();
        return;
    }
    int targetType = model->itemType(targetItem);
    QwtPlot* targetPlot = nullptr;
    switch (targetType) {
    case DAFigureTreeModel::NodeTypePlotFolder:
    case DAFigureTreeModel::NodeTypePlot:
    case DAFigureTreeModel::NodeTypeItemsFolder:
    case DAFigureTreeModel::NodeTypePlotItem:
        targetPlot = model->plotFromItem(targetItem);
        break;
    default:
        break;
    }
    if (!targetPlot || targetPlot == sourcePlot) {
        event->ignore();
        return;
    }

    // 执行移动（通过undo命令）
    DAChartWidget* sourceChart = qobject_cast< DAChartWidget* >(sourcePlot);
    DAChartWidget* targetChart = qobject_cast< DAChartWidget* >(targetPlot);
    if (!sourceChart || !targetChart) {
        event->ignore();
        return;
    }

    fig->push(new DAFigureWidgetCommandMoveItem(fig, sourceChart, targetChart, plotItem));

    // 不调用基类的dropEvent，防止QStandardItemModel移动行
    // 树模型已通过QwtPlot::itemAttached信号自动更新
    // 忽略事件使QDrag::exec返回IgnoreAction，阻止startDrag中的clearOrRemove
    event->ignore();

    // 选中移动后的条目
    QModelIndex newIdx = model->indexFromPlotItem(plotItem);
    if (newIdx.isValid()) {
        setCurrentIndex(newIdx);
    }
}

}
