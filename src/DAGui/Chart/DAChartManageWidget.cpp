#include "DAChartManageWidget.h"
#include "ui_DAChartManageWidget.h"
#include <functional>
#include <QTreeView>
#include <QPointer>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include "DALogCategory.h"
#include <QHash>
#include <QSet>
#include "DAChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAFigureTreeView.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_plot_item.h"
#include "qwt_text.h"
#include "qwt3d_plotitem.h"
#include "qwt3d_types.h"
#include "Models/DAFigureTreeModel.h"
namespace DA
{
//===============================================================
// PrivateData
//===============================================================
class DAChartManageWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartManageWidget)
public:
    PrivateData(DAChartManageWidget* p);
    DAFigureElementSelection::SelectionColumns standardItemToSelectionColumns(QStandardItem* item);
    // 懒加载构建树形控件右键菜单（仅构建一次，后续复用）
    void ensureTreeContextMenu(DAChartManageWidget* q);
    // 根据节点类型更新右键菜单各 action 的可见性
    void updateContextMenuActions(int nodeType);

public:
    QPointer< DAChartOperateWidget > mChartOptWidget;
    bool mSetCurrentChartOnClicked { true };
    bool mSetCurrentChartOnDbClicked { true };
    QHash< QwtFigure*, DAFigureWidget* > mFigToFigWidget;             ///< 建立figure和figureWidget的关系
    QHash< DAFigureWidget*, DAFigureTreeView* > mFigureWidgetToTree;  ///< 建立figurewidget和tree的关系

    // 右键菜单（懒加载复用，避免每次右键都栈上构造）
    QMenu* mTreeContextMenu { nullptr };
    QAction* mActRename { nullptr };
    QAction* mActVisible { nullptr };
    QAction* mActDelete { nullptr };
    QAction* mActSetting { nullptr };
    // 当前右键上下文（action 触发时使用）
    DAFigureTreeView* mContextTree { nullptr };
    DAFigureWidget* mContextFigureWidget { nullptr };
    QStandardItem* mContextItem { nullptr };
    int mContextNodeType { -1 };
};
DAChartManageWidget::PrivateData::PrivateData(DAChartManageWidget* p) : q_ptr(p)
{
}

DAFigureElementSelection::SelectionColumns DAChartManageWidget::PrivateData::standardItemToSelectionColumns(QStandardItem* item)
{
    switch (item->column()) {
    case 0:
        return DAFigureElementSelection::ColumnName;
    case 1:
        return DAFigureElementSelection::ColumnVisible;
    case 2:
        return DAFigureElementSelection::ColumnProperty;
    default:
        break;
    }
    return DAFigureElementSelection::ColumnName;
}

/**
 * @brief 懒加载构建树形控件右键菜单
 *
 * 首次调用时创建 QMenu 及所有 QAction 并连接信号槽，后续调用直接复用。
 * 菜单项的显隐在 @ref updateContextMenuActions 中根据节点类型动态控制。
 */
void DAChartManageWidget::PrivateData::ensureTreeContextMenu(DAChartManageWidget* q)
{
    if (mTreeContextMenu) {
        return;
    }
    mTreeContextMenu = new QMenu(q);
    mActRename  = mTreeContextMenu->addAction(tr("Rename"));   // cn:重命名
    mActVisible = mTreeContextMenu->addAction(tr("Visible"));  // cn:可见
    mActVisible->setCheckable(true);
    mActDelete  = mTreeContextMenu->addAction(tr("Delete"));   // cn:删除
    mTreeContextMenu->addSeparator();
    mActSetting = mTreeContextMenu->addAction(tr("Setting"));  // cn:设置
    QObject::connect(mActRename, &QAction::triggered, q, &DAChartManageWidget::onContextMenuRenameTriggered);
    QObject::connect(mActVisible, &QAction::triggered, q, &DAChartManageWidget::onContextMenuVisibleTriggered);
    QObject::connect(mActDelete, &QAction::triggered, q, &DAChartManageWidget::onContextMenuDeleteTriggered);
    QObject::connect(mActSetting, &QAction::triggered, q, &DAChartManageWidget::onContextMenuSettingTriggered);
}

/**
 * @brief 根据节点类型更新右键菜单各 action 的可见性
 *
 * - chart节点(NodeTypePlotFolder): 重命名、设置
 * - layer节点(NodeTypePlot): 无菜单（调用方在外部判断，不进入此函数）
 * - 坐标轴节点(NodeTypeAxis): 重命名、可见、设置
 * - 图元节点(NodeTypePlotItem): 重命名、可见、删除、(分割线)、设置
 */
void DAChartManageWidget::PrivateData::updateContextMenuActions(int nodeType)
{
    if (!mTreeContextMenu) {
        return;
    }
    switch (nodeType) {
    case DAFigureTreeModel::NodeTypePlotFolder:
        mActRename->setVisible(true);
        mActVisible->setVisible(false);
        mActDelete->setVisible(false);
        mActSetting->setVisible(true);
        break;
    case DAFigureTreeModel::NodeTypeAxis:
        mActRename->setVisible(true);
        mActVisible->setVisible(true);
        mActDelete->setVisible(false);
        mActSetting->setVisible(true);
        break;
    case DAFigureTreeModel::NodeTypePlotItem:
        mActRename->setVisible(true);
        mActVisible->setVisible(true);
        mActDelete->setVisible(true);
        mActSetting->setVisible(true);
        break;
    // 3D 节点
    case DAFigureTreeModel::NodeTypePlot3DFolder:
        mActRename->setVisible(true);
        mActVisible->setVisible(false);
        mActDelete->setVisible(false);
        mActSetting->setVisible(true);
        break;
    case DAFigureTreeModel::NodeTypePlot3DAxis:
        mActRename->setVisible(true);
        mActVisible->setVisible(false);
        mActDelete->setVisible(false);
        mActSetting->setVisible(true);
        break;
    case DAFigureTreeModel::NodeTypePlot3DItem:
        mActRename->setVisible(true);
        mActVisible->setVisible(true);
        mActDelete->setVisible(true);
        mActSetting->setVisible(true);
        break;
    default:
        mActRename->setVisible(false);
        mActVisible->setVisible(false);
        mActDelete->setVisible(false);
        mActSetting->setVisible(false);
        break;
    }
}
//===================================================
// DAChartManageWidget
//===================================================
DAChartManageWidget::DAChartManageWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartManageWidget)
{
    ui->setupUi(this);
    connect(ui->toolButtonExpandAll, &QToolButton::clicked, this, &DAChartManageWidget::expandCurrentTree);
    connect(ui->toolButtonCollapseAll, &QToolButton::clicked, this, &DAChartManageWidget::collapseCurrentTree);
    connect(ui->toolButtonFigureSetting, &QToolButton::clicked, this, &DAChartManageWidget::onToolButtonFigureSettingClicked);
    connect(ui->comboBoxFigure,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DAChartManageWidget::onComboboxCurrentIndexChanged);
}

DAChartManageWidget::~DAChartManageWidget()
{
    delete ui;
}

void DAChartManageWidget::setChartOperateWidget(DAChartOperateWidget* cow)
{
    if (d_ptr->mChartOptWidget) {
        DAChartOperateWidget* old = d_ptr->mChartOptWidget;
        disconnect(old, nullptr, this, nullptr);
    }
    d_ptr->mChartOptWidget = cow;
    connect(cow, &DAChartOperateWidget::figureCreated, this, &DAChartManageWidget::onFigureCreated);
    connect(cow, &DAChartOperateWidget::figureRemoving, this, &DAChartManageWidget::onFigureCloseing);
    connect(cow, &DAChartOperateWidget::currentFigureChanged, this, &DAChartManageWidget::onCurrentFigureChanged);
}

void DAChartManageWidget::setCurrentChartOnItemClicked(bool on)
{
    d_ptr->mSetCurrentChartOnClicked = on;
}

bool DAChartManageWidget::isSetCurrentChartOnItemClicked() const
{
    return d_ptr->mSetCurrentChartOnClicked;
}

void DAChartManageWidget::setCurrentChartOnItemDoubleClicked(bool on)
{
    d_ptr->mSetCurrentChartOnDbClicked = on;
}

bool DAChartManageWidget::isSetCurrentChartOnItemDoubleClicked() const
{
    return d_ptr->mSetCurrentChartOnDbClicked;
}

/**
 * @brief 通过plot获取figure
 * @param plot
 * @return
 */
DAFigureWidget* DAChartManageWidget::plotToFigureWidget(QwtPlot* plot) const
{
    if (!plot) {
        return nullptr;
    }
    QwtFigure* fig { nullptr };
    if (plot->isParasitePlot()) {
        fig = qobject_cast< QwtFigure* >(plot->hostPlot()->parentWidget());
    } else {
        fig = qobject_cast< QwtFigure* >(plot->parentWidget());
    }
    if (!fig) {
        return nullptr;
    }
    return d_ptr->mFigToFigWidget.value(fig, nullptr);
}

DAFigureWidget* DAChartManageWidget::getCurrentFigure() const
{
    return reinterpret_cast< DAFigureWidget* >(ui->comboBoxFigure->currentData().value< quintptr >());
}

void DAChartManageWidget::expandCurrentTree()
{
    DAFigureTreeView* tree = currentTreeView();
    if (!tree) {
        return;
    }
    QAbstractItemModel* model = tree->model();
    if (!model) {
        return;
    }
    // 展开图元节点（NodeTypeItemsFolder 及其上层节点），收起坐标轴文件夹节点（NodeTypeAxesFolder）
    std::function< void(const QModelIndex&) > expandIndex = [&](const QModelIndex& parent) {
        for (int row = 0; row < model->rowCount(parent); ++row) {
            QModelIndex index = model->index(row, 0, parent);
            if (!index.isValid()) {
                continue;
            }
            // 坐标轴文件夹节点收起（2D + 3D），其余节点展开
            int nt = index.data(DAFigureTreeModel::RoleNodeType).toInt();
            bool expand = (nt != DAFigureTreeModel::NodeTypeAxesFolder
                           && nt != DAFigureTreeModel::NodeTypePlot3DAxesFolder);
            tree->setExpanded(index, expand);
            if (expand) {
                expandIndex(index);
            }
        }
    };
    expandIndex(QModelIndex());
    if (tree->isAutoResizeColumnToContents()) {
        tree->resizeHeaderToContents();
    }
}

void DAChartManageWidget::collapseCurrentTree()
{
    DAFigureTreeView* tree = currentTreeView();
    if (!tree) {
        return;
    }
    tree->collapseAll();
}

DAFigureTreeView* DAChartManageWidget::currentTreeView() const
{
    return qobject_cast< DAFigureTreeView* >(ui->stackedWidget->currentWidget());
}

/**
 * @brief 刷新指定plotItem的可见性列显示
 * @param item 需要刷新的plotItem
 */
void DAChartManageWidget::refreshPlotItemVisibility(QwtPlotItem* item)
{
    DAFigureTreeView* tree = currentTreeView();
    if (tree) {
        tree->refreshPlotItemVisibility(item);
    }
}

/**
 * @brief 刷新指定坐标轴的可见性列显示
 * @param plot 坐标轴所在的plot
 * @param axisId 坐标轴ID
 */
void DAChartManageWidget::refreshAxisVisibility(QwtPlot* plot, QwtAxisId axisId)
{
    DAFigureTreeView* tree = currentTreeView();
    if (tree) {
        tree->refreshAxisVisibility(plot, axisId);
    }
}

/**
 * @brief 刷新指定plotItem的文字列显示（用于重命名后）
 * @param item 需要刷新的plotItem
 */
void DAChartManageWidget::refreshPlotItemText(QwtPlotItem* item)
{
    DAFigureTreeView* tree = currentTreeView();
    if (tree) {
        tree->refreshPlotItemText(item);
    }
}

/**
 * @brief 刷新指定坐标轴的文字列显示（用于重命名后）
 * @param plot 坐标轴所在的plot
 * @param axisId 坐标轴ID
 */
void DAChartManageWidget::refreshAxisText(QwtPlot* plot, QwtAxisId axisId)
{
    DAFigureTreeView* tree = currentTreeView();
    if (tree) {
        tree->refreshAxisText(plot, axisId);
    }
}

/**
 * @brief 刷新指定chart节点的文字列显示（用于重命名后）
 * @param plot chart对应的plot
 */
void DAChartManageWidget::refreshPlotFolderText(QwtPlot* plot)
{
    DAFigureTreeView* tree = currentTreeView();
    if (tree) {
        tree->refreshPlotFolderText(plot);
    }
}

void DAChartManageWidget::refresh3DPlotItemVisibility(Qwt3DPlotItem* item)
{
    DAFigureTreeView* tree = currentTreeView();
    if (tree) {
        tree->refresh3DPlotItemVisibility(item);
    }
}

void DAChartManageWidget::refresh3DPlotItemText(Qwt3DPlotItem* item)
{
    DAFigureTreeView* tree = currentTreeView();
    if (tree) {
        tree->refresh3DPlotItemText(item);
    }
}

void DAChartManageWidget::refresh3DPlot3DText(DAChart3DWidget* chart)
{
    DAFigureTreeView* tree = currentTreeView();
    if (tree) {
        tree->refresh3DPlot3DText(chart);
    }
}

void DAChartManageWidget::setCurrentDisplayView(DAFigureWidget* fig)
{
    setStackCurrentFigure(fig);
    QSignalBlocker b(ui->comboBoxFigure);  // 避免触发currentIndexChanged信号导致再次切换stack
    setComboboxCurrentFigure(fig);
}

DAFigureWidget* DAChartManageWidget::getComboboxFigure(int index) const
{
    return reinterpret_cast< DAFigureWidget* >(ui->comboBoxFigure->itemData(index).value< quintptr >());
}

void DAChartManageWidget::setStackCurrentFigure(DAFigureWidget* fig)
{
    DAFigureTreeView* tree = d_ptr->mFigureWidgetToTree.value(fig, nullptr);
    if (!tree) {
        return;
    }
    int c = ui->stackedWidget->count();
    for (int i = 0; i < c; ++i) {
        QWidget* w = ui->stackedWidget->widget(i);
        if (!w) {
            continue;
        }
        if (tree == w) {
            ui->stackedWidget->setCurrentWidget(w);
            return;
        }
    }
}

void DAChartManageWidget::setComboboxCurrentFigure(DAFigureWidget* fig)
{
    int c = ui->comboBoxFigure->count();
    for (int i = 0; i < c; ++i) {
        DAFigureWidget* w = getComboboxFigure(i);
        if (w == fig) {
            if (i != ui->comboBoxFigure->currentIndex()) {
                ui->comboBoxFigure->setCurrentIndex(i);
            }
        }
    }
}

void DAChartManageWidget::onFigureCreated(DAFigureWidget* fig)
{
    // figure 在 ADS 嵌套停靠下不再有稳定的线性布局索引，这里仅做存在性校验后追加
    if (!fig || d_ptr->mChartOptWidget->getFigureIndex(fig) < 0) {
        daCritical << tr("received figure create signal, but cannot find figure");  // cn:获取了绘图创建的信号，但无法找到绘图
        return;
    }

    DAFigureTreeView* figTreeview = new DAFigureTreeView(this);
    figTreeview->setFigureWidget(fig);
    d_ptr->mFigToFigWidget[ fig->figure() ] = fig;
    d_ptr->mFigureWidgetToTree[ fig ]       = figTreeview;
    connect(figTreeview, &DAFigureTreeView::itemCliecked, this, &DAChartManageWidget::figureElementClicked);
    connect(figTreeview, &DAFigureTreeView::itemDbCliecked, this, &DAChartManageWidget::figureElementDbClicked);
    // 设置右键菜单策略
    figTreeview->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(figTreeview, &QWidget::customContextMenuRequested, this, [ this, figTreeview ](const QPoint& pos) {
        onTreeViewContextMenuRequested(figTreeview, pos);
    });
    // combobox/stackedWidget 顺序 = 创建顺序（稳定），不再依赖停靠布局索引
    ui->stackedWidget->addWidget(figTreeview);

    ui->comboBoxFigure->addItem(fig->windowTitle(), reinterpret_cast< quintptr >(fig));
}

void DAChartManageWidget::onFigureCloseing(DAFigureWidget* fig)
{
    DAFigureTreeView* tree = d_ptr->mFigureWidgetToTree.value(fig, nullptr);
    if (!tree) {
        daCritical << tr("received figure close signal, but cannot find figure index");  // cn:获取了绘图关闭的信号，但无法找到绘图的索引
        return;
    }
    ui->stackedWidget->removeWidget(tree);
    tree->deleteLater();
    // 删除combobox
    const int comboboxCnt = ui->comboBoxFigure->count();
    for (int i = 0; i < comboboxCnt; ++i) {
        quintptr ptr        = ui->comboBoxFigure->itemData(i).value< quintptr >();
        DAFigureWidget* tmp = reinterpret_cast< DAFigureWidget* >(ptr);
        if (tmp == fig) {
            ui->comboBoxFigure->removeItem(i);
            break;
        }
    }
}

void DAChartManageWidget::onCurrentFigureChanged(DAFigureWidget* fig, int index)
{
    Q_UNUSED(fig);
    Q_UNUSED(index);
    setCurrentDisplayView(fig);
}

void DAChartManageWidget::onPlotClicked(QwtPlot* plot, QStandardItem* treeItem)
{
    Q_UNUSED(treeItem);
    DAFigureWidget* figWidget = plotToFigureWidget(plot);
    if (!figWidget) {
        return;
    }
    DAFigureElementSelection::SelectionColumns col = d_ptr->standardItemToSelectionColumns(treeItem);
    DAFigureElementSelection sel(figWidget, plot, col);
    Q_EMIT figureElementClicked(sel);
}

void DAChartManageWidget::onPlotItemClicked(QwtPlotItem* item, QwtPlot* plot, QStandardItem* treeItem)
{
    Q_UNUSED(treeItem);
    if (!plot || !item) {
        return;
    }
    DAFigureWidget* figWidget = plotToFigureWidget(plot);
    if (!figWidget) {
        return;
    }
    DAFigureElementSelection::SelectionColumns col = d_ptr->standardItemToSelectionColumns(treeItem);
    DAFigureElementSelection sel(figWidget, plot, item, col);
    Q_EMIT figureElementClicked(sel);
}

void DAChartManageWidget::onAxisClicked(QwtAxisId axisId, QwtPlot* plot, QStandardItem* treeItem)
{
    Q_UNUSED(treeItem);
    DAFigureWidget* figWidget = plotToFigureWidget(plot);
    if (!figWidget) {
        return;
    }
    if (axisId == QwtAxis::AxisPositions) {
        return;
    }
    DAFigureElementSelection::SelectionColumns col = d_ptr->standardItemToSelectionColumns(treeItem);
    DAFigureElementSelection sel(figWidget, plot, plot->axisWidget(axisId), axisId, col);
    Q_EMIT figureElementClicked(sel);
}

void DAChartManageWidget::onToolButtonFigureSettingClicked()
{
    if (DAFigureWidget* fig = getCurrentFigure()) {
        Q_EMIT requestFigureSetting(fig);
    }
}

void DAChartManageWidget::onComboboxCurrentIndexChanged(int index)
{
    // 用指针而非 index 联动 ChartOperateWidget（ADS 嵌套停靠下 index 不再对应停靠布局）
    // 此函数会触发槽onCurrentFigureChanged，调用setCurrentDisplayView，
    // 但在setCurrentDisplayView中，combobx的currentIndex已经是index，就不会再设置，从而避免递归调用
    DAFigureWidget* fig = getComboboxFigure(index);
    if (!fig) {
        return;
    }
    d_ptr->mChartOptWidget->setCurrentFigure(fig);
    setStackCurrentFigure(fig);
    Q_EMIT selectFigureChanged(fig);
}

/**
 * @brief 树形控件右键菜单请求处理
 *
 * 使用懒加载复用的 QMenu（@ref PrivateData::ensureTreeContextMenu），
 * 根据当前节点类型动态控制各 action 的显隐后 exec。
 * 具体的动作处理在 onContextMenuXxxTriggered 系列槽中实现。
 * @param tree 触发右键的树形控件
 * @param pos 右键位置（相对于树形控件的viewport）
 */
void DAChartManageWidget::onTreeViewContextMenuRequested(DAFigureTreeView* tree, const QPoint& pos)
{
    if (!tree) {
        return;
    }
    QModelIndex index = tree->indexAt(pos);
    if (!index.isValid()) {
        return;
    }
    DAFigureTreeModel* model = tree->getFigureTreeModel();
    if (!model) {
        return;
    }
    QStandardItem* item = model->itemFromIndex(index);
    if (!item) {
        return;
    }
    DAFigureWidget* figWidget = tree->getFigureWidget();
    if (!figWidget) {
        return;
    }

    const int nodeType = item->data(DAFigureTreeModel::RoleNodeType).toInt();
    // layout 节点和文件夹节点不弹出菜单（2D + 3D）
    if (nodeType == DAFigureTreeModel::NodeTypePlot
        || nodeType == DAFigureTreeModel::NodeTypeAxesFolder
        || nodeType == DAFigureTreeModel::NodeTypeItemsFolder
        || nodeType == DAFigureTreeModel::NodeTypePlot3D
        || nodeType == DAFigureTreeModel::NodeTypePlot3DAxesFolder
        || nodeType == DAFigureTreeModel::NodeTypePlot3DItemsFolder) {
        return;
    }

    // 记录当前右键上下文，供 action 槽函数使用
    d_ptr->ensureTreeContextMenu(this);
    d_ptr->mContextTree         = tree;
    d_ptr->mContextFigureWidget = figWidget;
    d_ptr->mContextItem         = item;
    d_ptr->mContextNodeType     = nodeType;
    d_ptr->updateContextMenuActions(nodeType);

    // 根据当前可见性同步"可见"action 的勾选状态，action->isVisible说明这个action起作用，在上面已经确认这个item需要可见性判断
    // (checked = 当前可见, unchecked = 当前隐藏)
    if (d_ptr->mActVisible->isVisible()) {
        bool visible = false;
        if (nodeType == DAFigureTreeModel::NodeTypeAxis) {
            QwtPlot* plot = model->plotFromItem(item);
            QwtAxisId axisId = model->axisIdFromItem(item);
            if (plot && axisId != QwtAxis::AxisPositions) {
                visible = plot->isAxisVisible(axisId);
            }
        } else if (nodeType == DAFigureTreeModel::NodeTypePlotItem) {
            if (QwtPlotItem* plotItem = model->plotItemFromItem(item)) {
                visible = plotItem->isVisible();
            }
        } else if (nodeType == DAFigureTreeModel::NodeTypePlot3DItem) {
            if (Qwt3DPlotItem* plot3DItem = model->plot3DItemFromItem(item)) {
                visible = plot3DItem->isVisible();
            }
        }
        d_ptr->mActVisible->setChecked(visible);
    }

    d_ptr->mTreeContextMenu->exec(tree->viewport()->mapToGlobal(pos));
}

/**
 * @brief 右键菜单"重命名"触发
 */
void DAChartManageWidget::onContextMenuRenameTriggered()
{
    DAFigureTreeView* tree = d_ptr->mContextTree;
    QStandardItem* item    = d_ptr->mContextItem;
    int nodeType           = d_ptr->mContextNodeType;
    if (!tree || !item) {
        return;
    }
    DAFigureTreeModel* model = tree->getFigureTreeModel();
    if (!model) {
        return;
    }
    if (nodeType == DAFigureTreeModel::NodeTypePlotFolder) {
        QwtPlot* plot = model->plotFromItem(item);
        if (plot) {
            QString oldName = plot->title().text();
            if (oldName.isEmpty()) {
                oldName = tr("chart");  // cn:绘图
            }
            bool ok = false;
            QString newName = QInputDialog::getText(tree,
                                                    tr("Rename"),     // cn:重命名
                                                    tr("New name:"),  // cn:新名称:
                                                    QLineEdit::Normal,
                                                    oldName,
                                                    &ok);
            if (ok && !newName.isEmpty()) {
                plot->setTitle(newName);
                tree->refreshPlotFolderText(plot);
            }
        }
    } else if (nodeType == DAFigureTreeModel::NodeTypeAxis) {
        QwtPlot* plot = model->plotFromItem(item);
        QwtAxisId axisId = model->axisIdFromItem(item);
        if (plot && axisId != QwtAxis::AxisPositions) {
            QString oldName = plot->axisTitle(axisId).text();
            bool ok = false;
            QString newName = QInputDialog::getText(tree,
                                                    tr("Rename"),     // cn:重命名
                                                    tr("New name:"),  // cn:新名称:
                                                    QLineEdit::Normal,
                                                    oldName,
                                                    &ok);
            if (ok && !newName.isEmpty()) {
                QwtText title = plot->axisTitle(axisId);
                title.setText(newName);
                plot->setAxisTitle(axisId, title);
                tree->refreshAxisText(plot, axisId);
                plot->replot();
            }
        }
    } else if (nodeType == DAFigureTreeModel::NodeTypePlotItem) {
        QwtPlotItem* plotItem = model->plotItemFromItem(item);
        QwtPlot* plot = model->plotFromItem(item);
        if (plotItem && plot) {
            QString oldName = plotItem->title().text();
            bool ok = false;
            QString newName = QInputDialog::getText(tree,
                                                    tr("Rename"),     // cn:重命名
                                                    tr("New name:"),  // cn:新名称:
                                                    QLineEdit::Normal,
                                                    oldName,
                                                    &ok);
            if (ok && !newName.isEmpty()) {
                plotItem->setTitle(newName);
                tree->refreshPlotItemText(plotItem);
                plot->replot();
            }
        }
    }
    // === 3D 节点重命名 ===
    else if (nodeType == DAFigureTreeModel::NodeTypePlot3DFolder) {
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        if (chart3D) {
            QString oldName = chart3D->getChart3DTitle();
            if (oldName.isEmpty()) {
                oldName = tr("3D Chart");  // cn:3D绘图
            }
            bool ok = false;
            QString newName = QInputDialog::getText(tree,
                                                    tr("Rename"),     // cn:重命名
                                                    tr("New name:"),  // cn:新名称:
                                                    QLineEdit::Normal,
                                                    oldName,
                                                    &ok);
            if (ok && !newName.isEmpty()) {
                chart3D->setChart3DTitle(newName);
                // 3D folder 节点是普通 QStandardItem，无动态 data() 查询，需显式更新文本
                item->setText(newName);
                // 同时刷新 layer 节点（NodeTypePlot3D，DAStandardItemPlot3D 动态查询标题）
                tree->refresh3DPlot3DText(chart3D);
            }
        }
    } else if (nodeType == DAFigureTreeModel::NodeTypePlot3DItem) {
        Qwt3DPlotItem* plot3DItem = model->plot3DItemFromItem(item);
        if (plot3DItem) {
            QString oldName = plot3DItem->title();
            bool ok = false;
            QString newName = QInputDialog::getText(tree,
                                                    tr("Rename"),     // cn:重命名
                                                    tr("New name:"),  // cn:新名称:
                                                    QLineEdit::Normal,
                                                    oldName,
                                                    &ok);
            if (ok && !newName.isEmpty()) {
                plot3DItem->setTitle(newName);
                tree->refresh3DPlotItemText(plot3DItem);
            }
        }
    } else if (nodeType == DAFigureTreeModel::NodeTypePlot3DAxis) {
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        int axis3DId = item->data(DAFigureTreeModel::RoleAxis3DId).toInt();
        if (chart3D && axis3DId >= 0) {
            QString oldName = chart3D->get3DAxisLabel(static_cast<AXIS>(axis3DId));
            bool ok = false;
            QString newName = QInputDialog::getText(tree,
                                                    tr("Rename"),     // cn:重命名
                                                    tr("New name:"),  // cn:新名称:
                                                    QLineEdit::Normal,
                                                    oldName,
                                                    &ok);
            if (ok && !newName.isEmpty()) {
                chart3D->set3DAxisLabel(static_cast<AXIS>(axis3DId), newName);
                // 3D 轴节点是普通 QStandardItem，无动态 data() 查询，需显式更新文本
                item->setText(newName);
                chart3D->update();  // 触发 GL 重绘
            }
        }
    }
}

/**
 * @brief 右键菜单"可见"触发
 *
 * action 为 checkable，触发时 checked 状态已由 Qt 自动翻转，
 * 这里直接读取新的 checked 值并应用到目标对象。
 */
void DAChartManageWidget::onContextMenuVisibleTriggered(bool on)
{
    DAFigureTreeView* tree = d_ptr->mContextTree;
    QStandardItem* item    = d_ptr->mContextItem;
    int nodeType           = d_ptr->mContextNodeType;
    DAFigureWidget* figWidget = d_ptr->mContextFigureWidget;
    if (!tree || !item || !figWidget) {
        return;
    }
    DAFigureTreeModel* model = tree->getFigureTreeModel();
    if (!model) {
        return;
    }
    if (nodeType == DAFigureTreeModel::NodeTypeAxis) {
        QwtPlot* plot = model->plotFromItem(item);
        QwtAxisId axisId = model->axisIdFromItem(item);
        if (plot && axisId != QwtAxis::AxisPositions) {
            // 优先走 DAChartWidget::setAxisVisible 以触发 AxisVisibilityChanged 通知（联动探针徽章重算）
            if (DAChartWidget* dacw = qobject_cast< DAChartWidget* >(plot)) {
                dacw->setAxisVisible(axisId, on);
            } else {
                plot->setAxisVisible(axisId, on);
            }
            tree->refreshAxisVisibility(plot, axisId);
            plot->replot();
            // 通知设置面板刷新——使用 ColumnProperty 而非 ColumnVisible，
            // 因为可见性已在此处设置完成，ColumnVisible 会触发 onFigureElementDbClicked 再次翻转
            DAFigureElementSelection sel(figWidget, plot, plot->axisWidget(axisId), axisId,
                                          DAFigureElementSelection::ColumnProperty);
            Q_EMIT figureElementClicked(sel);
        }
    } else if (nodeType == DAFigureTreeModel::NodeTypePlotItem) {
        QwtPlotItem* plotItem = model->plotItemFromItem(item);
        QwtPlot* plot = model->plotFromItem(item);
        if (plotItem && plot) {
            plotItem->setVisible(on);
            tree->refreshPlotItemVisibility(plotItem);
            plot->replot();
            // 同上，使用 ColumnProperty 避免二次翻转
            DAFigureElementSelection sel(figWidget, plot, plotItem,
                                          DAFigureElementSelection::ColumnProperty);
            Q_EMIT figureElementClicked(sel);
        }
    }
    // === 3D item 可见性 ===
    else if (nodeType == DAFigureTreeModel::NodeTypePlot3DItem) {
        Qwt3DPlotItem* plot3DItem = model->plot3DItemFromItem(item);
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        if (plot3DItem && chart3D) {
            plot3DItem->setVisible(on);
            tree->refresh3DPlotItemVisibility(plot3DItem);
            chart3D->update();  // 触发 GL 重绘
            DAFigureElementSelection sel(figWidget, chart3D, plot3DItem,
                                          DAFigureElementSelection::ColumnProperty);
            Q_EMIT figureElementClicked(sel);
        }
    }
}

/**
 * @brief 右键菜单"删除"触发（仅 plotItem 支持）
 */
void DAChartManageWidget::onContextMenuDeleteTriggered()
{
    DAFigureTreeView* tree = d_ptr->mContextTree;
    QStandardItem* item    = d_ptr->mContextItem;
    int nodeType           = d_ptr->mContextNodeType;
    if (!tree || !item) {
        return;
    }
    if (nodeType != DAFigureTreeModel::NodeTypePlotItem
        && nodeType != DAFigureTreeModel::NodeTypePlot3DItem) {
        return;
    }
    DAFigureTreeModel* model = tree->getFigureTreeModel();
    if (!model) {
        return;
    }
    if (nodeType == DAFigureTreeModel::NodeTypePlotItem) {
        QwtPlotItem* plotItem = model->plotItemFromItem(item);
        QwtPlot* plot = model->plotFromItem(item);
        if (!plotItem || !plot) {
            return;
        }
        int ret = QMessageBox::question(tree,
                                        tr("Delete"),                                       // cn:删除
                                        tr("Are you sure to delete \"%1\"?").arg(plotItem->title().text()),  // cn:确认删除"%1"吗?
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            // 优先使用DAChartWidget的removePlotItem,它会调用detach并delete
            if (DAChartWidget* chartWidget = qobject_cast< DAChartWidget* >(plot)) {
                chartWidget->removePlotItem(plotItem);
            } else {
                plotItem->detach();
                delete plotItem;
                plot->replot();
            }
            // 模型会通过QwtPlot::itemAttached信号自动更新
        }
    }
    // === 3D item 删除 ===
    else if (nodeType == DAFigureTreeModel::NodeTypePlot3DItem) {
        Qwt3DPlotItem* plot3DItem = model->plot3DItemFromItem(item);
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        if (!plot3DItem || !chart3D) {
            return;
        }
        int ret = QMessageBox::question(tree,
                                        tr("Delete"),                                // cn:删除
                                        tr("Are you sure to delete \"%1\"?").arg(plot3DItem->title()),  // cn:确认删除"%1"吗?
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            // 先从树中显式移除节点，确保树和 hash 一致，避免信号处理时序依赖
            model->remove3DPlotItemFromTree(plot3DItem);
            plot3DItem->detach();
            delete plot3DItem;
            chart3D->update();  // 触发 GL 重绘
        }
    }
}

/**
 * @brief 右键菜单"设置"触发
 *
 * 发出 figureElementClicked(ColumnProperty) 信号,DAAppController 会切换到对应的设置面板
 */
void DAChartManageWidget::onContextMenuSettingTriggered()
{
    DAFigureTreeView* tree = d_ptr->mContextTree;
    QStandardItem* item    = d_ptr->mContextItem;
    int nodeType           = d_ptr->mContextNodeType;
    DAFigureWidget* figWidget = d_ptr->mContextFigureWidget;
    if (!tree || !item || !figWidget) {
        return;
    }
    DAFigureTreeModel* model = tree->getFigureTreeModel();
    if (!model) {
        return;
    }
    DAFigureElementSelection::SelectionColumns col = DAFigureElementSelection::ColumnProperty;
    switch (nodeType) {
    case DAFigureTreeModel::NodeTypePlotFolder: {
        QwtPlot* plot = model->plotFromItem(item);
        if (plot) {
            Q_EMIT figureElementClicked(DAFigureElementSelection(figWidget, plot, col));
        }
        break;
    }
    case DAFigureTreeModel::NodeTypeAxis: {
        QwtPlot* plot = model->plotFromItem(item);
        QwtAxisId axisId = model->axisIdFromItem(item);
        if (plot && axisId != QwtAxis::AxisPositions) {
            Q_EMIT figureElementClicked(DAFigureElementSelection(figWidget, plot, plot->axisWidget(axisId), axisId, col));
        }
        break;
    }
    case DAFigureTreeModel::NodeTypePlotItem: {
        QwtPlot* plot = model->plotFromItem(item);
        QwtPlotItem* plotItem = model->plotItemFromItem(item);
        if (plot && plotItem) {
            Q_EMIT figureElementClicked(DAFigureElementSelection(figWidget, plot, plotItem, col));
        }
        break;
    }
    // === 3D 节点设置 ===
    case DAFigureTreeModel::NodeTypePlot3DFolder: {
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        if (chart3D) {
            Q_EMIT figureElementClicked(DAFigureElementSelection(figWidget, chart3D, col));
        }
        break;
    }
    case DAFigureTreeModel::NodeTypePlot3DAxis: {
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        int axis3DId = item->data(DAFigureTreeModel::RoleAxis3DId).toInt();
        if (chart3D && axis3DId >= 0) {
            Q_EMIT figureElementClicked(DAFigureElementSelection(figWidget, chart3D, axis3DId, col));
        }
        break;
    }
    case DAFigureTreeModel::NodeTypePlot3DItem: {
        DAChart3DWidget* chart3D = model->plot3DFromItem(item);
        Qwt3DPlotItem* plot3DItem = model->plot3DItemFromItem(item);
        if (chart3D && plot3DItem) {
            Q_EMIT figureElementClicked(DAFigureElementSelection(figWidget, chart3D, plot3DItem, col));
        }
        break;
    }
    default:
        break;
    }
}

}
