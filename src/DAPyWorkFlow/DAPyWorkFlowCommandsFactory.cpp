#include "DAPyWorkFlowCommandsFactory.h"
#include "DAPyWorkFlowScene.h"
#include "DAPyWorkFlowUndoCommands.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"
namespace DA
{
/**
 * @brief 默认构造函数
 */
DAPyWorkFlowCommandsFactory::DAPyWorkFlowCommandsFactory() : DAGraphicsCommandsFactory()
{
}

/**
 * @brief 析构函数
 */
DAPyWorkFlowCommandsFactory::~DAPyWorkFlowCommandsFactory()
{
}

/**
 * @brief 创建添加节点图形项的undo命令
 * @param[in] nodeItem 节点图形项
 * @return undo命令指针
 */
DAPyWorkFlowCommand_addNodeGraphics* DAPyWorkFlowCommandsFactory::createPyNodeItemAdd(DAPyNodeGraphicsItem* nodeItem)
{
    return new DAPyWorkFlowCommand_addNodeGraphics(pyWorkflowScene(), nodeItem);
}

/**
 * @brief 创建移除节点图形项的undo命令
 * @param[in] nodeItem 节点图形项
 * @return undo命令指针
 */
DAPyWorkFlowCommand_removeNodeGraphics* DAPyWorkFlowCommandsFactory::createPyNodeItemRemove(DAPyNodeGraphicsItem* nodeItem)
{
    return new DAPyWorkFlowCommand_removeNodeGraphics(pyWorkflowScene(), nodeItem);
}

/**
 * @brief 创建添加连接线图形项的undo命令
 * @param[in] linkItem 连接线图形项
 * @return undo命令指针
 */
DAPyWorkFlowCommand_addLinkGraphics* DAPyWorkFlowCommandsFactory::createPyLinkItemAdd(DAPyLinkGraphicsItem* linkItem)
{
    return new DAPyWorkFlowCommand_addLinkGraphics(pyWorkflowScene(), linkItem);
}

/**
 * @brief 创建移除连接线图形项的undo命令
 * @param[in] linkItem 连接线图形项
 * @return undo命令指针
 */
DAPyWorkFlowCommand_removeLinkGraphics* DAPyWorkFlowCommandsFactory::createPyLinkItemRemove(DAPyLinkGraphicsItem* linkItem)
{
    return new DAPyWorkFlowCommand_removeLinkGraphics(pyWorkflowScene(), linkItem);
}

/**
 * @brief 获取当前场景的DAPyWorkFlowScene指针
 * @return DAPyWorkFlowScene指针
 */
DAPyWorkFlowScene* DAPyWorkFlowCommandsFactory::pyWorkflowScene() const
{
    return qobject_cast< DAPyWorkFlowScene* >(scene());
}
}
