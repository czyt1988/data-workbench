#ifndef TST_WORKFLOW_LIFECYCLE_H
#define TST_WORKFLOW_LIFECYCLE_H

#include <QObject>

namespace DA
{

/**
 * @brief DAPyWorkFlowLifecycle 执行路径集成测试
 *
 * 验证工作流生命周期控制器在简单执行、失败执行和暂停/恢复场景下的
 * 信号发射和状态转换行为。
 *
 * 测试依赖 Python 迆行环境，若 Python 不可用则自动 QSKIP。
 */
class TestWorkflowLifecycle : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // 简单2节点工作流执行，验证finished(true)和execStateChanged
    void testSimpleWorkflow();

    // 包含失败节点的工作流执行，验证finished(false)和nodeExecuteFinished
    void testFailingWorkflow();

    // 暂停/恢复流程，验证execStateChanged状态转换序列
    void testPauseResume();
};

}  // namespace DA

#endif  // TST_WORKFLOW_LIFECYCLE_H