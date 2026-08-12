#include "DAStandardItemPlot.h"
#include "DAFigureTreeModel.h"
#include "qwt_plot.h"
#include "qwt_text.h"
namespace DA
{

/**
 * @brief 构造函数
 * @param plot 关联的QwtPlot指针
 * @param plotType 项目类型
 */
DAStandardItemPlot::DAStandardItemPlot(QwtPlot* plot, ItemType plotType)
    : QStandardItem(), mPlot(plot), mItemType(plotType)
{
    setData(DAFigureTreeModel::NodeTypePlot, DAFigureTreeModel::RoleNodeType);
    setData(QVariant::fromValue(reinterpret_cast< quintptr >(plot)), DAFigureTreeModel::RolePlot);
    setEditable(false);
}

/**
 * @brief 析构函数
 */
DAStandardItemPlot::~DAStandardItemPlot()
{
}

/**
 * @brief 重载data函数，根据角色返回对应的数据
 * @param role 数据角色
 * @return 对应的数据
 */
QVariant DAStandardItemPlot::data(int role) const
{
    if (!isValid()) {
        return QStandardItem::data(role);
    }

    switch (mItemType) {
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

/**
 * @brief 处理文本类型的项目数据
 * @param role 数据角色
 * @return 对应的数据
 */
QVariant DAStandardItemPlot::handleItemTextType(int role) const
{
    if (!isValid()) {
        return QVariant();
    }
    static QIcon s_plot_icon = QIcon(":/DAFigure/icon/layout.svg");
    switch (role) {
    case Qt::DisplayRole: {
        if (!mPlot) {
            return QVariant();
        }
        QString text;
        if (mPlot->isParasitePlot()) {
            int index = mPlot->hostPlot()->parasitePlotIndex(mPlot);
            text      = QObject::tr("layout-%1").arg(index + 1);  // cn:布局-%1
        } else {
            text = mPlot->title().text();
            if (text.isEmpty()) {
                text = QObject::tr("layout");  // cn:布局
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

/**
 * @brief 处理可见性类型的项目数据
 * @param role 数据角色
 * @return 对应的数据
 */
QVariant DAStandardItemPlot::handleItemVisibleType(int role) const
{
    return QStandardItem::data(role);
}

/**
 * @brief 处理缩放属性类型的项目数据
 * @param role 数据角色
 * @return 对应的数据
 */
QVariant DAStandardItemPlot::handleScalePropertyType(int role) const
{
    if (!isValid()) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole: {
        if (!mPlot) {
            return QVariant();
        }
        if (mPlot->isParasitePlot()) {
            return QObject::tr("Parasite Plot");  // cn:寄生绘图
        }
    } break;
    default:
        break;
    }
    return QStandardItem::data(role);
}

}  // end DA
