#ifndef DASTANDARDITEMPLOTITEM_H
#define DASTANDARDITEMPLOTITEM_H
#include "DAFigureAPI.h"
#include <QStandardItem>
class QwtPlotItem;
namespace DA
{
/**
 * @brief 针对QwtPlotItem的StandardItem类
 */
class DAFIGURE_API DAStandardItemPlotItem : public QStandardItem
{
public:
    enum
    {
        Type = QStandardItem::UserType + 3  // 任意 >= UserType 的值
    };
    /**
     * @brief 指定这个item的类型
     */
    enum ItemType
    {
        PlotItemText,     ///< 只显示item的文字和图标，用于树形第一列
        PlotItemVisible,  ///< 显示item的可见性，一般用于第二列
        PlotItemColor     ///< 显示item的颜色，一般用于第三列
    };
    explicit DAStandardItemPlotItem(QwtPlotItem* item, ItemType plotItemType);
    ~DAStandardItemPlotItem();

    QVariant data(int role = Qt::UserRole + 1) const override;
    QVariant handleItemTextType(int role) const;
    QVariant handleItemVisibleType(int role) const;
    QVariant handleItemColorType(int role) const;
    int type() const override
    {
        return Type;
    }
    // 获取关联的PlotItem
    QwtPlotItem* plotItem() const
    {
        return m_plotItem;
    }

    // 获取Item类型
    ItemType itemType() const
    {
        return m_itemType;
    }

private:
    QwtPlotItem* m_plotItem { nullptr };
    ItemType m_itemType { PlotItemText };
};
}  // end DA
#endif  // DASTANDARDITEMPLOTITEM_H
