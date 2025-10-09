#ifndef DAWORKFLOWGLOBAL_H
#define DAWORKFLOWGLOBAL_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAWORKFLOW_BUILDLIB)
#define DAWORKFLOW_API Q_DECL_EXPORT
#else
#ifdef Q_CC_MSVC
#define DAWORKFLOW_API Q_DECL_IMPORT
#else
#define DAWORKFLOW_API Q_DECL_IMPORT
#endif
#endif
#include <QGraphicsItem>
namespace DA
{

/**
 * @brief  定义本程序的Graphics item类型
 */
enum DANodeGraphicsItemType
{
	ItemType_GraphicsNodeItemTypeStart      = QGraphicsItem::UserType + 500,
	ItemType_GraphicsNodeItem               = ItemType_GraphicsNodeItemTypeStart + 1,  ///< 针对DANode的item
	ItemType_GraphicsNodeObject             = ItemType_GraphicsNodeItemTypeStart + 2,  ///< 针对DANode的item
	ItemType_GraphicsNodeLinkItem           = ItemType_GraphicsNodeItemTypeStart + 3,
	ItemType_GraphicsStandardNodePixmapItem = ItemType_GraphicsNodeItemTypeStart + 4,
	ItemType_GraphicsStandardNodeSvgItem    = ItemType_GraphicsNodeItemTypeStart + 5,
	ItemType_GraphicsStandardWidgetItem     = ItemType_GraphicsNodeItemTypeStart + 6,
	ItemType_GraphicsStandardRectItem       = ItemType_GraphicsNodeItemTypeStart + 7,
	ItemType_GraphicsStandardTextItem       = ItemType_GraphicsNodeItemTypeStart + 8,
	ItemType_GraphicsNodeItemTypeEnd        = QGraphicsItem::UserType + 999,
	ItemType_GraphicsNodeUserType = ItemType_GraphicsNodeItemTypeEnd + 1000  ///< 用户自定义的类型需要在此基础上加
};

/**
 * @brief DAGui 模块进行meta类型初始化
 *
 * 此函数的实现在DAWorkFlow.cpp
 */
void DAWORKFLOW_API da_workflow_register_metatypes();

}  // namespace DA

#endif  // DAWORKFLOWGLOBAL_H
