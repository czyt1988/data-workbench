#ifndef DAPYWORKFLOWSERIALIZER_H
#define DAPYWORKFLOWSERIALIZER_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyObjectWrapper.h"
#include <QString>
#include <QJsonObject>
#include "DAPyNodeFactory.h"
#include "DAPyWorkFlow.h"
namespace DA
{

/**
 * @brief Python DAWorkflowSerializer 的 C++ 代理类
 *
 * 提供 DAWorkflow 的 JSON 序列化和反序列化功能。
 * 序列化将工作流的节点拓扑、参数和连接关系保存为 JSON，
 * 反序列化从 JSON 重建 DAWorkflow 实例。
 *
 * @code
 * DAPyWorkFlowSerializer serializer(factory);
 * QJsonObject data = serializer.toDict(workflow);
 * DAPyWorkFlow restored = serializer.fromDict(data);
 * @endcode
 *
 * @see DAPyWorkFlow DAPyNodeFactory
 */
class DAPYWORKFLOW_API DAPyWorkFlowSerializer : public DAPyObjectWrapper
{
public:
    DAPyWorkFlowSerializer();
    DAPyWorkFlowSerializer(const pybind11::object& obj);
    DAPyWorkFlowSerializer(pybind11::object&& obj);
    DAPyWorkFlowSerializer(const DAPyObjectWrapper& obj);
    ~DAPyWorkFlowSerializer();

    // 将 DAWorkflow 序列化为 QJsonObject
    QJsonObject toDict(const DAPyWorkFlow& workflow);
    // 从 QJsonObject 反序列化重建 DAWorkflow
    DAPyWorkFlow fromDict(const QJsonObject& data, const DAPyNodeFactory& factory = DAPyNodeFactory());
    // 将 DAWorkflow 序列化为 JSON 字符串
    QString toJson(const DAPyWorkFlow& workflow);
    // 从 JSON 字符串反序列化重建 DAWorkflow
    DAPyWorkFlow fromJson(const QString& jsonStr, const DAPyNodeFactory& factory = DAPyNodeFactory());
    // 保存到文件
    bool saveToFile(const DAPyWorkFlow& workflow, const QString& filePath);
    // 从文件加载
    DAPyWorkFlow loadFromFile(const QString& filePath, const DAPyNodeFactory& factory = DAPyNodeFactory());

private:
    // 确保内部 Python DAWorkflowSerializer 实例已创建
    void ensureInitialized();
};

}  // namespace DA

#endif  // DAPYWORKFLOWSERIALIZER_H
