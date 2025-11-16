#include "DAChartManageWidget.h"
#include "ui_DAChartManageWidget.h"
#include <QTreeView>
#include <QPointer>
#include <QDebug>
#include <QHash>
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

void DAChartManageWidget::setCurrentDisplayView(DAFigureWidget* fig)
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
    d_ptr->mFigToFigWidget[ fig->figure() ] = fig;
    d_ptr->mFigureWidgetToTree[ fig ]       = figTreeview;
    connect(figTreeview, &DAFigureTreeView::plotClicked, this, &DAChartManageWidget::onPlotClicked);
    connect(figTreeview, &DAFigureTreeView::plotItemClicked, this, &DAChartManageWidget::onPlotItemClicked);
    connect(figTreeview, &DAFigureTreeView::axisClicked, this, &DAChartManageWidget::onAxisClicked);
    ui->stackedWidget->insertWidget(index, figTreeview);
}

void DAChartManageWidget::onFigureCloseing(DAFigureWidget* fig)
{
    DAFigureTreeView* tree = d_ptr->mFigureWidgetToTree.value(fig, nullptr);
    if (!tree) {
        qCritical() << tr(
            "get figure close signal,but can not find figure index");  // cn: 获取了绘图关闭的信号，但无法找到绘图的索引
        return;
    }
    ui->stackedWidget->removeWidget(tree);
    tree->deleteLater();
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
    DAFigureElementSelection sel(figWidget, plot);
    Q_EMIT figureElementSelected(sel);
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
    DAFigureElementSelection sel(figWidget, plot, item);
    Q_EMIT figureElementSelected(sel);
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
    DAFigureElementSelection sel(figWidget, plot, plot->axisWidget(axisId), axisId);
    Q_EMIT figureElementSelected(sel);
}

}
