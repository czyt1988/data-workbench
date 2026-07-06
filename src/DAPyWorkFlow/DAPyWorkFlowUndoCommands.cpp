#include "DAPyWorkFlowUndoCommands.h"
#include "DAPyWorkFlowScene.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAPyBindQt/DAPyGILGuard.h"


namespace DA
{

//===============================================================
// DAPyWorkFlowCommand_addNodeGraphics
//===============================================================
DAPyWorkFlowCommand_addNodeGraphics::DAPyWorkFlowCommand_addNodeGraphics(
    DAPyWorkFlowScene* scene, DAPyNodeGraphicsItem* nodeItem, QUndoCommand* parent
)
    : QUndoCommand(parent), m_scene(scene), m_nodeItem(nodeItem)
{
    if (m_nodeItem) {
        m_proxy = m_nodeItem->getProxy();
    }
    setText(QObject::tr("Add Node"));  // cn:添加节点
}

DAPyWorkFlowCommand_addNodeGraphics::~DAPyWorkFlowCommand_addNodeGraphics()
{
    if (m_needDelete && m_nodeItem) {
        DAPyGILGuard gil;
        delete m_nodeItem;
    }
}

/**
 * @brief 添加节点到场景
 *
 * 首次redo（push触发）：createPyNode()已完成Python注册，仅添加到场景
 * 后续redo（undo后恢复）：重新注册到Python + 添加到场景 + 更新映射表
 */
void DAPyWorkFlowCommand_addNodeGraphics::redo()
{
    QUndoCommand::redo();
    if (m_scene && m_nodeItem) {
        if (m_nodeItem->scene() != m_scene) {
            m_scene->addItem(m_nodeItem);
        }
        if (m_skipFirstSync) {
            // 首次redo，Python已在createPyNode()中注册，跳过同步
            m_skipFirstSync = false;
        } else {
            // undo后恢复：重新注册到Python + 更新映射表
            m_scene->syncPyNodeRegister(m_nodeItem);
        }
    }
    m_needDelete = false;
}

/**
 * @brief 从场景移除节点并同步Python
 */
void DAPyWorkFlowCommand_addNodeGraphics::undo()
{
    QUndoCommand::undo();
    if (m_scene && m_nodeItem) {
        m_scene->removeItem(m_nodeItem);
        m_scene->syncPyNodeUnregister(m_nodeItem);
    }
    m_needDelete = true;
}

//===============================================================
// DAPyWorkFlowCommand_removeNodeGraphics
//===============================================================
DAPyWorkFlowCommand_removeNodeGraphics::DAPyWorkFlowCommand_removeNodeGraphics(
    DAPyWorkFlowScene* scene, DAPyNodeGraphicsItem* nodeItem, QUndoCommand* parent
)
    : QUndoCommand(parent), m_scene(scene), m_nodeItem(nodeItem)
{
    if (m_nodeItem) {
        m_proxy = m_nodeItem->getProxy();
    }
    setText(QObject::tr("Remove Node"));  // cn:移除节点
}

DAPyWorkFlowCommand_removeNodeGraphics::~DAPyWorkFlowCommand_removeNodeGraphics()
{
    if (m_needDelete && m_nodeItem) {
        DAPyGILGuard gil;
        delete m_nodeItem;
    }
}

/**
 * @brief 从场景移除节点并同步Python（首次redo由push触发）
 */
void DAPyWorkFlowCommand_removeNodeGraphics::redo()
{
    QUndoCommand::redo();
    if (m_scene && m_nodeItem) {
        m_scene->removeItem(m_nodeItem);
        m_scene->syncPyNodeUnregister(m_nodeItem);
    }
    m_needDelete = true;
}

/**
 * @brief 恢复节点到场景并同步Python
 */
void DAPyWorkFlowCommand_removeNodeGraphics::undo()
{
    QUndoCommand::undo();
    if (m_scene && m_nodeItem) {
        if (m_nodeItem->scene() != m_scene) {
            m_scene->addItem(m_nodeItem);
        }
        m_scene->syncPyNodeRegister(m_nodeItem);
    }
    m_needDelete = false;
}

//===============================================================
// DAPyWorkFlowCommand_addLinkGraphics
//===============================================================
DAPyWorkFlowCommand_addLinkGraphics::DAPyWorkFlowCommand_addLinkGraphics(
    DAPyWorkFlowScene* scene, DAPyLinkGraphicsItem* linkItem, QUndoCommand* parent
)
    : QUndoCommand(parent), m_scene(scene), m_linkItem(linkItem)
{
    setText(QObject::tr("Add Link"));  // cn:添加连接
}

DAPyWorkFlowCommand_addLinkGraphics::~DAPyWorkFlowCommand_addLinkGraphics()
{
    if (m_needDelete) {
        delete m_linkItem;
    }
}

/**
 * @brief 添加连接线到场景并同步Python
 *
 * 调用scene的addPyNodeLink()完成Python连接同步和场景添加。
 * 此命令负责所有Python同步，调用方不需要预先同步。
 */
void DAPyWorkFlowCommand_addLinkGraphics::redo()
{
    QUndoCommand::redo();
    if (m_scene && m_linkItem) {
        m_scene->addPyNodeLink(m_linkItem);
    }
    m_needDelete = false;
}

/**
 * @brief 从场景移除连接线并同步Python
 */
void DAPyWorkFlowCommand_addLinkGraphics::undo()
{
    QUndoCommand::undo();
    if (m_scene && m_linkItem) {
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
    setText(QObject::tr("Remove Link"));  // cn:移除连接
}

DAPyWorkFlowCommand_removeLinkGraphics::~DAPyWorkFlowCommand_removeLinkGraphics()
{
    if (m_needDelete) {
        delete m_linkItem;
    }
}

/**
 * @brief 从场景移除连接线并同步Python
 */
void DAPyWorkFlowCommand_removeLinkGraphics::redo()
{
    QUndoCommand::redo();
    if (m_scene && m_linkItem) {
        m_scene->removePyNodeLink(m_linkItem, false);
    }
    m_needDelete = true;
}

/**
 * @brief 恢复连接线到场景并同步Python
 */
void DAPyWorkFlowCommand_removeLinkGraphics::undo()
{
    QUndoCommand::undo();
    if (m_scene && m_linkItem) {
        m_scene->addPyNodeLink(m_linkItem);
    }
    m_needDelete = false;
}

}
