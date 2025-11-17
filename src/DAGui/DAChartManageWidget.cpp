#include "DAChartManageWidget.h"
#include "ui_DAChartManageWidget.h"
#include <QTreeView>
#include <QPointer>
#include <QDebug>
#include "DAChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAFigureTreeView.h"
#include "DAChartWidget.h"
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
    QPointer< DAChartOperateWidget > mChartOptWidget;
    bool mSetCurrentChartOnClicked { true };
    bool mSetCurrentChartOnDbClicked { true };
};
DAChartManageWidget::PrivateData::PrivateData(DAChartManageWidget* p) : q_ptr(p)
{
}
//===================================================
// DAChartManageWidget
//===================================================
DAChartManageWidget::DAChartManageWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartManageWidget)
{
    ui->setupUi(this);
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
 * @brief 单击选中发射itemselect
 * @param item
 */
void DAChartManageWidget::treeviewChartItemClicked(DAChartItemStandardItem* item)
{
    auto chart     = item->getChart();
    auto fig       = chart->getFigure();
    QwtPlotItem* i = item->getItem();
    if (isSetCurrentChartOnItemClicked()) {
        fig->setCurrentChart(chart);
    }
    emit figureItemClicked(fig, chart, i);
}

void DAChartManageWidget::treeviewChartWidgetClicked(DAChartWidgetStandardItem* item)
{
    auto chart = item->getChart();
    auto fig   = item->getFigure();
    if (isSetCurrentChartOnItemClicked()) {
        fig->setCurrentChart(chart);
    }
    emit figureItemClicked(fig, chart, nullptr);
}

void DAChartManageWidget::treeviewChartItemDoubleClicked(DAChartItemStandardItem* item)
{
    auto chart     = item->getChart();
    auto fig       = chart->getFigure();
    QwtPlotItem* i = item->getItem();
    if (isSetCurrentChartOnItemDoubleClicked()) {
        fig->setCurrentChart(chart);
    }
    emit figureItemDoubleClicked(fig, chart, i);
}

void DAChartManageWidget::treeviewChartWidgetDoubleClicked(DAChartWidgetStandardItem* item)
{
    auto chart = item->getChart();
    auto fig   = item->getFigure();
    if (isSetCurrentChartOnItemDoubleClicked()) {
        fig->setCurrentChart(chart);
    }
    emit figureItemDoubleClicked(fig, chart, nullptr);
}

void DAChartManageWidget::setCurrentDisplayView(DAFigureWidget* fig)
{
    int c = ui->stackedWidget->count();
    for (int i = 0; i < c; ++i) {
        QTreeView* treeview = qobject_cast< QTreeView* >(ui->stackedWidget->widget(i));
        if (!treeview) {
            continue;
        }
        DAFigureTreeModel* treemodel = qobject_cast< DAFigureTreeModel* >(treeview->model());
        if (!treemodel) {
            continue;
        }
        if (treemodel->getFigureWidget() == fig) {
            ui->stackedWidget->setCurrentIndex(i);
            break;
        }
    }
}

void DAChartManageWidget::onFigureCreated(DAFigureWidget* fig)
{
    int index = d_ptr->mChartOptWidget->getFigureIndex(fig);
    if (index < 0) {
        qCritical() << tr(
            "get figure create signal,but can not find figure index");  // cn: 获取了绘图创建的信号，但无法找到绘图的索引
        return;
    }

    DAFigureTreeView* figTreeview = new DAFigureTreeView(this);
    figTreeview->setFigureWidget(fig);

    connect(treeview, &QTreeView::clicked, this, &DAChartManageWidget::onTreeViewClicked);
    connect(treeview, &QTreeView::doubleClicked, this, &DAChartManageWidget::onTreeViewDoubleClicked);
    ui->stackedWidget->insertWidget(index, treeview);
}

void DAChartManageWidget::onFigureCloseing(DAFigureWidget* fig)
{
    int index = d_ptr->mChartOptWidget->getFigureIndex(fig);
    if (index < 0) {
        qCritical() << tr(
            "get figure close signal,but can not find figure index");  // cn: 获取了绘图关闭的信号，但无法找到绘图的索引
        return;
    }
    QWidget* w = ui->stackedWidget->widget(index);
    ui->stackedWidget->removeWidget(w);
    w->deleteLater();
}

void DAChartManageWidget::onCurrentFigureChanged(DAFigureWidget* fig, int index)
{
    Q_UNUSED(fig);
    Q_UNUSED(index);
    setCurrentDisplayView(fig);
}

void DAChartManageWidget::onTreeViewClicked(const QModelIndex& index)
{
    QTreeView* treeview = qobject_cast< QTreeView* >(sender());
    if (!treeview) {
        return;
    }
    DAFigureTreeModel* treemodel = qobject_cast< DAFigureTreeModel* >(treeview->model());
    if (!treemodel) {
        return;
    }
    auto standItem = treemodel->itemFromIndex(index);
    if (standItem->type() == DAChartWidgetStandardItem_Type) {
        DAChartWidgetStandardItem* ii = static_cast< DAChartWidgetStandardItem* >(standItem);
        treeviewChartWidgetClicked(ii);
    } else if (standItem->type() == DAChartItemStandardItem_Type) {
        DAChartItemStandardItem* ii = static_cast< DAChartItemStandardItem* >(standItem);
        treeviewChartItemClicked(ii);
    }
}

void DAChartManageWidget::onTreeViewDoubleClicked(const QModelIndex& index)
{
    QTreeView* treeview = qobject_cast< QTreeView* >(sender());
    if (!treeview) {
        return;
    }
    DAFigureTreeModel* treemodel = qobject_cast< DAFigureTreeModel* >(treeview->model());
    if (!treemodel) {
        return;
    }
    auto standItem = treemodel->itemFromIndex(index);
    if (standItem->type() == DAChartWidgetStandardItem_Type) {
        DAChartWidgetStandardItem* ii = static_cast< DAChartWidgetStandardItem* >(standItem);
        treeviewChartWidgetClicked(ii);
    } else if (standItem->type() == DAChartItemStandardItem_Type) {
        DAChartItemStandardItem* ii = static_cast< DAChartItemStandardItem* >(standItem);
        treeviewChartItemClicked(ii);
    }
}
}
