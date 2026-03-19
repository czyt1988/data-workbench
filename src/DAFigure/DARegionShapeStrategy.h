#ifndef DAREGIONSHAPESTRATEGY_H
#define DAREGIONSHAPESTRATEGY_H

#include "DAFigureAPI.h"
#include "DAAbstractChartEditor.h"
#include "DAChartSelectRegionShapeItem.h"
#include <QPainterPath>
#include <QRectF>
#include <QPolygonF>
#include <QVariant>
#include <memory>

namespace DA
{

/**
 * \if ENGLISH
 * @brief Strategy interface for region shape creation
 * @details Defines the interface for creating and updating different region shapes.
 *          This interface follows the Strategy design pattern to allow different
 *          shape creation algorithms to be used interchangeably.
 * \endif
 *
 * \if CHINESE
 * @brief 区域形状策略接口
 * @details 定义创建和更新不同区域形状的接口。
 *          此接口遵循策略设计模式，允许不同的形状创建算法可以互换使用。
 * \endif
 */
class DAFIGURE_API DARegionShapeStrategy
{
public:
    virtual ~DARegionShapeStrategy() = default;

    /**
     * \if ENGLISH
     * @brief Create a QPainterPath from the given region data
     * @param data Region data (QRectF for drag-based, QPolygonF for polygon-based)
     * @return The created QPainterPath
     * \endif
     *
     * \if CHINESE
     * @brief 从给定的区域数据创建 QPainterPath
     * @param data 区域数据（拖拽模式为 QRectF，多边形模式为 QPolygonF）
     * @return 创建的 QPainterPath
     * \endif
     */
    virtual QPainterPath createPath(const QVariant& data) const = 0;

    /**
     * \if ENGLISH
     * @brief Update the temporary shape item with the given region data
     * @param item The temporary shape item to update
     * @param data The current region data
     * \endif
     *
     * \if CHINESE
     * @brief 使用给定的区域数据更新临时形状项
     * @param item 要更新的临时形状项
     * @param data 当前区域数据
     * \endif
     */
    virtual void updateTmpItem(DAChartSelectRegionShapeItem* item, const QVariant& data) const = 0;

    /**
     * \if ENGLISH
     * @brief Check if this strategy uses drag-based interaction
     * @return true for drag-based (rect, ellipse), false for click-based (polygon)
     * \endif
     *
     * \if CHINESE
     * @brief 检查此策略是否使用拖拽交互
     * @return 拖拽模式返回 true（矩形、椭圆），点击模式返回 false（多边形）
     * \endif
     */
    virtual bool isDragBased() const = 0;

    /**
     * \if ENGLISH
     * @brief Get the RTTI value for this shape type
     * @return The RTTI enum value
     * \endif
     *
     * \if CHINESE
     * @brief 获取此形状类型的 RTTI 值
     * @return RTTI 枚举值
     * \endif
     */
    virtual int rtti() const = 0;
};

/**
 * \if ENGLISH
 * @brief Rectangle shape strategy implementation
 * @details Creates rectangular regions through drag interaction.
 * \endif
 *
 * \if CHINESE
 * @brief 矩形形状策略实现
 * @details 通过拖拽交互创建矩形区域。
 * \endif
 */
class DAFIGURE_API DARectShapeStrategy : public DARegionShapeStrategy
{
public:
    QPainterPath createPath(const QVariant& data) const override
    {
        QPainterPath path;
        path.addRect(data.toRectF());
        return path;
    }

    void updateTmpItem(DAChartSelectRegionShapeItem* item, const QVariant& data) const override
    {
        if (item) {
            item->setRect(data.toRectF());
        }
    }

    bool isDragBased() const override { return true; }

    int rtti() const override { return DAAbstractChartEditor::RTTIRectRegionSelectEditor; }
};

/**
 * \if ENGLISH
 * @brief Ellipse shape strategy implementation
 * @details Creates elliptical regions through drag interaction.
 * \endif
 *
 * \if CHINESE
 * @brief 椭圆形状策略实现
 * @details 通过拖拽交互创建椭圆区域。
 * \endif
 */
class DAFIGURE_API DAEllipseShapeStrategy : public DARegionShapeStrategy
{
public:
    QPainterPath createPath(const QVariant& data) const override
    {
        QPainterPath path;
        path.addEllipse(data.toRectF());
        return path;
    }

    void updateTmpItem(DAChartSelectRegionShapeItem* item, const QVariant& data) const override
    {
        if (item) {
            item->setEllipse(data.toRectF());
        }
    }

    bool isDragBased() const override { return true; }

    int rtti() const override { return DAAbstractChartEditor::RTTIEllipseRegionSelectEditor; }
};

/**
 * \if ENGLISH
 * @brief Polygon shape strategy implementation
 * @details Creates polygon regions through click interaction.
 * \endif
 *
 * \if CHINESE
 * @brief 多边形形状策略实现
 * @details 通过点击交互创建多边形区域。
 * \endif
 */
class DAFIGURE_API DAPolygonShapeStrategy : public DARegionShapeStrategy
{
public:
    QPainterPath createPath(const QVariant& data) const override
    {
        QPainterPath path;
        QPolygonF polygon = data.value< QPolygonF >();
        if (polygon.size() > 2) {
            if (polygon.last() != polygon.first()) {
                polygon.append(polygon.first());
            }
        }
        path.addPolygon(polygon);
        return path;
    }

    void updateTmpItem(DAChartSelectRegionShapeItem* item, const QVariant& data) const override
    {
        if (item) {
            item->setPolygon(data.value< QPolygonF >());
        }
    }

    bool isDragBased() const override { return false; }

    int rtti() const override { return DAAbstractChartEditor::RTTIPolygonRegionSelectEditor; }
};

/**
 * \if ENGLISH
 * @brief Factory function to create shape strategy
 * @param type The shape type (0=Rect, 1=Ellipse, 2=Polygon)
 * @return A unique_ptr to the created strategy
 * \endif
 *
 * \if CHINESE
 * @brief 创建形状策略的工厂函数
 * @param type 形状类型（0=矩形, 1=椭圆, 2=多边形）
 * @return 创建的策略的 unique_ptr
 * \endif
 */
inline std::unique_ptr< DARegionShapeStrategy > createShapeStrategy(int type)
{
    switch (type) {
    case 0:
        return std::make_unique< DARectShapeStrategy >();
    case 1:
        return std::make_unique< DAEllipseShapeStrategy >();
    case 2:
        return std::make_unique< DAPolygonShapeStrategy >();
    default:
        return std::make_unique< DARectShapeStrategy >();
    }
}

}  // namespace DA

Q_DECLARE_METATYPE(QPolygonF)

#endif  // DAREGIONSHAPESTRATEGY_H
