#include "DADialogChartGuide.h"
#include "ui_DADialogChartGuide.h"
#include <QPen>
// DA
#include "DADataManager.h"
#include "DAAbstractChartAddItemWidget.h"
#include "DAChartAddCurveWidget.h"
#include "DAChartAddBarWidget.h"
#include "DAChartAddIntervalCurveWidget.h"
#include "DAChartAddTradingCurveWidget.h"
#include "DAChartAddErrorBarWidget.h"
#include "DAChartAddBoxChartWidget.h"
#include "DAChartAddMultiBarWidget.h"
#include "DAChartAddHistogramWidget.h"
#include "DAChartAddContourWidget.h"
#include "DAChartAddVectorFieldWidget.h"
#include "DAChartAddSpectrogramWidget.h"
#include "DAChartUtil.h"
// qwt
#include "qwt_plot_curve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_spectrogram.h"

namespace DA
{
class DADialogChartGuide::PrivateData
{
    DA_DECLARE_PUBLIC(DADialogChartGuide)
public:
    PrivateData(DADialogChartGuide* p);
    DAChartAddCurveWidget* mAddCurve { nullptr };
    DAChartAddBarWidget* mAddBar { nullptr };
    DAChartAddIntervalCurveWidget* mAddIntervalCurve { nullptr };
    DAChartAddTradingCurveWidget* mAddTradingCurve { nullptr };
    DAChartAddSpectrogramWidget* mAddSpectroGram { nullptr };
    DAChartAddErrorBarWidget* mAddErrorBar { nullptr };
    DAChartAddBoxChartWidget* mAddBoxChart { nullptr };
    DAChartAddMultiBarWidget* mAddMultiBar { nullptr };
    DAChartAddHistogramWidget* mAddHistogram { nullptr };
    DAChartAddContourWidget* mAddContour { nullptr };
    DAChartAddVectorFieldWidget* mAddVectorField { nullptr };
};

DADialogChartGuide::PrivateData::PrivateData(DADialogChartGuide* p) : q_ptr(p)
{
}
//----------------------------------------------------
// DADialogChartGuide
//----------------------------------------------------

DADialogChartGuide::DADialogChartGuide(QWidget* parent)
    : QDialog(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DADialogChartGuide)
{
    ui->setupUi(this);
    DA_D(d);
    initListWidget();
    d->mAddCurve         = new DAChartAddCurveWidget();
    d->mAddBar           = new DAChartAddBarWidget();
    d->mAddIntervalCurve = new DAChartAddIntervalCurveWidget();
    d->mAddTradingCurve  = new DAChartAddTradingCurveWidget();
    d->mAddSpectroGram   = new DAChartAddSpectrogramWidget();
    d->mAddErrorBar      = new DAChartAddErrorBarWidget();
    d->mAddBoxChart      = new DAChartAddBoxChartWidget();
    d->mAddMultiBar      = new DAChartAddMultiBarWidget();
    d->mAddHistogram     = new DAChartAddHistogramWidget();
    d->mAddContour       = new DAChartAddContourWidget();
    d->mAddVectorField   = new DAChartAddVectorFieldWidget();
    ui->stackedWidget->addWidget(d->mAddCurve);
    ui->stackedWidget->addWidget(d->mAddBar);
    ui->stackedWidget->addWidget(d->mAddIntervalCurve);
    ui->stackedWidget->addWidget(d->mAddTradingCurve);
    ui->stackedWidget->addWidget(d->mAddSpectroGram);
    ui->stackedWidget->addWidget(d->mAddErrorBar);
    ui->stackedWidget->addWidget(d->mAddBoxChart);
    ui->stackedWidget->addWidget(d->mAddMultiBar);
    ui->stackedWidget->addWidget(d->mAddHistogram);
    ui->stackedWidget->addWidget(d->mAddContour);
    ui->stackedWidget->addWidget(d->mAddVectorField);
    connect(ui->listWidgetChartType, &QListWidget::currentItemChanged, this, &DADialogChartGuide::onListWidgetCurrentItemChanged);
}

DADialogChartGuide::~DADialogChartGuide()
{
    delete ui;
}

void DADialogChartGuide::initListWidget()
{
    QListWidgetItem* item = nullptr;
    // curve
    item = new QListWidgetItem(QIcon(":/DAGui/ChartType/icon/chart-type/chart-curve.svg"), tr("curve"));  // cn:曲线
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::Curve));
    ui->listWidgetChartType->addItem(item);
    // scatter
    item = new QListWidgetItem(QIcon(":/DAGui/ChartType/icon/chart-type/chart-scatter.svg"), tr("scatter"));  // cn:散点
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::Scatter));
    ui->listWidgetChartType->addItem(item);
    // bar
    item = new QListWidgetItem(QIcon(":/DAGui/ChartType/icon/chart-type/chart-bar.svg"), tr("bar"));  // cn:柱状
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::Bar));
    ui->listWidgetChartType->addItem(item);
    // errorbar
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/chart-intervalcurve.svg"), tr("error bar"));  // cn:误差棒
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::ErrorBar));
    ui->listWidgetChartType->addItem(item);
    // boxplot
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/chart-OHLC.svg"), tr("box"));  // cn:箱体
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::Box));
    ui->listWidgetChartType->addItem(item);
    // spectrogram
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/chart-spectrogram.svg"), tr("cloud map"));  // cn:云图
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::Spectrogram));
    ui->listWidgetChartType->addItem(item);
    // multibar
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/chart-multibar.svg"), tr("multi bar"));  // cn:多重柱状
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::MultiBar));
    ui->listWidgetChartType->addItem(item);
    // histogram
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/chart-histogram.svg"), tr("histogram"));  // cn:直方图
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::Histogram));
    ui->listWidgetChartType->addItem(item);
    // contour
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/chart-spectrocurve.svg"), tr("contour"));  // cn:等高线
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::Contour));
    ui->listWidgetChartType->addItem(item);
    // vectorfield
    item = new QListWidgetItem(QIcon(":/app/chart-type/Icon/chart-type/chart-vectorfield.svg"), tr("vector field"));  // cn:向量场
    item->setData(Qt::UserRole, static_cast< int >(DA::DAChartTypes::VectorField));
    ui->listWidgetChartType->addItem(item);
    // 初始化
    ui->listWidgetChartType->setCurrentRow(0);
}
/**
 * @brief 设置datamanager,会把combox填入所有的dataframe
 * @param dmgr
 */
void DADialogChartGuide::setDataManager(DADataManager* dmgr)
{
    int c = ui->stackedWidget->count();
    for (int i = 0; i < c; ++i) {
        if (DAAbstractChartAddItemWidget* w = qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->widget(i))) {
            w->setDataManager(dmgr);
        }
    }
}

/**
 * @brief 获取当前的绘图类型
 * @return
 */
DA::DAChartTypes DADialogChartGuide::getCurrentChartType() const
{
    QListWidgetItem* item = ui->listWidgetChartType->currentItem();
    if (item == nullptr) {
        return DA::DAChartTypes::Unknow;
    }
    return static_cast< DA::DAChartTypes >(item->data(Qt::UserRole).toInt());
}

/**
 * @brief 获取绘图item
 * @return  如果没有返回nullptr
 */
QwtPlotItem* DADialogChartGuide::createPlotItem()
{
    DAAbstractChartAddItemWidget* w = qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->currentWidget());
    if (!w) {
        return nullptr;
    }
    QwtPlotItem* item = w->createPlotItem();
    if (nullptr == item) {
        return nullptr;
    }
    // 针对不同的类型设置item属性
    initSetPlotItem(item);
    return item;
}

DAAbstractChartAddItemWidget* DADialogChartGuide::getCurrentChartAddItemWidget() const
{
    return qobject_cast< DAAbstractChartAddItemWidget* >(ui->stackedWidget->currentWidget());
}

DAAbstractChartAddItemWidget* DADialogChartGuide::getChartAddItemWidget(DAChartTypes chartType) const
{
    DA_DC(d);
    switch (chartType) {
    case DA::DAChartTypes::Curve:
    case DA::DAChartTypes::Scatter:
        return d->mAddCurve;
    case DA::DAChartTypes::Bar:
        return d->mAddBar;
    case DA::DAChartTypes::ErrorBar:
        return d->mAddErrorBar;
    case DA::DAChartTypes::Box:
        return d->mAddBoxChart;
    case DA::DAChartTypes::Spectrogram:
        return d->mAddSpectroGram;
    case DA::DAChartTypes::MultiBar:
        return d->mAddMultiBar;
    case DA::DAChartTypes::Histogram:
        return d->mAddHistogram;
    case DA::DAChartTypes::Contour:
        return d->mAddContour;
    case DA::DAChartTypes::VectorField:
        return d->mAddVectorField;
    default:
        break;
    }
    return nullptr;
}

/**
 * @brief 根据当前绘图类型设置item属性
 * @param item
 */
void DADialogChartGuide::initSetPlotItem(QwtPlotItem* item)
{
    DA::DAChartTypes ct = getCurrentChartType();
    switch (ct) {
    case DA::DAChartTypes::Scatter: {
        if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
            QwtPlotCurve* cur = static_cast< QwtPlotCurve* >(item);
            cur->setStyle(QwtPlotCurve::Dots);
        }
    } break;
    default:
        break;
    }
}

/**
 * @brief 设置当前的绘图类型
 * @param t
 */
void DADialogChartGuide::setCurrentChartType(DA::DAChartTypes t)
{
    int c = ui->listWidgetChartType->count();
    for (int i = 0; i < c; ++i) {
        auto item = ui->listWidgetChartType->item(i);
        int v     = item->data(Qt::UserRole).toInt();
        if (v == static_cast< int >(t)) {
            ui->listWidgetChartType->setCurrentItem(item);
        }
    }
}

void DADialogChartGuide::onListWidgetCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);
    DA_D(d);
    DA::DAChartTypes ct = static_cast< DA::DAChartTypes >(current->data(Qt::UserRole).toInt());
    switch (ct) {
    case DA::DAChartTypes::Curve:
        ui->stackedWidget->setCurrentWidget(d->mAddCurve);
        break;
    case DA::DAChartTypes::Scatter:
        ui->stackedWidget->setCurrentWidget(d->mAddCurve);
        break;
    case DA::DAChartTypes::Bar:
        ui->stackedWidget->setCurrentWidget(d->mAddBar);
        break;
    case DA::DAChartTypes::ErrorBar:
        ui->stackedWidget->setCurrentWidget(d->mAddErrorBar);
        break;
    case DA::DAChartTypes::Box:
        ui->stackedWidget->setCurrentWidget(d->mAddBoxChart);
        break;
    case DA::DAChartTypes::Spectrogram:
        ui->stackedWidget->setCurrentWidget(d->mAddSpectroGram);
        break;
    case DA::DAChartTypes::MultiBar:
        ui->stackedWidget->setCurrentWidget(d->mAddMultiBar);
        break;
    case DA::DAChartTypes::Histogram:
        ui->stackedWidget->setCurrentWidget(d->mAddHistogram);
        break;
    case DA::DAChartTypes::Contour:
        ui->stackedWidget->setCurrentWidget(d->mAddContour);
        break;
    case DA::DAChartTypes::VectorField:
        ui->stackedWidget->setCurrentWidget(d->mAddVectorField);
        break;
    default:
        break;
    }
}
}
