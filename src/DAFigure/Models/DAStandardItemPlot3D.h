#ifndef DASTANDARDITEMPLOT3D_H
#define DASTANDARDITEMPLOT3D_H
#include "DAFigureAPI.h"
#include <QStandardItem>
class Qwt3DPlotItem;
namespace DA
{
class DAChart3DWidget;
// 3D chart layout 节点
class DAFIGURE_API DAStandardItemPlot3D : public QStandardItem
{
public:
    enum
    {
        Type = QStandardItem::UserType + 4
    };
    // 节点列类型
    enum ItemType
    {
        Plot3DText,      ///< 只显示3D chart的文字和图标，用于树形第一列
        Plot3DVisible,   ///< 显示3D chart的可见性，一般用于第二列
        Plot3DProperty    ///< 显示3D chart的属性，一般用于第三列
    };
    explicit DAStandardItemPlot3D(DAChart3DWidget* chart, ItemType type);
    ~DAStandardItemPlot3D();
    QVariant data(int role = Qt::UserRole + 1) const override;
    QVariant handleItemTextType(int role) const;
    QVariant handleItemVisibleType(int role) const;
    QVariant handleItemPropertyType(int role) const;
    int type() const override
    {
        return Type;
    }
    DAChart3DWidget* chart3D() const
    {
        return mChart3D;
    }
    ItemType itemType() const
    {
        return mItemType;
    }
    bool isValid() const
    {
        return (mChart3D != nullptr);
    }

private:
    DAChart3DWidget* mChart3D { nullptr };
    ItemType mItemType { Plot3DText };
};
}  // end DA
#endif  // DASTANDARDITEMPLOT3D_H
