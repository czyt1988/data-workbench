#include "DAPyNode.h"
#include <QDebug>
#include "DAPybind11InQt.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyWorkFlowEnumStringUtils.h"  // stringToEnum<DAPyNodeState>需要
#include "PythonBinding/DAPyWorkFlowPythonBinding.h"
namespace DA
{

//===================================================
// DAPyNode
//===================================================

/**
 * @brief 构造一个none node
 */
DAPyNode::DAPyNode() : DAPyObjectWrapper()
{
}

DAPyNode::DAPyNode(const pybind11::object& pyNode) : DAPyObjectWrapper(pyNode)
{
}

DAPyNode::DAPyNode(pybind11::object&& pyNode) : DAPyObjectWrapper(std::move(pyNode))
{
}

DAPyNode::DAPyNode(const DAPyObjectWrapper& pyNode) : DAPyObjectWrapper(pyNode)
{
}

DAPyNode::DAPyNode(const DAPyNode& pyNode) : DAPyObjectWrapper(pyNode)
{
}

DAPyNode::~DAPyNode()
{
}

QString DAPyNode::getNodeId() const
{
    if (isNone()) {
        return QString();
    }
    try {
        if (hasattr("node_id")) {
            return attr("node_id").cast< QString >();
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

QString DAPyNode::getQualifiedName() const
{
    if (isNone()) {
        return QString();
    }
    try {
        if (hasattr("qualified_name")) {
            return attr("qualified_name").cast< QString >();
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

QString DAPyNode::getNodeName() const
{
    if (isNone()) {
        return QString();
    }
    try {
        if (hasattr("name")) {
            return attr("name").cast< QString >();
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

QString DAPyNode::getNodeCategory() const
{
    if (isNone()) {
        return QString();
    }
    try {
        if (hasattr("category")) {
            return attr("category").cast< QString >();
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

QString DAPyNode::getIcon() const
{
    if (isNone()) {
        return QString();
    }
    try {
        if (hasattr("icon")) {
            return attr("icon").cast< QString >();
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

QList< QString > DAPyNode::getInputKeys() const
{
    if (isNone()) {
        return QList< QString >();
    }
    try {
        if (hasattr("input_keys")) {
            pybind11::list pyKeys = attr("input_keys").cast< pybind11::list >();
            QList< QString > keys;
            for (auto item : pyKeys) {
                keys.append(pybind11::cast< QString >(item));
            }
            return keys;
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QList< QString >();
}

QList< QString > DAPyNode::getOutputKeys() const
{
    if (isNone()) {
        return QList< QString >();
    }
    try {
        if (hasattr("output_keys")) {
            pybind11::list pyKeys = attr("output_keys").cast< pybind11::list >();
            QList< QString > keys;
            for (auto item : pyKeys) {
                keys.append(pybind11::cast< QString >(item));
            }
            return keys;
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QList< QString >();
}

DAPyNodeStyle DAPyNode::getNodeStyle() const
{
    if (isNone()) {
        return DAPyNodeStyle();  // 默认构造已调用 setDefaults()
    }
    try {
        if (hasattr("_node_display")) {
            return PY::toNodeStyle(attr("_node_display"));
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return DAPyNodeStyle();
}

DAPyNodeState DAPyNode::getNodeState() const
{
    return PY::getNodeState(object());
}

void DAPyNode::setPyInputData(const QString& key, const pybind11::object& data)
{
    if (isNone()) {
        qWarning() << "DAPyNode::setPyInputData: proxy is None";
        return;
    }
    try {
        if (hasattr("set_input_data")) {
            pybind11::str pyKey = pybind11::cast(key);
            attr("set_input_data")(pyKey, data);
        }
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

pybind11::object DAPyNode::getPyOutputData(const QString& key) const
{
    if (isNone()) {
        return pybind11::none();
    }
    try {
        if (hasattr("get_output_data")) {
            pybind11::str pyKey = pybind11::cast(key);
            return attr("get_output_data")(pyKey);
        }
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return pybind11::none();
}

/**
 * @brief 读取单个输入数据
 *
 * 从 _input_data dict 中按 key 读取数据，与 setPyInputData 对称。
 * Python 节点没有 get_input_data 方法，因此直接读取 _input_data 属性。
 *
 * @param[in] key 数据键名
 * @return 对应的 pybind11::object，不存在时返回 pybind11::none()
 */
pybind11::object DAPyNode::getPyInputData(const QString& key) const
{
    if (isNone()) {
        return pybind11::none();
    }
    try {
        if (hasattr("_input_data")) {
            pybind11::dict inputData = attr("_input_data").cast< pybind11::dict >();
            pybind11::str pyKey = pybind11::cast(key);
            if (inputData.contains(pyKey)) {
                return pybind11::reinterpret_borrow< pybind11::object >(inputData[pyKey]);
            }
        }
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return pybind11::none();
}

/**
 * @brief 批量读取所有输入数据
 *
 * 读取 _input_data 整个 dict，通过 type_caster 自动转为 QVariantHash。
 *
 * @return 所有输入数据，代理为空时返回空 QVariantHash
 */
QVariantHash DAPyNode::getInputDatas() const
{
    if (isNone()) {
        return {};
    }
    try {
        if (hasattr("_input_data")) {
            return attr("_input_data").cast< QVariantHash >();
        }
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return {};
}

/**
 * @brief 批量读取所有输出数据
 *
 * 读取 _output_data 整个 dict，通过 type_caster 自动转为 QVariantHash。
 *
 * @return 所有输出数据，代理为空时返回空 QVariantHash
 */
QVariantHash DAPyNode::getOutputDatas() const
{
    if (isNone()) {
        return {};
    }
    try {
        if (hasattr("_output_data")) {
            return attr("_output_data").cast< QVariantHash >();
        }
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return {};
}

/**
 * @brief 获取所有参数描述符列表
 *
 * 读取Python对象的parameters属性（dict[str, Parameter]），
 * 遍历dict的values构建DAPyNodeParameter代理列表。
 *
 * @return 参数描述符列表，无参数时返回空列表
 */
QList< DAPyNodeParameter > DAPyNode::getParameters() const
{
    if (isNone()) {
        return {};
    }
    try {
        if (!hasattr("parameters")) {
            return {};
        }
        pybind11::dict pyParams = attr("parameters").cast< pybind11::dict >();
        QList< DAPyNodeParameter > result;
        for (auto item : pyParams) {
            pybind11::object paramObj = pybind11::reinterpret_borrow< pybind11::object >(item.second);
            result.append(DAPyNodeParameter(paramObj));
        }
        return result;
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return {};
}

/**
 * @brief 设置单个参数值
 *
 * 通过setattr(node, name, value)写入Python节点实例。
 *
 * @param[in] name 参数名
 * @param[in] value 参数值
 */
void DAPyNode::setParameterValue(const QString& name, const QVariant& value)
{
    if (isNone() || name.isEmpty()) {
        return;
    }
    try {
        pybind11::object obj = object();
        pybind11::object pyVal = pybind11::cast(value);
        pybind11::setattr(obj, name.toUtf8().constData(), pyVal);
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 读取单个参数值
 *
 * 通过getattr(node, name)从Python节点实例读取参数值。
 *
 * @param[in] name 参数名
 * @return 参数值，不存在时返回无效QVariant
 */
QVariant DAPyNode::getParameterValue(const QString& name) const
{
    if (isNone() || name.isEmpty()) {
        return {};
    }
    try {
        pybind11::object obj = object();
        pybind11::object val = pybind11::getattr(obj, name.toUtf8().constData(), pybind11::none());
        if (val.is_none()) {
            return {};
        }
        return val.cast< QVariant >();
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return {};
}

/**
 * @brief 批量设置节点参数（已废弃）
 *
 * @param[in] params 参数键值对
 * @deprecated 使用 DAPyNodeParameter::setValue() 或 setParameterValue() 替代
 */
void DAPyNode::setParameters(const QVariantHash& params)
{
    if (isNone()) {
        return;
    }
    try {
        pybind11::object obj = object();
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            pybind11::object pyVal = pybind11::cast(it.value());
            pybind11::setattr(obj, it.key().toUtf8().constData(), pyVal);
        }
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 批量读取节点参数值（已废弃）
 *
 * @return 参数键值对
 * @deprecated 使用 getParameters() + getParameterValue() 替代
 */
QVariantHash DAPyNode::getParameterValues() const
{
    QVariantHash result;
    const auto params = getParameters();
    for (const auto& p : params) {
        QString n = p.name();
        QVariant v = getParameterValue(n);
        if (v.isValid()) {
            result[ n ] = v;
        }
    }
    return result;
}

/**
 * @brief 将配置写入Python节点
 *
 * QVariantHash → pybind11::dict（通过 type_caster 自动转换）→ 调用 set_input_data("config", dict)。
 *
 * @param[in] config 配置数据，key 为参数名，value 为参数值
 * @deprecated 使用 setParameters() 替代，此方法将参数存入 _input_data["config"]，与序列化器不兼容
 */
void DAPyNode::setConfig(const QVariantHash& config)
{
    if (isNone()) {
        return;
    }
    try {
        pybind11::dict pyConfig = pybind11::cast(config);
        if (hasattr("set_input_data")) {
            pybind11::str pyKey("config");
            attr("set_input_data")(pyKey, pyConfig);
        }
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 从Python节点读取已保存的配置
 *
 * 读取 _input_data["config"]，通过 type_caster 自动转为 QVariantHash。
 *
 * @return 配置数据，无配置时返回空 QVariantHash
 */
QVariantHash DAPyNode::getConfig() const
{
    if (isNone()) {
        return {};
    }
    try {
        if (hasattr("_input_data")) {
            pybind11::dict inputData = attr("_input_data").cast< pybind11::dict >();
            pybind11::str key("config");
            if (inputData.contains(key)) {
                pybind11::object configObj = inputData[key];
                if (pybind11::isinstance< pybind11::dict >(configObj)) {
                    return configObj.cast< QVariantHash >();
                }
            }
        }
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return {};
}

DAPyNodeMetaData DAPyNode::getMetaData() const
{
    return PY::toNodeMetaData(object());
}

bool DAPyNode::operator==(const DAPyNode& other) const
{
    return getNodeId() == other.getNodeId();
}

bool DAPyNode::operator!=(const DAPyNode& other) const
{
    return !(*this == other);
}

bool DAPyNode::operator<(const DAPyNode& other) const
{
    return getNodeId() < other.getNodeId();
}

uint qHash(const DAPyNode& key, uint seed)
{
    return ::qHash(key.getNodeId(), seed);
}

QDebug operator<<(QDebug dbg, const DAPyNode& node)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "DAPyNode(id=" << node.getNodeId()
                  << ", name=" << node.getNodeName()
                  << ", category=" << node.getNodeCategory() << ")";
    return dbg;
}

}  // namespace DA
