#include "DAChartManageWidget.h"
#include "ui_DAChartManageWidget.h"
#include <QTreeView>
#include <QPointer>
#include <QDebug>
#include "DAChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAFigureTreeModel.h"
namespace DA
{

class DAChartManageWidgetPrivate
{
public:
    DA_IMPL_PUBLIC(DAChartManageWidget)
    DAChartManageWidgetPrivate(DAChartManageWidget* p);
    QPointer< DAChartOperateWidget > mChartOptWidget;
};
DAChartManageWidgetPrivate::DAChartManageWidgetPrivate(DAChartManageWidget* p) : q_ptr(p)
{
}
//===================================================
// DAChartManageWidget
//===================================================
DAChartManageWidget::DAChartManageWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartManageWidget), d_ptr(new DAChartManageWidgetPrivate(this))
{
    ui->setupUi(this);
}

DAChartManageWidget::~DAChartManageWidget()
{
    delete ui;
}

void DAChartManageWidget::setChartOperateWidget(DAChartOperateWidget* cow)
{
    d_ptr->mChartOptWidget = cow;
    connect(cow, &DAChartOperateWidget::figureCreated, this, &DAChartManageWidget::onFigureCreated);
    connect(cow, &DAChartOperateWidget::figureCloseing, this, &DAChartManageWidget::onFigureCloseing);
    connect(cow, &DAChartOperateWidget::currentFigureChanged, this, &DAChartManageWidget::onCurrentFigureChanged);
}

void DAChartManageWidget::onFigureCreated(DAFigureWidget* fig)
{
    int index = d_ptr->mChartOptWidget->getFigureIndex(fig);
    if (index < 0) {
        qCritical() << tr("get figure create signal,but can not find figure index");  // cn: 获取了绘图创建的信号，但无法找到绘图的索引
        return;
    }
    QTreeView* treeview          = new QTreeView(this);
    DAFigureTreeModel* treemodel = new DAFigureTreeModel(treeview);
    treemodel->setFigure(fig);
    treeview->setModel(treemodel);
    ui->stackedWidget->insertWidget(index, treeview);
}

void DAChartManageWidget::onFigureCloseing(DAFigureWidget* fig)
{
    int index = d_ptr->mChartOptWidget->getFigureIndex(fig);
    if (index < 0) {
        qCritical() << tr("get figure close signal,but can not find figure index");  // cn: 获取了绘图关闭的信号，但无法找到绘图的索引
        return;
    }
    QWidget* w = ui->stackedWidget->widget(index);
    ui->stackedWidget->removeWidget(w);
    w->deleteLater();
}

void DAChartManageWidget::onCurrentFigureChanged(DAFigureWidget* fig, int index)
{
    Q_UNUSED(fig);
    ui->stackedWidget->setCurrentIndex(index);
}

}
