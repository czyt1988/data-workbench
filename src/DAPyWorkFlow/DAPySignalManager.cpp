#include "DAPySignalManager.h"
#include "DAPybind11InQt.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPyWorkFlowEnumStringUtils.h"

namespace DA
{

//===================================================
// DAPySignalManager
//===================================================

DAPySignalManager::DAPySignalManager() : DAPyObjectWrapper()
{
}

DAPySignalManager::DAPySignalManager(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
}

DAPySignalManager::DAPySignalManager(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
}

DAPySignalManager::DAPySignalManager(const DAPyObjectWrapper& obj) : DAPyObjectWrapper(obj)
{
}

DAPySignalManager::~DAPySignalManager()
{
}

/**
 * @brief 启动信号管理器
 *
 * 将状态切换为 Running，初始化入度计数器。
 */
void DAPySignalManager::start()
{
    if (isNone()) {
        return;
    }
    try {
        attr("start")();
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 停止信号管理器
 *
 * 将状态切换为 Stopped，清空信号队列和入度计数器。
 */
void DAPySignalManager::stop()
{
    if (isNone()) {
        return;
    }
    try {
        attr("stop")();
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 暂停信号管理器
 *
 * 只有在 Running 状态下才能暂停。
 */
void DAPySignalManager::pause()
{
    if (isNone()) {
        return;
    }
    try {
        attr("pause")();
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 恢复信号管理器
 *
 * 只有在 Paused 状态下才能恢复。
 */
void DAPySignalManager::resume()
{
    if (isNone()) {
        return;
    }
    try {
        attr("resume")();
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 获取当前工作流状态
 *
 * 从 Python 对象的 state 属性读取 DAWorkflowState 枚举值，
 * 通过字符串映射转换为 C++ DAPyWorkFlowState 枚举。
 *
 * @return 当前工作流状态
 */
DAPyWorkFlowState DAPySignalManager::getState() const
{
    if (isNone()) {
        return WorkflowStopped;
    }
    try {
        pybind11::object stateObj = attr("state");
        pybind11::object valueObj = stateObj.attr("value");
        QString stateStr          = valueObj.cast< QString >();
        return stringToEnum< DAPyWorkFlowState >(stateStr);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return WorkflowStopped;
}

/**
 * @brief 将节点输出数据加入信号队列
 *
 * @param[in] nodeId 输出节点的 ID
 * @param[in] outputChannel 输出端口名称
 * @param[in] data 输出数据（任意 Python 对象）
 */
void DAPySignalManager::sendOutput(const QString& nodeId, const QString& outputChannel, const pybind11::object& data)
{
    if (isNone()) {
        return;
    }
    try {
        attr("send_output")(nodeId, outputChannel, data);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 处理信号队列中的待传递信号
 *
 * @return 本次处理的信号数量
 */
int DAPySignalManager::processPending()
{
    if (isNone()) {
        return 0;
    }
    try {
        return attr("process_pending")().cast< int >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return 0;
}

/**
 * @brief 检查节点是否满足执行条件
 *
 * @param[in] nodeId 节点 ID
 * @return True 表示节点已收到所有必要输入数据
 */
bool DAPySignalManager::isNodeReady(const QString& nodeId) const
{
    if (isNone()) {
        return false;
    }
    try {
        return attr("is_node_ready")(nodeId).cast< bool >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 获取信号队列中待处理的信号数量
 *
 * @return 待处理信号数量
 */
int DAPySignalManager::getPendingCount() const
{
    if (isNone()) {
        return 0;
    }
    try {
        return attr("get_pending_count")().cast< int >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return 0;
}

}  // namespace DA
