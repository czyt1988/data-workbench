#ifndef DAGRAPHICSVIEWGLOBAL_H
#define DAGRAPHICSVIEWGLOBAL_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"
#include <QGraphicsItem>
#if defined(DAGRAPHICSVIEW_BUILDLIB)
#define DAGRAPHICSVIEW_API Q_DECL_EXPORT
#else
#ifdef Q_CC_MSVC
#define DAGRAPHICSVIEW_API Q_DECL_IMPORT
#else
#define DAGRAPHICSVIEW_API Q_DECL_IMPORT
#endif
#endif

/**
 * @brief 定义此宏使用object item
 */
#ifndef DA_USE_QGRAPHICSOBJECT
#define DA_USE_QGRAPHICSOBJECT 1
#endif

namespace DA
{

/**
 * @brief 方向
 */
enum class AspectDirection
{
    East,
    South,
    West,
    North
};

/**
 * @brief 定义一个形状的9个关键位置点
 *
 * @details
 *
 * ```
 * 1---2---3
 * |       |
 * 4---5---6
 * |       |
 * 7---8---9
 * ```
 *
 */
enum class ShapeKeyPoint
{
    TopLeft = 0,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    None = 128
};

/**
 * @brief QRect对应ShapeKeyPoint的点
 * @param r
 * @param kp 关键点
 * @return
 */
QPoint rectShapeKeyPoint(const QRect& r, ShapeKeyPoint kp);

/**
 * @brief QRectF对应ShapeKeyPoint的点
 * @param r
 * @param kp 关键点
 * @return
 */
QPointF rectShapeKeyPoint(const QRectF& r, ShapeKeyPoint kp);

/**
 * @brief 定义了Graphics相关的command id，用于标记相同的redo/undo
 */
enum DAGraphicsCommandIDType
{
    CmdID_GraphicsBegin    = 0x100,                    ///< Graphics相关的command id的起始
    CmdID_ItemMove         = CmdID_GraphicsBegin + 1,  ///< item移动
    CmdID_ItemMoveMerge    = CmdID_GraphicsBegin + 2,  ///< item移动
    CmdID_ItemsMove        = CmdID_GraphicsBegin + 3,  ///< 多个item移动
    CmdID_ItemsMoveMerge   = CmdID_GraphicsBegin + 4,  ///< 多个item移动
    CmdID_ItemResize       = CmdID_GraphicsBegin + 5,  ///< item resize
    CmdID_ItemResizeWidth  = CmdID_GraphicsBegin + 6,  ///< item resize width
    CmdID_ItemResizeHeight = CmdID_GraphicsBegin + 7,  ///< item resize height
    CmdID_ItemRotation     = CmdID_GraphicsBegin + 8,  ///< item rotation
    CmdID_GraphicsEnd      = 0x200                     ///< Graphics相关的command id的结束
};

/**
  @brief Item Type 枚举

  @note 此数值不要轻易改动，会影响旧工程文件的加载，xml文件记录了此枚举值，根据枚举值来创建item，如果数值变化，会导致旧工程文件创建异常
 */
enum DAGraphicsItemType
{
    ItemType_GraphicsItem_Begin   = QGraphicsItem::UserType + 10,     ///< 针对DAGraphicsResizeableItem的类型开始
    ItemType_DAGraphicsItem_Begin = ItemType_GraphicsItem_Begin + 1,  ///< DAGraphicsItem Type的开始范围
    ItemType_DAGraphicsItem       = ItemType_DAGraphicsItem_Begin + 1,  ///< 针对DAGraphicsResizeableItem的类型
    ItemType_DAGraphicsResizeableItem_Begin = ItemType_DAGraphicsItem_Begin + 2,
    ItemType_DAGraphicsResizeableItem = ItemType_DAGraphicsResizeableItem_Begin + 1,  ///< 针对DAGraphicsResizeableItem的类型
    ItemType_DAGraphicsRectItem = ItemType_DAGraphicsResizeableItem_Begin + 10,  ///< 针对DAGraphicsRectItem的类型
    ItemType_DAGraphicsTextItem = ItemType_DAGraphicsResizeableItem_Begin + 12,  ///< 针对DAGraphicsTextItem的类型
    ItemType_DAGraphicsPixmapItem = ItemType_DAGraphicsResizeableItem_Begin + 14,  ///< 针对DAGraphicsPixmapItem的类型
    ItemType_DAGraphicsResizeableItem_End = ItemType_DAGraphicsResizeableItem_Begin + 1000,
    ItemType_DAGraphicsItem_End           = 2000,  ///< DAGraphicsItem的结束范围
    ItemType_DAGraphicsLinkItem_Begin     = ItemType_DAGraphicsItem_End + 1,
    ItemType_DAGraphicsLinkItem         = ItemType_DAGraphicsLinkItem_Begin + 1,  ///< 针对DAGraphicsLinkItem的类型
    ItemType_DAGraphicsLinkItem_End     = 2500,                                   ///< DAGraphicsLinkItem的结束范围
    ItemType_DAGraphicsStandardTextItem = ItemType_GraphicsItem_Begin + 3000,     ///< 标准样式
    ItemType_DAGraphicsItemGroup        = ItemType_GraphicsItem_Begin + 4500,  ///< 针对DAGraphicsItemGroup的类型
    ItemType_GraphicsItem_End           = ItemType_GraphicsItem_Begin + 5000
};

// DANodeLinkPoint::Direction 的枚举转换
DAGRAPHICSVIEW_API QString enumToString(AspectDirection e);
// DANodeLinkPoint::Direction 的枚举转换
DAGRAPHICSVIEW_API AspectDirection stringToEnum(const QString& s, AspectDirection defaultEnum = AspectDirection::East);

// ShapeKeyPoint 的枚举转换
DAGRAPHICSVIEW_API QString enumToString(ShapeKeyPoint e);
// ShapeKeyPoint 的枚举转换
DAGRAPHICSVIEW_API ShapeKeyPoint stringToEnum(const QString& s, ShapeKeyPoint defaultEnum = ShapeKeyPoint::Center);
}

#endif  // FCMETHODNODEGLOBAL_H
