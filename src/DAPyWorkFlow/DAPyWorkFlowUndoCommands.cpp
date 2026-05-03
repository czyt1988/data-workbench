#include "DAPyWorkFlowUndoCommands.h"
#include "DAPyWorkFlowScene.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"


namespace DA
{
//===============================================================
// DAPyWorkFlowCommand_addLinkGraphics
//===============================================================
DAPyWorkFlowCommand_addLinkGraphics::DAPyWorkFlowCommand_addLinkGraphics(
    DAPyWorkFlowScene* scene, DAPyLinkGraphicsItem* linkItem, QUndoCommand* parent
)
    : QUndoCommand(parent), m_scene(scene), m_linkItem(linkItem)
{
}


DAPyWorkFlowCommand_addLinkGraphics::~DAPyWorkFlowCommand_addLinkGraphics()
{
    if (m_needDelete) {
        delete m_linkItem;
    }
}

void DAPyWorkFlowCommand_addLinkGraphics::redo()
{
    QUndoCommand::redo();
    if (m_scene) {
        m_scene->addPyNodeLink(m_linkItem);
    }
    m_needDelete = false;
}

void DAPyWorkFlowCommand_addLinkGraphics::undo()
{
    QUndoCommand::undo();
    if (m_scene) {
        m_scene->removePyNodeLink(m_linkItem, false);
    }
    m_needDelete = true;
}

//===============================================================
// DAPyWorkFlowCommand_removeLinkGraphics
//===============================================================
DAPyWorkFlowCommand_removeLinkGraphics::DAPyWorkFlowCommand_removeLinkGraphics(
    DAPyWorkFlowScene* scene, DAPyLinkGraphicsItem* linkItem, QUndoCommand* parent
)
    : QUndoCommand(parent), m_scene(scene), m_linkItem(linkItem)
{
}

DAPyWorkFlowCommand_removeLinkGraphics::~DAPyWorkFlowCommand_removeLinkGraphics()
{
    if (m_needDelete) {
        delete m_linkItem;
    }
}

void DAPyWorkFlowCommand_removeLinkGraphics::redo()
{
    QUndoCommand::undo();
    if (m_scene) {
        m_scene->removePyNodeLink(m_linkItem, false);
    }
    m_needDelete = true;
}

void DAPyWorkFlowCommand_removeLinkGraphics::undo()
{
    QUndoCommand::redo();
    if (m_scene) {
        m_scene->addPyNodeLink(m_linkItem);
    }
    m_needDelete = false;
}


}
