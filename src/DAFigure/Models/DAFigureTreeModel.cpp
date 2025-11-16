#include "DAFigureTreeModel.h"
#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include "DAChartUtil.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_grid.h"
#include "qwt_text.h"
#include "qwt_color_map.h"
#include "qwt_column_symbol.h"
#include "qwt_plot_textlabel.h"
#include "qwt_plot_zoneitem.h"
#include "qwt_plot_vectorfield.h"
#include "qwt_graphic.h"
#include "DAChartUtil.h"
#define DAFigureTreeModel_Debug_Print 1
#define DAChartWidgetStandardItem_Ptr_Role Qt::UserRole + 1

namespace DA
{
bool standardItemIterator(QStandardItem* startItem,
                          std::function< bool(QStandardItem*, QStandardItem*) > fun,
                          bool firstColumnOnly);
//===================================================
// DAChartItemStandardItem
//===================================================

DAChartItemStandardItem::DAChartItemStandardItem(DAChartWidget* c, QwtPlotItem* i, int col) : QStandardItem()
{
    setChart(c);
    setItem(i);
    if (0 == col) {
        setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable);
        setEditable(true);
    } else {
        setFlags(Qt::ItemIsSelectable);
        setEditable(false);
    }
}

QVariant DAChartItemStandardItem::data(int role) const
{
    int c = column();
    switch (role) {
    case Qt::DisplayRole:
        return dataDisplayRole(_item, c);
    case Qt::DecorationRole:
        return dataDecorationRole(_item, c);
    default:
        break;
    }
    return QVariant();
}

void DAChartItemStandardItem::setData(const QVariant& value, int role)
{
    int c = column();
    if (Qt::EditRole == role) {
        return setDataEditRole(value, _item, c);
    }
}

DAChartWidget* DAChartItemStandardItem::getChart() const
{
    return _chart;
}

void DAChartItemStandardItem::setChart(DAChartWidget* c)
{
    _chart = c;
}

QwtPlotItem* DAChartItemStandardItem::getItem() const
{
    return _item;
}

void DAChartItemStandardItem::setItem(QwtPlotItem* i)
{
    _item = i;
}

QVariant DAChartItemStandardItem::dataDisplayRole(QwtPlotItem* item, int c) const
{
    switch (c) {
    case 0:  // 名称
    {
        return getItemName(item);
    }
    default:
        break;
    }
    return QVariant();
}

void DAChartItemStandardItem::setDataEditRole(const QVariant& value, QwtPlotItem* item, int c)
{
    switch (c) {
    case 0:  // 名称 - 显示图标
    {
        QString str = value.toString();
        if (str.isEmpty()) {
            return;
        }
        item->setTitle(str);
        emitDataChanged();
    } break;
    default:
        break;
    }
}

QVariant DAChartItemStandardItem::dataDecorationRole(QwtPlotItem* item, int c) const
{
    switch (c) {
    case 0:  // 名称 - 显示图标
    {
        return DAFigureTreeModel::chartItemToIcon(item);
    }
    case 1:  // 颜色
    {
        QBrush brush = DAChartUtil::getPlotItemBrush(item);
        if (Qt::NoBrush != brush.style()) {
            return DAFigureTreeModel::brushIcon(brush);
        }
        // 如果是NoBrush，就返回空;
        return QVariant();
    }
    case 2:  // 可见性
    {
        static QIcon s_icon_invisible(":/DAFigure/icon/chartitem-invisible.svg");
        static QIcon s_icon_visible(":/DAFigure/icon/chartitem-visible.svg");
        return ((item->isVisible()) ? s_icon_visible : s_icon_invisible);
    } break;
    default:
        break;
    }
    return QVariant();
}

QString DAChartItemStandardItem::getItemName(QwtPlotItem* item)
{
    QString str  = item->title().text();
    bool isEmpty = str.isEmpty();
    if (isEmpty) {
        auto plot = item->plot();
        if (plot) {
            str = QString::number(plot->itemList().indexOf(item) + 1);
        } else {
            str = QObject::tr("untitle");  // cn:未命名
        }
    }
    switch (item->rtti()) {
    //! Unspecific value, that can be used, when it doesn't matter
    case QwtPlotItem::Rtti_PlotItem:
        return QObject::tr("item[%1]").arg(item->title().text());  // cn 图元[%1]
    //! For QwtPlotGrid
    case QwtPlotItem::Rtti_PlotGrid:
        return QObject::tr("grid");  // cn:网格
    //! For QwtPlotScaleItem
    case QwtPlotItem::Rtti_PlotScale:
        return (isEmpty ? QObject::tr("scale-%1").arg(str) : str);  // cn:比例图元-%1
    //! For QwtPlotLegendItem
    case QwtPlotItem::Rtti_PlotLegend:
        return QObject::tr("legend-%1").arg(str);  // cn:图例-%1
    //! For QwtPlotMarker
    case QwtPlotItem::Rtti_PlotMarker:
        return QObject::tr("marker-%1").arg(str);  // cn:标记-%1
    //! For QwtPlotCurve
    case QwtPlotItem::Rtti_PlotCurve:
        return (isEmpty ? QObject::tr("curve-%1").arg(str) : str);  // cn:曲线-%1
    //! For QwtPlotSpectroCurve
    case QwtPlotItem::Rtti_PlotSpectroCurve:  // Curve that displays 3D points as dots, where the z coordinate is mapped to a color.
        return (isEmpty ? QObject::tr("spectro-%1").arg(str) : str);  // cn:色谱图-%1
    //! For QwtPlotIntervalCurve
    case QwtPlotItem::Rtti_PlotIntervalCurve:  // interval curve represents a series of samples, where each value is associated with an interval
        return (isEmpty ? QObject::tr("interval curve-%1").arg(str) : str);  // cn:区间图-%1
    //! For QwtPlotHistogram
    case QwtPlotItem::Rtti_PlotHistogram:  // histogram represents a series of samples, where an interval is associated with a value
        return (isEmpty ? QObject::tr("histogram-%1").arg(str) : str);  // cn:直方图-%1
    //! For QwtPlotSpectrogram
    case QwtPlotItem::Rtti_PlotSpectrogram:  // A spectrogram displays 3-dimensional data, where the 3rd dimension ( the intensity ) is displayed using colors.
        return (isEmpty ? QObject::tr("spectrogram-%1").arg(str) : str);  // cn:谱图-%1
    //! For QwtPlotGraphicItem, QwtPlotSvgItem
    case QwtPlotItem::Rtti_PlotGraphic:                               // display graphic
        return (isEmpty ? QObject::tr("graphic-%1").arg(str) : str);  // cn:图像-%1
    //! For QwtPlotTradingCurve
    case QwtPlotItem::Rtti_PlotTradingCurve:  // OHLC illustrates movements in the price of a financial instrument over time
        return (isEmpty ? QObject::tr("OHLC-%1").arg(str) : str);  // cn:OHLC图-%1
    //! For QwtPlotBarChart
    case QwtPlotItem::Rtti_PlotBarChart:                          // bar chart displays a series of a values as bars
        return (isEmpty ? QObject::tr("bar-%1").arg(str) : str);  // cn:柱状图-%1
    //! For QwtPlotMultiBarChart
    case QwtPlotItem::Rtti_PlotMultiBarChart:  // multibar chart displays a series of a samples that consist each of a set of values
        return (isEmpty ? QObject::tr("multibar-%1").arg(str) : str);  // cn:柱状图-%1
    //! For QwtPlotShapeItem
    case QwtPlotItem::Rtti_PlotShape:                               // displays any graphical shape
        return (isEmpty ? QObject::tr("shape-%1").arg(str) : str);  // cn:形状-%1
    //! For QwtPlotTextLabel
    case QwtPlotItem::Rtti_PlotTextLabel:                          // displays a text label
        return (isEmpty ? QObject::tr("text-%1").arg(str) : str);  // cn:文本-%1
    //! For QwtPlotZoneItem
    case QwtPlotItem::Rtti_PlotZone:                               // displays a zone
        return (isEmpty ? QObject::tr("zone-%1").arg(str) : str);  // cn:区间-%1
    //! For QwtPlotVectorField
    case QwtPlotItem::Rtti_PlotVectorField:                          // quiver chart represents a vector field
        return (isEmpty ? QObject::tr("quiver-%1").arg(str) : str);  // cn:流场图-%1
    default:
        break;
    }
    return QObject::tr("unknow-%1").arg(str);
}

//===================================================
// DAChartWidgetStandardItem
//===================================================

DAChartWidgetStandardItem::DAChartWidgetStandardItem(DAFigureWidget* fig, DAChartWidget* w, int col) : QStandardItem()
{
    setFigure(fig);
    setChart(w);
    if (0 == col) {
        setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable);
        setEditable(true);
    } else {
        setFlags(Qt::ItemIsSelectable);
        setEditable(false);
    }
}

DAFigureWidget* DAChartWidgetStandardItem::getFigure() const
{
    return _figure;
}

void DAChartWidgetStandardItem::setFigure(DAFigureWidget* figure)
{
    _figure = figure;
}

DAChartWidget* DAChartWidgetStandardItem::getChart() const
{
    return _chart;
}

void DAChartWidgetStandardItem::setChart(DAChartWidget* chart)
{
    _chart = chart;
}

void DAChartWidgetStandardItem::appendChartItem(QwtPlotItem* i)
{
    appendRow({ new DAChartItemStandardItem(_chart, i),
                new DAChartItemStandardItem(_chart, i, 1),
                new DAChartItemStandardItem(_chart, i, 2) });
}

QString DAChartWidgetStandardItem::getChartTitle(DAFigureWidget* fig, DAChartWidget* c)
{
    if (!c) {
        return QObject::tr("untitle-chart");  // cn:图-未命名
    }
    QString str = c->getChartTitle();
    if (!str.isEmpty()) {
        return str;
    }
    if (!fig) {
        return QObject::tr("untitle-chart");  // cn:图-未命名
    }
    auto charts = fig->getCharts();
    return QObject::tr("chart-%1").arg(charts.indexOf(c));
}

QVariant DAChartWidgetStandardItem::data(int role) const
{
    int c = column();
    switch (role) {
    case Qt::DisplayRole: {
        return dataDisplayRole(c);
    } break;
    case Qt::DecorationRole: {
        return dataDecorationRole(c);
    } break;
    default:
        break;
    }
    return QVariant();
}

QVariant DAChartWidgetStandardItem::dataDisplayRole(int c) const
{
    switch (c) {
    case 0: {
        return getChartTitle(_figure, _chart);
    }
    default:
        break;
    }
    return QVariant();
}

QVariant DAChartWidgetStandardItem::dataDecorationRole(int c) const
{
    switch (c) {
    case 0: {
        static QIcon s_charticon(":/DAFigure/icon/chart.svg");
        return s_charticon;
    } break;
    case 1: {
        QBrush b = _chart->getChartBackBrush();
        if (b.style() == Qt::NoBrush) {
            return QVariant();
        }
        return DAFigureTreeModel::brushIcon(b);
    } break;
    case 2: {
        // 第3列显示可见性
        static QIcon s_icon_invisible(":/DAFigure/icon/chartitem-invisible.svg");
        static QIcon s_icon_visible(":/DAFigure/icon/chartitem-visible.svg");
        return ((_chart->isVisible()) ? s_icon_visible : s_icon_invisible);
    } break;
    default:
        break;
    }
    return QVariant();
}

//===================================================
// DAFigureTreeModelPrivate
//===================================================

class DAFigureTreeModel::PrivateData
{
    DA_DECLARE_PUBLIC(DAFigureTreeModel)
public:
    PrivateData(DAFigureTreeModel* p);
    //
    DAChartWidgetStandardItem* chartWidgetToChartItem(const DAChartWidget* chart);

public:
    QwtFigure* mFig { nullptr };
    QMap< QwtPlot*, QStandardItem* > m_plotItems;
    QMap< QwtPlotItem*, QStandardItem* > m_plotItemItems;
    QList< DAChartWidget* > mCharts;
};

DAFigureTreeModel::PrivateData::PrivateData(DAFigureTreeModel* p) : q_ptr(p)
{
}

/**
 * @brief DAChartWidget获取对应的DAChartWidgetStandardItem
 * @param chart
 * @return
 */
DAChartWidgetStandardItem* DAFigureTreeModel::PrivateData::chartWidgetToChartItem(const DAChartWidget* chart)
{
    DAChartWidgetStandardItem* chartItem = nullptr;

    int r = q_ptr->rowCount();
    for (int i = 0; i < r; ++i) {
        QStandardItem* it = q_ptr->item(i);
        if (DAChartWidgetStandardItem_Type == it->type()) {
            DAChartWidgetStandardItem* ci = static_cast< DAChartWidgetStandardItem* >(it);
            if (ci->getChart() == chart) {
                chartItem = ci;
                break;
            }
        }
    }
    return chartItem;
}

//===================================================
// DAFigureTreeModel
//===================================================
DAFigureTreeModel::DAFigureTreeModel(QObject* parent) : QStandardItemModel(parent), DA_PIMPL_CONSTRUCT
{
}

DAFigureTreeModel::~DAFigureTreeModel()
{
}

void DAFigureTreeModel::onChartRemoved(DAChartWidget* c)
{
    int index = d_ptr->mCharts.indexOf(c);
    if (index < 0) {
        return;
    }
    QStandardItem* ci = takeItem(index);
    if (ci) {
        delete ci;
    }
}

void DAFigureTreeModel::onChartAdded(DAChartWidget* c)
{
    QList< DAChartWidget* > charts = d_ptr->mFig->getCharts();

    int index = charts.indexOf(c);
    if (index < 0) {
        return;
    }
    d_ptr->mCharts = charts;
    insertRow(index,
              { new DAChartWidgetStandardItem(d_ptr->mFig, c, 0),
                new DAChartWidgetStandardItem(d_ptr->mFig, c, 1),
                new DAChartWidgetStandardItem(d_ptr->mFig, c, 2) });
    //
    QwtPlotItemList its = c->itemList();
    for (QwtPlotItem* i : qAsConst(its)) {
        addChartItem(i);
    }
    connect(c, &DAChartWidget::itemAttached, this, &DAFigureTreeModel::onChartItemAttached);
    connect(c, &DAChartWidget::legendDataChanged, this, &DAFigureTreeModel::onLegendDataChanged);
}

void DAFigureTreeModel::onChartItemAttached(QwtPlotItem* plotItem, bool on)
{
    if (nullptr == plotItem) {
        return;
    }
    if (on) {
        addChartItem(plotItem);
    } else {
        removeChartItem(plotItem);
    }
}

void DAFigureTreeModel::onLegendDataChanged(const QVariant& itemInfo, const QList< QwtLegendData >& data)
{
    QwtPlot* p = qobject_cast< QwtPlot* >(sender());
    if (nullptr == p) {
        qWarning() << tr("receive legend data changed signal,but can not cast sender to qwt plot");  // cn:接收到legend data changed信号，但无法把发送方转换为qwt plot
        return;
    }
    QwtPlotItem* item = p->infoToItem(itemInfo);
    if (nullptr == item) {
        qWarning() << tr("can not conver info to qwt plot item");  // cn:无法把信息转换为qwt plot item
        return;
    }
    QList< QStandardItem* > chartitems = findChartItems(item);
    for (QStandardItem* i : qAsConst(chartitems)) {
        qDebug() << "onLegendDataChanged,emit itemchanged item(" << i->row() << "," << i->column() << ")";
        emit itemChanged(i);
    }
}

/**
 * @brief 绘图删除
 * @param c
 */
void DAFigureTreeModel::onFigureDestroyed(QObject* c)
{
    clear();
    setHorizontalHeaderLabels({
        tr("name"),      // cn:名称
        tr("property"),  // cn:属性
        tr("visible")    // cn:可见性
    });
    d_ptr->mFig = nullptr;
    d_ptr->mCharts.clear();
}

void DAFigureTreeModel::onAxesAdded(QwtPlot* plot)
{
}

void DAFigureTreeModel::onAxesRemoved(QwtPlot* plot)
{
}

void DAFigureTreeModel::onFigureCleared()
{
}

void DAFigureTreeModel::onCurrentAxesChanged(QwtPlot* plot)
{
}

void DAFigureTreeModel::onItemAttached(QwtPlotItem* item, bool on)
{
}

/**
 * @brief 用于生成绘图对应的文字
 *
 * 如果想改变文字内容，可重写此函数
 * @param plot 绘图指针
 * @param fig figure指针
 * @return
 */
QString DAFigureTreeModel::plotTitleText(QwtPlot* plot)
{
    if (!plot) {
        return tr("unknow chart");  // cn:未知绘图
    }
    QString str = plot->title().text();
    if (!str.isEmpty()) {
        return str;
    }
    // 如果没有名字，则以第几个绘图命名
    QwtFigure* fig = getFigure();
    if (!fig) {
        return tr("untitle-chart");  // cn:绘图-未命名
    }
    auto charts = fig->allAxes(true);
    int index   = charts.indexOf(plot);
    if (index >= 0) {
        return QObject::tr("chart-%1").arg(index + 1);
    }
    return tr("untitle-chart");  // cn：绘图-未命名
}

void DAFigureTreeModel::addChartItem(QwtPlotItem* i)
{
    DAChartWidget* chart                       = static_cast< DAChartWidget* >(i->plot());
    DAChartWidgetStandardItem* chartWidgetItem = d_ptr->chartWidgetToChartItem(chart);
    if (nullptr == chartWidgetItem) {
        qCritical() << tr("Unable to find the tree node corresponding to the chart widget");  // cn:无法找到绘图窗口对应的树形节点
        return;
    }
    chartWidgetItem->appendChartItem(i);
}

/**
 * @brief item删除
 * @param chartItem
 */
void DAFigureTreeModel::removeChartItem(QwtPlotItem* i)
{
    QList< QStandardItem* > chartitems = findChartItems(i);
    for (QStandardItem* i : qAsConst(chartitems)) {
        delete i;
    }
}

/**
 * @brief 设置fig
 * @param fig
 */
void DAFigureTreeModel::setFigure(QwtFigure* fig)
{
    DA_D(d);
    if (d->mFig == fig) {
        return;
    }

    d->mFig = fig;
    // 清除所有现有连接
    clearAllConnections();

    rebuild();

    if (d_ptr->mFig) {
        disconnect(d_ptr->mFig, &DAFigureWidget::chartAdded, this, &DAFigureTreeModel::onChartAdded);
        disconnect(d_ptr->mFig, &DAFigureWidget::chartRemoved, this, &DAFigureTreeModel::onChartRemoved);
        disconnect(d_ptr->mFig, &DAFigureWidget::destroyed, this, &DAFigureTreeModel::onFigureDestroyed);
        for (DAChartWidget* w : qAsConst(d_ptr->mCharts)) {
            if (w) {
                disconnect(w, &DAChartWidget::itemAttached, this, &DAFigureTreeModel::onChartItemAttached);
                disconnect(w, &DAChartWidget::legendDataChanged, this, &DAFigureTreeModel::onLegendDataChanged);
            }
        }
    }
    d_ptr->mFig    = fig;
    d_ptr->mCharts = fig->getCharts();
    connect(fig, &DAFigureWidget::chartAdded, this, &DAFigureTreeModel::onChartAdded);
    connect(fig, &DAFigureWidget::chartRemoved, this, &DAFigureTreeModel::onChartRemoved);
    connect(fig, &DAFigureWidget::destroyed, this, &DAFigureTreeModel::onFigureDestroyed);
    for (DAChartWidget* w : qAsConst(d_ptr->mCharts)) {
        if (w) {
            // 把已有的chart添加
            onChartAdded(w);
            connect(w, &DAChartWidget::itemAttached, this, &DAFigureTreeModel::onChartItemAttached);
            connect(w, &DAChartWidget::legendDataChanged, this, &DAFigureTreeModel::onLegendDataChanged);
        }
    }
}

/**
 * @brief 获取管理的窗口
 * @return
 */
QwtFigure* DAFigureTreeModel::getFigure() const
{
    return d_ptr->mFig;
}

Qt::ItemFlags DAFigureTreeModel::flags(const QModelIndex& index) const
{
}

QVariant DAFigureTreeModel::data(const QModelIndex& index, int role) const
{
}

bool DAFigureTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
}

/**
 * @brief 重构整个树
 */
void DAFigureTreeModel::rebuild()
{
    clear();
    setHorizontalHeaderLabels({
        tr("name"),      // cn:名称
        tr("property"),  // cn:属性
        tr("visible")    // cn:可见性
    });
    DA_D(d);
    if (!d->mFig) {
        return;
    }
    d->m_plotItemItems.clear();
    d->m_plotItems.clear();
    const auto plots        = d->mFig->allAxes(true);
    QStandardItem* rootItem = invisibleRootItem();
    for (QwtPlot* plot : plots) {
        addPlotToModel(plot, rootItem);
    }
}

void DAFigureTreeModel::clearAllConnections()
{
}

void DAFigureTreeModel::addPlotToModel(QwtPlot* plot, QStandardItem* parentItem)
{
    DA_D(d);
    if (!plot || d->m_plotItems.contains(plot)) {
        // 避免重复添加和空指针
        return;
    }

    // 创建绘图节点 - 四列
    QStandardItem* plotItem = new QStandardItem(plotTitleText(plot));
    plotItem->setData(QVariant::fromValue(plot), ObjectRole);
    plotItem->setData(PlotItem, ItemTypeRole);
    // plotItem->setIcon(QIcon(":/icons/plot.png"));

    // 可见性列 - 绘图节点不需要可见性控制
    QStandardItem* visibilityItem = createVisibilityItem();

    // 颜色列 - 绘图节点不需要颜色显示
    QStandardItem* colorItem = createColorItem();

    parentItem->appendRow(QList< QStandardItem* >() << plotItem << visibilityItem << colorItem);
    d->m_plotItems[ plot ] = plotItem;

    // 添加宿主图层
    addLayerToModel(plot, plotItem);

    // 添加寄生图层
    QList< QwtPlot* > parasites = plot->parasitePlots();
    for (QwtPlot* parasite : parasites) {
        addLayerToModel(parasite, plotItem);
    }

    connect(plot, &QwtPlot::itemAttached, this, &DAFigureTreeModel::onItemAttached);
}

void DAFigureTreeModel::addLayerToModel(QwtPlot* plot, QStandardItem* parentItem)
{
    if (!plot)
        return;
    bool isHost       = plot->isHostPlot();
    QString layerName = isHost ? tr("Layer-1") :                             // cn: 图层-1
                            tr("Layer-%1").arg(parentItem->rowCount() + 1);  // cn:图层-%1

    QStandardItem* layerItem = new QStandardItem(layerName);
    layerItem->setData(QVariant::fromValue(static_cast< QObject* >(plot)), ObjectRole);
    layerItem->setData(LayerItem, ItemTypeRole);
    layerItem->setIcon(QIcon(":/icons/layer.png"));

    QStandardItem* layerTypeItem = new QStandardItem(tr("图层"));
    layerTypeItem->setData(LayerItem, ItemTypeRole);

    // 可见性列 - 图层节点不需要可见性控制
    QStandardItem* visibilityItem = createVisibilityItem();

    // 颜色列 - 图层节点不需要颜色显示
    QStandardItem* colorItem = createColorItem();

    parentItem->appendRow(QList< QStandardItem* >() << layerItem << layerTypeItem << visibilityItem << colorItem);

    addAxesToLayer(plot, layerItem);
    addPlotItemsToLayer(plot, layerItem);
}

QStandardItem* DAFigureTreeModel::createVisibilityItem(QwtPlotItem* item) const
{
    static QIcon s_icon_not_visible(":/DAFigure/icon/chartitem-invisible.svg");
    static QIcon s_icon_visible(":/DAFigure/icon/chartitem-visible.svg");
    QStandardItem* visibilityItem = new QStandardItem();
    if (item) {
        // 图元节点：显示复选框
        visibilityItem->setCheckable(true);
        visibilityItem->setEditable(false);
        visibilityItem->setData(item->isVisible() ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
        visibilityItem->setData(QVariant::fromValue(item), ObjectRole);

        // 设置图标表示可见性
        if (item->isVisible()) {
            visibilityItem->setIcon(s_icon_visible);
        } else {
            visibilityItem->setIcon(s_icon_not_visible);
        }
    } else {
        // 非图元节点：显示横线或空白
        visibilityItem->setText("");
        visibilityItem->setEditable(false);
    }

    return visibilityItem;
}

QStandardItem* DAFigureTreeModel::createColorItem(QwtPlotItem* item) const
{
    QStandardItem* colorItem = new QStandardItem();

    if (item) {
        QBrush brush = DAChartUtil::getPlotItemBrush(item);
        if (Qt::NoBrush != brush.style()) {
            // 创建颜色块
            QPixmap pixmap(22, 22);
            QPainter p(&pixmap);
            p.fillRect(pixmap.rect(), b);
            colorItem->setIcon(QIcon(pixmap));
            colorItem->setData(brush, ColorRole);
        }
        colorItem->setData(QVariant::fromValue(item), ObjectRole);
    } else {
        // 非图元节点：显示横线或空白
        colorItem->setText("");
    }

    colorItem->setEditable(false);
    return colorItem;
}

int DAFigureTreeModel::indexOfChart(DAChartWidget* c) const
{
    return d_ptr->mCharts.indexOf(c);
}

/**
 * @brief 查找和QwtPlotItem相关的QStandardItem
 * 复杂度为O(n)
 * @param i
 * @return
 */
QList< QStandardItem* > DAFigureTreeModel::findChartItems(QwtPlotItem* i)
{
    // 找到item
    QList< QStandardItem* > chartitems;
    standardItemIterator(
        invisibleRootItem(),
        [ &chartitems, i ](QStandardItem* par, QStandardItem* ite) -> bool {
            if (DAChartItemStandardItem_Type == ite->type()) {
                DAChartItemStandardItem* chartitem = static_cast< DAChartItemStandardItem* >(ite);
                if (chartitem->getItem() == i) {
                    chartitems.append(ite);
                }
            }
            return true;  // 遍历所有节点
        },
        false);
    return chartitems;
}

QIcon DAFigureTreeModel::chartItemToIcon(const QwtPlotItem* i)
{
    static QIcon s_default_chart_icon(":/DAFigure/icon/chart-item.svg");
    switch (i->rtti()) {
    //! Unspecific value, that can be used, when it doesn't matter
    case QwtPlotItem::Rtti_PlotItem:
        break;
    //! For QwtPlotGrid
    case QwtPlotItem::Rtti_PlotGrid: {
        static QIcon s_icon(":/DAFigure/icon/chart-grid.svg");
        return s_icon;
    }
    //! For QwtPlotScaleItem
    case QwtPlotItem::Rtti_PlotScale: {
        static QIcon s_icon(":/DAFigure/icon/chart-scale.svg");
        return s_icon;
    }
    //! For QwtPlotLegendItem
    case QwtPlotItem::Rtti_PlotLegend: {
        static QIcon s_icon(":/DAFigure/icon/chart-legend.svg");
        return s_icon;
    }
    //! For QwtPlotMarker
    case QwtPlotItem::Rtti_PlotMarker: {
        static QIcon s_icon(":/DAFigure/icon/chart-marker.svg");
        return s_icon;
    }

    //! For QwtPlotCurve
    case QwtPlotItem::Rtti_PlotCurve: {
        static QIcon s_icon(":/DAFigure/icon/chart-curve.svg");
        return s_icon;
    }
    //! For QwtPlotSpectroCurve
    case QwtPlotItem::Rtti_PlotSpectroCurve: {
        static QIcon s_icon(":/DAFigure/icon/chart-spectrocurve.svg");
        return s_icon;
    }
    //! For QwtPlotIntervalCurve
    case QwtPlotItem::Rtti_PlotIntervalCurve: {
        static QIcon s_icon(":/DAFigure/icon/chart-intervalcurve.svg");
        return s_icon;
    }

    //! For QwtPlotHistogram
    case QwtPlotItem::Rtti_PlotHistogram: {
        static QIcon s_icon(":/DAFigure/icon/chart-histogram.svg");
        return s_icon;
    }
    //! For QwtPlotSpectrogram
    case QwtPlotItem::Rtti_PlotSpectrogram: {
        static QIcon s_icon(":/DAFigure/icon/chart-spectrogram.svg");
        return s_icon;
    }

    case QwtPlotItem::Rtti_PlotGraphic: {
        static QIcon s_icon(":/DAFigure/icon/chart-graphic.svg");
        return s_icon;
    }

    //! For QwtPlotTradingCurve
    case QwtPlotItem::Rtti_PlotTradingCurve: {
        static QIcon s_icon(":/DAFigure/icon/chart-OHLC.svg");
        return s_icon;
    }

    //! For QwtPlotBarChart
    case QwtPlotItem::Rtti_PlotBarChart: {
        static QIcon s_icon(":/DAFigure/icon/chart-bar.svg");
        return s_icon;
    }

    //! For QwtPlotMultiBarChart
    case QwtPlotItem::Rtti_PlotMultiBarChart: {
        static QIcon s_icon(":/DAFigure/icon/chart-multibar.svg");
        return s_icon;
    }
    //! For QwtPlotShapeItem
    case QwtPlotItem::Rtti_PlotShape: {
        static QIcon s_icon(":/DAFigure/icon/chart-shapes.svg");
        return s_icon;
    }

    //! For QwtPlotTextLabel
    case QwtPlotItem::Rtti_PlotTextLabel: {
        static QIcon s_icon(":/DAFigure/icon/chart-textlabel.svg");
        return s_icon;
    }

    //! For QwtPlotZoneItem
    case QwtPlotItem::Rtti_PlotZone: {
        static QIcon s_icon(":/DAFigure/icon/chart-zone.svg");
        return s_icon;
    }
    //! For QwtPlotVectorField
    case QwtPlotItem::Rtti_PlotVectorField: {
        static QIcon s_icon(":/DAFigure/icon/chart-vectorfield.svg");
        return s_icon;
    }
    default:
        break;
    }
    return s_default_chart_icon;
}

/**
 * @brief 通过颜色绘制icon
 * @param c
 * @param drawBorder 是否绘制边框
 * @return
 */
QIcon DAFigureTreeModel::colorIcon(const QColor& c, bool drawBorder)
{
    QPixmap pixmap(32, 32);
    QPainter p(&pixmap);
    p.fillRect(pixmap.rect(), c);
    if (drawBorder) {
        p.setPen(Qt::black);
        p.drawRect(pixmap.rect());
    }
    return QIcon(pixmap);
}

/**
 * @brief 通过画刷绘制icon
 * @param b
 * @param drawBorder 是否绘制边框
 * @return
 */
QIcon DAFigureTreeModel::brushIcon(const QBrush& b, bool drawBorder)
{
    QPixmap pixmap(32, 32);
    QPainter p(&pixmap);
    p.fillRect(pixmap.rect(), b);
    if (drawBorder) {
        p.setPen(Qt::black);
        p.drawRect(pixmap.rect());
    }
    return QIcon(pixmap);
}

/**
 * @brief 递归遍历startItem下所有的QStandardItem，迭代过程会调用函数指针，函数指针第一个参数为父节点，第二个参数为遍历到的子节点
 * @param startItem
 * @param fun 回调函数第一个参数为父节点，第二个参数为遍历到的子节点,如果回调返回true，则继续递归，
 * 如果返回false则终止回调
 * @param firstColumnOnly 只迭代第一列，其余列不迭代@default false
 * @return 如果返回true，说明整个递归过程遍历了所有节点，如果返回false，说明遍历过程中断，
 * 此返回取决于回调函数，如果回调函数返回过false，则此函数必会返回false
 */
bool standardItemIterator(QStandardItem* startItem, std::function< bool(QStandardItem*, QStandardItem*) > fun, bool firstColumnOnly)
{
    if (startItem == nullptr) {
        return false;
    }
    int rc = startItem->rowCount();
    int cc = (firstColumnOnly ? 1 : startItem->columnCount());
    for (int r = 0; r < rc; ++r) {
        for (int c = 0; c < cc; ++c) {
            QStandardItem* citem = startItem->child(r, c);
            if (nullptr == citem) {
                continue;
            }
            if (!fun(startItem, citem)) {
                return false;
            }
            if (!standardItemIterator(citem, fun, firstColumnOnly)) {
                return false;
            }
        }
    }
    return true;
}
}  // End Of Namespace DA
