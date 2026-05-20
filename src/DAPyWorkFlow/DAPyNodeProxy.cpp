#include "DAPyNodeProxy.h"
#include "DAPybind11InQt.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPyBindQt/DAPyJsonCast.h"
#include "DAPyDictConverter.h"
#include "DAPybind11QtCaster.hpp"
#include <QDebug>
#include "DAPyWorkFlowEnumStringUtils.h"  // stringToEnum<DAPyNodeState>需要

namespace DA
{

//===================================================
// DAPyNodeProxy
//===================================================

DAPyNodeProxy::DAPyNodeProxy() : DAPyObjectWrapper()
{
}

DAPyNodeProxy::DAPyNodeProxy(const pybind11::object& pyNode) : DAPyObjectWrapper(pyNode)
{
}

DAPyNodeProxy::DAPyNodeProxy(pybind11::object&& pyNode) : DAPyObjectWrapper(std::move(pyNode))
{
}

DAPyNodeProxy::DAPyNodeProxy(const DAPyObjectWrapper& pyNode) : DAPyObjectWrapper(pyNode)
{
}

DAPyNodeProxy::DAPyNodeProxy(const DAPyNodeProxy& pyNode) : DAPyObjectWrapper(pyNode)
{
}

DAPyNodeProxy::~DAPyNodeProxy()
{
}

QString DAPyNodeProxy::getNodeId() const
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

QString DAPyNodeProxy::getQualifiedName() const
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

QString DAPyNodeProxy::getNodeName() const
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

QString DAPyNodeProxy::getNodeGroup() const
{
    if (isNone()) {
        return QString();
    }
    try {
        if (hasattr("category")) {
            return attr("category").cast< QString >();
        }
        if (hasattr("group")) {
            return attr("group").cast< QString >();
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

QString DAPyNodeProxy::getIcon() const
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

QList< QString > DAPyNodeProxy::getInputKeys() const
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

QList< QString > DAPyNodeProxy::getOutputKeys() const
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

QVector< DAPortDescriptor > DAPyNodeProxy::getInputPorts() const
{
    if (isNone()) {
        return QVector< DAPortDescriptor >();
    }
    try {
        if (hasattr("inputs")) {
            pybind11::list pyInputs = attr("inputs").cast< pybind11::list >();
            return DictConverter::portListFromPyList(pyInputs);
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QVector< DAPortDescriptor >();
}

QVector< DAPortDescriptor > DAPyNodeProxy::getOutputPorts() const
{
    if (isNone()) {
        return QVector< DAPortDescriptor >();
    }
    try {
        if (hasattr("outputs")) {
            pybind11::list pyOutputs = attr("outputs").cast< pybind11::list >();
            return DictConverter::portListFromPyList(pyOutputs);
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QVector< DAPortDescriptor >();
}

QVector< DAParameterDescriptor > DAPyNodeProxy::getParameters() const
{
    if (isNone()) {
        return QVector< DAParameterDescriptor >();
    }
    try {
        if (hasattr("parameters")) {
            pybind11::list pyParams = attr("parameters").cast< pybind11::list >();
            return DictConverter::paramListFromPyList(pyParams);
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QVector< DAParameterDescriptor >();
}

DANodeStyle DAPyNodeProxy::getNodeStyle() const
{
    DANodeStyle defaultStyle;  // 默认构造已调用 setDefaults()
    if (isNone()) {
        return defaultStyle;
    }
    try {
        // 读取 _node_display.style 属性
        if (hasattr("_node_display")) {
            pybind11::object displayObj = attr("_node_display");
            if (pybind11::hasattr(displayObj, "style")
                && !pybind11::cast< pybind11::object >(displayObj.attr("style")).is_none()) {
                pybind11::object styleObj = displayObj.attr("style");
                if (pybind11::isinstance< pybind11::dict >(styleObj)) {
                    pybind11::dict styleDict = pybind11::cast< pybind11::dict >(styleObj);
                    return DictConverter::nodeStyleFromDict(styleDict);
                }
            }
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return defaultStyle;
}

RenderTemplate DAPyNodeProxy::getRenderTemplate() const
{
    if (isNone()) {
        return RenderTemplate::NodeStyleTemplate;
    }
    try {
        if (hasattr("_node_display")) {
            pybind11::object displayObj = attr("_node_display");
            if (pybind11::hasattr(displayObj, "render_template")) {
                QString rtStr = pybind11::cast< QString >(displayObj.attr("render_template"));
                return DictConverter::renderTemplateFromString(rtStr);
            }
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return RenderTemplate::NodeStyleTemplate;
}

DAPyNodeState DAPyNodeProxy::getNodeState() const
{
    if (isNone()) {
        return DAPyNodeState::Idle;
    }
    try {
        // 尝试从 Python 对象读取 _node_state 属性
        if (hasattr("_node_state")) {
            pybind11::object stateObj = attr("_node_state");
            // Python 端 _node_state 可能是字符串或 da_py_workflow.DAPyNodeState
            if (pybind11::isinstance< pybind11::str >(stateObj)) {
                QString stateStr = pybind11::cast< QString >(stateObj);
                return stringToEnum(stateStr, DAPyNodeState::Idle);
            }
            // 如果是整数枚举值
            if (pybind11::isinstance< pybind11::int_ >(stateObj)) {
                int val = pybind11::cast< int >(stateObj);
                return static_cast< DAPyNodeState >(val);
            }
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
    return DAPyNodeState::Idle;
}

void DAPyNodeProxy::setPyInputData(const QString& key, const pybind11::object& data)
{
    if (isNone()) {
        qWarning() << "DAPyNodeProxy::setPyInputData: proxy is None";
        return;
    }
    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        qWarning() << "DAPyNodeProxy::setPyInputData: Failed to acquire GIL";
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

pybind11::object DAPyNodeProxy::getPyOutputData(const QString& key) const
{
    if (isNone()) {
        return pybind11::none();
    }
    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
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

void DAPyNodeProxy::setConfig(const QJsonObject& config)
{
    if (isNone()) {
        return;
    }
    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        qWarning() << "DAPyNodeProxy::setConfig: Failed to acquire GIL";
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
}  // namespace DA
