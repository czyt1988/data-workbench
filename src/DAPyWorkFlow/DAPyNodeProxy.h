#ifndef DAPYNODEPROXY_H
#define DAPYNODEPROXY_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyNodeState.h"
#include "DAAbstractNode.h"
#include "DAPyGILGuard.h"
#include <QString>
#include <QJsonObject>

namespace DA
{

class DAPyNodeGraphicsItem;

/**
 * @brief Python节点的C++代理类
 *
 * 代理Python定义的节点，继承DAAbstractNode接口，
 * 通过pybind11桥接C++节点操作到Python节点执行。
 * 遵循PIMPL模式，使用DA_DECLARE_PRIVATE宏。
 *
 * @code
 * // 创建代理节点并执行
 * auto proxy = std::make_shared<DA::DAPyNodeProxy>();
 * proxy->setPyNodeRef(pyNodeObj);
 * if (proxy->exec()) {
 *     qDebug() << "Node executed successfully";
 * } else {
 *     qDebug() << "Error:" << proxy->getLastErrorString();
 * }
 * @endcode
 *
 * @see DAAbstractNode DAPyGILGuard DAPyModuleWorkflow DAPyNodeState
 */
class DAPYWORKFLOW_API DAPyNodeProxy : public DAAbstractNode
{
    DA_DECLARE_PRIVATE(DAPyNodeProxy)
public:
    // 构造/析构
    DAPyNodeProxy();
    ~DAPyNodeProxy();

    // 执行节点（重写DAAbstractNode::exec）
    bool exec() override;

    // 创建图形项（重写DAAbstractNode::createGraphicsItem）
    DAAbstractNodeGraphicsItem* createGraphicsItem() override;

    // Python节点引用操作
    void setPyNodeRef(const pybind11::object& pyNode);
    pybind11::object getPyNodeRef() const;
    bool hasPyNodeRef() const;

    // 设置Python限定名
    void setQualifiedName(const QString& name);
    // 获取Python限定名
    QString getQualifiedName() const;

    // 状态管理
    DAPyNodeState getNodeState() const;
    void setNodeState(DAPyNodeState state);

    // 错误信息
    QString getLastErrorString() const;

    // Python原生数据传递
    void setPyInputData(const QString& key, const pybind11::object& data);
    pybind11::object getPyOutputData(const QString& key) const;

    // 获取节点描述符（从Python节点获取元信息）
    QJsonObject getDescriptor() const;

    // 有效性检查
    bool isValid() const;

    // 设置Python节点的配置参数（通过dict传递）
    bool setConfig(const QJsonObject& config);

    // 获取Python节点的配置参数
    QJsonObject getConfig() const;
};

}  // namespace DA

#endif  // DAPYNODEPROXY_H