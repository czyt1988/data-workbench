#ifndef DADATAWORKFLOW_H
#define DADATAWORKFLOW_H
#include <QtCore/qglobal.h>
#include <QHash>
#include <QPointer>
#include "DAPyWorkFlow.h"
namespace DA
{
/**
 * @brief 创建节点命令
 *
 * 这里所有工作流是共享一批工厂
 */
class DADataWorkFlow : public DAPyWorkFlow
{
public:
    explicit DADataWorkFlow();
    ~DADataWorkFlow();
    //
};
}  // namespace DA
#endif  // DADATAWORKFLOW_H
