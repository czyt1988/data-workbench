#ifndef DASTANDARDITEMPLOTSCALE_H
#define DASTANDARDITEMPLOTSCALE_H
#include "DAFigureAPI.h"
#include <QStandardItem>
#include "qwt_axis_id.h"
class QwtScaleWidget;
class QwtPlot;
namespace DA
{
/**
 * @brief 针对坐标轴QwtScaleWidget的StandardItem类
 */
class DAStandardItemPlotScale : public QStandardItem
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
        PlotScaleText,     ///< 只显示QwtScaleWidget的文字和图标，用于树形第一列
        PlotScaleVisible,  ///< 显示QwtScaleWidget的可见性，一般用于第二列
        PlotScaleProperty  ///< 显示QwtScaleWidget的属性，一般用于第三列
    };
    explicit DAStandardItemPlotScale(QwtPlot* plot, QwtAxisId axisid, ItemType plotScaleType);
    ~DAStandardItemPlotScale();
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
        return m_plot;
    }

    QwtAxisId axisId() const
    {
        return m_axisId;
    }
    // 获取Item类型
    ItemType itemType() const
    {
        return m_itemType;
    }

    bool isValid() const
    {
        return (m_plot && (m_axisId != QwtAxis::AxisPositions));
    }
    // 获取坐标轴的类型文本，线性轴、时间轴、对数轴等
    QString axisScaleTypeString(const QwtPlot* plot, QwtAxisId axisId);
    // 坐标轴id对应文本
    static QString axisIdToText(QwtAxisId id);

private:
    QwtPlot* m_plot { nullptr };
    QwtAxisId m_axisId { QwtAxis::AxisPositions };
    ItemType m_itemType { PlotScaleText };
};
}  // end DA
#endif  // DASTANDARDITEMPLOTSCALE_H
