#ifndef DAPYSIGNALMANAGER_H
#define DAPYSIGNALMANAGER_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyObjectWrapper.h"
#include "DAPyWorkFlowState.h"
#include <QString>

namespace DA
{

/**
 * @brief Python DASignalManager 的 C++ 代理类
 *
 * 代理 Python DASignalManager 定义的信号管理器，继承 DAPyObjectWrapper，
 * 通过 attr() 与 Python 对象交互，不缓存任何 Python 数据到 C++ 成员变量。
 *
 * DASignalManager 负责管理节点间基于事件驱动的数据传播，
 * 维护信号队列并将节点输出数据传递到下游节点的输入端口。
 *
 * @code
 * DAPySignalManager mgr(pySignalManagerObj);
 * mgr.start();
 * mgr.sendOutput("node_1", "result", pyData);
 * mgr.processPending();
 * @endcode
 *
 * @see DAPyObjectWrapper DAPyWorkFlowExecutor DAPyWorkFlowState
 */
class DAPYWORKFLOW_API DAPySignalManager : public DAPyObjectWrapper
{
public:
    DAPySignalManager();
    DAPySignalManager(const pybind11::object& obj);
    DAPySignalManager(pybind11::object&& obj);
    DAPySignalManager(const DAPyObjectWrapper& obj);
    ~DAPySignalManager();

    // 启动信号管理器，切换到 Running 状态
    void start();
    // 停止信号管理器，切换到 Stopped 状态并清空队列
    void stop();
    // 暂停信号管理器，切换到 Paused 状态
    void pause();
    // 恢复信号管理器，从 Paused 恢复到 Running 状态
    void resume();

    // 获取当前工作流状态
    DAPyWorkFlowState getState() const;

    // 将节点输出数据加入信号队列
    void sendOutput(const QString& nodeId, const QString& outputChannel, const pybind11::object& data);

    // 处理信号队列中的待传递信号，返回本次处理的信号数量
    int processPending();

    // 检查节点是否满足执行条件（入度计数 == 总入度数）
    bool isNodeReady(const QString& nodeId) const;

    // 获取信号队列中待处理的信号数量
    int getPendingCount() const;
};

}  // namespace DA

#endif  // DAPYSIGNALMANAGER_H
