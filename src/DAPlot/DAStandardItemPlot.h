#ifndef DASTANDARDITEMPLOT_H
#define DASTANDARDITEMPLOT_H
#include "DAPlotAPI.h"
#include <QStandardItem>
#include <QPointer>
namespace QIM
{
class QImPlotNode;
}
namespace DA
{
class DAPLOT_API DAStandardItemPlot : public QStandardItem
{
public:
    enum
    {
        Type = QStandardItem::UserType + 1  // 任意 >= UserType 的值
    };
    /**
     * @brief 指定这个item的类型
     */
    enum ItemType
    {
        PlotText,     ///< 只显示Plot的文字和图标，用于树形第一列
        PlotVisible,  ///< 显示Plot的可见性，一般用于第二列
        PlotProperty  ///< 显示Plot的属性，一般用于第三列
    };
    explicit DAStandardItemPlot(QIM::QImPlotNode* plot, ItemType plotType);
    ~DAStandardItemPlot();
    QVariant data(int role = Qt::UserRole + 1) const override;
    QVariant handleItemTextType(int role) const;
    QVariant handleItemVisibleType(int role) const;
    QVariant handlePlotPropertyType(int role) const;
    int type() const override
    {
        return Type;
    }
    // 获取关联的PlotItem
    QIM::QImPlotNode* plot() const
    {
        return m_plot;
    }

    // 获取Item类型
    ItemType itemType() const
    {
        return m_itemType;
    }

    bool isValid() const
    {
        return (m_plot != nullptr);
    }

private:
    QIM::QImPlotNode* m_plot { nullptr };
    ItemType m_itemType { PlotText };
};
}  // end DA
#endif  // DASTANDARDITEMPLOT_H
