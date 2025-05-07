#ifndef DAGRAPHICSCOMMANDSFACTORY_H
#define DAGRAPHICSCOMMANDSFACTORY_H
#include "DAGraphicsViewGlobal.h"
#include "DACommandsForGraphics.h"
namespace DA
{

/**
 * @brief 命令工厂
 *
 * 命令工厂有两个作用
 *
 * - 第一，可以根据字符串查找生成命令（这个暂时不实现）
 * - 第二，可以用户自定义命令，例如移动命令，用户实现的移动命令需要记录其它的特殊功能，需要继承原来的移动命令则用户可以定义一个自己的命令工厂，针对移动命令生成一个用户自己的移动命令
 */
class DAGRAPHICSVIEW_API DAGraphicsCommandsFactory
{
public:
    DAGraphicsCommandsFactory();
    virtual ~DAGraphicsCommandsFactory();
    virtual DACommandsForGraphicsItemAdd* createItemAdd(QGraphicsItem* item,
                                                        QGraphicsScene* scene,
                                                        QUndoCommand* parent = nullptr);
    virtual DACommandsForGraphicsItemsAdd* createItemsAdd(const QList< QGraphicsItem* > its,
                                                          QGraphicsScene* scene,
                                                          QUndoCommand* parent = nullptr);
};
}  // end DA
#endif  // DAGRAPHICSVIEWCOMMANDSFACTORY_H
