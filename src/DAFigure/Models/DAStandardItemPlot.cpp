#include "DAStandardItemPlot.h"
#include "DAFigureTreeModel.h"
#include "qwt_plot.h"
#include "qwt_text.h"
namespace DA
{
DAStandardItemPlot::DAStandardItemPlot(QwtPlot* plot, ItemType plotType)
    : QStandardItem(), m_plot(plot), m_itemType(plotType)
{
    setData(DAFigureTreeModel::NodeTypePlot, DAFigureTreeModel::RoleNodeType);
    setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), DAFigureTreeModel::RolePlot);
    setEditable(false);
}

DAStandardItemPlot::~DAStandardItemPlot()
{
}

QVariant DAStandardItemPlot::data(int role) const
{
    if (!isValid()) {
        return QStandardItem::data(role);
    }

    switch (m_itemType) {
    case PlotText:
        return handleItemTextType(role);
    case PlotVisible:
        return handleItemVisibleType(role);
    case PlotProperty:
        return handleScalePropertyType(role);
    default:
        break;
    }

    return QStandardItem::data(role);
}

QVariant DAStandardItemPlot::handleItemTextType(int role) const
{
    if (!isValid()) {
        return QVariant();
    }
    static QIcon s_plot_icon = QIcon(":/DAFigure/icon/chart.svg");
    switch (role) {
    case Qt::DisplayRole: {
        if (!m_plot) {
            return QVariant();
        }
        QString text;
        if (m_plot->isParasitePlot()) {
            int index = m_plot->hostPlot()->parasitePlotIndex(m_plot);
            text      = QObject::tr("layout-%1").arg(index + 1);
        } else {
            text = m_plot->title().text();
            if (text.isEmpty()) {
                text = QObject::tr("layout");
            }
        }
        return text;
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        return s_plot_icon;
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

QVariant DAStandardItemPlot::handleItemVisibleType(int role) const
{
    return QStandardItem::data(role);
}

QVariant DAStandardItemPlot::handleScalePropertyType(int role) const
{
    if (!isValid()) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole: {
        if (!m_plot) {
            return QVariant();
        }
        if (m_plot->isParasitePlot()) {
            return QObject::tr("Parasite Plot");  // cn: 寄生绘图
        }
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

}  // end DA
