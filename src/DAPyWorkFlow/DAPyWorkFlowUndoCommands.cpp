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

/**
 * @brief 构造添加节点命令
 * @param[in] scene 目标场景
 * @param[in] nodeItem 要添加的节点图形项
 * @param[in] parent 父命令
 */
DAPyWorkFlowCommand_addNodeGraphics::DAPyWorkFlowCommand_addNodeGraphics(
    DAPyWorkFlowScene* scene, DAPyNodeGraphicsItem* nodeItem, QUndoCommand* parent
)
    : QUndoCommand(parent), mScene(scene), mNodeItem(nodeItem)
{
    if (mNodeItem) {
        mProxy = mNodeItem->getProxy();
    }
    setText(QObject::tr("Add Node"));  // cn:添加节点
}

/**
 * @brief 析构添加节点命令
 *
 * 若 mNeedDelete 为 true 且节点项存在，在 GIL 守卫下删除节点图形项。
 */
DAPyWorkFlowCommand_addNodeGraphics::~DAPyWorkFlowCommand_addNodeGraphics()
{
    if (mNeedDelete && mNodeItem) {
        DAPyGILGuard gil;
        delete mNodeItem;
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
    if (mScene && mNodeItem) {
        if (mNodeItem->scene() != mScene) {
            mScene->addItem(mNodeItem);
        }
        if (mSkipFirstSync) {
            // 首次redo，Python已在createPyNode()中注册，跳过同步
            mSkipFirstSync = false;
        } else {
            // undo后恢复：重新注册到Python + 更新映射表
            mScene->syncPyNodeRegister(mNodeItem);
        }
    }
    mNeedDelete = false;
}

/**
 * @brief 从场景移除节点并同步Python
 */
void DAPyWorkFlowCommand_addNodeGraphics::undo()
{
    QUndoCommand::undo();
    if (mScene && mNodeItem) {
        mScene->removeItem(mNodeItem);
        mScene->syncPyNodeUnregister(mNodeItem);
    }
    mNeedDelete = true;
}

//===============================================================
// DAPyWorkFlowCommand_removeNodeGraphics
//===============================================================

/**
 * @brief 构造移除节点命令
 * @param[in] scene 目标场景
 * @param[in] nodeItem 要移除的节点图形项
 * @param[in] parent 父命令
 */
DAPyWorkFlowCommand_removeNodeGraphics::DAPyWorkFlowCommand_removeNodeGraphics(
    DAPyWorkFlowScene* scene, DAPyNodeGraphicsItem* nodeItem, QUndoCommand* parent
)
    : QUndoCommand(parent), mScene(scene), mNodeItem(nodeItem)
{
    if (mNodeItem) {
        mProxy = mNodeItem->getProxy();
    }
    setText(QObject::tr("Remove Node"));  // cn:移除节点
}

/**
 * @brief 析构移除节点命令
 *
 * 若 mNeedDelete 为 true 且节点项存在，在 GIL 守卫下删除节点图形项。
 */
DAPyWorkFlowCommand_removeNodeGraphics::~DAPyWorkFlowCommand_removeNodeGraphics()
{
    if (mNeedDelete && mNodeItem) {
        DAPyGILGuard gil;
        delete mNodeItem;
    }
}

/**
 * @brief 从场景移除节点并同步Python（首次redo由push触发）
 */
void DAPyWorkFlowCommand_removeNodeGraphics::redo()
{
    QUndoCommand::redo();
    if (mScene && mNodeItem) {
        mScene->removeItem(mNodeItem);
        mScene->syncPyNodeUnregister(mNodeItem);
    }
    mNeedDelete = true;
}

/**
 * @brief 恢复节点到场景并同步Python
 */
void DAPyWorkFlowCommand_removeNodeGraphics::undo()
{
    QUndoCommand::undo();
    if (mScene && mNodeItem) {
        if (mNodeItem->scene() != mScene) {
            mScene->addItem(mNodeItem);
        }
        mScene->syncPyNodeRegister(mNodeItem);
    }
    mNeedDelete = false;
}

//===============================================================
// DAPyWorkFlowCommand_addLinkGraphics
//===============================================================

/**
 * @brief 构造添加连接线命令
 * @param[in] scene 目标场景
 * @param[in] linkItem 要添加的连接线图形项
 * @param[in] parent 父命令
 */
DAPyWorkFlowCommand_addLinkGraphics::DAPyWorkFlowCommand_addLinkGraphics(
    DAPyWorkFlowScene* scene, DAPyLinkGraphicsItem* linkItem, QUndoCommand* parent
)
    : QUndoCommand(parent), mScene(scene), mLinkItem(linkItem)
{
    setText(QObject::tr("Add Link"));  // cn:添加连接
}

/**
 * @brief 析构添加连接线命令
 *
 * 若 mNeedDelete 为 true，删除连接线图形项。
 */
DAPyWorkFlowCommand_addLinkGraphics::~DAPyWorkFlowCommand_addLinkGraphics()
{
    if (mNeedDelete) {
        delete mLinkItem;
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
    if (mScene && mLinkItem) {
        mScene->addPyNodeLink(mLinkItem);
    }
    mNeedDelete = false;
}

/**
 * @brief 从场景移除连接线并同步Python
 */
void DAPyWorkFlowCommand_addLinkGraphics::undo()
{
    QUndoCommand::undo();
    if (mScene && mLinkItem) {
        mScene->removePyNodeLink(mLinkItem, false);
    }
    mNeedDelete = true;
}

//===============================================================
// DAPyWorkFlowCommand_removeLinkGraphics
//===============================================================

/**
 * @brief 构造移除连接线命令
 * @param[in] scene 目标场景
 * @param[in] linkItem 要移除的连接线图形项
 * @param[in] parent 父命令
 */
DAPyWorkFlowCommand_removeLinkGraphics::DAPyWorkFlowCommand_removeLinkGraphics(
    DAPyWorkFlowScene* scene, DAPyLinkGraphicsItem* linkItem, QUndoCommand* parent
)
    : QUndoCommand(parent), mScene(scene), mLinkItem(linkItem)
{
    setText(QObject::tr("Remove Link"));  // cn:移除连接
}

/**
 * @brief 析构移除连接线命令
 *
 * 若 mNeedDelete 为 true，删除连接线图形项。
 */
DAPyWorkFlowCommand_removeLinkGraphics::~DAPyWorkFlowCommand_removeLinkGraphics()
{
    if (mNeedDelete) {
        delete mLinkItem;
    }
}

/**
 * @brief 从场景移除连接线并同步Python
 */
void DAPyWorkFlowCommand_removeLinkGraphics::redo()
{
    QUndoCommand::redo();
    if (mScene && mLinkItem) {
        mScene->removePyNodeLink(mLinkItem, false);
    }
    mNeedDelete = true;
}

/**
 * @brief 恢复连接线到场景并同步Python
 */
void DAPyWorkFlowCommand_removeLinkGraphics::undo()
{
    QUndoCommand::undo();
    if (mScene && mLinkItem) {
        mScene->addPyNodeLink(mLinkItem);
    }
    mNeedDelete = false;
}

}
