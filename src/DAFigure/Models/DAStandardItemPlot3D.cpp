#include "DAStandardItemPlot3D.h"
#include "DAFigureTreeModel.h"
#include "DAChart3DWidget.h"
#include "qwt3d_plot.h"
namespace DA
{
DAStandardItemPlot3D::DAStandardItemPlot3D(DAChart3DWidget* chart, ItemType type)
    : QStandardItem(), m_chart3D(chart), m_itemType(type)
{
    setData(DAFigureTreeModel::NodeTypePlot3D, DAFigureTreeModel::RoleNodeType);
    setData(QVariant::fromValue(reinterpret_cast< quintptr >(chart)), DAFigureTreeModel::RolePlot3D);
    setEditable(false);
}

DAStandardItemPlot3D::~DAStandardItemPlot3D()
{
}

/**
 * @brief 根据列类型和role返回数据
 * @param role
 * @return
 */
QVariant DAStandardItemPlot3D::data(int role) const
{
    if (!isValid()) {
        return QStandardItem::data(role);
    }
    switch (m_itemType) {
    case Plot3DText:
        return handleItemTextType(role);
    case Plot3DVisible:
        return handleItemVisibleType(role);
    case Plot3DProperty:
        return handleItemPropertyType(role);
    default:
        break;
    }
    return QStandardItem::data(role);
}

/**
 * @brief 处理文字列显示，实时查询3D chart标题
 * @param role
 * @return
 */
QVariant DAStandardItemPlot3D::handleItemTextType(int role) const
{
    if (!isValid()) {
        return QVariant();
    }
    static QIcon s_icon(":/DAFigure/icon/layout.svg");
    switch (role) {
    case Qt::DisplayRole: {
        if (!m_chart3D) {
            return QVariant();
        }
        QString text = m_chart3D->getChart3DTitle();
        if (text.isEmpty()) {
            text = QObject::tr("3D Chart");  // cn:3D绘图
        }
        return text;
    } break;
    case Qt::DecorationRole: {
        return s_icon;
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

QVariant DAStandardItemPlot3D::handleItemVisibleType(int role) const
{
    return QStandardItem::data(role);
}

QVariant DAStandardItemPlot3D::handleItemPropertyType(int role) const
{
    return QStandardItem::data(role);
}

}  // end DA
