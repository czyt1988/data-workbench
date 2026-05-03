#include "DAPyWorkFlowCommandsFactory.h"
#include "DAPyWorkFlowScene.h"
#include "DAPyWorkFlowUndoCommands.h"
namespace DA
{
DAPyWorkFlowCommandsFactory::DAPyWorkFlowCommandsFactory() : DAGraphicsCommandsFactory()
{
}

DAPyWorkFlowCommandsFactory::~DAPyWorkFlowCommandsFactory()
{
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
