#ifndef DAPYWORKFLOWTYPES_H
#define DAPYWORKFLOWTYPES_H

#include <QObject>
#include <QString>
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

/**
 * @brief 工作流连接描述结构体
 *
 * 描述从源节点的输出端口到目标节点的输入端口的数据传递关系。
 * 每条连接拥有唯一的 connectionId，支持相等比较。
 * 对应 Python 侧的 DAConnection 类。
 */
struct DAPYWORKFLOW_API DAPyWorkFlowConnection
{
    QString sourceNodeId;      ///< 源节点 ID
    QString sourceChannel;     ///< 源节点输出端口名称（对应 Python source_output_channel）
    QString targetNodeId;      ///< 目标节点 ID
    QString targetChannel;     ///< 目标节点输入端口名称（对应 Python target_input_channel）
    QString connectionId;      ///< 连接的唯一 ID

    // 如果 connectionId 非空，则视为有效连接
    bool isValid() const
    {
        return !connectionId.isEmpty();
    }

    // 以 connectionId 作为相等判据（对应 Python __eq__）
    bool operator==(const DAPyWorkFlowConnection& other) const
    {
        return connectionId == other.connectionId;
    }

    bool operator!=(const DAPyWorkFlowConnection& other) const
    {
        return !(*this == other);
    }
};

}  // namespace DA

Q_DECLARE_METATYPE(DA::ExecState)
Q_DECLARE_METATYPE(DA::DAPyWorkFlowConnection)
Q_DECLARE_METATYPE(std::shared_ptr<DA::DAPyNodeProxy>)

#endif  // DAPYWORKFLOWTYPES_H
