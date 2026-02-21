#ifndef DASTANDARDITEMPLOTAXIS_H
#define DASTANDARDITEMPLOTAXIS_H
#include "DAPlotAPI.h"
#include <QStandardItem>
#include <QPointer>
#include "plot/QImPlot.h"
namespace QIM
{
class QImPlotAxisInfo;
class QImPlotNode;
}
namespace DA
{
/**
 * @brief 针对坐标轴QwtScaleWidget的StandardItem类
 */
class DAPLOT_API DAStandardItemPlotAxis : public QStandardItem
{
public:
    enum
    {
        Type = QStandardItem::UserType + 2  // 任意 >= UserType 的值
    };
    /**
     * @brief 指定这个item的类型
     */
    enum ItemType
    {
        PlotAxisText,     ///< 只显示QwtScaleWidget的文字和图标，用于树形第一列
        PlotAxisVisible,  ///< 显示QwtScaleWidget的可见性，一般用于第二列
        PlotAxisProperty  ///< 显示QwtScaleWidget的属性，一般用于第三列
    };
    explicit DAStandardItemPlotAxis(QIM::QImPlotAxisInfo* axis, ItemType type);
    ~DAStandardItemPlotAxis();
    QVariant data(int role = Qt::UserRole + 1) const override;
    QVariant handleItemTextType(int role) const;
    QVariant handleItemVisibleType(int role) const;
    QVariant handleItemPropertyType(int role) const;
    int type() const override
    {
        return Type;
    }
    // 获取关联的PlotItem
    QIM::QImPlotNode* plot() const;
    //
    QIM::QImPlotAxisInfo* axis() const;
    // 获取Item类型
    ItemType itemType() const;

    bool isValid() const;
    // 获取坐标轴的类型文本，线性轴、时间轴、对数轴等
    static QString toString(QIM::QImPlotScaleType scaleType);
    // 坐标轴id对应文本
    static QString toString(QIM::QImPlotAxisId id);

private:
    QPointer< QIM::QImPlotAxisInfo > m_axis;
    ItemType m_itemType { PlotAxisText };
};
}  // end DA
#endif  // DASTANDARDITEMPLOTAXIS_H
