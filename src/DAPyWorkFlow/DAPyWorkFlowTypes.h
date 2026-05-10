#ifndef DAPYWORKFLOWTYPES_H
#define DAPYWORKFLOWTYPES_H

#include <QObject>
#include "DAPyWorkFlowAPI.h"
#include <memory>

namespace DA
{

// 前向声明
class DAPyNodeProxy;

/**
 * @brief 工作流执行状态枚举
 *
 * 定义工作流在执行过程中可能处于的各种状态。
 */
enum ExecState
{
    StateIdle = 0,      ///< 空闲，未开始执行
    StateRunning = 1,   ///< 运行中
    StatePaused = 2,    ///< 已暂停
    StateError = 3,     ///< 执行出错
    StateFinished = 4   ///< 执行完成
};

}  // namespace DA

Q_DECLARE_METATYPE(DA::ExecState)
Q_DECLARE_METATYPE(std::shared_ptr<DA::DAPyNodeProxy>)

#endif  // DAPYWORKFLOWTYPES_H
