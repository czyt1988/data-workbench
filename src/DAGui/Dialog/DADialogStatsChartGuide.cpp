#include "DADialogStatsChartGuide.h"
#include "ui_DADialogStatsChartGuide.h"
#include <QDebug>
// DA
#include "DADataManager.h"
#include "DAAbstractStatsChartAddWidget.h"
#include "DAChartAddStatsHistplotWidget.h"
#include "DAChartAddStatsKdeplot1dWidget.h"
#include "DAChartAddStatsKdeplot2dWidget.h"
#include "DAChartAddStatsBoxplotWidget.h"
#include "DAChartAddStatsHeatmapWidget.h"
#include "DAChartAddStatsScatterplotWidget.h"
#include "DAChartAddStatsBarplotWidget.h"
#include "DAChartAddStatsRegplotWidget.h"
#include "DAChartAddStatsEcdfplotWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DALogCategory.h"

namespace DA
{

class DADialogStatsChartGuide::PrivateData
{
    DA_DECLARE_PUBLIC(DADialogStatsChartGuide)
public:
    PrivateData(DADialogStatsChartGuide* p);

    DAChartAddStatsHistplotWidget* mHistplot { nullptr };
    DAChartAddStatsKdeplot1dWidget* mKdeplot1d { nullptr };
    DAChartAddStatsKdeplot2dWidget* mKdeplot2d { nullptr };
    DAChartAddStatsBoxplotWidget* mBoxplot { nullptr };
    DAChartAddStatsHeatmapWidget* mHeatmap { nullptr };
    DAChartAddStatsScatterplotWidget* mScatterplot { nullptr };
    DAChartAddStatsBarplotWidget* mBarplot { nullptr };
    DAChartAddStatsRegplotWidget* mRegplot { nullptr };
    DAChartAddStatsEcdfplotWidget* mEcdfplot { nullptr };
};

DADialogStatsChartGuide::PrivateData::PrivateData(DADialogStatsChartGuide* p) : q_ptr(p)
{
}

//----------------------------------------------------
// DADialogStatsChartGuide
//----------------------------------------------------

DADialogStatsChartGuide::DADialogStatsChartGuide(QWidget* parent)
    : QDialog(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DADialogStatsChartGuide)
{
    ui->setupUi(this);
    DA_D(d);

    initListWidget();

    // 创建 9 个统计绘图设置 widget
    d->mHistplot    = new DAChartAddStatsHistplotWidget();
    d->mKdeplot1d   = new DAChartAddStatsKdeplot1dWidget();
    d->mKdeplot2d   = new DAChartAddStatsKdeplot2dWidget();
    d->mBoxplot     = new DAChartAddStatsBoxplotWidget();
    d->mHeatmap     = new DAChartAddStatsHeatmapWidget();
    d->mScatterplot = new DAChartAddStatsScatterplotWidget();
    d->mBarplot     = new DAChartAddStatsBarplotWidget();
    d->mRegplot     = new DAChartAddStatsRegplotWidget();
    d->mEcdfplot    = new DAChartAddStatsEcdfplotWidget();

    // 添加到 stackedWidget
    ui->stackedWidget->addWidget(d->mHistplot);
    ui->stackedWidget->addWidget(d->mKdeplot1d);
    ui->stackedWidget->addWidget(d->mKdeplot2d);
    ui->stackedWidget->addWidget(d->mBoxplot);
    ui->stackedWidget->addWidget(d->mHeatmap);
    ui->stackedWidget->addWidget(d->mScatterplot);
    ui->stackedWidget->addWidget(d->mBarplot);
    ui->stackedWidget->addWidget(d->mRegplot);
    ui->stackedWidget->addWidget(d->mEcdfplot);

    // 信号连接
    connect(ui->listWidgetChartType, &QListWidget::currentItemChanged,
            this, &DADialogStatsChartGuide::onListWidgetCurrentItemChanged);
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &DADialogStatsChartGuide::onAccepted);

    // 将每个 widget 的 plotRequested 信号转发为本对话框的同名信号
    auto forwardSignal = [this](const QJsonObject& params, DAFigureWidget* fig, DAChartWidget* chart) {
        Q_EMIT plotRequested(params, fig, chart);
    };
    connect(d->mHistplot, &DAAbstractStatsChartAddWidget::plotRequested, this, forwardSignal);
    connect(d->mKdeplot1d, &DAAbstractStatsChartAddWidget::plotRequested, this, forwardSignal);
    connect(d->mKdeplot2d, &DAAbstractStatsChartAddWidget::plotRequested, this, forwardSignal);
    connect(d->mBoxplot, &DAAbstractStatsChartAddWidget::plotRequested, this, forwardSignal);
    connect(d->mHeatmap, &DAAbstractStatsChartAddWidget::plotRequested, this, forwardSignal);
    connect(d->mScatterplot, &DAAbstractStatsChartAddWidget::plotRequested, this, forwardSignal);
    connect(d->mBarplot, &DAAbstractStatsChartAddWidget::plotRequested, this, forwardSignal);
    connect(d->mRegplot, &DAAbstractStatsChartAddWidget::plotRequested, this, forwardSignal);
    connect(d->mEcdfplot, &DAAbstractStatsChartAddWidget::plotRequested, this, forwardSignal);
}

DADialogStatsChartGuide::~DADialogStatsChartGuide()
{
    delete ui;
}

void DADialogStatsChartGuide::initListWidget()
{
    QListWidgetItem* item = nullptr;
    // histplot
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/stats-histplot.svg"),
                               tr("Histplot"));  // cn:直方图
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::StatsHistplot));
    ui->listWidgetChartType->addItem(item);
    // kdeplot 1d
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/stats-kdeplot.svg"),
                               tr("KDE 1D"));  // cn:一维核密度
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::StatsKdeplot1d));
    ui->listWidgetChartType->addItem(item);
    // kdeplot 2d
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/stats-kdeplot-2d.svg"),
                               tr("KDE 2D"));  // cn:二维核密度
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::StatsKdeplot2d));
    ui->listWidgetChartType->addItem(item);
    // boxplot
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/stats-boxplot.svg"),
                               tr("Boxplot"));  // cn:箱线图
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::StatsBoxplot));
    ui->listWidgetChartType->addItem(item);
    // heatmap
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/stats-heatmap.svg"),
                               tr("Heatmap"));  // cn:热力图
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::StatsHeatmap));
    ui->listWidgetChartType->addItem(item);
    // scatterplot
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/stats-scatterplot.svg"),
                               tr("Scatter"));  // cn:散点图
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::StatsScatterplot));
    ui->listWidgetChartType->addItem(item);
    // barplot
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/stats-barplot.svg"),
                               tr("Barplot"));  // cn:柱状图
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::StatsBarplot));
    ui->listWidgetChartType->addItem(item);
    // regplot
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/stats-regplot.svg"),
                               tr("Regplot"));  // cn:回归图
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::StatsRegplot));
    ui->listWidgetChartType->addItem(item);
    // ecdfplot
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/stats-ecdfplot.svg"),
                               tr("ECDF"));  // cn:经验累积分布
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::StatsECDFplot));
    ui->listWidgetChartType->addItem(item);

    // 默认选中第一项
    ui->listWidgetChartType->setCurrentRow(0);
}

void DADialogStatsChartGuide::ensureWidgetDataManager(DAAbstractStatsChartAddWidget* w)
{
    if (w && !mInitializedWidgets.contains(w)) {
        w->setDataManager(mDataMgr);
        mInitializedWidgets.insert(w);
    }
}

void DADialogStatsChartGuide::propagateFigureChart(DAAbstractStatsChartAddWidget* w)
{
    if (w) {
        w->setFigureWidget(mFigureWidget);
        w->setChartWidget(mChartWidget);
    }
}

void DADialogStatsChartGuide::setDataManager(DADataManager* dmgr)
{
    mDataMgr = dmgr;
    mInitializedWidgets.clear();
    // 预初始化当前选中的 widget
    QListWidgetItem* cur = ui->listWidgetChartType->currentItem();
    if (cur) {
        DA::DAChartTypes ct = static_cast< DA::DAChartTypes >(cur->data(Qt::UserRole).toInt());
        if (DAAbstractStatsChartAddWidget* w = getStatsChartAddWidget(ct)) {
            ensureWidgetDataManager(w);
        }
    }
}

void DADialogStatsChartGuide::setFigureWidget(DAFigureWidget* fig)
{
    mFigureWidget = fig;
    for (auto* w : mInitializedWidgets) {
        w->setFigureWidget(fig);
    }
}

void DADialogStatsChartGuide::setChartWidget(DAChartWidget* chart)
{
    mChartWidget = chart;
    for (auto* w : mInitializedWidgets) {
        w->setChartWidget(chart);
    }
}

DA::DAChartTypes DADialogStatsChartGuide::getCurrentChartType() const
{
    QListWidgetItem* item = ui->listWidgetChartType->currentItem();
    if (item == nullptr) {
        return DA::DAChartTypes::Unknow;
    }
    return static_cast< DA::DAChartTypes >(item->data(Qt::UserRole).toInt());
}

void DADialogStatsChartGuide::setCurrentChartType(DA::DAChartTypes t)
{
    int c = ui->listWidgetChartType->count();
    for (int i = 0; i < c; ++i) {
        auto item = ui->listWidgetChartType->item(i);
        int v     = item->data(Qt::UserRole).toInt();
        if (v == static_cast< int >(t)) {
            ui->listWidgetChartType->setCurrentItem(item);
            break;
        }
    }
    // 确保当前 widget 始终获取到 figure/chart（同类型重复调用时
    // currentItemChanged 不触发，需要手动传播）
    if (DAAbstractStatsChartAddWidget* w = getCurrentStatsChartAddWidget()) {
        ensureWidgetDataManager(w);
        propagateFigureChart(w);
    }
}

DAAbstractStatsChartAddWidget* DADialogStatsChartGuide::getCurrentStatsChartAddWidget() const
{
    return qobject_cast< DAAbstractStatsChartAddWidget* >(ui->stackedWidget->currentWidget());
}

DAAbstractStatsChartAddWidget* DADialogStatsChartGuide::getStatsChartAddWidget(DAChartTypes chartType) const
{
    DA_DC(d);
    switch (chartType) {
    case DA::DAChartTypes::StatsHistplot:
        return d->mHistplot;
    case DA::DAChartTypes::StatsKdeplot1d:
        return d->mKdeplot1d;
    case DA::DAChartTypes::StatsKdeplot2d:
        return d->mKdeplot2d;
    case DA::DAChartTypes::StatsBoxplot:
        return d->mBoxplot;
    case DA::DAChartTypes::StatsHeatmap:
        return d->mHeatmap;
    case DA::DAChartTypes::StatsScatterplot:
        return d->mScatterplot;
    case DA::DAChartTypes::StatsBarplot:
        return d->mBarplot;
    case DA::DAChartTypes::StatsRegplot:
        return d->mRegplot;
    case DA::DAChartTypes::StatsECDFplot:
        return d->mEcdfplot;
    default:
        break;
    }
    return nullptr;
}

void DADialogStatsChartGuide::onListWidgetCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);
    DA_D(d);
    DA::DAChartTypes ct = static_cast< DA::DAChartTypes >(current->data(Qt::UserRole).toInt());
    // 延迟初始化：切换到该 widget 时才设置 dataManager
    if (DAAbstractStatsChartAddWidget* w = getStatsChartAddWidget(ct)) {
        ensureWidgetDataManager(w);
        propagateFigureChart(w);
    }
    // 切换 stackedWidget 页面
    switch (ct) {
    case DA::DAChartTypes::StatsHistplot:
        ui->stackedWidget->setCurrentWidget(d->mHistplot);
        break;
    case DA::DAChartTypes::StatsKdeplot1d:
        ui->stackedWidget->setCurrentWidget(d->mKdeplot1d);
        break;
    case DA::DAChartTypes::StatsKdeplot2d:
        ui->stackedWidget->setCurrentWidget(d->mKdeplot2d);
        break;
    case DA::DAChartTypes::StatsBoxplot:
        ui->stackedWidget->setCurrentWidget(d->mBoxplot);
        break;
    case DA::DAChartTypes::StatsHeatmap:
        ui->stackedWidget->setCurrentWidget(d->mHeatmap);
        break;
    case DA::DAChartTypes::StatsScatterplot:
        ui->stackedWidget->setCurrentWidget(d->mScatterplot);
        break;
    case DA::DAChartTypes::StatsBarplot:
        ui->stackedWidget->setCurrentWidget(d->mBarplot);
        break;
    case DA::DAChartTypes::StatsRegplot:
        ui->stackedWidget->setCurrentWidget(d->mRegplot);
        break;
    case DA::DAChartTypes::StatsECDFplot:
        ui->stackedWidget->setCurrentWidget(d->mEcdfplot);
        break;
    default:
        break;
    }
}

void DADialogStatsChartGuide::onAccepted()
{
    DAAbstractStatsChartAddWidget* w = getCurrentStatsChartAddWidget();
    qDebug() << "[DADialogStatsChartGuide::onAccepted] widget=" << w
             << "type=" << static_cast<int>(getCurrentChartType())
             << "fig=" << mFigureWidget << "chart=" << mChartWidget;
    if (!w) {
        return;
    }
    // 调用 widget 的确认逻辑：验证数据选择、设置 __da_data__ 属性、
    // 构建完整参数（含 column, __plot_type__ 等），并发射 widget 自身的
    // plotRequested 信号。该信号已在构造函数中被转发到本对话框的同名信号。
    w->onButtonBoxAccepted();

    // 检查 widget 是否正确设置了 __da_data__
    QVariant dataVar = w->property("__da_data__");
    qDebug() << "[DADialogStatsChartGuide::onAccepted] __da_data__ valid="
             << dataVar.isValid() << "canConvert=" << dataVar.canConvert<DAData>();
}

}  // namespace DA
