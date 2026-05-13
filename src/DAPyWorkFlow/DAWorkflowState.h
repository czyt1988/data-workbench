#ifndef DAWORKFLOWSTATE_H
#define DAWORKFLOWSTATE_H

#include "DAPyWorkFlowAPI.h"
#include "DAPyNodeFactory.h"
#include <QString>
#include <QVector>
#include <QPointF>

class QDomDocument;

namespace DA
{

/**
 * @brief 工作流节点状态
 *
 * 轻量级数据结构，用于序列化保存工作流图中单个节点的信息。
 * 包含节点ID、限定名、元数据引用及场景位置。
 * 与 DAParameterDescriptor 同模式：公开字段、无 Q_OBJECT、无 PIMPL。
 *
 * @see DAWorkflowConnectionState DAWorkflowState
 */
struct DAPYWORKFLOW_API DAWorkflowNodeState
{
    QString nodeId;            ///< 节点唯一标识（Python侧node_id）
    QString qualifiedName;     ///< 节点限定名（Python qualified_name）
    DAPyNodeMetaData metaData; ///< 节点元数据描述（用于重建节点）
    QPointF position;          ///< 节点在场景中的位置
};

/**
 * @brief 工作流连接线状态
 *
 * 轻量级数据结构，用于序列化保存工作流图中连接线的信息。
 * 通过节点ID和通道编号引用源节点与目标节点。
 *
 * @see DAWorkflowNodeState DAWorkflowState
 */
struct DAPYWORKFLOW_API DAWorkflowConnectionState
{
    QString connectionId;  ///< 连接唯一标识
    QString fromNodeId;    ///< 源节点ID
    int fromChannel;       ///< 源节点输出通道编号
    QString toNodeId;      ///< 目标节点ID
    int toChannel;         ///< 目标节点输入通道编号
};

/**
 * @brief 工作流图的可序列化状态
 *
 * 轻量级数据类，代表工作流图的完整状态，包含名称、节点列表和连接线列表。
 * 支持 QDomDocument 格式的 XML 序列化/反序列化，用于工作流的持久化存储。
 * 与 DAParameterDescriptor 同模式：公开字段、无 Q_OBJECT、无 PIMPL。
 *
 * 使用示例：
 * @code
 * DAWorkflowState state;
 * state.name = "MyWorkflow";
 * DAWorkflowNodeState node;
 * node.nodeId = "node_1";
 * node.qualifiedName = "data_workbench.filter_node";
 * node.position = QPointF(100, 200);
 * state.nodes.append(node);
 * QDomDocument doc;
 * state.toXml(doc);
 * DAWorkflowState restored = DAWorkflowState::fromXml(doc);
 * @endcode
 *
 * @see DAWorkflowNodeState DAWorkflowConnectionState
 */
class DAPYWORKFLOW_API DAWorkflowState
{
public:
    QString name;                              ///< 工作流名称
    QVector< DAWorkflowNodeState > nodes;      ///< 节点状态列表
    QVector< DAWorkflowConnectionState > connections;  ///< 连接线状态列表

    /**
     * @brief 枚举说明
     */
    enum Version
    {
        Version_1_0_0 = 0  ///< 版本1.0.0
    };

    // 默认构造函数
    DAWorkflowState();

    // 序列化为XML文档
    void toXml(QDomDocument& doc) const;

    // 从XML文档反序列化
    static DAWorkflowState fromXml(const QDomDocument& doc);
};

}  // namespace DA

#endif  // DAWORKFLOWSTATE_H