#ifndef DAPYWORKFLOWAPI_H
#define DAPYWORKFLOWAPI_H
#include <QtCore/QtGlobal>
#include "DAGlobals.h"

#if defined(DAPYWORKFLOW_BUILDLIB)
#define DAPYWORKFLOW_API Q_DECL_EXPORT
#else
#ifdef Q_CC_MSVC
#define DAPYWORKFLOW_API Q_DECL_IMPORT
#else
#define DAPYWORKFLOW_API Q_DECL_IMPORT
#endif
#endif

/**
 * @brief 工作流调试宏
 *
 * 启用后，C++ 和 Python 侧的工作流执行过程会通过 qDebug 输出详细日志，
 * 包括节点分类、执行顺序、数据传播、状态变更等信息。
 *
 * 取消注释下一行以启用调试输出：
 */
#define DA_WORKFLOW_DEBUG 1

#ifdef DA_WORKFLOW_DEBUG
#define DA_WF_DBG(fmt, ...) qDebug("[WF-DBG] " fmt, ##__VA_ARGS__)
#else
#define DA_WF_DBG(fmt, ...) ((void)0)
#endif

namespace DA
{

}  // namespace DA

#endif  // DAPYWORKFLOWAPI_H
