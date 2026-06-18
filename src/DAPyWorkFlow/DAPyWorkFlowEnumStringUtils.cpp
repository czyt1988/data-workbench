#include "DAPyWorkFlowEnumStringUtils.h"
#include "DAPyNodeStyle.h"

// ================================== DA::DAPyNodeState ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeState,
                                  DA::Idle,
                                  { DA::Idle, "idle" },
                                  { DA::Waiting, "waiting" },
                                  { DA::Running, "running" },
                                  { DA::Success, "success" },
                                  { DA::Error, "error" },
                                  { DA::Skipped, "skipped" });

// ================================== DA::DAPyExecutorState ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyExecutorState,
                                  DA::ExecutorIdle,
                                  { DA::ExecutorIdle, "idle" },
                                  { DA::ExecutorRunning, "running" },
                                  { DA::ExecutorPaused, "paused" },
                                  { DA::ExecutorError, "error" },
                                  { DA::ExecutorFinished, "finished" });

// ================================== DA::DAPyWorkFlowState ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyWorkFlowState,
                                  DA::WorkflowStopped,
                                  { DA::WorkflowStopped, "stopped" },
                                  { DA::WorkflowPaused, "paused" },
                                  { DA::WorkflowRunning, "running" });

// ================================== DA::PortSide (AspectDirection) ==================================
// PortSide 是 AspectDirection 的类型别名，其 DAEnumTraits 定义已在
// DAGraphicsViewEnumStringUtils.cpp 中完成（DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::AspectDirection, ...)）
// 不重复定义，避免 ODR 违反和链接时多重定义错误
// enumToString(PortSide::West) 和 stringToEnum<PortSide>() 自动复用 AspectDirection 的转换
