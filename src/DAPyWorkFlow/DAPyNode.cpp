#include "DAPyNode.h"
#include <QDebug>
#include "DAPybind11InQt.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPyBindQt/DAPyJsonCast.h"
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

void DAPyNode::setConfig(const QJsonObject& config)
{
    if (isNone()) {
        return;
    }
    try {
        pybind11::dict pyConfig = DA::PY::qjsonObjectToPyDict(config);
        if (hasattr("set_input_data")) {
            pybind11::str pyKey = pybind11::cast(std::string("config"));
            attr("set_input_data")(pyKey, pyConfig);
        }
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
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
