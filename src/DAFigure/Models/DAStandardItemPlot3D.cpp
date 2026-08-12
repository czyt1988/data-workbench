#include "DAStandardItemPlot3D.h"
#include "DAFigureTreeModel.h"
#include "DAChart3DWidget.h"
#include "qwt3d_plot.h"
namespace DA
{

/**
 * @brief 构造函数
 * @param chart 关联的3D图表控件
 * @param type item类型
 */
DAStandardItemPlot3D::DAStandardItemPlot3D(DAChart3DWidget* chart, ItemType type)
    : QStandardItem(), mChart3D(chart), mItemType(type)
{
    setData(DAFigureTreeModel::NodeTypePlot3D, DAFigureTreeModel::RoleNodeType);
    setData(QVariant::fromValue(reinterpret_cast< quintptr >(chart)), DAFigureTreeModel::RolePlot3D);
    setEditable(false);
}

/**
 * @brief 析构函数
 */
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
    switch (mItemType) {
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
        if (!mChart3D) {
            return QVariant();
        }
        QString text = mChart3D->getChart3DTitle();
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

/**
 * @brief 处理可见性列显示
 * @param role 数据角色
 * @return 对应角色数据
 */
QVariant DAStandardItemPlot3D::handleItemVisibleType(int role) const
{
    return QStandardItem::data(role);
}

/**
 * @brief 处理属性列显示
 * @param role 数据角色
 * @return 对应角色数据
 */
QVariant DAStandardItemPlot3D::handleItemPropertyType(int role) const
{
    return QStandardItem::data(role);
}

}  // end DA
