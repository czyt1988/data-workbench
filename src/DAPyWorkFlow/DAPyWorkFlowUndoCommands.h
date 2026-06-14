#ifndef DAPYWORKFLOWUNDOCOMMANDS_H
#define DAPYWORKFLOWUNDOCOMMANDS_H
#include <QUndoCommand>
#include "DAPyNode.h"
namespace DA
{
class DAPyWorkFlowScene;
class DAPyNodeGraphicsItem;
class DAPyLinkGraphicsItem;

/**
 * @brief 添加Python节点到场景（带Python状态同步）
 *
 * 用于createPyNode_()之后推入undo栈。
 * redo(): 将节点添加到QGraphicsScene + 注册到Python workflow + 更新映射表
 * undo(): 将节点从QGraphicsScene移除 + 从Python workflow注销 + 清理映射表
 *
 * @note 首次redo由push()触发时，节点已在createPyNode()中完成Python注册，
 *       因此跳过Python同步；后续redo会重新注册。
 */
class DAPyWorkFlowCommand_addNodeGraphics : public QUndoCommand
{
public:
    DAPyWorkFlowCommand_addNodeGraphics(DAPyWorkFlowScene* scene, DAPyNodeGraphicsItem* nodeItem, QUndoCommand* parent = nullptr);
    ~DAPyWorkFlowCommand_addNodeGraphics();
    void redo() override;
    void undo() override;

private:
    DAPyWorkFlowScene* m_scene { nullptr };
    DAPyNodeGraphicsItem* m_nodeItem { nullptr };
    DAPyNode m_proxy;               ///< Python节点代理，用于redo时重新注册
    bool m_needDelete { false };    ///< 命令析构时是否删除节点
    bool m_skipFirstSync { true };  ///< 首次redo跳过Python同步（createPyNode已完成注册）
};

/**
 * @brief 从场景移除Python节点（带Python状态同步）
 *
 * 用于removePyNodeItem_()推入undo栈。
 * redo(): 将节点从QGraphicsScene移除 + 从Python workflow注销 + 清理映射表
 * undo(): 将节点添加到QGraphicsScene + 重新注册到Python workflow + 更新映射表
 */
class DAPyWorkFlowCommand_removeNodeGraphics : public QUndoCommand
{
public:
    DAPyWorkFlowCommand_removeNodeGraphics(DAPyWorkFlowScene* scene, DAPyNodeGraphicsItem* nodeItem, QUndoCommand* parent = nullptr);
    ~DAPyWorkFlowCommand_removeNodeGraphics();
    void redo() override;
    void undo() override;

private:
    DAPyWorkFlowScene* m_scene { nullptr };
    DAPyNodeGraphicsItem* m_nodeItem { nullptr };
    DAPyNode m_proxy;            ///< Python节点代理，用于undo时重新注册
    bool m_needDelete { false }; ///< 命令析构时是否删除节点
};

/**
 * @brief 添加Python连接线到场景（带Python状态同步）
 *
 * redo(): 将连接线添加到QGraphicsScene + 同步Python端创建连接 + 更新映射表
 * undo(): 将连接线从QGraphicsScene移除 + 同步Python端删除连接 + 清理映射表
 */
class DAPyWorkFlowCommand_addLinkGraphics : public QUndoCommand
{
public:
    DAPyWorkFlowCommand_addLinkGraphics(DAPyWorkFlowScene* scene, DAPyLinkGraphicsItem* linkItem, QUndoCommand* parent = nullptr);
    ~DAPyWorkFlowCommand_addLinkGraphics();
    void redo() override;
    void undo() override;

private:
    DAPyWorkFlowScene* m_scene { nullptr };
    DAPyLinkGraphicsItem* m_linkItem { nullptr };
    bool m_needDelete { false };
};

/**
 * @brief 从场景移除Python连接线（带Python状态同步）
 *
 * redo(): 将连接线从QGraphicsScene移除 + 同步Python端删除连接 + 清理映射表
 * undo(): 将连接线添加到QGraphicsScene + 同步Python端创建连接 + 更新映射表
 */
class DAPyWorkFlowCommand_removeLinkGraphics : public QUndoCommand
{
public:
    DAPyWorkFlowCommand_removeLinkGraphics(DAPyWorkFlowScene* scene, DAPyLinkGraphicsItem* linkItem, QUndoCommand* parent = nullptr);
    ~DAPyWorkFlowCommand_removeLinkGraphics();
    void redo() override;
    void undo() override;

private:
    DAPyWorkFlowScene* m_scene { nullptr };
    DAPyLinkGraphicsItem* m_linkItem { nullptr };
    bool m_needDelete { false };
};
}
#endif  // DAPYWORKFLOWUNDOCOMMANDS_H
