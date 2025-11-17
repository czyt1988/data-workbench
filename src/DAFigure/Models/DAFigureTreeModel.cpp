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
#include "qwt_symbol.h"
#include "DAChartUtil.h"
#define DAFigureTreeModel_Debug_Print 1

namespace DA
{

//----------------------------------------------------
//
//----------------------------------------------------

DAFigureTreeModel::DAFigureTreeModel(QObject* parent) : QStandardItemModel(parent), m_figure(nullptr)
{
    // 设置三列表头
    setHorizontalHeaderLabels(QStringList() << tr("element")   // cn:绘图元素
                                            << tr("visible")   // cn:可见性
                                            << tr("property")  // cn:属性
    );
}

DAFigureTreeModel::~DAFigureTreeModel()
{
    clearAllConnections();
}

void DAFigureTreeModel::setFigure(QwtFigure* figure)
{
    if (m_figure == figure) {
        return;
    }

    // 清除所有现有连接
    clearAllConnections();

    m_figure = figure;
    m_plotItems.clear();
    m_plotItemItems.clear();

    if (m_figure) {
        // 连接figure信号
        m_figureConnections << connect(m_figure, &QwtFigure::axesAdded, this, &DAFigureTreeModel::onAxesAdded);
        m_figureConnections << connect(m_figure, &QwtFigure::axesRemoved, this, &DAFigureTreeModel::onAxesRemoved);
        m_figureConnections << connect(m_figure, &QwtFigure::figureCleared, this, &DAFigureTreeModel::onFigureCleared);
        m_figureConnections << connect(
            m_figure, &QwtFigure::currentAxesChanged, this, &DAFigureTreeModel::onCurrentAxesChanged);
    }

    setupModel();
}

void DAFigureTreeModel::clearAllConnections()
{
    // 断开所有figure连接
    for (const QMetaObject::Connection& conn : m_figureConnections) {
        disconnect(conn);
    }
    m_figureConnections.clear();

    // 断开所有plot连接
    for (auto it = m_plotConnections.begin(); it != m_plotConnections.end(); ++it) {
        for (const QMetaObject::Connection& conn : it.value()) {
            disconnect(conn);
        }
    }
    m_plotConnections.clear();
}

void DAFigureTreeModel::setupModel()
{
    clear();
    m_plotItems.clear();
    m_plotItemItems.clear();

    if (!m_figure)
        return;

    QStandardItem* rootItem = invisibleRootItem();

    QList< QwtPlot* > plots = m_figure->allAxes();
    for (QwtPlot* plot : plots) {
        addPlotToModel(plot, rootItem);
    }
}

void DAFigureTreeModel::addPlotToModel(QwtPlot* plot, QStandardItem* parentItem)
{
    if (!plot || m_plotItems.contains(plot))
        return;

    // 创建绘图节点 - 三列
    QStandardItem* plotItem = new QStandardItem(plot->title().text());
    plotItem->setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), RolePlot);
    // plotItem->setData(QVariant::fromValue(static_cast< QObject* >(plot)), RolePlot);
    plotItem->setData(NodeTypePlot, RoleNodeType);
    plotItem->setIcon(QIcon(":/icons/plot.png"));

    // 可见性列 - 绘图节点不需要可见性控制
    QStandardItem* visibilityItem = createVisibilityItem();

    // 颜色列 - 绘图节点不需要颜色显示
    QStandardItem* colorItem = createColorItem();

    parentItem->appendRow(QList< QStandardItem* >() << plotItem << visibilityItem << colorItem);
    m_plotItems[ plot ] = plotItem;

    // 连接plot的信号
    QList< QMetaObject::Connection > plotConnections;
    plotConnections << connect(plot, &QwtPlot::itemAttached, this, &DAFigureTreeModel::onItemAttached);
    m_plotConnections[ plot ] = plotConnections;

    // 添加宿主图层
    addLayerToModel(plot, plotItem);

    // 添加寄生图层
    QList< QwtPlot* > parasites = plot->parasitePlots();
    for (QwtPlot* parasite : parasites) {
        addLayerToModel(parasite, plotItem);
    }
}

void DAFigureTreeModel::addLayerToModel(QwtPlot* plot, QStandardItem* parentItem)
{
    if (!plot)
        return;
    bool isHost       = plot->isHostPlot();
    QString layerName = isHost ? tr("Layer-1") :                             // cn: 图层-1
                            tr("Layer-%1").arg(parentItem->rowCount() + 1);  // cn:图层-%1

    QStandardItem* layerItem = new QStandardItem(layerName);
    layerItem->setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), RolePlot);
    // layerItem->setData(QVariant::fromValue(static_cast< QObject* >(plot)), RolePlot);
    layerItem->setData(NodeTypeLayer, RoleNodeType);
    // layerItem->setIcon(QIcon(":/icons/layer.png"));

    // 可见性列 - 图层节点不需要可见性控制
    QStandardItem* visibilityItem = createVisibilityItem();

    // 颜色列 - 图层节点不需要颜色显示
    QStandardItem* colorItem = createColorItem();

    parentItem->appendRow(QList< QStandardItem* >() << layerItem << visibilityItem << colorItem);

    addAxesToLayer(plot, layerItem);
    addPlotItemsToLayer(plot, layerItem);
}

void DAFigureTreeModel::addAxesToLayer(QwtPlot* plot, QStandardItem* layerItem)
{
    QStandardItem* axesFolder = new QStandardItem(tr("Axis"));  // cn:坐标轴
    axesFolder->setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), RolePlot);
    axesFolder->setData(NodeTypeAxesFolder, RoleNodeType);
    // axesFolder->setIcon(QIcon(":/icons/axes.png"));

    // 可见性列 - 坐标轴文件夹不需要可见性控制
    QStandardItem* visibilityItem = createVisibilityItem();

    // 颜色列 - 坐标轴文件夹不需要颜色显示
    QStandardItem* colorItem = createColorItem();

    layerItem->appendRow(QList< QStandardItem* >() << axesFolder << visibilityItem << colorItem);

    // 添加可见的坐标轴
    auto addAxisIfVisible = [ & ](QwtAxisId axisId, const QString& axisName) {
        if (plot->isAxisVisible(axisId)) {
            QStandardItem* axisItem = new QStandardItem(axisName);
            axisItem->setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), RolePlot);
            axisItem->setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot->axisWidget(axisId))), RoleScale);
            // axisItem->setData(QVariant::fromValue(static_cast< QObject* >(plot->axisWidget(axisId))), RoleScale);
            axisItem->setData(QVariant::fromValue(axisId), RoleAxisId);
            axisItem->setData(NodeTypeAxis, RoleNodeType);
            // axisItem->setIcon(QIcon(":/icons/axis.png"));

            // 可见性列 - 坐标轴节点不需要可见性控制
            QStandardItem* axisVisibilityItem = createVisibilityItem();

            // 颜色列 - 坐标轴节点不需要颜色显示
            QStandardItem* axisColorItem = createColorItem();

            axesFolder->appendRow(QList< QStandardItem* >() << axisItem << axisVisibilityItem << axisColorItem);
        }
    };

    addAxisIfVisible(QwtAxis::XBottom, tr("x bottom"));
    addAxisIfVisible(QwtAxis::XTop, tr("x top"));
    addAxisIfVisible(QwtAxis::YLeft, tr("y left"));
    addAxisIfVisible(QwtAxis::YRight, tr("y right"));
}

void DAFigureTreeModel::addPlotItemsToLayer(QwtPlot* plot, QStandardItem* layerItem)
{
    QStandardItem* itemsFolder = new QStandardItem(tr("plot item"));  // cn:图元
    itemsFolder->setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), RolePlot);
    itemsFolder->setData(NodeTypeItemsFolder, RoleNodeType);
    // itemsFolder->setIcon(QIcon(":/icons/items.png"));

    // 可见性列 - 图元文件夹不需要可见性控制
    QStandardItem* visibilityItem = createVisibilityItem();

    // 颜色列 - 图元文件夹不需要颜色显示
    QStandardItem* colorItem = createColorItem();

    layerItem->appendRow(QList< QStandardItem* >() << itemsFolder << visibilityItem << colorItem);

    // 添加所有图元
    const QwtPlotItemList& items = plot->itemList();
    for (QwtPlotItem* item : items) {
        if (item) {
            addPlotItem(item, itemsFolder);
        }
    }
}

void DAFigureTreeModel::addPlotItem(QwtPlotItem* item, QStandardItem* parentItem)
{
    QStandardItem* itemNode = new QStandardItem(plotItemName(item));
    // 存储QwtPlotItem指针
    itemNode->setData(QVariant::fromValue(reinterpret_cast< quintptr >(item->plot())), RolePlot);
    itemNode->setData(QVariant::fromValue(reinterpret_cast< quintptr >(item)), RolePlotItem);
    itemNode->setData(NodeTypePlotItem, RoleNodeType);

    // 根据图元类型设置图标
    itemNode->setIcon(plotItemIcon(item));

    // 可见性列 - 图元节点需要可见性控制
    QStandardItem* itemVisibilityItem = createVisibilityItem(item);

    // 颜色列 - 图元节点需要颜色显示
    QStandardItem* itemColorItem = createColorItem(item);
    parentItem->appendRow(QList< QStandardItem* >() << itemNode << itemVisibilityItem << itemColorItem);
    m_plotItemItems[ item ] = itemNode;
}

void DAFigureTreeModel::removePlotItem(QwtPlotItem* item, QStandardItem* parentItem)
{
    QStandardItem* itemNode = m_plotItemItems.value(item);
    if (itemNode) {
        parentItem->removeRow(itemNode->row());
        m_plotItemItems.remove(item);
    }
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

        // 存储QwtPlotItem指针
        visibilityItem->setData(QVariant::fromValue(reinterpret_cast< quintptr >(item)), RolePlotItem);
        visibilityItem->setData(NodeTypePlotItem, RoleNodeType);

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
            colorItem->setIcon(brushIcon(brush));
        }
        // 存储QwtPlotItem指针
        colorItem->setData(QVariant::fromValue(reinterpret_cast< quintptr >(item)), RolePlotItem);
        colorItem->setData(NodeTypePlotItem, RoleNodeType);
    } else {
        // 非图元节点：显示空白
        colorItem->setText("");
    }

    colorItem->setEditable(false);
    return colorItem;
}

/**
 * @brief 用于生成绘图对应的文字
 *
 * 如果想改变文字内容，可重写此函数
 * @param plot 绘图指针
 * @param fig figure指针
 * @return
 */
QString DAFigureTreeModel::plotTitleText(QwtPlot* plot) const
{
    if (!plot) {
        return tr("unknow chart");  // cn:未知绘图
    }
    QString str = plot->title().text();
    if (!str.isEmpty()) {
        return str;
    }
    // 如果没有名字，则以第几个绘图命名
    QwtFigure* fig = figure();
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

QString DAFigureTreeModel::plotItemName(QwtPlotItem* item) const
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

QIcon DAFigureTreeModel::plotItemIcon(QwtPlotItem* item) const
{
    static QIcon s_default_chart_icon(":/DAFigure/icon/chart-item.svg");
    switch (item->rtti()) {
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

QIcon DAFigureTreeModel::brushIcon(const QBrush& b) const
{
    QPixmap pixmap(22, 22);
    QPainter p(&pixmap);
    p.fillRect(pixmap.rect(), b);
    return QIcon(pixmap);
}

void DAFigureTreeModel::onAxesAdded(QwtPlot* plot)
{
    if (plot && !m_plotItems.contains(plot)) {
        addPlotToModel(plot, invisibleRootItem());
    }
}

void DAFigureTreeModel::onAxesRemoved(QwtPlot* plot)
{
    removePlotFromModel(plot);
}

void DAFigureTreeModel::onFigureCleared()
{
    setupModel();  // 完全重建
}

void DAFigureTreeModel::onCurrentAxesChanged(QwtPlot* plot)
{
    // 可以高亮当前激活的坐标系
    Q_UNUSED(plot)
}

void DAFigureTreeModel::onItemAttached(QwtPlotItem* item, bool on)
{
    QwtPlot* plot = qobject_cast< QwtPlot* >(sender());
    if (!plot) {
        return;
    }

    QStandardItem* plotItem = findPlotItem(plot);
    if (!plotItem) {
        return;
    }

    QStandardItem* itemsFolder = findItemsFolderForPlot(plotItem, plot);
    if (!itemsFolder) {
        return;
    }

    if (on) {
        if (!m_plotItemItems.contains(item)) {
            addPlotItem(item, itemsFolder);
        }
    } else {
        removePlotItem(item, itemsFolder);
    }
}

void DAFigureTreeModel::removePlotFromModel(QwtPlot* plot)
{
    QStandardItem* plotItem = m_plotItems.value(plot);
    if (plotItem) {
        // 断开该plot的所有连接
        if (m_plotConnections.contains(plot)) {
            const auto& connects = m_plotConnections[ plot ];
            for (const QMetaObject::Connection& conn : connects) {
                disconnect(conn);
            }
            m_plotConnections.remove(plot);
        }

        // 移除所有相关的图元记录
        const QwtPlotItemList& items = plot->itemList();
        for (QwtPlotItem* item : items) {
            m_plotItemItems.remove(item);
        }

        // 移除寄生绘图的记录和连接
        if (plot->isHostPlot()) {
            QList< QwtPlot* > parasites = plot->parasitePlots();
            for (QwtPlot* parasite : parasites) {
                m_plotItems.remove(parasite);
                const QwtPlotItemList& parasiteItems = parasite->itemList();
                for (QwtPlotItem* item : parasiteItems) {
                    m_plotItemItems.remove(item);
                }

                // 断开寄生绘图的连接
                if (m_plotConnections.contains(parasite)) {
                    for (const QMetaObject::Connection& conn : m_plotConnections[ parasite ]) {
                        disconnect(conn);
                    }
                    m_plotConnections.remove(parasite);
                }
            }
        }

        invisibleRootItem()->removeRow(plotItem->row());
        m_plotItems.remove(plot);
    }
}

QwtPlot* DAFigureTreeModel::plotFromItem(const QStandardItem* item) const
{
    return pointerFromItem< QwtPlot >(item, RolePlot);
}

QwtPlot* DAFigureTreeModel::plotFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< QwtPlot >(index, RolePlot);
}

QwtScaleWidget* DAFigureTreeModel::scaleFromItem(const QStandardItem* item) const
{
    return pointerFromItem< QwtScaleWidget >(item, RoleScale);
}

QwtScaleWidget* DAFigureTreeModel::scaleFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< QwtScaleWidget >(index, RoleScale);
}

QwtPlotItem* DAFigureTreeModel::plotItemFromItem(const QStandardItem* item) const
{
    return pointerFromItem< QwtPlotItem >(item, RolePlotItem);
}

QwtPlotItem* DAFigureTreeModel::plotItemFromIndex(const QModelIndex& index) const
{
    return pointerFromIndex< QwtPlotItem >(index, RolePlotItem);
}

QwtAxisId DAFigureTreeModel::axisIdFromItem(const QStandardItem* item) const
{
    if (!item) {
        return QwtAxis::AxisPositions;
    }
    QVariant v = item->data(RolePlotItem);
    if (!v.isValid()) {
        return QwtAxis::AxisPositions;
    }
    return v.toInt();
}

QwtAxisId DAFigureTreeModel::axisIdFromItem(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QwtAxis::AxisPositions;
    }

    QStandardItem* item = itemFromIndex(index);
    if (!item) {
        return QwtAxis::AxisPositions;
    }
    return axisIdFromItem(item);
}

void DAFigureTreeModel::refresh()
{
    setupModel();
}

DAFigureTreeModel::NodeType DAFigureTreeModel::itemType(QStandardItem* item) const
{
    if (!item) {
        return NodeTypeUnknow;
    }
    QVariant v = item->data(RoleNodeType);
    if (!v.isValid()) {
        return NodeTypeUnknow;
    }
    return static_cast< NodeType >(v.toInt());
}

QStandardItem* DAFigureTreeModel::findPlotItem(QwtPlot* plot) const
{
    return m_plotItems.value(plot, nullptr);
}

QStandardItem* DAFigureTreeModel::findItemsFolderForPlot(QStandardItem* plotItem, QwtPlot* plot) const
{
    for (int i = 0; i < plotItem->rowCount(); ++i) {
        QStandardItem* layerItem = plotItem->child(i);
        if (itemType(layerItem) == NodeTypeLayer) {
            QwtPlot* layerPlot = plotFromItem(layerItem);
            if (layerPlot == plot) {
                for (int j = 0; j < layerItem->rowCount(); ++j) {
                    QStandardItem* folderItem = layerItem->child(j);
                    if (itemType(folderItem) == NodeTypeItemsFolder) {
                        return folderItem;
                    }
                }
                break;
            }
        }
    }
    return nullptr;
}

}  // End Of Namespace DA
