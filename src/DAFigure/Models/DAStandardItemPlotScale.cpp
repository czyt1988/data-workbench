#include "DAStandardItemPlotScale.h"
#include "DAFigureTreeModel.h"
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_engine.h"
#include "qwt_date_scale_engine.h"
namespace DA
{

DAStandardItemPlotScale::DAStandardItemPlotScale(QwtPlot* plot, QwtAxisId axisid, ItemType plotScaleType)
    : QStandardItem(), m_plot(plot), m_axisId(axisid), m_itemType(plotScaleType)
{
    // 设置节点类型角色
    setData(DAFigureTreeModel::NodeTypeAxis, DAFigureTreeModel::RoleNodeType);
    setData(static_cast< int >(axisid), DAFigureTreeModel::RoleAxisId);
    setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot->axisWidget(axisid))), DAFigureTreeModel::RoleScale);
    setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), DAFigureTreeModel::RolePlot);
    setEditable(false);
}

DAStandardItemPlotScale::~DAStandardItemPlotScale()
{
}

QVariant DAStandardItemPlotScale::data(int role) const
{
    if (!isValid()) {
        return QStandardItem::data(role);
    }

    switch (m_itemType) {
    case PlotScaleText:
        return handleItemTextType(role);
    case PlotScaleVisible:
        return handleItemVisibleType(role);
    case PlotScaleProperty:
        return handleScalePropertyType(role);
    default:
        break;
    }

    return QStandardItem::data(role);
}

/**
 * @brief 处理文字显示
 * @param role
 * @return
 */
QVariant DAStandardItemPlotScale::handleItemTextType(int role) const
{
    if (!isValid()) {
        return QVariant();
    }
    static QIcon s_icon_yleft(":/DAFigure/icon/axis-yleft.svg");
    static QIcon s_icon_yright(":/DAFigure/icon/axis-yright.svg");
    static QIcon s_icon_xbottom(":/DAFigure/icon/axis-xbottom.svg");
    static QIcon s_icon_xtop(":/DAFigure/icon/axis-xtop.svg");
    switch (role) {
    case Qt::DisplayRole: {
        if (QwtScaleWidget* sw = m_plot->axisWidget(m_axisId)) {
            return QString("[%1]%2").arg(axisIdToText(m_axisId), sw->title().text());
        }
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        switch (m_axisId) {
        case QwtAxis::YLeft:
            return s_icon_yleft;
        case QwtAxis::YRight:
            return s_icon_yright;
        case QwtAxis::XBottom:
            return s_icon_xbottom;
        case QwtAxis::XTop:
            return s_icon_xtop;
        default:
            break;
        }
        return QVariant();
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

QVariant DAStandardItemPlotScale::handleItemVisibleType(int role) const
{
    if (!isValid()) {
        return QVariant();
    }
    static QIcon s_icon_not_visible(":/DAFigure/icon/chartitem-invisible.svg");
    static QIcon s_icon_visible(":/DAFigure/icon/chartitem-visible.svg");
    switch (role) {
    case Qt::DisplayRole: {
        return QVariant();
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        if (m_plot->isAxisVisible(m_axisId)) {
            return s_icon_visible;
        } else {
            return s_icon_not_visible;
        }
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

QVariant DAStandardItemPlotScale::handleScalePropertyType(int role) const
{
    if (!isValid()) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole: {
        return QVariant();
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

QString DAStandardItemPlotScale::axisScaleTypeString(const QwtPlot* plot, QwtAxisId axisId)
{
    if (!plot || axisId == QwtAxis::AxisPositions) {
        return QString();
    }
    const QwtScaleEngine* engine = plot->axisScaleEngine(axisId);
    if (dynamic_cast< const QwtDateScaleEngine* >(engine))
        return QObject::tr("DateTime Scale");  // cn:时间轴
    if (dynamic_cast< const QwtLogScaleEngine* >(engine))
        return QObject::tr("Log Scale");  // cn:对数轴
    // 普通线性轴不做特殊描述
    //  if (dynamic_cast< const QwtLinearScaleEngine* >(engine))
    //     return QObject::tr("Linear Scale");  // cn:线性轴
    return QString();
}

QString DAStandardItemPlotScale::axisIdToText(QwtAxisId id)
{
    switch (id) {
    case QwtAxis::YLeft:
        return QObject::tr("Y Left");
    case QwtAxis::YRight:
        return QObject::tr("Y Right");
    case QwtAxis::XBottom:
        return QObject::tr("X Bottom");
    case QwtAxis::XTop:
        return QObject::tr("X Top");
    default:
        break;
    }
    return QObject::tr("Unknow");
}

}  // end DA
