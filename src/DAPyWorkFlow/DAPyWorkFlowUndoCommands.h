#ifndef DAPYWORKFLOWUNDOCOMMANDS_H
#define DAPYWORKFLOWUNDOCOMMANDS_H
#include <QUndoCommand>
namespace DA
{
class DAPyWorkFlowScene;
class DAPyNodeGraphicsItem;
class DAPyLinkGraphicsItem;

/**
 * @brief 添加链接
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
    bool m_skipFirst { false };
};


/**
 * @brief 移除链接
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
