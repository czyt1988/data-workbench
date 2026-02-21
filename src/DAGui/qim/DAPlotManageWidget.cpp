#include "DAPlotManageWidget.h"
#include "ui_DAPlotManageWidget.h"
#include <QTreeView>
#include <QPointer>
#include <QDebug>
#include <QHash>
#include <QSet>
#include "DAPlotOperateWidget.h"
#include "DAFigureScrollArea.h"
#include "DAFigureTreeView.h"
namespace DA
{
//===============================================================
// PrivateData
//===============================================================
class DAPlotManageWidget::PrivateData
{
public:
    DA_DECLARE_PUBLIC(DAPlotManageWidget)
    PrivateData(DAPlotManageWidget* p);
    DAFigureElementSelection::SelectionColumns standardItemToSelectionColumns(QStandardItem* item);

public:
    QPointer< DAPlotOperateWidget > mChartOptWidget;
    bool mSetCurrentChartOnClicked { true };
    bool mSetCurrentChartOnDbClicked { true };
    QHash< DAFigureScrollArea*, DAFigureTreeView* > mFigureWidgetToTree;  ///< 建立figurewidget和tree的关系
};
DAPlotManageWidget::PrivateData::PrivateData(DAPlotManageWidget* p) : q_ptr(p)
{
}

DAFigureElementSelection::SelectionColumns DAPlotManageWidget::PrivateData::standardItemToSelectionColumns(QStandardItem* item)
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
// DAPlotManageWidget
//===================================================
DAPlotManageWidget::DAPlotManageWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAPlotManageWidget)
{
    ui->setupUi(this);
    connect(ui->toolButtonExpandAll, &QToolButton::clicked, this, &DAPlotManageWidget::expandCurrentTree);
    connect(ui->toolButtonCollapseAll, &QToolButton::clicked, this, &DAPlotManageWidget::collapseCurrentTree);
    connect(ui->toolButtonFigureSetting, &QToolButton::clicked, this, &DAPlotManageWidget::onToolButtonFigureSettingClicked);
    connect(
        ui->comboBoxFigure, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DAPlotManageWidget::onComboboxCurrentIndexChanged
    );
}

DAPlotManageWidget::~DAPlotManageWidget()
{
    delete ui;
}

void DAPlotManageWidget::setPlotOperateWidget(DAPlotOperateWidget* cow)
{
    if (d_ptr->mChartOptWidget) {
        DAPlotOperateWidget* old = d_ptr->mChartOptWidget;
        disconnect(old, nullptr, this, nullptr);
    }
    d_ptr->mChartOptWidget = cow;
    connect(cow, &DAPlotOperateWidget::figureCreated, this, &DAPlotManageWidget::onFigureCreated);
    connect(cow, &DAPlotOperateWidget::figureRemoving, this, &DAPlotManageWidget::onFigureCloseing);
    connect(cow, &DAPlotOperateWidget::currentFigureChanged, this, &DAPlotManageWidget::onCurrentFigureChanged);
}

void DAPlotManageWidget::setCurrentChartOnItemClicked(bool on)
{
    d_ptr->mSetCurrentChartOnClicked = on;
}

bool DAPlotManageWidget::isSetCurrentChartOnItemClicked() const
{
    return d_ptr->mSetCurrentChartOnClicked;
}

void DAPlotManageWidget::setCurrentChartOnItemDoubleClicked(bool on)
{
    d_ptr->mSetCurrentChartOnDbClicked = on;
}

bool DAPlotManageWidget::isSetCurrentChartOnItemDoubleClicked() const
{
    return d_ptr->mSetCurrentChartOnDbClicked;
}


void DAPlotManageWidget::expandCurrentTree()
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

void DAPlotManageWidget::collapseCurrentTree()
{
    DAFigureTreeView* tree = currentTreeView();
    if (!tree) {
        return;
    }
    tree->collapseAll();
}

DAFigureTreeView* DAPlotManageWidget::currentTreeView() const
{
    return qobject_cast< DAFigureTreeView* >(ui->stackedWidget->currentWidget());
}

void DAPlotManageWidget::setCurrentDisplayView(DAFigureScrollArea* fig)
{
    setStackCurrentFigure(fig);
    QSignalBlocker b(ui->comboBoxFigure);  // 避免触发currentIndexChanged信号导致再次切换stack
    setComboboxCurrentFigure(fig);
}

DAFigureScrollArea* DAPlotManageWidget::getComboboxFigure(int index) const
{
    return reinterpret_cast< DAFigureScrollArea* >(ui->comboBoxFigure->itemData(index).value< quintptr >());
}


DAFigureScrollArea* DAPlotManageWidget::getCurrentFigure() const
{
    return reinterpret_cast< DAFigureScrollArea* >(ui->comboBoxFigure->currentData().value< quintptr >());
}

void DAPlotManageWidget::setStackCurrentFigure(DAFigureScrollArea* fig)
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

void DAPlotManageWidget::setComboboxCurrentFigure(DAFigureScrollArea* fig)
{
    int c = ui->comboBoxFigure->count();
    for (int i = 0; i < c; ++i) {
        DAFigureScrollArea* w = getComboboxFigure(i);
        if (w == fig) {
            if (i != ui->comboBoxFigure->currentIndex()) {
                ui->comboBoxFigure->setCurrentIndex(i);
            }
        }
    }
}

void DAPlotManageWidget::onFigureCreated(DAFigureScrollArea* fig)
{
    int index = d_ptr->mChartOptWidget->getFigureIndex(fig);
    if (index < 0) {
        qCritical() << tr("get figure create signal,but can not find figure index"
        );  // cn: 获取了绘图创建的信号，但无法找到绘图的索引
        return;
    }

    DAFigureTreeView* figTreeview = new DAFigureTreeView(this);
    figTreeview->setFigureWidget(fig);
    d_ptr->mFigureWidgetToTree[ fig ] = figTreeview;
    connect(figTreeview, &DAFigureTreeView::itemCliecked, this, &DAPlotManageWidget::figureElementClicked);
    connect(figTreeview, &DAFigureTreeView::itemDbCliecked, this, &DAPlotManageWidget::figureElementDbClicked);
    ui->stackedWidget->insertWidget(index, figTreeview);

    ui->comboBoxFigure->insertItem(index, fig->windowTitle(), reinterpret_cast< quintptr >(fig));
}

void DAPlotManageWidget::onFigureCloseing(DAFigureScrollArea* fig)
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
        DAFigureScrollArea* tmp = getComboboxFigure(i);
        if (tmp == fig) {
            ui->comboBoxFigure->removeItem(i);
            break;
        }
    }
}

void DAPlotManageWidget::onCurrentFigureChanged(DAFigureScrollArea* fig, int index)
{
    Q_UNUSED(fig);
    Q_UNUSED(index);
    setCurrentDisplayView(fig);
}


void DAPlotManageWidget::onToolButtonFigureSettingClicked()
{
    if (DAFigureScrollArea* fig = getCurrentFigure()) {
        Q_EMIT requestFigureSetting(fig);
    }
}

void DAPlotManageWidget::onComboboxCurrentIndexChanged(int index)
{
    // 联动ChartOptWidget,此函数会触发槽onCurrentFigureChanged，调用setCurrentDisplayView，
    // 但在setCurrentDisplayView中，combobx的currentIndex已经是index，就不会再设置，从而避免递归调用
    d_ptr->mChartOptWidget->setCurrentFigure(index);
    if (DAFigureScrollArea* fig = getCurrentFigure()) {
        setStackCurrentFigure(fig);
        Q_EMIT selectFigureChanged(fig);
    }
}

}
