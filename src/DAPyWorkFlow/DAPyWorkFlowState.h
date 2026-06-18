#ifndef DAPYWORKFLOWSTATE_H
#define DAPYWORKFLOWSTATE_H
#include <QtCore/qglobal.h>
#include "DAPyWorkFlowAPI.h"

namespace DA
{
/**
 * @brief Python工作流运行状态枚举
 *
 * 对应Python DAWorkflowState枚举，定义工作流信号管理器的运行状态。
 *
 * @see DAPySignalManager
 */
enum DAPyWorkFlowState
{
    WorkflowStopped = 0,  ///< 工作流未运行，信号队列清空
    WorkflowPaused,       ///< 工作流暂停，信号队列暂停处理
    WorkflowRunning       ///< 工作流运行中，信号队列正常处理
};

}  // namespace DA

#endif  // DAPYWORKFLOWSTATE_H
