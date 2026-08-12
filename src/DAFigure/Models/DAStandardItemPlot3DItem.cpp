#include "DAStandardItemPlot3DItem.h"
#include "DAFigureTreeModel.h"
#include "DAChart3DWidget.h"
#include "qwt3d_plotitem.h"
namespace DA
{

/**
 * @brief 构造函数，构建3D绘图项的标准项
 * @param item 3D绘图项指针
 * @param type 项类型
 */
DAStandardItemPlot3DItem::DAStandardItemPlot3DItem(Qwt3DPlotItem* item, ItemType type)
    : QStandardItem(), mPlot3DItem(item), mItemType(type)
{
    setEditable(false);
    setData(DAFigureTreeModel::NodeTypePlot3DItem, DAFigureTreeModel::RoleNodeType);
    if (mPlot3DItem) {
        setData(QVariant::fromValue(reinterpret_cast< quintptr >(mPlot3DItem->plot())),
                DAFigureTreeModel::RolePlot3D);
        setData(QVariant::fromValue(reinterpret_cast< quintptr >(mPlot3DItem)),
                DAFigureTreeModel::RolePlot3DItem);
    }
}

/**
 * @brief 析构函数
 */
DAStandardItemPlot3DItem::~DAStandardItemPlot3DItem()
{
}

/**
 * @brief 根据列类型和role返回数据
 * @param role
 * @return
 */
QVariant DAStandardItemPlot3DItem::data(int role) const
{
    if (!mPlot3DItem) {
        return QStandardItem::data(role);
    }
    switch (mItemType) {
    case Plot3DItemText:
        return handleItemTextType(role);
    case Plot3DItemVisible:
        return handleItemVisibleType(role);
    case Plot3DItemColor:
        return handleItemColorType(role);
    default:
        break;
    }
    return QStandardItem::data(role);
}

/**
 * @brief 处理文字列显示，从模型获取名称和图标
 * @param role
 * @return
 */
QVariant DAStandardItemPlot3DItem::handleItemTextType(int role) const
{
    if (!mPlot3DItem) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole: {
        if (DAFigureTreeModel* m = qobject_cast< DAFigureTreeModel* >(model())) {
            return m->generate3DPlotItemName(mPlot3DItem);
        }
        return mPlot3DItem->title();
    } break;
    case Qt::DecorationRole: {
        if (DAFigureTreeModel* m = qobject_cast< DAFigureTreeModel* >(model())) {
            return m->generate3DPlotItemIcon(mPlot3DItem);
        }
        return QVariant();
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

/**
 * @brief 处理可见性列显示
 * @param role
 * @return
 */
QVariant DAStandardItemPlot3DItem::handleItemVisibleType(int role) const
{
    if (!mPlot3DItem) {
        return QVariant();
    }
    static QIcon s_icon_not_visible(":/DAFigure/icon/chartitem-invisible.svg");
    static QIcon s_icon_visible(":/DAFigure/icon/chartitem-visible.svg");
    switch (role) {
    case Qt::DisplayRole: {
        return QVariant();
    } break;
    case Qt::DecorationRole: {
        if (mPlot3DItem->isVisible()) {
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
 * @brief 处理颜色列显示，暂用占位颜色图标
 * @param role
 * @return
 */
QVariant DAStandardItemPlot3DItem::handleItemColorType(int role) const
{
    if (!mPlot3DItem) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole: {
        return QVariant();
    } break;
    case Qt::DecorationRole: {
        if (DAFigureTreeModel* m = qobject_cast< DAFigureTreeModel* >(model())) {
            return m->generateBrushIcon(QBrush(QColor(100, 150, 200)));
        }
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

}  // end DA
