#ifndef DAPYNODECONNECTION_H
#define DAPYNODECONNECTION_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyObjectWrapper.h"
#include <QDebug>
#include <QString>

namespace DA
{

/**
 * @brief 连接DAConnection的C++纯代理类
 *
 * 代理Python DAConnection定义的连接，继承DAPyObjectWrapper，
 * 通过attr()与Python对象交互，不缓存任何Python数据到C++成员变量。
 *
 * 所有方法实时从Python对象读取属性，不做本地缓存。
 *
 * @code
 * DAPyNodeConnection proxy(pyConnObj);
 * if (!proxy.isNone()) {
 *     QString srcId = proxy.getSourceNodeId();
 *     QString srcCh = proxy.getSourceOutputChannel();
 * }
 * @endcode
 *
 * @see DAPyObjectWrapper DAPyNode
 */
class DAPYWORKFLOW_API DAPyNodeConnection : public DAPyObjectWrapper
{
public:
    // 构造/析构
    DAPyNodeConnection();
    DAPyNodeConnection(const pybind11::object& pyConnection);
    DAPyNodeConnection(pybind11::object&& pyConnection);
    DAPyNodeConnection(const DAPyObjectWrapper& pyConnection);
    DAPyNodeConnection(const DAPyNodeConnection& other);

    // 从4个字符串构造（内部创建Python DAConnection实例）
    DAPyNodeConnection(const QString& source_node_id,
                       const QString& source_output_channel,
                       const QString& target_node_id,
                       const QString& target_input_channel);

    ~DAPyNodeConnection();

    // 源节点ID（从Python对象属性source_node_id读取）
    QString getSourceNodeId() const;
    void setSourceNodeId(const QString& source_node_id);

    // 源节点输出端口名称（从Python对象属性source_output_channel读取）
    QString getSourceOutputChannel() const;
    void setSourceOutputChannel(const QString& source_output_channel);

    // 目标节点ID（从Python对象属性target_node_id读取）
    QString getTargetNodeId() const;
    void setTargetNodeId(const QString& target_node_id);

    // 目标节点输入端口名称（从Python对象属性target_input_channel读取）
    QString getTargetInputChannel() const;
    void setTargetInputChannel(const QString& target_input_channel);

    // 连接唯一ID（从Python对象属性connection_id读取）
    QString getConnectionId() const;
    void setConnectionId(const QString& connection_id);

    // 一次性全部设置连接属性
    void setConnection(const QString& source_node_id,
                       const QString& source_output_channel,
                       const QString& target_node_id,
                       const QString& target_input_channel);

    // 比较运算符（以connection_id为判据，对应Python __eq__）
    bool operator==(const DAPyNodeConnection& other) const;
    bool operator!=(const DAPyNodeConnection& other) const;
    bool operator<(const DAPyNodeConnection& other) const;
};

// QDebug输出
DAPYWORKFLOW_API QDebug operator<<(QDebug dbg, const DAPyNodeConnection& conn);

// qHash函数（用于QHash/QSet容器）
DAPYWORKFLOW_API uint qHash(const DAPyNodeConnection& key, uint seed = 0);

}  // namespace DA

Q_DECLARE_METATYPE(DA::DAPyNodeConnection)

#endif  // DAPYNODECONNECTION_H
