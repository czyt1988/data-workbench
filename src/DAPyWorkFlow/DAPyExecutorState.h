#ifndef DAPYEXECUTORSTATE_H
#define DAPYEXECUTORSTATE_H
#include <QtCore/qglobal.h>
#include "DAPyWorkFlowAPI.h"

namespace DA
{
/**
 * @brief Python工作流执行器状态枚举
 *
 * 对应Python DAExecutorState枚举，定义执行器的各种运行状态。
 *
 * @see DAPyWorkFlowExecutor
 */
enum DAPyExecutorState
{
    ExecutorIdle = 0,  ///< 空闲状态，未开始执行
    ExecutorRunning,   ///< 运行中
    ExecutorPaused,    ///< 已暂停
    ExecutorError,     ///< 执行出错
    ExecutorFinished   ///< 执行完成
};

}  // namespace DA

#endif  // DAPYEXECUTORSTATE_H
