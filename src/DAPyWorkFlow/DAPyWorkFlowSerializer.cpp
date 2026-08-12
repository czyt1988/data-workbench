#include "DAPyWorkFlowSerializer.h"
#include "DAPyWorkFlow.h"
#include "DAPyNodeFactory.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyBindQt/DAPyJsonCast.h"
#include "DAPybind11InQt.h"
#include "DAPybind11QtCaster.hpp"
#include <QDebug>

namespace DA
{

//===================================================
// DAPyWorkFlowSerializer
//===================================================

/**
 * @brief 默认构造函数
 */
DAPyWorkFlowSerializer::DAPyWorkFlowSerializer() : DAPyObjectWrapper()
{
}

/**
 * @brief 构造函数（从pybind11::object构造）
 * @param[in] obj Python对象
 */
DAPyWorkFlowSerializer::DAPyWorkFlowSerializer(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
}

/**
 * @brief 构造函数（从pybind11::object右值构造）
 * @param[in] obj Python对象右值
 */
DAPyWorkFlowSerializer::DAPyWorkFlowSerializer(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
}

/**
 * @brief 构造函数（从DAPyObjectWrapper构造）
 * @param[in] obj DAPyObjectWrapper对象
 */
DAPyWorkFlowSerializer::DAPyWorkFlowSerializer(const DAPyObjectWrapper& obj) : DAPyObjectWrapper(obj)
{
}

/**
 * @brief 析构函数
 */
DAPyWorkFlowSerializer::~DAPyWorkFlowSerializer()
{
}

/**
 * @brief 确保内部 Python DAWorkflowSerializer 实例已创建
 */
void DAPyWorkFlowSerializer::ensureInitialized()
{
    if (!isNone()) {
        return;
    }
    try {
        DAPyModuleWorkflow pyModule = DAPyModuleWorkflow();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                qWarning() << "DAPyWorkFlowSerializer: cannot import DAWorkbench.DAWorkFlowPy";
                return;
            }
        }
        pybind11::object serializerClass = pyModule.attr("DAWorkflowSerializer");
        if (serializerClass.is_none()) {
            qWarning() << "DAPyWorkFlowSerializer: DAWorkflowSerializer class not available";
            return;
        }
        object() = serializerClass();
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 将 DAWorkflow 序列化为 QJsonObject
 *
 * 调用 Python DAWorkflowSerializer.to_dict() 获取 dict，
 * 再通过 DAPyJsonCast 转换为 QJsonObject。
 *
 * @param[in] workflow 工作流代理
 * @return 序列化后的 QJsonObject
 */
QJsonObject DAPyWorkFlowSerializer::toDict(const DAPyWorkFlow& workflow)
{
    ensureInitialized();
    if (isNone() || workflow.isNone()) {
        return QJsonObject();
    }

    try {
        pybind11::dict pyDict = attr("to_dict")(workflow.object());
        return PY::pyDictToQJsonObject(pyDict);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QJsonObject();
}

/**
 * @brief 从 QJsonObject 反序列化重建 DAWorkflow
 *
 * @param[in] data 序列化数据
 * @param[in] factory 节点工厂（可选）
 * @return 重建的 DAWorkflow 实例
 */
DAPyWorkFlow DAPyWorkFlowSerializer::fromDict(const QJsonObject& data, const DAPyNodeFactory& factory)
{
    ensureInitialized();
    if (isNone()) {
        return DAPyWorkFlow();
    }

    try {
        pybind11::dict pyDict = PY::qjsonObjectToPyDict(data);
        pybind11::object result;
        if (!factory.isNone()) {
            result = attr("from_dict")(pyDict, pybind11::arg("node_factory") = factory.object());
        } else {
            result = attr("from_dict")(pyDict);
        }
        return DAPyWorkFlow(result);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return DAPyWorkFlow();
}

/**
 * @brief 将 DAWorkflow 序列化为 JSON 字符串
 */
QString DAPyWorkFlowSerializer::toJson(const DAPyWorkFlow& workflow)
{
    ensureInitialized();
    if (isNone() || workflow.isNone()) {
        return QString();
    }

    try {
        return attr("to_json")(workflow.object()).cast< QString >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 从 JSON 字符串反序列化重建 DAWorkflow
 */
DAPyWorkFlow DAPyWorkFlowSerializer::fromJson(const QString& jsonStr, const DAPyNodeFactory& factory)
{
    ensureInitialized();
    if (isNone()) {
        return DAPyWorkFlow();
    }

    try {
        pybind11::object pyJsonStr = pybind11::cast(jsonStr);
        pybind11::object result;
        if (!factory.isNone()) {
            result = attr("from_json")(pyJsonStr, pybind11::arg("node_factory") = factory.object());
        } else {
            result = attr("from_json")(pyJsonStr);
        }
        return DAPyWorkFlow(result);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return DAPyWorkFlow();
}

/**
 * @brief 将 DAWorkflow 序列化为 XML 字符串
 *
 * 调用 Python DAWorkflowSerializer.to_xml() 方法，
 * 将工作流的节点拓扑、参数值和连接关系序列化为 XML 格式。
 *
 * @param[in] workflow 工作流代理
 * @return XML 字符串，失败返回空字符串
 */
QString DAPyWorkFlowSerializer::toXml(const DAPyWorkFlow& workflow)
{
    ensureInitialized();
    if (isNone() || workflow.isNone()) {
        return QString();
    }

    try {
        return attr("to_xml")(workflow.object()).cast< QString >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 从 XML 字符串反序列化重建 DAWorkflow
 *
 * 调用 Python DAWorkflowSerializer.from_xml() 方法，
 * 从 XML 字符串重建包含节点、参数值和连接的 DAWorkflow 实例。
 *
 * @param[in] xmlStr XML 字符串
 * @param[in] factory 节点工厂（可选）
 * @return 重建的 DAWorkflow 实例
 */
DAPyWorkFlow DAPyWorkFlowSerializer::fromXml(const QString& xmlStr, const DAPyNodeFactory& factory)
{
    ensureInitialized();
    if (isNone()) {
        return DAPyWorkFlow();
    }

    try {
        pybind11::object pyXmlStr = pybind11::cast(xmlStr);
        pybind11::object result;
        if (!factory.isNone()) {
            result = attr("from_xml")(pyXmlStr, pybind11::arg("node_factory") = factory.object());
        } else {
            result = attr("from_xml")(pyXmlStr);
        }
        return DAPyWorkFlow(result);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return DAPyWorkFlow();
}

/**
 * @brief 保存工作流到文件
 */
bool DAPyWorkFlowSerializer::saveToFile(const DAPyWorkFlow& workflow, const QString& filePath)
{
    ensureInitialized();
    if (isNone() || workflow.isNone()) {
        return false;
    }

    try {
        attr("save_to_file")(workflow.object(), pybind11::cast(filePath));
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}

/**
 * @brief 从文件加载工作流
 */
DAPyWorkFlow DAPyWorkFlowSerializer::loadFromFile(const QString& filePath, const DAPyNodeFactory& factory)
{
    ensureInitialized();
    if (isNone()) {
        return DAPyWorkFlow();
    }

    try {
        pybind11::object pyFilePath = pybind11::cast(filePath);
        pybind11::object result;
        if (!factory.isNone()) {
            result = attr("load_from_file")(pyFilePath, pybind11::arg("node_factory") = factory.object());
        } else {
            result = attr("load_from_file")(pyFilePath);
        }
        return DAPyWorkFlow(result);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return DAPyWorkFlow();
}

}  // namespace DA
