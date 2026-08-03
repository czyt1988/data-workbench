#ifndef DASTANDARDITEMPLOT3DITEM_H
#define DASTANDARDITEMPLOT3DITEM_H
#include "DAFigureAPI.h"
#include <QStandardItem>
class Qwt3DPlotItem;
namespace DA
{
class DAChart3DWidget;
// 3D plot item 节点
class DAFIGURE_API DAStandardItemPlot3DItem : public QStandardItem
{
public:
    enum
    {
        Type = QStandardItem::UserType + 5
    };
    // 节点列类型
    enum ItemType
    {
        Plot3DItemText,     ///< 只显示item的文字和图标，用于树形第一列
        Plot3DItemVisible,  ///< 显示item的可见性，一般用于第二列
        Plot3DItemColor     ///< 显示item的颜色，一般用于第三列
    };
    explicit DAStandardItemPlot3DItem(Qwt3DPlotItem* item, ItemType type);
    ~DAStandardItemPlot3DItem();
    QVariant data(int role = Qt::UserRole + 1) const override;
    QVariant handleItemTextType(int role) const;
    QVariant handleItemVisibleType(int role) const;
    QVariant handleItemColorType(int role) const;
    int type() const override
    {
        return Type;
    }
    Qwt3DPlotItem* plot3DItem() const
    {
        return m_plot3DItem;
    }
    ItemType itemType() const
    {
        return m_itemType;
    }

private:
    Qwt3DPlotItem* m_plot3DItem { nullptr };
    ItemType m_itemType { Plot3DItemText };
};
}  // end DA
#endif  // DASTANDARDITEMPLOT3DITEM_H
