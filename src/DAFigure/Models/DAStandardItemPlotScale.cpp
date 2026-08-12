#include "DAStandardItemPlotScale.h"
#include "DAFigureTreeModel.h"
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_engine.h"
#include "qwt_date_scale_engine.h"
namespace DA
{

/**
 * @brief 构造函数
 * @param plot 关联的QwtPlot
 * @param axisid 坐标轴id
 * @param plotScaleType 坐标轴item类型
 */
DAStandardItemPlotScale::DAStandardItemPlotScale(QwtPlot* plot, QwtAxisId axisid, ItemType plotScaleType)
    : QStandardItem(), mPlot(plot), mAxisId(axisid), mItemType(plotScaleType)
{
    // 设置节点类型角色
    setData(DAFigureTreeModel::NodeTypeAxis, DAFigureTreeModel::RoleNodeType);
    setData(static_cast< int >(axisid), DAFigureTreeModel::RoleAxisId);
    setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot->axisWidget(axisid))), DAFigureTreeModel::RoleScale);
    setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), DAFigureTreeModel::RolePlot);
    setEditable(false);
}

/**
 * @brief 析构函数
 */
DAStandardItemPlotScale::~DAStandardItemPlotScale()
{
}

/**
 * @brief 返回item的数据
 * @param role 数据角色
 * @return 对应角色的数据
 */
QVariant DAStandardItemPlotScale::data(int role) const
{
    if (!isValid()) {
        return QStandardItem::data(role);
    }

    switch (mItemType) {
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
        if (QwtScaleWidget* sw = mPlot->axisWidget(mAxisId)) {
            return QString("[%1]%2").arg(axisIdToText(mAxisId), sw->title().text());
        }
    } break;
    case Qt::DecorationRole: {
        // 返回类型图标
        switch (mAxisId) {
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

/**
 * @brief 处理可见性显示
 * @param role 数据角色
 * @return 对应角色的数据
 */
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
        if (mPlot->isAxisVisible(mAxisId)) {
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

/**
 * @brief 处理属性显示
 * @param role 数据角色
 * @return 对应角色的数据
 */
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

/**
 * @brief 获取坐标轴的缩放类型文本
 * @param plot 关联的QwtPlot
 * @param axisId 坐标轴id
 * @return 缩放类型文本，如时间轴、对数轴等，普通线性轴返回空字符串
 */
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

/**
 * @brief 将坐标轴id转换为文本描述
 * @param id 坐标轴id
 * @return 坐标轴文本描述，如Y左轴、Y右轴等
 */
QString DAStandardItemPlotScale::axisIdToText(QwtAxisId id)
{
    switch (id) {
    case QwtAxis::YLeft:
        return QObject::tr("Y Left");  // cn:Y左轴
    case QwtAxis::YRight:
        return QObject::tr("Y Right");  // cn:Y右轴
    case QwtAxis::XBottom:
        return QObject::tr("X Bottom");  // cn:X底轴
    case QwtAxis::XTop:
        return QObject::tr("X Top");  // cn:X顶轴
    default:
        break;
    }
    return QObject::tr("Unknown");  // cn:未知
}

}  // end DA
