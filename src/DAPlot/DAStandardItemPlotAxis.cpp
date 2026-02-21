#include "DAStandardItemPlotAxis.h"
#include "DAPlotUtils.h"
#include "plot/QImPlotAxisInfo.h"
namespace DA
{

DAStandardItemPlotAxis::DAStandardItemPlotAxis(QIM::QImPlotAxisInfo* axis, ItemType type)
    : QStandardItem(), m_axis(axis), m_itemType(type)
{
    // 设置节点类型角色
    setData(static_cast< int >(DAPlotTreeItemType::Axis), static_cast< int >(DAPlotTreeItemRole::RoleItemType));
    setData(QVariant::fromValue(reinterpret_cast< quintptr >(axis)), static_cast< int >(DAPlotTreeItemRole::RoleInnerPointer));
    setEditable(false);
}

DAStandardItemPlotAxis::~DAStandardItemPlotAxis()
{
}

QVariant DAStandardItemPlotAxis::data(int role) const
{
    if (!isValid()) {
        return QStandardItem::data(role);
    }

    switch (m_itemType) {
    case PlotAxisText:
        return handleItemTextType(role);
    case PlotAxisVisible:
        return handleItemVisibleType(role);
    case PlotAxisProperty:
        return handleItemPropertyType(role);
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
QVariant DAStandardItemPlotAxis::handleItemTextType(int role) const
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
        return QString("[%1]%2").arg(toString(m_axis->axisId()), m_axis->label());
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        if (m_axis->isOpposite()) {
            if (QIM::isXAxisId(m_axis->axisId())) {
                return s_icon_xtop;
            } else {
                return s_icon_yright;
            }
        } else {
            if (QIM::isXAxisId(m_axis->axisId())) {
                return s_icon_xbottom;
            } else {
                return s_icon_yleft;
            }
        }
        return QVariant();
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

QVariant DAStandardItemPlotAxis::handleItemVisibleType(int role) const
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
        if (m_axis->isEnabled()) {
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

QVariant DAStandardItemPlotAxis::handleItemPropertyType(int role) const
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

QIM::QImPlotNode* DAStandardItemPlotAxis::plot() const
{
    if (m_axis) {
        return m_axis->plotNode();
    }
    return nullptr;
}

QIM::QImPlotAxisInfo* DAStandardItemPlotAxis::axis() const
{
    return m_axis.data();
}

DAStandardItemPlotAxis::ItemType DAStandardItemPlotAxis::itemType() const
{
    return m_itemType;
}

bool DAStandardItemPlotAxis::isValid() const
{
    return !(m_axis.isNull());
}

QString DAStandardItemPlotAxis::toString(QIM::QImPlotScaleType scaleType)
{
    switch (scaleType) {
    case QIM::QImPlotScaleType::Linear:
        return QObject::tr("Linear");
    case QIM::QImPlotScaleType::Log10:
        return QObject::tr("Log10");
    case QIM::QImPlotScaleType::SymLog:
        return QObject::tr("SymLog");
    case QIM::QImPlotScaleType::Time:
        return QObject::tr("Time");
    default:
        break;
    }
    return QObject::tr("Unknow");
}


QString DAStandardItemPlotAxis::toString(QIM::QImPlotAxisId id)
{
    switch (id) {
    case QIM::QImPlotAxisId::X1:
        return QObject::tr("x1");
    case QIM::QImPlotAxisId::X2:
        return QObject::tr("x2");
    case QIM::QImPlotAxisId::X3:
        return QObject::tr("x3");
    case QIM::QImPlotAxisId::Y1:
        return QObject::tr("y1");
    case QIM::QImPlotAxisId::Y2:
        return QObject::tr("y2");
    case QIM::QImPlotAxisId::Y3:
        return QObject::tr("y3");
    default:
        break;
    }
    return QObject::tr("Unknow");
}

}  // end DA
