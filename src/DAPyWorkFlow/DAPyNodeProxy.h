#ifndef DAPYNODEPROXY_H
#define DAPYNODEPROXY_H
#include "DAPyWorkFlowAPI.h"
#include "DANodeDescriptor.h"
#include "DAPyNodeState.h"
#include "DAPyNodeStyle.h"
#include "DAGlobals.h"
#include "DAPybind11InQt.h"
#include <QString>
#include <QJsonObject>
#include <QList>

namespace DA
{

/**
 * @brief Python节点的C++独立代理类
 *
 * 代理Python定义的节点，不继承DAAbstractNode或任何QObject基类，
 * 遵循非QObject+PIMPL+pybind11::object模式（参考DAPyDataFrame）。
 * 通过pybind11桥接C++节点操作到Python节点执行。
 *
 * @code
 * DAPyNodeProxy proxy;
 * proxy.setPyNodeRef(pyNodeObj);
 * if (proxy.exec()) {
 *     qDebug() << "Node executed successfully";
 * } else {
 *     qDebug() << "Error:" << proxy.getLastErrorString();
 * }
 * @endcode
 *
 * @see DAPyGILGuard DAPyModuleWorkflow DAPyNodeState DAPyDataFrame
 */
class DAPYWORKFLOW_API DAPyNodeProxy
{
    DA_DECLARE_PRIVATE(DAPyNodeProxy)
public:
    // 构造/析构
    DAPyNodeProxy();
    ~DAPyNodeProxy();

    // 执行节点（GIL+Python execute）
    bool exec();

    // Python节点引用操作
    void setPyNodeRef(const pybind11::object& pyNode);
    pybind11::object getPyNodeRef() const;
    bool hasPyNodeRef() const;

    // Python限定名
    void setQualifiedName(const QString& name);
    QString getQualifiedName() const;

    // 节点名称（从Python描述符或本地存储）
    QString getNodeName() const;
    void setNodeName(const QString& name);

    // 输入/输出key列表（从Python描述符获取）
    QList<QString> getInputKeys() const;
    QList<QString> getOutputKeys() const;

    // 节点原型（从Python描述符获取）
    QString getNodePrototype() const;

    // 节点分组（从Python描述符获取）
    QString getNodeGroup() const;

    // 节点描述符（从Python _node_descriptor获取）
    // @deprecated 使用 getDescriptorStruct() 代替
    QJsonObject getDescriptor() const;

    // 节点样式（从Python描述符同步）
    DANodeStyle getNodeStyle() const;

    // 节点描述符结构体（直接访问mDescriptor）
    const DANodeDescriptor& getDescriptorStruct() const;

    // 状态管理
    DAPyNodeState getNodeState() const;
    void setNodeState(DAPyNodeState state);

    // 错误信息
    QString getLastErrorString() const;

    // Python原生数据传递
    void setPyInputData(const QString& key, const pybind11::object& data);
    pybind11::object getPyOutputData(const QString& key) const;

    // 配置参数（QJsonObject <-> Python dict）
    bool setConfig(const QJsonObject& config);
    QJsonObject getConfig() const;

    // 节点ID（独立管理，不继承DAAbstractNode）
    unsigned int getID() const;
    void setID(unsigned int id);

    // 有效性检查
    bool isValid() const;
};

}  // namespace DA

#endif  // DAPYNODEPROXY_H