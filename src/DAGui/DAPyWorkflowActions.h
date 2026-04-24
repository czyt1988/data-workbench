#ifndef DAPYWORKFLOWACTIONS_H
#define DAPYWORKFLOWACTIONS_H
#include "DAGuiAPI.h"
#include <QAction>

namespace DA
{
/**
 * @brief Python工作流操作Action集合
 * 
 * 用于在Ribbon面板构建时传入Python工作流相关的Action按钮，
 * 避免DAGui模块直接依赖APP模块的DAAppActions类。
 * 
 * @code
 * DA::DAPyWorkflowActions pyActions;
 * pyActions.actionPyWorkflowNew       = appActions->actionPyWorkflowNew;
 * pyActions.actionPyWorkflowOpen      = appActions->actionPyWorkflowOpen;
 * pyActions.actionPyWorkflowExecute   = appActions->actionPyWorkflowExecute;
 * pyActions.actionPyWorkflowTerminate = appActions->actionPyWorkflowTerminate;
 * m_pyWorkflowRibbonGroup->buildPyWorkflowPanel(panel, pyActions);
 * @endcode
 */
struct DAGUI_API DAPyWorkflowActions
{
    QAction* actionPyWorkflowNew       { nullptr };  ///< 新建Python工作流
    QAction* actionPyWorkflowOpen      { nullptr };  ///< 打开Python工作流
    QAction* actionPyWorkflowExecute   { nullptr };  ///< 执行Python工作流
    QAction* actionPyWorkflowTerminate { nullptr };  ///< 终止Python工作流执行
};
}

#endif  // DAPYWORKFLOWACTIONS_H