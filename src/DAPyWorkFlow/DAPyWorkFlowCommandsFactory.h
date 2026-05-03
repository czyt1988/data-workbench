#ifndef DAPYWORKFLOWCOMMANDSFACTORY_H
#define DAPYWORKFLOWCOMMANDSFACTORY_H
#include "DAGraphicsCommandsFactory.h"
namespace DA
{
class DAPyWorkFlowScene;
class DAPyWorkFlowCommand_addLinkGraphics;
class DAPyWorkFlowCommand_removeLinkGraphics;
class DAPyWorkFlowScene;
class DAPyLinkGraphicsItem;
/**
 * @brief 命令工厂
 *
 * - 第一，可以根据字符串查找生成命令（这个暂时还未实现）
 * - 第二，可以用户自定义命令，例如移动命令，用户实现的移动命令需要记录其它的特殊功能，需要继承原来的移动命令则用户可以定义一个自己的命令工厂，针对移动命令生成一个用户自己的移动命令
 */
class DAPyWorkFlowCommandsFactory : public DAGraphicsCommandsFactory
{
    friend class DAPyWorkFlowScene;

public:
    DAPyWorkFlowCommandsFactory();
    virtual ~DAPyWorkFlowCommandsFactory();
    virtual DAPyWorkFlowCommand_addLinkGraphics* createPyLinkItemAdd(DAPyLinkGraphicsItem* linkItem);
    virtual DAPyWorkFlowCommand_removeLinkGraphics* createPyLinkItemRemove(DAPyLinkGraphicsItem* linkItem);
    DAPyWorkFlowScene* pyWorkflowScene() const;
};
}
#endif  // DAPYWORKFLOWCOMMANDSFACTORY_H
