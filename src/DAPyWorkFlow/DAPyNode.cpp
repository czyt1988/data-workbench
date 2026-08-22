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

/**
 * @brief 从 pybind11::object 构造节点代理
 * @param[in] pyNode Python 节点对象
 */
DAPyNode::DAPyNode(const pybind11::object& pyNode) : DAPyObjectWrapper(pyNode)
{
}

/**
 * @brief 从 pybind11::object 移动构造节点代理
 * @param[in] pyNode Python 节点对象（右值引用）
 */
DAPyNode::DAPyNode(pybind11::object&& pyNode) : DAPyObjectWrapper(std::move(pyNode))
{
}

/**
 * @brief 从 DAPyObjectWrapper 构造节点代理
 * @param[in] pyNode 已有的 Python 对象包装器
 */
DAPyNode::DAPyNode(const DAPyObjectWrapper& pyNode) : DAPyObjectWrapper(pyNode)
{
}

/**
 * @brief 拷贝构造节点代理
 * @param[in] pyNode 另一个节点代理
 */
DAPyNode::DAPyNode(const DAPyNode& pyNode) : DAPyObjectWrapper(pyNode)
{
}

/**
 * @brief 析构函数
 */
DAPyNode::~DAPyNode()
{
}

/**
 * @brief 获取节点ID
 * @return 节点ID字符串，代理为空或属性不存在时返回空字符串
 */
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

/**
 * @brief 获取节点限定名
 * @return 限定名字符串（如"pkg.module.ClassName"），代理为空时返回空字符串
 */
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

/**
 * @brief 获取节点名称
 * @return 节点显示名称字符串，代理为空时返回空字符串
 */
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

/**
 * @brief 获取节点分组
 * @return 节点分类字符串，代理为空时返回空字符串
 */
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

/**
 * @brief 获取节点图标路径
 * @return 图标资源路径字符串，代理为空时返回空字符串
 */
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

QString DAPyNode::getNodeDescription() const
{
    if (isNone()) {
        return QString();
    }
    try {
        if (hasattr("__node_description")) {
            return attr("__node_description").cast< QString >();
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

/**
 * @brief 获取输出端口key列表
 * @return 输出端口key字符串列表，代理为空时返回空列表
 */
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

/**
 * @brief 获取节点样式
 * @return 节点样式结构体，代理为空时返回默认构造的 DAPyNodeStyle
 */
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

/**
 * @brief 获取节点执行状态
 * @return 节点状态枚举值
 */
DAPyNodeState DAPyNode::getNodeState() const
{
    return PY::getNodeState(object());
}

/**
 * @brief 设置单个输入数据
 * @param[in] key 数据键名
 * @param[in] data Python 数据对象
 */
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

/**
 * @brief 获取单个输出数据
 * @param[in] key 数据键名
 * @return 对应的 Python 数据对象，不存在时返回 pybind11::none()
 */
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
 * @brief 获取节点元数据
 * @return 节点元数据结构体
 */
DAPyNodeMetaData DAPyNode::getMetaData() const
{
    return PY::toNodeMetaData(object());
}

/**
 * @brief 相等比较运算符（以 node_id 为判据）
 * @param[in] other 另一个节点代理
 * @return node_id 相同返回 true
 */
bool DAPyNode::operator==(const DAPyNode& other) const
{
    return getNodeId() == other.getNodeId();
}

/**
 * @brief 不等比较运算符
 * @param[in] other 另一个节点代理
 * @return node_id 不同返回 true
 */
bool DAPyNode::operator!=(const DAPyNode& other) const
{
    return !(*this == other);
}

/**
 * @brief 小于运算符（用于排序）
 * @param[in] other 另一个节点代理
 * @return 当前节点 node_id 小于 other 的返回 true
 */
bool DAPyNode::operator<(const DAPyNode& other) const
{
    return getNodeId() < other.getNodeId();
}

/**
 * @brief 计算 DAPyNode 的哈希值
 * @param[in] key 节点代理
 * @param[in] seed 哈希种子
 * @return 哈希值
 */
uint qHash(const DAPyNode& key, uint seed)
{
    return static_cast< uint >(::qHash(key.getNodeId(), seed));
}

/**
 * @brief 输出 DAPyNode 信息到 QDebug
 * @param[in] dbg QDebug 对象
 * @param[in] node 节点代理
 * @return QDebug 对象
 */
QDebug operator<<(QDebug dbg, const DAPyNode& node)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "DAPyNode(id=" << node.getNodeId()
                  << ", name=" << node.getNodeName()
                  << ", category=" << node.getNodeCategory() << ")";
    return dbg;
}

}  // namespace DA
