#ifndef DAPYNODESTATE_H
#define DAPYNODESTATE_H
#include <QtCore/qglobal.h>
#include "DAPyWorkFlowAPI.h"

namespace DA
{
/**
 * @brief Python节点状态枚举
 * 
 * 定义了Python工作流节点的各种执行状态
 */
enum DAPyNodeState
{
    Idle = 0,    ///< 空闲状态，节点未开始执行
    Waiting,     ///< 等待状态，节点等待依赖项完成
    Running,     ///< 运行状态，节点正在执行
    Success,     ///< 成功状态，节点执行成功
    Error,       ///< 错误状态，节点执行失败
    Skipped      ///< 跳过状态，节点被跳过执行
};

}  // end of namespace DA

// 枚举字符串转换声明已移至DAPyWorkFlowEnumStringUtils.h，避免重复声明导致C2766错误

#endif  // DAPYNODESTATE_H