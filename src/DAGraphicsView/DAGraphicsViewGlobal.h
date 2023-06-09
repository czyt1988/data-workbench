﻿#ifndef DAGRAPHICSVIEWGLOBAL_H
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
 * @brief 定义了Graphics相关的command id，用于标记相同的redo/undo
 */
enum DAGraphicsCommandIDType
{
    CmdID_GraphicsBegin    = 0x100,                    ///< Graphics相关的command id的起始
    CmdID_ItemMove         = CmdID_GraphicsBegin + 1,  ///< item移动
    CmdID_ItemsMove        = CmdID_GraphicsBegin + 2,  ///< 多个item移动
    CmdID_ItemResize       = CmdID_GraphicsBegin + 3,  ///< item resize
    CmdID_ItemResizeWidth  = CmdID_GraphicsBegin + 4,  ///< item resize width
    CmdID_ItemResizeHeight = CmdID_GraphicsBegin + 5,  ///< item resize height
    CmdID_ItemRotation     = CmdID_GraphicsBegin + 6,  ///< item rotation
    CmdID_GraphicsEnd      = 0x200                     ///< Graphics相关的command id的结束
};

/**
 * @brief The DAGraphicsResizeableItemType enum
 */
enum DAGraphicsItemType
{
    ItemType_GraphicsItemBegin      = QGraphicsItem::UserType + 10,    ///<针对DAGraphicsResizeableItem的类型开始
    ItemType_GraphicsItem           = ItemType_GraphicsItemBegin + 1,  ///< 针对DAGraphicsResizeableItem的类型
    ItemType_GraphicsResizeableItem = ItemType_GraphicsItemBegin + 2,  ///< 针对DAGraphicsResizeableItem的类型
    ItemType_GraphicsResizeableObjectItem = ItemType_GraphicsItemBegin + 3,  ///< 针对DAGraphicsResizeableObjecrtItem的类型
    ItemType_GraphicsResizeableRectItem = ItemType_GraphicsItemBegin + 4,  ///< 针对DAGraphicsResizeableRectItem的类型
    ItemType_GraphicsResizeablePixmapItem = ItemType_GraphicsItemBegin + 5,  ///< 针对DAGraphicsResizeablePixmapItem的类型
    ItemType_GraphicsStandardTextItem  = ItemType_GraphicsItemBegin + 100,  ///<标准样式
    ItemType_GraphicsResizeableItemEnd = ItemType_GraphicsItemBegin + 1000
};

}

#endif  // FCMETHODNODEGLOBAL_H
