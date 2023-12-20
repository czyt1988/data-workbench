#ifndef DAGRAPHICSVIEWGLOBAL_H
#define DAGRAPHICSVIEWGLOBAL_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAGRAPHICSVIEW_BUILDLIB)
#define DAGRAPHICSVIEW_API Q_DECL_EXPORT
#else
#ifdef Q_CC_MSVC
#define DAGRAPHICSVIEW_API Q_DECL_IMPORT
#else
#define DAGRAPHICSVIEW_API Q_DECL_IMPORT
#endif
#endif

#include <QGraphicsItem>
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
 * @brief The DAGraphicsResizeableItemType enum
 */
enum DAGraphicsItemType
{
    ItemType_GraphicsItemBegin      = QGraphicsItem::UserType + 10,  ///< 针对DAGraphicsResizeableItem的类型开始
    ItemType_GraphicsItem           = ItemType_GraphicsItemBegin + 1,       ///< 针对DAGraphicsResizeableItem的类型
    ItemType_GraphicsResizeableItem = ItemType_GraphicsItemBegin + 2,       ///< 针对DAGraphicsResizeableItem的类型
    ItemType_GraphicsRectItem       = ItemType_GraphicsItemBegin + 10,      ///< 针对DAGraphicsRectItem的类型
    ItemType_GraphicsTextItem       = ItemType_GraphicsItemBegin + 12,      ///< 针对DAGraphicsTextItem的类型
    ItemType_GraphicsPixmapItem     = ItemType_GraphicsItemBegin + 14,      ///< 针对DAGraphicsPixmapItem的类型
    ItemType_GraphicsLinkItem       = ItemType_GraphicsItemBegin + 300,     ///< 针对DAGraphicsLinkItem的类型
    ItemType_GraphicsStandardTextItem  = ItemType_GraphicsItemBegin + 500,  ///< 标准样式
    ItemType_GraphicsResizeableItemEnd = ItemType_GraphicsItemBegin + 1000,
    ItemType_GraphicsItemGroup         = ItemType_GraphicsItemBegin + 1100  ///< 针对DAGraphicsItemGroup的类型
};
// 实现位于DAGraphicsLinkItem.cpp
// DANodeLinkPoint::Direction 的枚举转换
DAGRAPHICSVIEW_API QString enumToString(AspectDirection e);
// DANodeLinkPoint::Direction 的枚举转换
DAGRAPHICSVIEW_API AspectDirection stringToEnum(const QString& s, AspectDirection defaultEnum = AspectDirection::East);

}

#endif  // FCMETHODNODEGLOBAL_H
