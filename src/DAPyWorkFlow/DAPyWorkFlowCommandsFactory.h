#ifndef DAPYWORKFLOWCOMMANDSFACTORY_H
#define DAPYWORKFLOWCOMMANDSFACTORY_H
#include "DAGraphicsCommandsFactory.h"
namespace DA
{
class DAPyWorkFlowScene;
class DAPyWorkFlowCommand_addNodeGraphics;
class DAPyWorkFlowCommand_removeNodeGraphics;
class DAPyWorkFlowCommand_addLinkGraphics;
class DAPyWorkFlowCommand_removeLinkGraphics;
class DAPyNodeGraphicsItem;
class DAPyLinkGraphicsItem;

/**
 * @brief Python工作流命令工厂
 *
 * 继承DAGraphicsCommandsFactory，新增Python工作流专用的节点和连接线命令创建方法。
 * 所有命令在redo/undo时同时同步C++场景和Python workflow状态。
 */
class DAPyWorkFlowCommandsFactory : public DAGraphicsCommandsFactory
{
    friend class DAPyWorkFlowScene;

public:
    DAPyWorkFlowCommandsFactory();
    virtual ~DAPyWorkFlowCommandsFactory();

    // 创建Python节点添加命令（带Python同步）
    virtual DAPyWorkFlowCommand_addNodeGraphics* createPyNodeItemAdd(DAPyNodeGraphicsItem* nodeItem);
    // 创建Python节点移除命令（带Python同步）
    virtual DAPyWorkFlowCommand_removeNodeGraphics* createPyNodeItemRemove(DAPyNodeGraphicsItem* nodeItem);
    // 创建Python连接线添加命令（带Python同步）
    virtual DAPyWorkFlowCommand_addLinkGraphics* createPyLinkItemAdd(DAPyLinkGraphicsItem* linkItem);
    // 创建Python连接线移除命令（带Python同步）
    virtual DAPyWorkFlowCommand_removeLinkGraphics* createPyLinkItemRemove(DAPyLinkGraphicsItem* linkItem);

    DAPyWorkFlowScene* pyWorkflowScene() const;
};
}
#endif  // DAPYWORKFLOWCOMMANDSFACTORY_H
