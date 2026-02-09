#include "DAChartManageWidget.h"
#include "ui_DAChartManageWidget.h"
#include <QTreeView>
#include <QPointer>
#include <QDebug>
#include <QHash>
#include <QSet>
#include "DAChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAFigureTreeView.h"
#include "DAChartWidget.h"
#include "qwt_figure.h"
#include "Models/DAFigureTreeModel.h"
namespace DA
{
//===============================================================
// PrivateData
//===============================================================
class DAChartManageWidget::PrivateData
{
public:
    DA_DECLARE_PUBLIC(DAChartManageWidget)
    PrivateData(DAChartManageWidget* p);
    DAFigureElementSelection::SelectionColumns standardItemToSelectionColumns(QStandardItem* item);

public:
    QPointer< DAChartOperateWidget > mChartOptWidget;
    bool mSetCurrentChartOnClicked { true };
    bool mSetCurrentChartOnDbClicked { true };
    QHash< QwtFigure*, DAFigureWidget* > mFigToFigWidget;             ///< 建立figure和figureWidget的关系
    QHash< DAFigureWidget*, DAFigureTreeView* > mFigureWidgetToTree;  ///< 建立figurewidget和tree的关系
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
    connect(
        ui->comboBoxFigure, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DAChartManageWidget::onComboboxCurrentIndexChanged
    );
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
    tree->expandAll();
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
    int index = d_ptr->mChartOptWidget->getFigureIndex(fig);
    if (index < 0) {
        qCritical() << tr("get figure create signal,but can not find figure index"
        );  // cn: 获取了绘图创建的信号，但无法找到绘图的索引
        return;
    }

    DAFigureTreeView* figTreeview = new DAFigureTreeView(this);
    figTreeview->setFigureWidget(fig);
    d_ptr->mFigToFigWidget[ fig->figure() ] = fig;
    d_ptr->mFigureWidgetToTree[ fig ]       = figTreeview;
    connect(figTreeview, &DAFigureTreeView::itemCliecked, this, &DAChartManageWidget::figureElementClicked);
    connect(figTreeview, &DAFigureTreeView::itemDbCliecked, this, &DAChartManageWidget::figureElementDbClicked);
    ui->stackedWidget->insertWidget(index, figTreeview);

    ui->comboBoxFigure->insertItem(index, fig->windowTitle(), reinterpret_cast< quintptr >(fig));
}

void DAChartManageWidget::onFigureCloseing(DAFigureWidget* fig)
{
    DAFigureTreeView* tree = d_ptr->mFigureWidgetToTree.value(fig, nullptr);
    if (!tree) {
        qCritical() << tr("get figure close signal,but can not find figure index"
        );  // cn: 获取了绘图关闭的信号，但无法找到绘图的索引
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
    // 联动ChartOptWidget,此函数会触发槽onCurrentFigureChanged，调用setCurrentDisplayView，
    // 但在setCurrentDisplayView中，combobx的currentIndex已经是index，就不会再设置，从而避免递归调用
    d_ptr->mChartOptWidget->setCurrentFigure(index);
    if (DAFigureWidget* fig = getCurrentFigure()) {
        setStackCurrentFigure(fig);
        Q_EMIT selectFigureChanged(fig);
    }
}

}
