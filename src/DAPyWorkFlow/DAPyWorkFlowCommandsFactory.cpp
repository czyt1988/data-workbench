#include "DAPyWorkFlowCommandsFactory.h"
#include "DAPyWorkFlowScene.h"
#include "DAPyWorkFlowUndoCommands.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"
namespace DA
{
DAPyWorkFlowCommandsFactory::DAPyWorkFlowCommandsFactory() : DAGraphicsCommandsFactory()
{
}

DAPyWorkFlowCommandsFactory::~DAPyWorkFlowCommandsFactory()
{
}

DAPyWorkFlowCommand_addNodeGraphics* DAPyWorkFlowCommandsFactory::createPyNodeItemAdd(DAPyNodeGraphicsItem* nodeItem)
{
    return new DAPyWorkFlowCommand_addNodeGraphics(pyWorkflowScene(), nodeItem);
}

DAPyWorkFlowCommand_removeNodeGraphics* DAPyWorkFlowCommandsFactory::createPyNodeItemRemove(DAPyNodeGraphicsItem* nodeItem)
{
    return new DAPyWorkFlowCommand_removeNodeGraphics(pyWorkflowScene(), nodeItem);
}

DAPyWorkFlowCommand_addLinkGraphics* DAPyWorkFlowCommandsFactory::createPyLinkItemAdd(DAPyLinkGraphicsItem* linkItem)
{
    return new DAPyWorkFlowCommand_addLinkGraphics(pyWorkflowScene(), linkItem);
}

DAPyWorkFlowCommand_removeLinkGraphics* DAPyWorkFlowCommandsFactory::createPyLinkItemRemove(DAPyLinkGraphicsItem* linkItem)
{
    return new DAPyWorkFlowCommand_removeLinkGraphics(pyWorkflowScene(), linkItem);
}

DAPyWorkFlowScene* DAPyWorkFlowCommandsFactory::pyWorkflowScene() const
{
    return qobject_cast< DAPyWorkFlowScene* >(scene());
}
}
