#include "DAPyWorkFlowPythonBinding.h"
#include <QPointF>
#include "DAPyWorkFlow/DAPyWorkFlowScene.h"
#include "DAPyWorkFlow/DAPyNodeProxy.h"
#include "DAPyWorkFlow/DAPyNodeState.h"
#include "DAPyWorkFlow/DAPyNodeGraphicsItem.h"
#include "DAPyWorkFlow/DAPyLinkGraphicsItem.h"
#include "DAPyWorkFlow/DAPyPainterProxy.h"
#include "DAPyWorkFlow/DAPyLinkPoint.h"
#include "DAPyWorkFlow/DAPyNodeFactory.h"
#include "DAPyWorkFlow/DAPyNodeStyle.h"
#include "DAPyWorkFlow/DAPyWorkFlowEnumStringUtils.h"
#include "DAPyWorkFlow/DAPortDescriptor.h"
#include "DAPyWorkFlow/DANodeDescriptor.h"
#include "DAPyWorkFlow/DAParameterDescriptor.h"
#include "DAPyWorkFlow/DAWorkflowState.h"
#include "DAPyBindQt/DAPybind11QtCaster.hpp"
#include "DAPyBindQt/DAPyJsonCast.h"
#include "DAPyBindQt/DAPybind11InQt.h"  // slots workaround，必须第一个pybind11相关头文件

namespace DA
{

/**
 * @brief 通过 qualified_name 获取 DAPyNodeProxy 实例
 * @param[in] qualified_name Python 节点的完整限定名（模块.类名）
 * @return 代理节点智能指针，如果节点未注册返回空指针
 */
std::shared_ptr< DA::DAPyNodeProxy > getNodeProxy(const std::string& qualified_name)
{
    // TODO: 实现通过 DAPyModuleWorkflow 获取 Python 节点类并创建代理
    // 目前返回空指针，待 DAPyModuleWorkflow 和节点注册系统实现后完善
    return std::shared_ptr< DA::DAPyNodeProxy >();
}

}  // namespace DA

PYBIND11_EMBEDDED_MODULE(da_py_workflow, m)
{

    // =================================================================================
    //                      样式枚举绑定
    // =================================================================================
    // 导出 DAPyNodeState 枚举（Scenario C 模式）
    pybind11::enum_< DA::DAPyNodeState >(m, "DAPyNodeState")
        .value("Idle", DA::DAPyNodeState::Idle)
        .value("Waiting", DA::DAPyNodeState::Waiting)
        .value("Running", DA::DAPyNodeState::Running)
        .value("Success", DA::DAPyNodeState::Success)
        .value("Error", DA::DAPyNodeState::Error)
        .value("Skipped", DA::DAPyNodeState::Skipped)
        .export_values();

    // 导出 DAPyLinkPoint::Way 枚举
    pybind11::enum_< DA::DAPyLinkPoint::Way >(m, "DAPyLinkPointWay")
        .value("Input", DA::DAPyLinkPoint::Way::Input)
        .value("Output", DA::DAPyLinkPoint::Way::Output)
        .export_values();

    // 导出 AspectDirection 枚举（PortSide 是 AspectDirection 的类型别名）
    pybind11::enum_< DA::AspectDirection >(m, "AspectDirection")
        .value("East", DA::AspectDirection::East)
        .value("South", DA::AspectDirection::South)
        .value("West", DA::AspectDirection::West)
        .value("North", DA::AspectDirection::North)
        .export_values();
    // PortSide 是 AspectDirection 的别名，在 Python 中也作为属性导出
    m.attr("PortSide") = m.attr("AspectDirection");

    // 导出 BodyShape 枚举
    pybind11::enum_< DA::BodyShape >(m, "BodyShape")
        .value("RoundedRect", DA::BodyShape::RoundedRect)
        .value("Ellipse", DA::BodyShape::Ellipse)
        .export_values();

    // 导出 PortShape 枚举
    pybind11::enum_< DA::PortShape >(m, "PortShape")
        .value("Rect", DA::PortShape::Rect)
        .value("Circle", DA::PortShape::Circle)
        .value("Diamond", DA::PortShape::Diamond)
        .export_values();

    // 导出 NamePosition 枚举
    pybind11::enum_< DA::NamePosition >(m, "NamePosition")
        .value("Inside", DA::NamePosition::Inside)
        .value("Below", DA::NamePosition::Below)
        .export_values();

    // 导出 IconPosition 枚举
    pybind11::enum_< DA::IconPosition >(m, "IconPosition")
        .value("LeftOfText", DA::IconPosition::LeftOfText)
        .value("AboveText", DA::IconPosition::AboveText)
        .export_values();

    // 导出 BodyIconType 枚举
    pybind11::enum_< DA::BodyIconType >(m, "BodyIconType")
        .value("None", DA::BodyIconType::None)
        .value("Pixmap", DA::BodyIconType::Pixmap)
        .value("Svg", DA::BodyIconType::Svg)
        .export_values();

    // 导出 LinkPointLayoutStrategy 枚举
    pybind11::enum_< DA::LinkPointLayoutStrategy >(m, "LinkPointLayoutStrategy")
        .value("Auto", DA::LinkPointLayoutStrategy::Auto)
        .value("Manual", DA::LinkPointLayoutStrategy::Manual)
        .export_values();

    // 导出 RenderTemplate 枚举
    pybind11::enum_< DA::RenderTemplate >(m, "RenderTemplate")
        .value("NodeStyleTemplate", DA::RenderTemplate::NodeStyleTemplate)
        .value("WidgetTemplate", DA::RenderTemplate::WidgetTemplate)
        .export_values();

    // 绑定 DAPyLinkPoint 类
    pybind11::class_< DA::DAPyLinkPoint >(m, "DAPyLinkPoint")
        .def(pybind11::init<>())
        .def(pybind11::init< const QPointF&, const QString&, DA::DAPyLinkPoint::Way, DA::AspectDirection >(),
             pybind11::arg("position"),
             pybind11::arg("name"),
             pybind11::arg("way")       = DA::DAPyLinkPoint::Way::Output,
             pybind11::arg("direction") = DA::AspectDirection::East)
        .def_readwrite("position", &DA::DAPyLinkPoint::position, "Connection point position relative to graphics item")
        .def_readwrite("name", &DA::DAPyLinkPoint::name, "Connection point name")
        .def_readwrite("way", &DA::DAPyLinkPoint::way, "Input or Output attribute")
        .def_readwrite("direction", &DA::DAPyLinkPoint::direction, "Link line extending direction")
        .def("isValid", &DA::DAPyLinkPoint::isValid, "Check if connection point is valid (name not empty)")
        .def("isInput", &DA::DAPyLinkPoint::isInput, "Check if this is an input connection point")
        .def("isOutput", &DA::DAPyLinkPoint::isOutput, "Check if this is an output connection point")
        .def(
            "__eq__",
            [](const DA::DAPyLinkPoint& a, const DA::DAPyLinkPoint& b) { return a == b; },
            pybind11::arg("other"),
            "Equality comparison with another DAPyLinkPoint")
        .def(
            "__eq__",
            [](const DA::DAPyLinkPoint& a, const std::string& b) { return a == QString::fromStdString(b); },
            pybind11::arg("other"),
            "Equality comparison with a string (by name)")
        .def("__repr__", [](const DA::DAPyLinkPoint& a) {
            return QString("DAPyLinkPoint(name=%1, way=%2)")
                .arg(a.name)
                .arg(a.way == DA::DAPyLinkPoint::Way::Input ? "Input" : "Output")
                .toStdString();
        });

    // 绑定 DAPyNodeMetaData 结构体
    pybind11::class_< DA::DAPyNodeMetaData >(m, "DAPyNodeMetaData")
        .def(pybind11::init<>())
        .def_readwrite("name", &DA::DAPyNodeMetaData::name, "Node display name")
        .def_readwrite("qualifiedName", &DA::DAPyNodeMetaData::qualifiedName, "Node unique qualified name (qualified_name)")
        .def_readwrite("group", &DA::DAPyNodeMetaData::group, "Node group/category")
        .def_readwrite("iconPath", &DA::DAPyNodeMetaData::iconPath, "Node icon path")
        .def_readwrite("tooltip", &DA::DAPyNodeMetaData::tooltip, "Node tooltip text")
        .def_readwrite("inputKeys", &DA::DAPyNodeMetaData::inputKeys, "Input key list")
        .def_readwrite("outputKeys", &DA::DAPyNodeMetaData::outputKeys, "Output key list")
        .def("isValid", &DA::DAPyNodeMetaData::isValid, "Check if metadata is valid (qualifiedName not empty)")
        .def(
            "__eq__",
            [](const DA::DAPyNodeMetaData& a, const DA::DAPyNodeMetaData& b) { return a == b; },
            pybind11::arg("other"),
            "Equality comparison by qualifiedName")
        .def("__repr__", [](const DA::DAPyNodeMetaData& m) {
            return QString("DAPyNodeMetaData(name=%1, qualifiedName=%2, group=%3, inputs=%4, outputs=%5)")
                .arg(m.name)
                .arg(m.qualifiedName)
                .arg(m.group)
                .arg(m.inputKeys.size())
                .arg(m.outputKeys.size())
                .toStdString();
        });

    // =================================================================================
    //                      DAPortDescriptor 绑定
    // =================================================================================

    /**
     * @brief 绑定 DAPortDescriptor 端口描述符结构体
     *
     * 暴露端口名称、数据类型、是否必需、描述信息四个字段，
     */
    pybind11::class_< DA::DAPortDescriptor >(m, "DAPortDescriptor")
        .def(pybind11::init<>())
        .def(pybind11::init< const QString&, const QString&, bool, const QString& >(),
             pybind11::arg("name"),
             pybind11::arg("data_type"),
             pybind11::arg("required")    = true,
             pybind11::arg("description") = QString())
        .def_readwrite("name", &DA::DAPortDescriptor::name, "端口名称（唯一标识）")
        .def_readwrite("dataType", &DA::DAPortDescriptor::dataType, "数据类型（如 DataFrame、Series、int 等）")
        .def_readwrite("required", &DA::DAPortDescriptor::required, "是否为必需端口（默认 true）")
        .def_readwrite("description", &DA::DAPortDescriptor::description, "端口描述信息（可选）")
        .def("isValid", &DA::DAPortDescriptor::isValid, "判断端口描述符是否有效（name 和 dataType 均非空）")
        .def("__repr__", [](const DA::DAPortDescriptor& desc) {
            return QString("DAPortDescriptor(name=%1, dataType=%2, required=%3)")
                .arg(desc.name)
                .arg(desc.dataType)
                .arg(desc.required ? "True" : "False")
                .toStdString();
        });

    // =================================================================================
    //                      ParameterDescriptor 绑定
    // =================================================================================

    /**
     * @brief 绑定 DAParameterDescriptor 参数描述符结构体
     *
     * 暴露 name/type/description 字段（def_readwrite），
     * defaultValue 使用自定义 setDefaultValue/getDefaultValue 方法，
     * 支持 None/bool/int/float/str/list 类型转换，兼容 Qt5/Qt6 QVariant API。
     * rawDescriptor 使用自定义 setRawDescriptor/getRawDescriptor 方法，
     * 通过 pyDictToQJsonObject/qjsonObjectToPyDict 实现 Python dict ↔ QJsonObject 转换。
     * 不暴露 propertyId 为 def_readwrite（仅由面板构建器内部设置）。
     */
    pybind11::class_< DA::DAParameterDescriptor >(m, "DAParameterDescriptor")
        .def(pybind11::init<>())
        .def_readwrite("name", &DA::DAParameterDescriptor::name, "参数名称")
        .def_readwrite("type", &DA::DAParameterDescriptor::type, "参数类型 (str/int/float/bool/list/dict)")
        .def_readwrite("description", &DA::DAParameterDescriptor::description, "参数描述")
        .def(
            "setDefaultValue",
            [](DA::DAParameterDescriptor& pd, pybind11::object py) {
                pd.defaultValue = DA::PY::fromPyVariant(py);
            },
            pybind11::arg("value"),
            "设置默认值（支持 None/bool/int/float/str/list/dict/datetime 等所有类型）")
        .def(
            "getDefaultValue",
            [](const DA::DAParameterDescriptor& pd) -> pybind11::object {
                if (!pd.defaultValue.isValid()) {
                    return pybind11::none();
                }
                return DA::PY::toPyObject(pd.defaultValue);
            },
            "获取默认值")
        // rawDescriptor: 自定义 getter/setter，Python dict ↔ QVariantHash 转换
        .def(
            "setRawDescriptor",
            [](DA::DAParameterDescriptor& pd, pybind11::dict pyDict) {
                QVariantHash props;
                for (auto item : pyDict) {
                    QString key          = QString::fromStdString(item.first.cast< std::string >());
                    pybind11::object val = item.second.cast< pybind11::object >();
                    props[ key ]         = DA::PY::fromPyVariant(val);
                }
                pd.setRawDescriptor(props);
            },
            pybind11::arg("descriptor"),
            "设置原始参数描述符属性（接受 Python dict，转换为 QVariantHash 存储）")
        .def(
            "getRawDescriptor",
            [](const DA::DAParameterDescriptor& pd) -> pybind11::dict {
                QVariantHash props = pd.getRawDescriptor();
                pybind11::dict result;
                for (auto it = props.constBegin(); it != props.constEnd(); ++it) {
                    QString key                         = it.key();
                    QVariant val                        = it.value();
                    result[ key.toStdString().c_str() ] = DA::PY::toPyObject(val);
                }
                return result;
            },
            "获取原始参数描述符属性（返回 Python dict）")
        .def("__repr__", [](const DA::DAParameterDescriptor& pd) {
            return QString("ParameterDescriptor(name=%1, type=%2, description=%3)")
                .arg(pd.name)
                .arg(pd.type)
                .arg(pd.description)
                .toStdString();
        });

    // =================================================================================
    //                      DAPyLinkPointStyle 绑定
    // =================================================================================

    pybind11::class_< DA::DAPyLinkPointStyle >(m, "DAPyLinkPointStyle")
        .def(pybind11::init<>())
        .def_readwrite("shape", &DA::DAPyLinkPointStyle::shape, "Port shape (PortShape enum)")
        .def_readwrite("borderWidth", &DA::DAPyLinkPointStyle::borderWidth, "Border width (default 1.0)")
        .def(
            "setFillColor",
            [](DA::DAPyLinkPointStyle& s, QColor c) { s.fillColor = c; },
            pybind11::arg("color"),
            "Set fill color (QColor or tuple)")
        .def(
            "setFillColor",
            [](DA::DAPyLinkPointStyle& s, int r, int g, int b, int a) { s.fillColor = QColor(r, g, b, a); },
            pybind11::arg("r"),
            pybind11::arg("g"),
            pybind11::arg("b"),
            pybind11::arg("a") = 255,
            "Set fill color (RGBA, a defaults to 255)")
        .def(
            "getFillColor",
            [](const DA::DAPyLinkPointStyle& s) -> QColor { return s.fillColor; },
            "Get fill color")
        .def(
            "setBorderColor",
            [](DA::DAPyLinkPointStyle& s, QColor c) { s.borderColor = c; },
            pybind11::arg("color"),
            "Set border color (QColor or tuple)")
        .def(
            "setBorderColor",
            [](DA::DAPyLinkPointStyle& s, int r, int g, int b, int a) { s.borderColor = QColor(r, g, b, a); },
            pybind11::arg("r"),
            pybind11::arg("g"),
            pybind11::arg("b"),
            pybind11::arg("a") = 255,
            "Set border color (RGBA, a defaults to 255)")
        .def(
            "getBorderColor",
            [](const DA::DAPyLinkPointStyle& s) -> QColor { return s.borderColor; },
            "Get border color")
        .def("isFillColorValid", &DA::DAPyLinkPointStyle::isFillColorValid, "Check if fill color is valid (non-default)")
        .def("isBorderColorValid", &DA::DAPyLinkPointStyle::isBorderColorValid, "Check if border color is valid (non-default)");

    // =================================================================================
    //                      DANodeStyle 绑定
    // =================================================================================

    pybind11::class_< DA::DANodeStyle >(m, "DANodeStyle")
        .def(pybind11::init<>())
        // 主体样式
        .def_readwrite("bodyShape", &DA::DANodeStyle::bodyShape, "Node body shape (BodyShape enum)")
        .def_readwrite("namePosition", &DA::DANodeStyle::namePosition, "Name position (NamePosition enum)")
        .def_readwrite("iconPosition", &DA::DANodeStyle::iconPosition, "Icon position (IconPosition enum)")
        .def_readwrite("borderWidth", &DA::DANodeStyle::borderWidth, "Border width (default 1.0)")
        .def_readwrite("cornerRadius", &DA::DANodeStyle::cornerRadius, "Corner radius (default 4.0)")
        .def_readwrite("iconSize", &DA::DANodeStyle::iconSize, "Icon size (default 24.0)")
        .def(
            "setBackgroundColor",
            [](DA::DANodeStyle& s, QColor c) { s.backgroundColor = c; },
            pybind11::arg("color"),
            "Set background color (QColor or tuple)")
        .def(
            "setBackgroundColor",
            [](DA::DANodeStyle& s, int r, int g, int b, int a) { s.backgroundColor = QColor(r, g, b, a); },
            pybind11::arg("r"),
            pybind11::arg("g"),
            pybind11::arg("b"),
            pybind11::arg("a") = 255,
            "Set background color (RGBA, a defaults to 255)")
        .def(
            "getBackgroundColor",
            [](const DA::DANodeStyle& s) -> QColor { return s.backgroundColor; },
            "Get background color")
        .def(
            "setBorderColor",
            [](DA::DANodeStyle& s, QColor c) { s.borderColor = c; },
            pybind11::arg("color"),
            "Set border color (QColor or tuple)")
        .def(
            "setBorderColor",
            [](DA::DANodeStyle& s, int r, int g, int b, int a) { s.borderColor = QColor(r, g, b, a); },
            pybind11::arg("r"),
            pybind11::arg("g"),
            pybind11::arg("b"),
            pybind11::arg("a") = 255,
            "Set border color (RGBA, a defaults to 255)")
        .def(
            "getBorderColor",
            [](const DA::DANodeStyle& s) -> QColor { return s.borderColor; },
            "Get border color")
        // 端口配置
        .def_readwrite("inputPortSide", &DA::DANodeStyle::inputPortSide, "Input port side (AspectDirection/PortSide enum)")
        .def_readwrite("outputPortSide", &DA::DANodeStyle::outputPortSide, "Output port side (AspectDirection/PortSide enum)")
        .def_readwrite("inputPortStyle", &DA::DANodeStyle::inputPortStyle, "Input port style (DAPyLinkPointStyle)")
        .def_readwrite("outputPortStyle", &DA::DANodeStyle::outputPortStyle, "Output port style (DAPyLinkPointStyle)")
        .def_readwrite("layoutStrategy",
                       &DA::DANodeStyle::layoutStrategy,
                       "Link point layout strategy (LinkPointLayoutStrategy enum)")
        // 节点体图标
        .def_readwrite("bodyIconType", &DA::DANodeStyle::bodyIconType, "Body icon type (BodyIconType enum)")
        .def_readwrite(
            "bodyIconSource", &DA::DANodeStyle::bodyIconSource, "Body icon source path (SVG file path or resource path)")
        .def_readwrite("bodyIconScale", &DA::DANodeStyle::bodyIconScale, "Body icon scale ratio (default 0.8)")
        // 辅助方法
        .def("setDefaults", &DA::DANodeStyle::setDefaults, "Reset all fields to default values");

    // =================================================================================
    //                      DANodeDescriptor 绑定
    // =================================================================================

    /**
     * @brief 绑定 DANodeDescriptor 节点描述符结构体
     *
     * 暴露 name/qualifiedName/category/icon/renderTemplate/style 字段（def_readwrite），
     * inputs/outputs/parameters QVector 字段使用自定义 setInputs/getInputs 等 lambda 方法，
     * 逐项转换 Python list ↔ QVector<T>。style 字段可直接 def_readwrite（DANodeStyle 已绑定）。
     * 暴露 isValid()/toMetaData()/toJson()/fromJson() 方法。
     */
    pybind11::class_< DA::DANodeDescriptor >(m, "DANodeDescriptor")
        .def(pybind11::init<>())
        .def_readwrite("name", &DA::DANodeDescriptor::name, "节点显示名称")
        .def_readwrite("qualifiedName", &DA::DANodeDescriptor::qualifiedName, "节点唯一标识名（Python qualified_name）")
        .def_readwrite("category", &DA::DANodeDescriptor::category, "节点分组/分类")
        .def_readwrite("icon", &DA::DANodeDescriptor::icon, "节点图标路径")
        .def_readwrite("renderTemplate", &DA::DANodeDescriptor::renderTemplate, "渲染模板类型 (RenderTemplate enum)")
        .def_readwrite("style", &DA::DANodeDescriptor::style, "节点样式配置 (DANodeStyle)")
        // inputs: 自定义 getter/setter，Python list ↔ QVector<DAPortDescriptor>
        .def(
            "setInputs",
            [](DA::DANodeDescriptor& nd, const pybind11::list& pyList) {
                nd.inputs.clear();
                for (auto item : pyList) {
                    nd.inputs.append(item.cast< DA::DAPortDescriptor >());
                }
            },
            pybind11::arg("inputs"),
            "设置输入端口描述符列表")
        .def(
            "getInputs",
            [](const DA::DANodeDescriptor& nd) {
                pybind11::list pyList;
                for (const DA::DAPortDescriptor& desc : nd.inputs) {
                    pyList.append(desc);
                }
                return pyList;
            },
            "获取输入端口描述符列表")
        // outputs: 自定义 getter/setter，Python list ↔ QVector<DAPortDescriptor>
        .def(
            "setOutputs",
            [](DA::DANodeDescriptor& nd, const pybind11::list& pyList) {
                nd.outputs.clear();
                for (auto item : pyList) {
                    nd.outputs.append(item.cast< DA::DAPortDescriptor >());
                }
            },
            pybind11::arg("outputs"),
            "设置输出端口描述符列表")
        .def(
            "getOutputs",
            [](const DA::DANodeDescriptor& nd) {
                pybind11::list pyList;
                for (const DA::DAPortDescriptor& desc : nd.outputs) {
                    pyList.append(desc);
                }
                return pyList;
            },
            "获取输出端口描述符列表")
        // parameters: 自定义 getter/setter，Python list ↔ QVector<ParameterDescriptor>
        .def(
            "setParameters",
            [](DA::DANodeDescriptor& nd, const pybind11::list& pyList) {
                nd.parameters.clear();
                for (auto item : pyList) {
                    nd.parameters.append(item.cast< DA::DAParameterDescriptor >());
                }
            },
            pybind11::arg("parameters"),
            "设置参数描述符列表")
        .def(
            "getParameters",
            [](const DA::DANodeDescriptor& nd) {
                pybind11::list pyList;
                for (const DA::DAParameterDescriptor& desc : nd.parameters) {
                    pyList.append(desc);
                }
                return pyList;
            },
            "获取参数描述符列表")
        // 方法
        .def("isValid", &DA::DANodeDescriptor::isValid, "判断描述符是否有效（qualifiedName 非空）")
        .def("toMetaData", &DA::DANodeDescriptor::toMetaData, "转换为 DAPyNodeMetaData（提取注册所需字段）")
        .def(
            "toJson",
            [](const DA::DANodeDescriptor& desc) {
                QJsonObject json = desc.toJson();
                return DA::PY::qjsonObjectToPyDict(json);
            },
            "序列化为 Python dict（camelCase 键名，稀疏策略）")
        .def_static(
            "fromJson",
            [](const pybind11::dict& d) {
                QJsonObject json = DA::PY::pyDictToQJsonObject(d);
                return DA::DANodeDescriptor::fromJson(json);
            },
            pybind11::arg("dict"),
            "从 Python dict 反序列化")
        .def("__repr__", [](const DA::DANodeDescriptor& nd) {
            return QString(
                       "DANodeDescriptor(name=%1, qualifiedName=%2, category=%3, inputs=%4, outputs=%5, parameters=%6)")
                .arg(nd.name)
                .arg(nd.qualifiedName)
                .arg(nd.category)
                .arg(nd.inputs.size())
                .arg(nd.outputs.size())
                .arg(nd.parameters.size())
                .toStdString();
        });

    // =================================================================================
    //                      DAWorkflowNodeState 绑定
    // =================================================================================

    pybind11::class_< DA::DAWorkflowNodeState >(m, "DAWorkflowNodeState")
        .def(pybind11::init<>())
        .def_readwrite("nodeId", &DA::DAWorkflowNodeState::nodeId, "节点唯一标识（Python侧node_id）")
        .def_readwrite("qualifiedName", &DA::DAWorkflowNodeState::qualifiedName, "节点限定名（Python qualified_name）")
        .def_readwrite("metaData", &DA::DAWorkflowNodeState::metaData, "节点元数据描述（DAPyNodeMetaData）")
        .def(
            "setPosition",
            [](DA::DAWorkflowNodeState& ns, QPointF pos) { ns.position = pos; },
            pybind11::arg("pos"),
            "设置节点位置 (QPointF or tuple)")
        .def(
            "setPosition",
            [](DA::DAWorkflowNodeState& ns, double x, double y) { ns.position = QPointF(x, y); },
            pybind11::arg("x"),
            pybind11::arg("y"),
            "设置节点位置 (x, y)")
        .def(
            "getPosition",
            [](const DA::DAWorkflowNodeState& ns) -> QPointF { return ns.position; },
            "获取节点位置");

    // =================================================================================
    //                      DAWorkflowConnectionState 绑定
    // =================================================================================

    pybind11::class_< DA::DAWorkflowConnectionState >(m, "DAWorkflowConnectionState")
        .def(pybind11::init<>())
        .def_readwrite("connectionId", &DA::DAWorkflowConnectionState::connectionId, "连接唯一标识")
        .def_readwrite("fromNodeId", &DA::DAWorkflowConnectionState::fromNodeId, "源节点ID")
        .def_readwrite("fromChannel", &DA::DAWorkflowConnectionState::fromChannel, "源节点输出通道编号")
        .def_readwrite("toNodeId", &DA::DAWorkflowConnectionState::toNodeId, "目标节点ID")
        .def_readwrite("toChannel", &DA::DAWorkflowConnectionState::toChannel, "目标节点输入通道编号");

    // =================================================================================
    //                      DAWorkflowState 绑定
    // =================================================================================

    pybind11::class_< DA::DAWorkflowState >(m, "DAWorkflowState")
        .def(pybind11::init<>())
        .def_readwrite("name", &DA::DAWorkflowState::name, "工作流名称")
        // nodes: QVector<DAWorkflowNodeState> ↔ Python list
        .def(
            "setNodes",
            [](DA::DAWorkflowState& ws, const pybind11::list& pyList) {
                ws.nodes.clear();
                for (auto item : pyList) {
                    ws.nodes.append(item.cast< DA::DAWorkflowNodeState >());
                }
            },
            pybind11::arg("nodes"),
            "设置节点状态列表")
        .def(
            "getNodes",
            [](const DA::DAWorkflowState& ws) {
                pybind11::list pyList;
                for (const DA::DAWorkflowNodeState& ns : ws.nodes) {
                    pyList.append(ns);
                }
                return pyList;
            },
            "获取节点状态列表")
        // connections: QVector<DAWorkflowConnectionState> ↔ Python list
        .def(
            "setConnections",
            [](DA::DAWorkflowState& ws, const pybind11::list& pyList) {
                ws.connections.clear();
                for (auto item : pyList) {
                    ws.connections.append(item.cast< DA::DAWorkflowConnectionState >());
                }
            },
            pybind11::arg("connections"),
            "设置连接线状态列表")
        .def(
            "getConnections",
            [](const DA::DAWorkflowState& ws) {
                pybind11::list pyList;
                for (const DA::DAWorkflowConnectionState& cs : ws.connections) {
                    pyList.append(cs);
                }
                return pyList;
            },
            "获取连接线状态列表")
        // toXml: 返回XML字符串
        .def(
            "toXml",
            [](const DA::DAWorkflowState& ws) {
                QDomDocument doc;
                const_cast< DA::DAWorkflowState& >(ws).toXml(doc);
                return doc.toString().toStdString();
            },
            "序列化为XML字符串")
        // fromXml: 从XML字符串反序列化
        .def_static(
            "fromXml",
            [](const std::string& xmlStr) {
                QDomDocument doc;
                doc.setContent(QString::fromStdString(xmlStr));
                return DA::DAWorkflowState::fromXml(doc);
            },
            pybind11::arg("xml_string"),
            "从XML字符串反序列化")
        .def("__repr__", [](const DA::DAWorkflowState& ws) {
            return QString("DAWorkflowState(name=%1, nodes=%2, connections=%3)")
                .arg(ws.name)
                .arg(ws.nodes.size())
                .arg(ws.connections.size())
                .toStdString();
        });

    // 绑定 DAPyWorkFlowScene 类（Scenario B 模式：Qt 接口类 + lambda 包装）
    pybind11::class_< DA::DAPyWorkFlowScene >(m, "DAPyWorkFlowScene")
        .def(pybind11::init<>())
        // 节点管理
        .def(
            "createPyNode",
            [](DA::DAPyWorkFlowScene& self, const DA::DANodeDescriptor& descriptor, QPointF pos) {
                return self.createPyNode(descriptor, pos);
            },
            pybind11::arg("descriptor"),
            pybind11::arg("pos"),
            "Create a Python node at specified position (QPointF or tuple)")
        .def(
            "createPyNode",
            [](DA::DAPyWorkFlowScene& self, const DA::DANodeDescriptor& descriptor, double x, double y) {
                return self.createPyNode(descriptor, QPointF(x, y));
            },
            pybind11::arg("descriptor"),
            pybind11::arg("x"),
            pybind11::arg("y"),
            "Create a Python node at (x, y) position")
        .def(
            "removePyNodeItem",
            [](DA::DAPyWorkFlowScene& self, DA::DAPyNodeGraphicsItem* item) { return self.removePyNodeItem(item); },
            pybind11::arg("item"),
            "Remove a node item from scene")
        .def(
            "nodeItemAt",
            [](DA::DAPyWorkFlowScene& self, QPointF pos) { return self.nodeItemAt(pos); },
            pybind11::arg("pos"),
            "Get node item at specified position (QPointF or tuple), returns None if no node")
        .def(
            "nodeItemAt",
            [](DA::DAPyWorkFlowScene& self, double x, double y) { return self.nodeItemAt(QPointF(x, y)); },
            pybind11::arg("x"),
            pybind11::arg("y"),
            "Get node item at (x, y) position")
        .def(
            "getPyNodeItems",
            [](DA::DAPyWorkFlowScene& self) {
                QList< DA::DAPyNodeGraphicsItem* > items = self.getPyNodeItems();
                pybind11::list pyList;
                for (DA::DAPyNodeGraphicsItem* item : std::as_const(items)) {
                    pyList.append(item);
                }
                return pyList;
            },
            "Get all node items in scene as a list")
        // 连接管理
        .def(
            "addPyNodeLink",
            [](DA::DAPyWorkFlowScene& self,
               DA::DAPyNodeGraphicsItem* fromItem,
               const std::string& fromOutput,
               DA::DAPyNodeGraphicsItem* toItem,
               const std::string& toInput) {
                return self.addPyNodeLink(
                    fromItem, QString::fromStdString(fromOutput), toItem, QString::fromStdString(toInput));
            },
            pybind11::arg("fromItem"),
            pybind11::arg("fromOutput"),
            pybind11::arg("toItem"),
            pybind11::arg("toInput"),
            "Add link between node items")
        .def(
            "removePyNodeLink",
            [](DA::DAPyWorkFlowScene& self, DA::DAPyLinkGraphicsItem* linkItem) {
                return self.removePyNodeLink(linkItem);
            },
            pybind11::arg("linkItem"),
            "Remove link from scene")
        .def(
            "getPyNodeLinkItems",
            [](DA::DAPyWorkFlowScene& self) {
                QList< DA::DAPyLinkGraphicsItem* > links = self.getPyNodeLinkItems();
                pybind11::list pyList;
                for (DA::DAPyLinkGraphicsItem* link : std::as_const(links)) {
                    pyList.append(link);
                }
                return pyList;
            },
            "Get all link items in scene as a list")
        // 清空场景
        .def("clearPyScene", &DA::DAPyWorkFlowScene::clearPyScene, "Clear scene, remove all nodes and links");

    // 绑定 DAPyNodeProxy 类（独立类，不再继承DAAbstractNode）
    pybind11::class_< DA::DAPyNodeProxy >(m, "DAPyNodeProxy")
        // 状态管理
        .def("getNodeState", &DA::DAPyNodeProxy::getNodeState, "Get current node state")
        .def(
            "setNodeState",
            [](DA::DAPyNodeProxy& self, DA::DAPyNodeState state) { self.setNodeState(state); },
            pybind11::arg("state"),
            "Set node state")
        .def(
            "getLastErrorString",
            [](const DA::DAPyNodeProxy& self) { return self.getLastErrorString().toStdString(); },
            "Get error message string (when state is Error)")
        // 执行
        .def("exec", &DA::DAPyNodeProxy::exec, "Execute node logic, returns success flag")
        // 数据传递
        .def(
            "setPyInputData",
            [](DA::DAPyNodeProxy& self, const std::string& key, pybind11::object data) {
                self.setPyInputData(QString::fromStdString(key), data);
            },
            pybind11::arg("key"),
            pybind11::arg("data"),
            "Set input data (Python object reference)")
        .def(
            "getPyOutputData",
            [](DA::DAPyNodeProxy& self, const std::string& key) {
                return self.getPyOutputData(QString::fromStdString(key));
            },
            pybind11::arg("key"),
            "Get output data (Python object reference)")
        // 信息查询（新增方法）
        .def(
            "getNodeName", [](const DA::DAPyNodeProxy& self) { return self.getNodeName().toStdString(); }, "Get node display name")
        .def(
            "setNodeName",
            [](DA::DAPyNodeProxy& self, const std::string& name) { self.setNodeName(QString::fromStdString(name)); },
            pybind11::arg("name"),
            "Set node display name")
        .def(
            "getInputKeys",
            [](const DA::DAPyNodeProxy& self) {
                pybind11::list pyList;
                for (const QString& key : self.getInputKeys()) {
                    pyList.append(key.toStdString());
                }
                return pyList;
            },
            "Get input key list")
        .def(
            "getOutputKeys",
            [](const DA::DAPyNodeProxy& self) {
                pybind11::list pyList;
                for (const QString& key : self.getOutputKeys()) {
                    pyList.append(key.toStdString());
                }
                return pyList;
            },
            "Get output key list")
        .def(
            "getNodePrototype",
            [](const DA::DAPyNodeProxy& self) { return self.getNodePrototype().toStdString(); },
            "Get node prototype (qualified_name)")
        .def(
            "getNodeGroup",
            [](const DA::DAPyNodeProxy& self) { return self.getNodeGroup().toStdString(); },
            "Get node group/category")
        .def(
            "getID", [](const DA::DAPyNodeProxy& self) { return self.getID(); }, "Get node ID")
        .def(
            "setID", [](DA::DAPyNodeProxy& self, unsigned int id) { self.setID(id); }, pybind11::arg("id"), "Set node ID")
        // 原有信息查询
        .def(
            "getQualifiedName",
            [](const DA::DAPyNodeProxy& self) { return self.getQualifiedName().toStdString(); },
            "Get Python node's qualified name (module.class)");

    // 绑定 DAPyNodeFactory 类（独立QObject，不再继承DAAbstractNodeFactory）
    pybind11::class_< DA::DAPyNodeFactory >(m, "DAPyNodeFactory")
        // 节点发现
        .def(
            "discoverNodes",
            [](DA::DAPyNodeFactory& self, const std::vector< std::string >& scanPaths, bool useEntryPoints) {
                QStringList paths;
                for (const auto& p : scanPaths) {
                    paths.append(QString::fromStdString(p));
                }
                return self.discoverNodes(paths, useEntryPoints);
            },
            pybind11::arg("scan_paths")       = std::vector< std::string >(),
            pybind11::arg("use_entry_points") = false,
            "Discover Python nodes in specified paths")
        // 节点创建
        .def(
            "createNodeProxy",
            [](DA::DAPyNodeFactory& self, const std::string& qualifiedName) {
                return self.createNodeProxy(QString::fromStdString(qualifiedName));
            },
            pybind11::arg("qualified_name"),
            "Create DAPyNodeProxy by qualified name")
        // 元数据查询
        .def(
            "getNodeMetadataList",
            [](DA::DAPyNodeFactory& self) {
                pybind11::list pyList;
                for (const DA::DAPyNodeMetaData& meta : self.getNodeMetadataList()) {
                    pyList.append(meta);
                }
                return pyList;
            },
            "Get all discovered node metadata as list")
        .def(
            "getNodePrototypes",
            [](DA::DAPyNodeFactory& self) {
                pybind11::list pyList;
                for (const QString& proto : self.getNodePrototypes()) {
                    pyList.append(proto.toStdString());
                }
                return pyList;
            },
            "Get all discovered node prototype identifiers")
        // 工厂信息
        .def(
            "factoryName", [](DA::DAPyNodeFactory& self) { return self.factoryName().toStdString(); }, "Get factory name")
        .def(
            "factoryDescribe",
            [](DA::DAPyNodeFactory& self) { return self.factoryDescribe().toStdString(); },
            "Get factory description");

    // 绑定 DAPyPainterProxy 类（Scenario B 模式：非QObject代理类）
    // 注意：DAPyPainterProxy不是QObject，是QPainter的轻量代理
    pybind11::class_< DA::DAPyPainterProxy >(m, "DAPyPainterProxy")
        // 绘制操作
        .def("drawRect",
             &DA::DAPyPainterProxy::drawRect,
             pybind11::arg("x"),
             pybind11::arg("y"),
             pybind11::arg("w"),
             pybind11::arg("h"),
             "Draw rectangle outline")
        .def("drawText",
             &DA::DAPyPainterProxy::drawText,
             pybind11::arg("x"),
             pybind11::arg("y"),
             pybind11::arg("text"),
             "Draw text at specified position")
        .def("drawLine",
             &DA::DAPyPainterProxy::drawLine,
             pybind11::arg("x1"),
             pybind11::arg("y1"),
             pybind11::arg("x2"),
             pybind11::arg("y2"),
             "Draw line from (x1,y1) to (x2,y2)")
        .def("drawEllipse",
             &DA::DAPyPainterProxy::drawEllipse,
             pybind11::arg("x"),
             pybind11::arg("y"),
             pybind11::arg("w"),
             pybind11::arg("h"),
             "Draw ellipse in specified rectangle")
        .def("fillRect",
             &DA::DAPyPainterProxy::fillRect,
             pybind11::arg("x"),
             pybind11::arg("y"),
             pybind11::arg("w"),
             pybind11::arg("h"),
             pybind11::arg("r"),
             pybind11::arg("g"),
             pybind11::arg("b"),
             pybind11::arg("a") = 255,
             "Fill rectangle with RGBA color")
        // 样式设置
        .def("setPenColor",
             &DA::DAPyPainterProxy::setPenColor,
             pybind11::arg("r"),
             pybind11::arg("g"),
             pybind11::arg("b"),
             pybind11::arg("a") = 255,
             "Set pen color (RGBA)")
        .def("setPenWidth", &DA::DAPyPainterProxy::setPenWidth, pybind11::arg("width"), "Set pen width")
        .def("setBrushColor",
             &DA::DAPyPainterProxy::setBrushColor,
             pybind11::arg("r"),
             pybind11::arg("g"),
             pybind11::arg("b"),
             pybind11::arg("a") = 255,
             "Set brush color (RGBA)")
        .def("setFont", &DA::DAPyPainterProxy::setFont, pybind11::arg("family"), pybind11::arg("size"), "Set font family and size")
        .def("setNoPen", &DA::DAPyPainterProxy::setNoPen, "Set no pen (disable outline drawing)")
        .def("setNoBrush", &DA::DAPyPainterProxy::setNoBrush, "Set no brush (disable fill)")
        .def("isValid", &DA::DAPyPainterProxy::isValid, "Check if painter proxy is valid");

    // 自由函数（Scenario A 模式）
    m.def("getNodeProxy",
          &DA::getNodeProxy,
          pybind11::arg("qualified_name"),
          "Get DAPyNodeProxy instance by Python node's qualified name");

    // 节点状态变化通知说明
    m.def(
        "_note_signal_handler",
        []() {
            return "Python→C++ state updates use DAPythonSignalHandler::callInMainThread from da_interface module "
                   "(NOT a custom bridge class). Example: core.getPythonSignalHandler().callInMainThread(callback)";
        },
        "Note about state update mechanism");
}
