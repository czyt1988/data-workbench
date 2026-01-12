#ifndef DASTANDARDITEMPLOT_H
#define DASTANDARDITEMPLOT_H
#include "DAFigureAPI.h"
#include <QStandardItem>
#include <QPointer>
class QwtPlot;
namespace DA
{
class DAFIGURE_API DAStandardItemPlot : public QStandardItem
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
        PlotText,     ///< 只显示QwtPlot的文字和图标，用于树形第一列
        PlotVisible,  ///< 显示QwtPlot的可见性，一般用于第二列
        PlotProperty  ///< 显示QwtPlot的属性，一般用于第三列
    };
    explicit DAStandardItemPlot(QwtPlot* plot, ItemType plotType);
    ~DAStandardItemPlot();
    QVariant data(int role = Qt::UserRole + 1) const override;
    QVariant handleItemTextType(int role) const;
    QVariant handleItemVisibleType(int role) const;
    QVariant handleScalePropertyType(int role) const;
    int type() const override
    {
        return Type;
    }
    // 获取关联的PlotItem
    QwtPlot* plot() const
    {
        return m_plot.data();
    }

    // 获取Item类型
    ItemType itemType() const
    {
        return m_itemType;
    }

    bool isValid() const
    {
        return !(m_plot.isNull());
    }

private:
    QPointer< QwtPlot > m_plot { nullptr };
    ItemType m_itemType { PlotText };
};
}  // end DA
#endif  // DASTANDARDITEMPLOT_H
