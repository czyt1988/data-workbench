# DANodeDescriptor 结构体重构实现计划

> **类型**: 重构
> **目标**: 将 Python 工作流节点描述信息从 JSON dict 传递模式迁移到 C++ 结构体直传模式
> **参考**: `docs/zh/dev-guide/workflow-node-descriptor-refactor-evaluation.md`

---

## 构建依赖链（不可违反）

```
DAShared → DAUtils → DAPyBindQt → DAPyScripts → DAPyCommonWidgets → DAPyWorkFlow
→ DAData → DACommonWidgets → DAGraphicsView → DAFigure → DAGui
→ DAInterface → DAPluginSupport → APP
```

**关键约束**: DAPyWorkFlow 不能依赖 DAGui（违反依赖方向），但可以依赖 DAUtils。

---

## 阶段 0：前置准备 — ParameterDescriptor 模块迁移

> **原因**: `ParameterDescriptor` 当前位于 `src/DAGui/NodeSetting/ParameterDescriptor.h`，`DANodeDescriptor` 需要在 `DAPyWorkFlow` 中使用它。DAPyWorkFlow 不能依赖 DAGui（违反构建依赖方向），但 DAPyWorkFlow 已经依赖 DAUtils。因此将 `ParameterDescriptor` 移到 DAUtils。

### 任务 0.1：移动 ParameterDescriptor.h 到 DAUtils

**操作**:
1. 将 `src/DAGui/NodeSetting/ParameterDescriptor.h` 的内容复制到 `src/DAUtils/ParameterDescriptor.h`
2. 在新文件中：
   - 将 `DAGUI_API` 替换为 `DAUTILS_API`（如果存在）
   - 确认 include 宏头文件改为 `#include "DAUtilsAPI.h"`（需要确认 DAUtils 的导出宏名称）
   - 保持 `namespace DA` 不变
3. 保留原位置 `src/DAGui/NodeSetting/ParameterDescriptor.h` 作为**转发头文件**，内容仅为:
   ```cpp
   #ifndef PARAMETERDESCRIPTOR_H
   #define PARAMETERDESCRIPTOR_H
   // 此头文件已迁移至 DAUtils，请直接使用 #include "DAUtils/ParameterDescriptor.h"
   #include "DAUtils/ParameterDescriptor.h"
   #endif
   ```
   **保留转发头文件是为了向后兼容**——现有 `#include "ParameterDescriptor.h"` 的代码无需修改。

**验证**: 全量编译通过

### 任务 0.2：更新 DAGui 的 #include 路径

**操作**: 在 `src/DAGui/NodeSetting/` 下的所有 `.h` 和 `.cpp` 文件中，将:
```cpp
#include "ParameterDescriptor.h"
```
替换为:
```cpp
#include "DAUtils/ParameterDescriptor.h"
```

**涉及文件**（需逐一检查）:
- `src/DAGui/NodeSetting/DANodeParamSettingPanel.h`
- `src/DAGui/NodeSetting/DANodeParamSettingPanel.cpp`
- `src/DAGui/NodeSetting/DAParamTypeRegistry.h`
- `src/DAGui/NodeSetting/DAParamTypeRegistry.cpp`
- `src/DAGui/NodeSetting/DANodeParamSettingPanelFactory.h`
- `src/DAGui/NodeSetting/DANodeParamSettingPanelFactory.cpp`
- `src/DAGui/NodeSetting/DANodeParamSettingPanelWidget.h`
- `src/DAGui/NodeSetting/DANodeParamSettingPanelWidget.cpp`
- `src/DAGui/DAAbstractNodeSettingWidget.h`
- `src/DAGui/DAAbstractNodeSettingWidget.cpp`
- 以及 DAGui 中其他引用 ParameterDescriptor 的文件

**验证**: 全量编译通过

### 任务 0.3：更新 DAUtils CMakeLists.txt

**操作**: 在 `src/DAUtils/CMakeLists.txt` 中确认 `ParameterDescriptor.h` 被包含在头文件列表中。由于 CMakeLists 使用 `file(GLOB ...)` 模式自动扫描 `*.h`，新增的文件会被自动包含，**无需手动修改**。但仍需确认安装规则也覆盖新文件。

**验证**: 全量编译通过

### 任务 0.4：更新测试文件的 #include

**操作**: 在 `src/tst/DAPyWorkFlow/tst_node_param_setting_panel.cpp` 中，将:
```cpp
#include "ParameterDescriptor.h"
```
或类似路径替换为:
```cpp
#include "DAUtils/ParameterDescriptor.h"
```

**验证**: 测试编译通过

### 阶段 0 完成标志

全量编译通过，所有现有测试通过，无链接错误。

---

## 阶段 1：创建 C++ DANodeDescriptor 结构体

### 任务 1.1：创建 DAPortDescriptor 结构体

**文件**: `src/DAPyWorkFlow/DAPortDescriptor.h`（新建）

```cpp
#ifndef DAPORTDESCRIPTOR_H
#define DAPORTDESCRIPTOR_H

#include "DAPyWorkFlowAPI.h"
#include <QString>

namespace DA
{

/**
 * @brief 工作流节点端口描述符
 *
 * 描述节点的输入/输出端口信息，对应 Python 侧 Input/Output 声明的序列化结果。
 * 替代原来 JSON dict 中的 `{"name": str, "data_type": str, "required": bool, "description": str}` 格式。
 *
 * @see DANodeDescriptor DAParamTypeRegistry
 */
struct DAPYWORKFLOW_API DAPortDescriptor
{
    QString name;           ///< 端口名称
    QString dataType;       ///< 数据类型标签（如 "DataFrame"、"int"、"str"）
    bool required = true;  ///< 是否为必填端口（仅输入端口使用）
    QString description;    ///< 端口描述

    // 默认构造函数
    DAPortDescriptor() = default;

    // 带参构造函数
    DAPortDescriptor(const QString& n, const QString& dt, bool req = true, const QString& desc = "")
        : name(n), dataType(dt), required(req), description(desc)
    {
    }

    // 有效性判断
    bool isValid() const
    {
        return !name.isEmpty();
    }

    // 从 JSON 对象解析（兼容旧格式）
    static DAPortDescriptor fromJson(const QJsonObject& obj);

    // 序列化为 JSON 对象
    QJsonObject toJson() const;
};

}  // namespace DA

#endif  // DAPORTDESCRIPTOR_H
```

**文件**: `src/DAPyWorkFlow/DAPortDescriptor.cpp`（新建）

```cpp
#include "DAPortDescriptor.h"
#include <QJsonObject>

namespace DA
{

/**
 * @brief 从 JSON 对象解析端口描述符
 *
 * @param[in] obj JSON 对象，包含 name、data_type、required、description 字段
 * @return 解析后的 DAPortDescriptor
 */
DAPortDescriptor DAPortDescriptor::fromJson(const QJsonObject& obj)
{
    DAPortDescriptor desc;
    desc.name        = obj.value(QStringLiteral("name")).toString();
    desc.dataType    = obj.value(QStringLiteral("data_type")).toString();
    desc.required    = obj.value(QStringLiteral("required")).toBool(true);
    desc.description = obj.value(QStringLiteral("description")).toString();
    return desc;
}

/**
 * @brief 序列化为 JSON 对象
 *
 * @return QJsonObject
 */
QJsonObject DAPortDescriptor::toJson() const
{
    QJsonObject obj;
    obj[ QStringLiteral("name") ]        = name;
    obj[ QStringLiteral("data_type") ]    = dataType;
    obj[ QStringLiteral("required") ]     = required;
    obj[ QStringLiteral("description") ]  = description;
    return obj;
}

}  // namespace DA
```

**验证**: 编译通过

### 任务 1.2：创建 DANodeDescriptor 结构体

**文件**: `src/DAPyWorkFlow/DANodeDescriptor.h`（新建）

```cpp
#ifndef DANODEDESCRIPTOR_H
#define DANODEDESCRIPTOR_H

#include "DAPyWorkFlowAPI.h"
#include "DAPortDescriptor.h"
#include "DAPyNodeStyle.h"
#include "DAUtils/ParameterDescriptor.h"
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

namespace DA
{

struct DAPyNodeMetaData;

/**
 * @brief 工作流节点完整描述符
 *
 * 替代原来通过 py::dict → QJsonObject 传递节点元信息的方式。
 * 包含节点所有元数据：名称、分类、端口信息、参数信息、渲染模板、样式配置。
 * Python 侧 @NodeDef 装饰器直接创建此结构体实例并填充字段，
 * C++ 侧通过 pybind11::cast 一次性获取，无需 JSON 中间转换。
 *
 * 与 DAPyNodeMetaData 的关系：
 * - DAPyNodeMetaData 是发现阶段的轻量摘要（仅含 name/qualifiedName/inputKeys/outputKeys）
 * - DANodeDescriptor 是完整描述（含 inputs/outputs/parameters/style 等）
 * - 通过 toMetaData() 可从 DANodeDescriptor 提取 DAPyNodeMetaData
 *
 * @code
 * DANodeDescriptor desc;
 * desc.name = "Data Filter";
 * desc.qualifiedName = "pkg.module.DataFilter";
 * desc.inputs = { DAPortDescriptor("data", "DataFrame", true) };
 * // ...
 * DAPyNodeMetaData meta = desc.toMetaData();
 * @endcode
 *
 * @see DAPyNodeMetaData DAPortDescriptor ParameterDescriptor DANodeStyle
 */
struct DAPYWORKFLOW_API DANodeDescriptor
{
    // === 基础标识字段 ===
    QString name;           ///< 节点显示名称
    QString qualifiedName;   ///< 唯一标识（模块名.类名）
    QString category;        ///< 节点分类/分组
    QString icon;           ///< 图标标识或路径

    // === 端口信息 ===
    QVector<DAPortDescriptor> inputs;    ///< 输入端口描述符列表
    QVector<DAPortDescriptor> outputs;   ///< 输出端口描述符列表

    // === 参数信息 ===
    QVector<ParameterDescriptor> parameters;  ///< 参数描述符列表（复用 DAUtils/ParameterDescriptor）

    // === 渲染配置 ===
    RenderTemplate renderTemplate = RenderTemplate::NodeStyleTemplate;  ///< 渲染模板类型

    // === 样式配置 ===
    DANodeStyle style;  ///< 节点样式（默认构造为默认样式）

    // === 辅助方法 ===

    // 有效性判断（qualifiedName 非空即为有效）
    bool isValid() const;

    // 提取轻量摘要（用于节点发现阶段）
    DAPyNodeMetaData toMetaData() const;

    // === JSON 序列化（稀疏策略，参考 DANodeStyleToJson） ===

    // 序列化为 JSON（仅含非默认/非空字段）
    QJsonObject toJson() const;

    // 从 JSON 反序列化
    static DANodeDescriptor fromJson(const QJsonObject& json);
};

}  // namespace DA

#endif  // DANODEDESCRIPTOR_H
```

**文件**: `src/DAPyWorkFlow/DANodeDescriptor.cpp`（新建）

```cpp
#include "DANodeDescriptor.h"
#include "DAPyNodeFactory.h"
#include "DAEnumStringUtils.hpp"
#include <QJsonObject>
#include <QJsonArray>

namespace DA
{

/**
 * @brief 判断描述符是否有效
 *
 * 有效条件：qualifiedName 非空。qualifiedName 是节点的唯一标识，
 * 是创建节点和查找注册信息的必要字段。
 *
 * @return true 表示有效，false 表示无效
 */
bool DANodeDescriptor::isValid() const
{
    return !qualifiedName.isEmpty();
}

/**
 * @brief 从完整描述符提取轻量摘要
 *
 * 将 DANodeDescriptor 的基础标识字段和端口 key 列表提取为 DAPyNodeMetaData。
 * DAPyNodeMetaData 用于节点发现阶段的 UI 展示（节点列表面板），
 * 不包含完整的端口描述、参数描述和样式信息。
 *
 * @return DAPyNodeMetaData 轻量摘要
 */
DAPyNodeMetaData DANodeDescriptor::toMetaData() const
{
    DAPyNodeMetaData meta;
    meta.name           = name;
    meta.qualifiedName = qualifiedName;
    meta.group         = category;
    meta.iconPath      = icon;
    meta.tooltip      = name;
    if (!qualifiedName.isEmpty()) {
        meta.tooltip += " (" + qualifiedName + ")";
    }
    // 提取端口 key 列表
    for (const DAPortDescriptor& inp : inputs) {
        meta.inputKeys.append(inp.name);
    }
    for (const DAPortDescriptor& outp : outputs) {
        meta.outputKeys.append(outp.name);
    }
    return meta;
}

/**
 * @brief 序列化为 JSON 对象
 *
 * 采用稀疏策略：仅序列化非空/非默认值字段。
 * 输出格式与原 Python _node_descriptor dict 格式完全一致，确保工程文件向后兼容。
 *
 * @return QJsonObject
 */
QJsonObject DANodeDescriptor::toJson() const
{
    QJsonObject json;

    // 基础标识字段（均需序列化，这些字段没有"默认值"概念）
    if (!name.isEmpty()) {
        json[ QStringLiteral("name") ] = name;
    }
    if (!qualifiedName.isEmpty()) {
        json[ QStringLiteral("qualified_name") ] = qualifiedName;
    }
    if (!category.isEmpty()) {
        json[ QStringLiteral("category") ] = category;
    }
    if (!icon.isEmpty()) {
        json[ QStringLiteral("icon") ] = icon;
    }

    // 端口信息
    if (!inputs.isEmpty()) {
        QJsonArray inputsArr;
        for (const DAPortDescriptor& inp : inputs) {
            inputsArr.append(inp.toJson());
        }
        json[ QStringLiteral("inputs") ] = inputsArr;
    }
    if (!outputs.isEmpty()) {
        QJsonArray outputsArr;
        for (const DAPortDescriptor& outp : outputs) {
            outputsArr.append(outp.toJson());
        }
        json[ QStringLiteral("outputs") ] = outputsArr;
    }

    // 参数信息
    if (!parameters.isEmpty()) {
        QJsonArray paramsArr;
        for (const ParameterDescriptor& param : parameters) {
            // 使用 rawDescriptor 保留完整参数信息（含 default、enum_values 等扩展字段）
            if (!param.rawDescriptor.isEmpty()) {
                paramsArr.append(param.rawDescriptor);
            } else {
                // 如果 rawDescriptor 为空，手动构建基础 JSON
                QJsonObject paramObj;
                paramObj[ QStringLiteral("name") ]        = param.name;
                paramObj[ QStringLiteral("type") ]         = param.type;
                if (!param.description.isEmpty()) {
                    paramObj[ QStringLiteral("description") ] = param.description;
                }
                if (param.defaultValue.isValid()) {
                    paramObj[ QStringLiteral("default") ] = QJsonValue::fromVariant(param.defaultValue);
                }
                paramsArr.append(paramObj);
            }
        }
        json[ QStringLiteral("parameters") ] = paramsArr;
    }

    // 渲染模板（仅非默认值时写入）
    if (renderTemplate != RenderTemplate::NodeStyleTemplate) {
        json[ QStringLiteral("render_template") ] = enumToString(renderTemplate);
    }

    // 样式配置（稀疏策略，仅非默认字段）
    QJsonObject styleJson = DANodeStyleToJson(style);
    if (!styleJson.isEmpty()) {
        json[ QStringLiteral("style") ] = styleJson;
    }

    return json;
}

/**
 * @brief 从 JSON 反序列化
 *
 * 先构造默认实例，再覆盖 JSON 中存在的字段。
 * 缺失字段使用默认值，确保向前兼容。
 *
 * @param[in] json JSON 对象
 * @return 解析后的 DANodeDescriptor
 */
DANodeDescriptor DANodeDescriptor::fromJson(const QJsonObject& json)
{
    DANodeDescriptor desc;

    // 基础标识字段
    desc.name           = json.value(QStringLiteral("name")).toString();
    desc.qualifiedName = json.value(QStringLiteral("qualified_name")).toString();
    desc.category      = json.value(QStringLiteral("category")).toString();
    desc.icon          = json.value(QStringLiteral("icon")).toString();

    // 端口信息
    if (json.contains(QStringLiteral("inputs"))) {
        QJsonArray inputsArr = json.value(QStringLiteral("inputs")).toArray();
        for (const QJsonValue& val : inputsArr) {
            desc.inputs.append(DAPortDescriptor::fromJson(val.toObject()));
        }
    }
    if (json.contains(QStringLiteral("outputs"))) {
        QJsonArray outputsArr = json.value(QStringLiteral("outputs")).toArray();
        for (const QJsonValue& val : outputsArr) {
            desc.outputs.append(DAPortDescriptor::fromJson(val.toObject()));
        }
    }

    // 参数信息
    if (json.contains(QStringLiteral("parameters"))) {
        QJsonArray paramsArr = json.value(QStringLiteral("parameters")).toArray();
        desc.parameters = ParameterDescriptor::fromJsonArray(paramsArr);
    }

    // 渲染模板
    if (json.contains(QStringLiteral("render_template"))) {
        QString tmplStr = json.value(QStringLiteral("render_template")).toString();
        // 兼容旧值 "rect"/"svg" → NodeStyleTemplate
        if (tmplStr == "rect" || tmplStr == "svg") {
            tmplStr = "NodeStyleTemplate";
        }
        desc.renderTemplate = stringToEnum<RenderTemplate>(tmplStr, RenderTemplate::NodeStyleTemplate);
    }

    // 样式配置
    if (json.contains(QStringLiteral("style"))) {
        desc.style = DANodeStyleFromJson(json.value(QStringLiteral("style")).toObject());
    }

    return desc;
}

}  // namespace DA
```

**验证**: 编译通过

### 任务 1.3：pybind11 绑定 DANodeDescriptor 和 DAPortDescriptor

**文件**: `src/DAPyWorkFlow/PythonBinding/DAPyWorkFlowPythonBinding.cpp`（修改）

**操作**: 在 `PYBIND11_EMBEDDED_MODULE(da_py_workflow, m)` 块中，**在 DANodeStyle 绑定之后**（约第551行之后），添加以下绑定代码：

```cpp
// =================================================================================
//                      DAPortDescriptor 绑定
// =================================================================================

pybind11::class_<DA::DAPortDescriptor>(m, "DAPortDescriptor")
    .def(pybind11::init<>())
    .def(pybind11::init<const QString&, const QString&, bool, const QString&>(),
         pybind11::arg("name") = QString(),
         pybind11::arg("data_type") = QString(),
         pybind11::arg("required") = true,
         pybind11::arg("description") = QString())
    .def_readwrite("name", &DA::DAPortDescriptor::name)
    .def_readwrite("dataType", &DA::DAPortDescriptor::dataType)
    .def_readwrite("required", &DA::DAPortDescriptor::required)
    .def_readwrite("description", &DA::DAPortDescriptor::description)
    .def("isValid", &DA::DAPortDescriptor::isValid)
    .def("__repr__", [](const DA::DAPortDescriptor& d) {
        return QString("DAPortDescriptor(name=%1, dataType=%2, required=%3)")
            .arg(d.name).arg(d.dataType).arg(d.required).toStdString();
    });

// =================================================================================
//                      DANodeDescriptor 绑定
// =================================================================================

pybind11::class_<DA::DANodeDescriptor>(m, "DANodeDescriptor")
    .def(pybind11::init<>())
    // 基础标识字段
    .def_readwrite("name", &DA::DANodeDescriptor::name)
    .def_readwrite("qualifiedName", &DA::DANodeDescriptor::qualifiedName)
    .def_readwrite("category", &DA::DANodeDescriptor::category)
    .def_readwrite("icon", &DA::DANodeDescriptor::icon)
    // 端口信息
    .def_readwrite("inputs", &DA::DANodeDescriptor::inputs)
    .def_readwrite("outputs", &DA::DANodeDescriptor::outputs)
    // 参数信息（使用自定义 setter/getter 处理 defaultValue）
    .def_readwrite("parameters", &DA::DANodeDescriptor::parameters)
    // 渲染配置
    .def_readwrite("renderTemplate", &DA::DANodeDescriptor::renderTemplate)
    .def_readwrite("style", &DA::DANodeDescriptor::style)
    // 辅助方法
    .def("isValid", &DA::DANodeDescriptor::isValid)
    .def("toMetaData", &DA::DANodeDescriptor::toMetaData)
    .def("toJson", [](const DA::DANodeDescriptor& d) {
        return DA::PY::qjsonObjectToPyDict(d.toJson());
    })
    .def_static("fromJson", [](const pybind11::dict& d) {
        return DA::DANodeDescriptor::fromJson(DA::PY::pyDictToQJsonObject(d));
    }, pybind11::arg("dict"))
    .def("__repr__", [](const DA::DANodeDescriptor& d) {
        return QString("DANodeDescriptor(name=%1, qualifiedName=%2, inputs=%3, outputs=%4)")
            .arg(d.name).arg(d.qualifiedName).arg(d.inputs.size()).arg(d.outputs.size()).toStdString();
    });

// ParameterDescriptor 的 defaultValue 需要自定义 setter
// 因为 pybind11::def_readwrite 不支持 QVariant ← py::object 的自动转换
pybind11::class_<DA::ParameterDescriptor>(m, "DAParameterDescriptor")
    .def(pybind11::init<>())
    .def_readwrite("name", &DA::ParameterDescriptor::name)
    .def_readwrite("type", &DA::ParameterDescriptor::type)
    .def_readwrite("description", &DA::ParameterDescriptor::description)
    .def_readwrite("propertyId", &DA::ParameterDescriptor::propertyId)
    .def_readwrite("rawDescriptor", &DA::ParameterDescriptor::rawDescriptor)
    // defaultValue 的自定义 setter：将 Python 对象转换为 QVariant
    .def("setDefaultValue", [](DA::ParameterDescriptor& pd, pybind11::object val) {
        if (val.is_none()) {
            pd.defaultValue = QVariant();
        } else if (pybind11::isinstance<pybind11::str>(val)) {
            pd.defaultValue = QVariant(QString::fromStdString(pybind11::str(val)));
        } else if (pybind11::isinstance<pybind11::bool_>(val)) {
            pd.defaultValue = QVariant(pybind11::cast<bool>(val));
        } else if (pybind11::isinstance<pybind11::int_>(val)) {
            pd.defaultValue = QVariant(pybind11::cast<int>(val));
        } else if (pybind11::isinstance<pybind11::float_>(val)) {
            pd.defaultValue = QVariant(pybind11::cast<double>(val));
        } else if (pybind11::isinstance<pybind11::list>(val)) {
            // list 类型：通过 QJsonArray 中转
            QJsonArray arr;
            pybind11::list pyList = pybind11::cast<pybind11::list>(val);
            for (auto item : pyList) {
                // 简单处理：仅支持 str/int/float/bool 元素
                if (pybind11::isinstance<pybind11::str>(item)) {
                    arr.append(QString::fromStdString(pybind11::str(item)));
                } else if (pybind11::isinstance<pybind11::int_>(item)) {
                    arr.append(pybind11::cast<int>(item));
                } else if (pybind11::isinstance<pybind11::float_>(item)) {
                    arr.append(pybind11::cast<double>(item));
                } else if (pybind11::isinstance<pybind11::bool_>(item)) {
                    arr.append(pybind11::cast<bool>(item));
                }
            }
            pd.defaultValue = QVariant(arr);
        } else {
            // 其他类型：尝试通过 QJsonObject 中转
            // 将 Python 对象转为 QJsonValue，再转为 QVariant
            pd.defaultValue = QVariant();  // 无法转换的类型设为无效 QVariant
        }
    })
    .def("getDefaultValue", [](const DA::ParameterDescriptor& pd) -> pybind11::object {
        if (!pd.defaultValue.isValid()) {
            return pybind11::none();
        }
        // QVariant → Python 对象转换
        switch (pd.defaultValue.type()) {
        case QVariant::String:
            return pybind11::cast(pd.defaultValue.toString().toStdString());
        case QVariant::Bool:
            return pybind11::cast(pd.defaultValue.toBool());
        case QVariant::Int:
            return pybind11::cast(pd.defaultValue.toInt());
        case QVariant::Double:
            return pybind11::cast(pd.defaultValue.toDouble());
        default:
            // 复杂类型通过 rawDescriptor["default"] 回退读取
            if (pd.rawDescriptor.contains("default")) {
                QJsonValue jv = pd.rawDescriptor.value("default");
                return DA::PY::qjsonValueToPyObject(jv);
            }
            return pybind11::none();
        }
    })
    .def("hasField", &DA::ParameterDescriptor::hasField, pybind11::arg("fieldName"))
    .def("getField", [](const DA::ParameterDescriptor& pd, const std::string& fieldName) -> pybind11::object {
        QJsonValue jv = pd.getField(QString::fromStdString(fieldName));
        return DA::PY::qjsonValueToPyObject(jv);
    }, pybind11::arg("fieldName"))
    .def("__repr__", [](const DA::ParameterDescriptor& pd) {
        return QString("DAParameterDescriptor(name=%1, type=%2)").arg(pd.name).arg(pd.type).toStdString();
    });
```

**⚠️ 注意**: 
1. `QVector<DAPortDescriptor>` 的 `def_readwrite` 需要 pybind11 支持。如果编译失败，改用 `QList<DAPortDescriptor>` 并在 DANodeDescriptor 定义中同步修改。
2. `DA::PY::qjsonValueToPyObject` 可能不存在，需要确认 `DAPyJsonCast.h` 中是否有此函数。如果没有，需新增。
3. `ParameterDescriptor` 使用了 `QVariant::type()`，Qt6 中此方法已被废弃，需使用 `QMetaType` 或添加 Qt5/Qt6 兼容宏。

**验证**: Python 侧能创建 `da_py_workflow.DANodeDescriptor()` 并设置所有字段

### 任务 1.4：在 DAPyWorkFlowPythonBinding.h 中添加 include

**文件**: `src/DAPyWorkFlow/PythonBinding/DAPyWorkFlowPythonBinding.h`（修改）

**操作**: 在已有 include 列表中添加:
```cpp
#include "DAPyWorkFlow/DANodeDescriptor.h"
#include "DAPyWorkFlow/DAPortDescriptor.h"
```

**验证**: 编译通过

### 任务 1.5：确认 DAPyJsonCast 辅助函数

**操作**: 检查 `src/DAPyBindQt/DAPyJsonCast.h` 和 `.cpp`，确认以下函数存在或需要新增：
- `qjsonValueToPyObject(const QJsonValue&)` → 返回 `pybind11::object`：如果不存在则新增

**验证**: 编译通过

### 阶段 1 完成标志

- Python 侧 `import da_py_workflow` 后能创建 `DANodeDescriptor()`、`DAPortDescriptor()`、`DAParameterDescriptor()`
- 所有字段读写正常
- `toJson()`/`fromJson()` 正确
- `toMetaData()` 正确返回 `DAPyNodeMetaData`

---

## 阶段 2：Python 侧替换 dict 为结构体（单轨）

### 任务 2.1：修改 types.py — 新增结构体转换方法

**文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/types.py`（修改）

**操作**: 为 `Input`、`Output`、`Parameter` 类各新增一个方法，返回对应的 C++ 结构体：

**Input 类** — 新增 `to_port_descriptor()` 方法:
```python
def to_port_descriptor(self, name: str) -> "da_py_workflow.DAPortDescriptor":
    """将 Input 声明转换为 C++ DAPortDescriptor 结构体"""
    import da_py_workflow
    desc = da_py_workflow.DAPortDescriptor()
    desc.name = name
    desc.dataType = self.data_type
    desc.required = self.required
    desc.description = self.description
    return desc
```

**Output 类** — 新增 `to_port_descriptor()` 方法:
```python
def to_port_descriptor(self, name: str) -> "da_py_workflow.DAPortDescriptor":
    """将 Output 声明转换为 C++ DAPortDescriptor 结构体"""
    import da_py_workflow
    desc = da_py_workflow.DAPortDescriptor()
    desc.name = name
    desc.dataType = self.data_type
    desc.description = self.description
    return desc
```

**Parameter 类** — 新增 `to_parameter_descriptor()` 方法:
```python
def to_parameter_descriptor(self, name: str) -> "da_py_workflow.DAParameterDescriptor":
    """将 Parameter 声明转换为 C++ DAParameterDescriptor 结构体"""
    import da_py_workflow
    pd = da_py_workflow.DAParameterDescriptor()
    pd.name = name
    # 将 Python 类型转为字符串标签
    type_map = {str: "str", int: "int", float: "float", bool: "bool", list: "list", dict: "dict"}
    pd.type = type_map.get(self.param_type, "str")
    pd.description = self.description
    # 设置默认值（通过自定义 setter 处理 QVariant 转换）
    if self.default is not None:
        pd.setDefaultValue(self.default)
    # 保留原始 dict 作为 rawDescriptor，供 C++ 端扩展字段访问
    pd.rawDescriptor = self.to_dict(name)
    return pd
```

**注意**: `Parameter.to_parameter_descriptor()` 中 `pd.rawDescriptor = self.to_dict(name)` 需要确认 pybind11 是否支持 `QJsonObject` 的直接赋值。如果不支持，需要通过 `DA::PY::pyDictToQJsonObject()` 转换后赋值，或者使用一个 helper 方法。

**验证**: Python 侧能调用 `Input("DataFrame").to_port_descriptor("data")` 返回 `DAPortDescriptor`

### 任务 2.2：修改 node_def.py — @NodeDef 直接创建 DANodeDescriptor 结构体

**文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_def.py`（修改）

**操作**: 替换整个 `NodeDef` 函数实现。核心变化：不再构建 dict，直接创建 `da_py_workflow.DANodeDescriptor` 结构体。

```python
"""
工作流节点定义装饰器模块

本模块定义了 NodeDef 装饰器，用于声明工作流节点类型。
NodeDef 装饰器会收集类中的 Input、Output、Parameter 声明，
并自动生成 C++ 导出的 DANodeDescriptor 结构体，供 C++ 侧直接读取。

使用示例::

    @NodeDef(name="Data Filter", category="Data Processing")
    class DataFilter:
        column = Parameter(str, default="value", description="列名")

        class Inputs:
            data = Input("DataFrame", required=True)

        class Outputs:
            filtered = Output("DataFrame")

        def execute(self, inputs, params):
            ...
"""

from .types import Input, Output, Parameter


def _normalize_render_template(render_template: str) -> str:
    """
    规范化渲染模板值，将旧的 rect/svg 映射到 nodestyle。

    :param render_template: 原始渲染模板字符串
    :return: 规范化后的模板字符串（'nodestyle' 或 'widget'）
    """
    if render_template in ("rect", "svg"):
        return "nodestyle"
    if render_template == "widget":
        return "widget"
    return "nodestyle"


def _collect_parameters_struct(cls: type) -> list:
    """
    从类属性中收集 Parameter 声明并转换为 DAParameterDescriptor 结构体

    :param cls: 被装饰的节点类
    :return: DAParameterDescriptor 列表
    """
    import da_py_workflow
    params = []
    for attr_name in dir(cls):
        if attr_name.startswith("_"):
            continue
        attr_value = getattr(cls, attr_name, None)
        if isinstance(attr_value, Parameter):
            params.append(attr_value.to_parameter_descriptor(attr_name))
    return params


def _collect_from_nested_class_struct(cls: type, nested_name: str, decl_type: type) -> list:
    """
    从嵌套类中收集 Input 或 Output 声明并转换为 DAPortDescriptor 结构体

    :param cls: 被装饰的节点类
    :param nested_name: 嵌套类名（"Inputs" 或 "Outputs"）
    :param decl_type: 声明类型（Input 或 Output）
    :return: DAPortDescriptor 列表
    """
    import da_py_workflow
    items = []
    nested_cls = getattr(cls, nested_name, None)
    if nested_cls is None:
        return items
    for attr_name in dir(nested_cls):
        if attr_name.startswith("_"):
            continue
        attr_value = getattr(nested_cls, attr_name, None)
        if isinstance(attr_value, decl_type):
            items.append(attr_value.to_port_descriptor(attr_name))
    return items


def NodeDef(name: str, category: str = "", icon: str = "", render_template: str = "rect", style=None):
    """
    工作流节点定义装饰器

    此装饰器用于声明工作流节点类型。它会收集被装饰类中的 Input、Output、Parameter
    声明，并自动在类上设置 _node_descriptor 属性为 C++ 导出的 DANodeDescriptor 结构体。

    结构体包含以下字段：
    - name: 节点显示名称
    - qualifiedName: 节点的唯一标识（模块名.类名）
    - category: 节点所属分类
    - icon: 节点图标标识
    - inputs: 输入端口列表（DAPortDescriptor 列表）
    - outputs: 输出端口列表（DAPortDescriptor 列表）
    - parameters: 参数列表（DAParameterDescriptor 列表）
    - renderTemplate: 渲染模板类型
    - style: 节点样式配置（DANodeStyle）

    :param name: 节点显示名称
    :param category: 节点所属分类，默认为空字符串
    :param icon: 节点图标标识，默认为空字符串
    :param render_template: 渲染模板类型，默认为 'rect'（映射到 NodeStyleTemplate）
    :param style: 节点样式配置，可为 DANodeStyle 实例，默认为 None
    :return: 装饰器函数
    """
    import da_py_workflow
    normalized_template = _normalize_render_template(render_template)
    # 将字符串映射为 C++ 枚举值
    template_enum = (
        da_py_workflow.RenderTemplate.NodeStyleTemplate
        if normalized_template == "nodestyle"
        else da_py_workflow.RenderTemplate.WidgetTemplate
    )

    def decorator(cls: type) -> type:
        # 收集参数声明（返回 DAParameterDescriptor 列表）
        parameters = _collect_parameters_struct(cls)

        # 收集输入端口声明（返回 DAPortDescriptor 列表）
        inputs = _collect_from_nested_class_struct(cls, "Inputs", Input)

        # 收集输出端口声明（返回 DAPortDescriptor 列表）
        outputs = _collect_from_nested_class_struct(cls, "Outputs", Output)

        # 生成唯一标识：模块名.类限定名
        qualified_name = f"{cls.__module__}.{cls.__qualname__}"

        # 直接创建 C++ 导出的 DANodeDescriptor 结构体
        descriptor = da_py_workflow.DANodeDescriptor()
        descriptor.name = name
        descriptor.category = category
        descriptor.icon = icon
        descriptor.qualifiedName = qualified_name
        descriptor.inputs = inputs
        descriptor.outputs = outputs
        descriptor.parameters = parameters
        descriptor.renderTemplate = template_enum

        # 处理样式参数
        if style is not None:
            descriptor.style = style

        # 在类上设置 _node_descriptor 属性（结构体而非 dict）
        cls._node_descriptor = descriptor

        # 设置 input_keys / output_keys 为类属性（兼容 C++ syncMetaFromPyNode）
        cls.input_keys = [inp.name for inp in inputs]
        cls.output_keys = [outp.name for outp in outputs]

        @classmethod
        def _get_descriptor(cls):
            return getattr(cls, "_node_descriptor", None)

        cls.get_descriptor = _get_descriptor

        return cls

    return decorator
```

**验证**: 现有节点代码无需修改仍能正常运行

### 任务 2.3：废弃 node_descriptor.py

**文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_descriptor.py`（修改）

**操作**: 将文件内容改为废弃警告：

```python
"""
[废弃] 工作流节点描述符模块

此模块已废弃。DANodeDescriptor 类已被 C++ 导出的 da_py_workflow.DANodeDescriptor 结构体替代。
Python 侧节点描述信息现在由 @NodeDef 装饰器直接创建 C++ 结构体实例，
不再使用 Python dict 作为中间载体。

如需调试输出，可使用 da_py_workflow.DANodeDescriptor.toJson() 获取 dict 格式。
"""

import warnings

warnings.warn(
    "DAWorkbench.DAWorkFlowPy.node_descriptor 模块已废弃，"
    "请使用 da_py_workflow.DANodeDescriptor 替代。",
    DeprecationWarning,
    stacklevel=2,
)
```

**验证**: 现有 `from DAWorkbench.DAWorkFlowPy import DANodeDescriptor` 的代码会收到废弃警告但仍能运行（向下兼容）

### 任务 2.4：修改 __init__.py 导出

**文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/__init__.py`（修改）

**操作**: 
1. 保留 `DANodeDescriptor` 的导出但改为从 `da_py_workflow` 引入：
   ```python
   # DANodeDescriptor 现由 C++ 导出
   try:
       import da_py_workflow
       DANodeDescriptor = da_py_workflow.DANodeDescriptor
   except ImportError:
       # fallback：如果 da_py_workflow 不可用（纯 Python 测试环境）
       from .node_descriptor import DANodeDescriptor
   ```
2. 添加 `DAPortDescriptor` 和 `DAParameterDescriptor` 导出：
   ```python
   try:
       import da_py_workflow
       DAPortDescriptor = da_py_workflow.DAPortDescriptor
       DAParameterDescriptor = da_py_workflow.DAParameterDescriptor
   except ImportError:
       pass
   ```

**验证**: `from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter, DANodeDescriptor` 正常工作

### 任务 2.5：修改 node_registry.py — discover 返回结构体

**文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_registry.py`（修改）

**操作**: 修改 `discover()` 方法，使其返回 `_node_descriptor` 结构体（已经是 `da_py_workflow.DANodeDescriptor` 实例）而非调用 `.to_dict()`。

当前代码中，`discover()` 遍历发现的节点类，提取 `_node_descriptor` 属性。由于 `@NodeDef` 现在直接设置 `_node_descriptor` 为 `DANodeDescriptor` 结构体，`discover()` 应直接返回这些结构体实例。

**具体修改**: 在 `node_registry.py` 中找到提取描述符的逻辑，修改为：
```python
# 旧代码：可能调用了 .to_dict()
# 新代码：直接返回 _node_descriptor 属性（已经是 DANodeDescriptor 结构体）
descriptor = getattr(node_class, "_node_descriptor", None)
```

**验证**: `DAPyNodeFactory::discoverNodes()` 能正确从结构体提取元数据

### 阶段 2 完成标志

- 所有现有节点（DataAnalysis, CrewAIAdapter, style_demo_nodes）能正确发现和创建
- `DAPyNodeFactory::discoverNodes()` 返回的 `DAPyNodeMetaData` 列表与之前完全一致
- 无功能退化

---

## 阶段 3：C++ 侧统一数据路径

### 任务 3.1：重写 DAPyNodeProxy::syncMetaFromPyNode — 一次性读取结构体

**文件**: `src/DAPyWorkFlow/DAPyNodeProxy.cpp`（修改）

**操作**: 替换 `syncMetaFromPyNode()` 方法。新实现从 `pyNode.attr("_node_descriptor")` 一次性 cast 为 `DA::DANodeDescriptor` 结构体，存储到 PrivateData 的 `mDescriptor` 字段中。

```cpp
/**
 * @brief 从Python节点同步元信息到本地缓存
 *
 * 在setPyNodeRef时调用，从Python节点的 _node_descriptor 属性一次性读取
 * DANodeDescriptor 结构体，缓存到 C++ 本地变量。
 * 替代原来逐属性 hasattr/cast 的 7 次循环方式，
 * 改为 pybind11::cast<DANodeDescriptor> 一次读取，消除 JSON 中间转换。
 *
 * @param[in] pyNode Python节点实例对象
 * @note 需在GIL保护下调用此函数
 */
void DAPyNodeProxy::PrivateData::syncMetaFromPyNode(const pybind11::object& pyNode)
{
    try {
        // 一次性读取整个 descriptor 结构体
        if (pybind11::hasattr(pyNode, "_node_descriptor")) {
            pybind11::object descObj = pyNode.attr("_node_descriptor");
            // pybind11::cast 将 Python 侧的 DANodeDescriptor 直接转为 C++ 结构体
            mDescriptor = pybind11::cast<DA::DANodeDescriptor>(descObj);
        }
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
}
```

**⚠️ 注意**: `pybind11::cast<DA::DANodeDescriptor>` 要求 Python 侧的对象确实是 `da_py_workflow.DANodeDescriptor` 类型。如果 Python 侧 `_node_descriptor` 仍然是 dict（旧插件），cast 会失败。需要添加 fallback：先检查是否为 dict，如果是 dict 则走 `DANodeDescriptor::fromJson(DA::PY::pyDictToQJsonObject(...))` 旧路径。

**完整实现**（含 fallback）:
```cpp
void DAPyNodeProxy::PrivateData::syncMetaFromPyNode(const pybind11::object& pyNode)
{
    try {
        if (pybind11::hasattr(pyNode, "_node_descriptor")) {
            pybind11::object descObj = pyNode.attr("_node_descriptor");
            // 优先：结构体路径（高性能）
            try {
                mDescriptor = pybind11::cast<DA::DANodeDescriptor>(descObj);
                return;  // 成功则直接返回
            } catch (...) {
                // cast 失败（旧插件可能仍使用 dict），fallback 到 JSON 路径
            }
            // Fallback：dict → QJsonObject → DANodeDescriptor::fromJson
            if (pybind11::isinstance<pybind11::dict>(descObj)) {
                pybind11::dict descDict = pybind11::cast<pybind11::dict>(descObj);
                QJsonObject json = DA::PY::pyDictToQJsonObject(descDict);
                mDescriptor = DA::DANodeDescriptor::fromJson(json);
            }
        }
    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
}
```

**验证**: 新插件（使用结构体）和旧插件（仍使用 dict）都能正确同步

### 任务 3.2：重构 DAPyNodeProxy::PrivateData — 聚合为 mDescriptor

**文件**: `src/DAPyWorkFlow/DAPyNodeProxy.cpp`（修改 PrivateData 定义）

**操作**: 在 `PrivateData` 类中，将散落的字段替换为 `DANodeDescriptor mDescriptor`：

```cpp
class DAPyNodeProxy::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyNodeProxy)
public:
    PrivateData(DAPyNodeProxy* p);
    void dealException(const std::exception& e) const;
    void clearPyNodeRef();

    // 从Python节点同步元信息到本地缓存
    void syncMetaFromPyNode(const pybind11::object& pyNode);

public:
    DANodeDescriptor mDescriptor;                           ///< 节点完整描述符（统一数据源）
    unsigned int mId { 0 };                              ///< 节点ID（独立管理）
    DAPySafePyObjectHolder mPyNodeRef;                     ///< Python节点实例的安全持有者
    DAPyNodeState mNodeState { DAPyNodeState::Idle };     ///< 节点执行状态
    mutable QString mLastErrorString;                        ///< 最后一次错误信息
};
```

**删除的字段**: `mQualifiedName`, `mNodeName`, `mInputKeys`, `mOutputKeys`, `mNodePrototype`, `mNodeGroup`, `mNodeStyle` — 这些现在从 `mDescriptor` 读取。

**验证**: 编译通过

### 任务 3.3：修改 DAPyNodeProxy 的 getter 方法 — 从 mDescriptor 读取

**文件**: `src/DAPyWorkFlow/DAPyNodeProxy.cpp`（修改）

**操作**: 修改所有从 PrivateData 旧字段读取的 getter，改为从 `mDescriptor` 读取：

```cpp
QString DAPyNodeProxy::getQualifiedName() const
{
    DA_DC(d);
    return d->mDescriptor.qualifiedName;
}

QString DAPyNodeProxy::getNodeName() const
{
    DA_DC(d);
    return d->mDescriptor.name;
}

QList<QString> DAPyNodeProxy::getInputKeys() const
{
    DA_DC(d);
    QList<QString> keys;
    for (const DAPortDescriptor& port : d->mDescriptor.inputs) {
        keys.append(port.name);
    }
    return keys;
}

QList<QString> DAPyNodeProxy::getOutputKeys() const
{
    DA_DC(d);
    QList<QString> keys;
    for (const DAPortDescriptor& port : d->mDescriptor.outputs) {
        keys.append(port.name);
    }
    return keys;
}

QString DAPyNodeProxy::getNodePrototype() const
{
    DA_DC(d);
    return d->mDescriptor.qualifiedName;
}

QString DAPyNodeProxy::getNodeGroup() const
{
    DA_DC(d);
    return d->mDescriptor.category;
}

DANodeStyle DAPyNodeProxy::getNodeStyle() const
{
    DA_DC(d);
    return d->mDescriptor.style;
}
```

**验证**: 编译通过，getter 方法返回值与之前完全一致

### 任务 3.4：新增 getDescriptorStruct() 方法，标记旧 getDescriptor() 为 deprecated

**文件**: `src/DAPyWorkFlow/DAPyNodeProxy.h`（修改）

**操作**: 在 `DAPyNodeProxy` 类中新增方法：

```cpp
    // 节点描述符结构体（新路径，高性能）
    const DANodeDescriptor& getDescriptorStruct() const;

    // 节点描述符（旧路径，返回 QJsonObject — 已废弃，保留一个版本周期）
    QJsonObject getDescriptor() const;  // TODO: deprecated, 将在下一版本移除
```

**文件**: `src/DAPyWorkFlow/DAPyNodeProxy.cpp`（修改）

**新增实现**:
```cpp
/**
 * @brief 获取节点描述符结构体
 *
 * 返回缓存的 DANodeDescriptor 结构体引用，无 GIL 获取和 JSON 转换开销。
 * 此方法替代旧的 getDescriptor()（返回 QJsonObject）。
 *
 * @return DANodeDescriptor 结构体的 const 引用
 */
const DANodeDescriptor& DAPyNodeProxy::getDescriptorStruct() const
{
    DA_DC(d);
    return d->mDescriptor;
}

/**
 * @brief 获取Python节点描述符（已废弃）
 *
 * @deprecated 请使用 getDescriptorStruct() 替代。此方法将在下一版本移除。
 * @return QJsonObject 格式的节点描述符
 */
QJsonObject DAPyNodeProxy::getDescriptor() const
{
    DA_DC(d);
    // 从结构体序列化为 JSON（向后兼容）
    return d->mDescriptor.toJson();
}
```

**验证**: 编译通过

### 任务 3.5：简化 DAPyNodeFactory::convertDescriptorToMetaData

**文件**: `src/DAPyWorkFlow/DAPyNodeFactory.cpp`（修改）

**操作**: 修改 `discoverNodes()` 方法中处理描述符的逻辑。当前代码通过 `descObj.attr("to_dict")()` 获取 dict → `convertDescriptorToMetaData(descDict)` 转换。新代码应直接 cast 为 `DANodeDescriptor` 结构体 → `toMetaData()` 提取元数据。

**修改 discoverNodes() 中的遍历逻辑**:

```cpp
// 5. 遍历返回的描述符对象列表
QList<DAPyNodeMetaData> discoveredList;
for (pybind11::handle item : result) {
    pybind11::object descriptorObj = pybind11::reinterpret_borrow<pybind11::object>(item);
    // 优先：直接 cast 为 DANodeDescriptor 结构体
    DANodeDescriptor descStruct;
    try {
        descStruct = pybind11::cast<DA::DANodeDescriptor>(descriptorObj);
    } catch (...) {
        // Fallback：dict → fromJson
        if (pybind11::isinstance<pybind11::dict>(descriptorObj)) {
            pybind11::dict descDict = descriptorObj.cast<pybind11::dict>();
            QJsonObject json = DA::PY::pyDictToQJsonObject(descDict);
            descStruct = DA::DANodeDescriptor::fromJson(json);
        } else {
            qWarning() << "发现无法解析的描述符对象，跳过";
            continue;
        }
    }
    DAPyNodeMetaData metaData = descStruct.toMetaData();
    if (!metaData.isValid()) {
        qWarning() << "发现无效的节点元数据，跳过";
        continue;
    }
    discoveredList.append(metaData);
}
```

**删除**: `convertDescriptorToMetaData()` 静态函数不再需要，可删除。

**验证**: 节点发现功能与之前完全一致

### 任务 3.6：新增 DAPyWorkFlowScene::createPyNode(DANodeDescriptor) 路径

**文件**: `src/DAPyWorkFlow/DAPyWorkFlowScene.h`（修改）

**操作**: 在 `createPyNode` 方法组中新增：
```cpp
    // 通过完整描述符创建Python节点图形项（不添加到场景）
    DAPyNodeGraphicsItem* createPyNode(const DANodeDescriptor& descriptor, const QPointF& pos);
    // 通过完整描述符创建Python节点图形项（带undo/redo）
    DAPyNodeGraphicsItem* createPyNode_(const DANodeDescriptor& descriptor, const QPointF& pos);
```

**文件**: `src/DAPyWorkFlow/DAPyWorkFlowScene.cpp`（修改）

**新增实现**: 参考现有 `createPyNode(DAPyNodeMetaData)` 方法，但不再需要从 JSON 提取字段：

```cpp
DAPyNodeGraphicsItem* DAPyWorkFlowScene::createPyNode(const DANodeDescriptor& descriptor, const QPointF& pos)
{
    DA_D(d);
    if (!d->mPyWorkflow) {
        qWarning() << tr("DAPyWorkFlowScene::createPyNode: Python workflow is not set");
        return nullptr;
    }
    if (!descriptor.isValid()) {
        qWarning() << tr("DAPyWorkFlowScene::createPyNode: descriptor invalid (qualifiedName: %1)")
                      .arg(descriptor.qualifiedName);
        return nullptr;
    }

    // 通过工厂创建代理
    DAPyNodeProxy* proxy = nullptr;
    if (d->mPyNodeFactory) {
        proxy = d->mPyNodeFactory->createNodeProxy(descriptor.qualifiedName);
        if (!proxy) {
            qWarning() << tr("DAPyWorkFlowScene::createPyNode: factory failed for %1").arg(descriptor.qualifiedName);
            return nullptr;
        }
    } else {
        proxy = new DAPyNodeProxy();
        proxy->setQualifiedName(descriptor.qualifiedName);
    }

    // 在 Python 侧注册节点到 DAWorkflow
    // ...（与现有 createPyNode(DAPyNodeMetaData) 相同的 GIL 逻辑）

    // 创建图形项 — 不再调用 setDescriptor(QJsonObject)
    DAPyNodeGraphicsItem* item = new DAPyNodeGraphicsItem(proxy);

    // 从结构体设置显示属性（无需 JSON 中间转换）
    if (!descriptor.name.isEmpty()) {
        item->setNodeName(descriptor.name);
    }
    item->setNodeStyle(descriptor.style);
    item->setRenderTemplate(descriptor.renderTemplate);
    if (!descriptor.icon.isEmpty()) {
        item->setIcon(QIcon(descriptor.icon));
    }
    item->updateLinkPoints();
    item->updateNodeBody();
    item->setPos(pos);

    return item;
}
```

**验证**: 编译通过

### 任务 3.7：新增 DAPyNodeGraphicsItem::setDescriptorStruct() 方法

**文件**: `src/DAPyWorkFlow/DAPyNodeGraphicsItem.h`（修改）

**操作**: 新增方法：
```cpp
    // 节点描述符结构体（新路径）
    void setDescriptorStruct(const DANodeDescriptor& desc);
    DANodeDescriptor getDescriptorStruct() const;
```

**文件**: `src/DAPyWorkFlow/DAPyNodeGraphicsItem.cpp`（修改）

**新增实现**: `setDescriptorStruct()` 从结构体更新连接点、名称等，替代 `setDescriptor(QJsonObject)` 的逻辑：
```cpp
void DAPyNodeGraphicsItem::setDescriptorStruct(const DANodeDescriptor& desc)
{
    DA_D(d);
    d->mDescriptor = desc.toJson();  // 保留 JSON 用于 saveToXml 向后兼容
    // 从结构体直接生成连接点（无需 JSON 解析）
    // ...更新 inputLinkPoints/outputLinkPoints
    updateLinkPoints();
}
```

**验证**: 编译通过

### 任务 3.8：标记 QJsonObject descriptor 路径为 deprecated

**文件**: `src/DAPyWorkFlow/DAPyWorkFlowScene.h`（修改）

**操作**: 在 `createPyNode(const QJsonObject&, const QPointF&)` 方法声明旁添加注释：
```cpp
    // 通过QJsonObject创建Python节点（已废弃，请使用 createPyNode(DANodeDescriptor) 替代）
    DAPyNodeGraphicsItem* createPyNode(const QJsonObject& descriptor, const QPointF& pos);  // TODO: deprecated
    DAPyNodeGraphicsItem* createPyNode_(const QJsonObject& descriptor, const QPointF& pos);  // TODO: deprecated
```

**验证**: 编译通过

### 任务 3.9：更新 DANodeParamSettingPanel — 直接使用结构体参数

**文件**: `src/DAGui/NodeSetting/DANodeParamSettingPanel.cpp`（修改）

**操作**: 在 `buildPropertyPanel()` 中，当前代码通过 `DAAbstractNodeSettingWidget::getParameters()` 获取 `QJsonArray`，然后调用 `ParameterDescriptor::fromJsonArray()`。修改为：如果 proxy 有 `getDescriptorStruct()` 方法，直接从 `mDescriptor.parameters` 获取 `QVector<ParameterDescriptor>`。

```cpp
// 旧代码路径：
// QJsonArray params = getParameters();  // 从 getDescriptor()["parameters"] 提取
// QVector<ParameterDescriptor> paramDescs = ParameterDescriptor::fromJsonArray(params);

// 新代码路径：
QVector<ParameterDescriptor> paramDescs;
if (mNodeProxy) {
    paramDescs = mNodeProxy->getDescriptorStruct().parameters;
} else {
    // Fallback: 从 JSON 解析（兼容旧节点）
    QJsonArray params = getParameters();
    paramDescs = ParameterDescriptor::fromJsonArray(params);
}
```

**验证**: 参数面板功能与之前完全一致

### 阶段 3 完成标志

- 所有节点发现、创建、执行功能正常
- 参数面板正常渲染和更新
- 保存/加载工程文件正常（内部使用 `DANodeDescriptor.toJson()` 序列化）
- 新插件使用结构体路径，旧插件 fallback 到 JSON 路径均正常

---

## 阶段 4：清理与文档更新

### 任务 4.1：删除 node_descriptor.py

**文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_descriptor.py`（删除）

**操作**: 此文件已在任务 2.3 中标记为废弃。在确认所有依赖已迁移后，完全删除此文件。

**前置条件**: 确认没有任何代码仍引用 `from .node_descriptor import DANodeDescriptor`

**验证**: 编译通过，运行正常

### 任务 4.2：从 __init__.py 移除 DANodeDescriptor fallback

**文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/__init__.py`（修改）

**操作**: 移除 `DANodeDescriptor` 的 `try/except ImportError` fallback：
```python
# 直接从 C++ 模块导出
import da_py_workflow
DANodeDescriptor = da_py_workflow.DANodeDescriptor
DAPortDescriptor = da_py_workflow.DAPortDescriptor
DAParameterDescriptor = da_py_workflow.DAParameterDescriptor
```

**验证**: 编译通过

### 任务 4.3：移除 DAPyNodeProxy 中废弃的 QJsonObject 方法

**文件**: `src/DAPyWorkFlow/DAPyNodeProxy.h`（修改）
**文件**: `src/DAPyWorkFlow/DAPyNodeProxy.cpp`（修改）

**操作**: 删除 `QJsonObject getDescriptor() const` 方法

**前置条件**: 确认所有消费方已迁移到 `getDescriptorStruct()`

**验证**: 编译通过

### 任务 4.4：移除 DAPyWorkFlowScene 中废弃的 QJsonObject createPyNode

**文件**: `src/DAPyWorkFlow/DAPyWorkFlowScene.h`（修改）
**文件**: `src/DAPyWorkFlow/DAPyWorkFlowScene.cpp`（修改）

**操作**: 删除 `createPyNode(const QJsonObject&, const QPointF&)` 和 `createPyNode_(const QJsonObject&, const QPointF&)`

**前置条件**: 确认 `DAPyWorkFlowGraphicsView` 已不再使用 QJsonObject 创建路径

**验证**: 编译通过

### 任务 4.5：移除 DAPyNodeGraphicsItem 中废弃的 QJsonObject setDescriptor

**文件**: `src/DAPyWorkFlow/DAPyNodeGraphicsItem.h`（修改）
**文件**: `src/DAPyWorkFlow/DAPyNodeGraphicsItem.cpp`（修改）

**操作**: 删除 `void setDescriptor(const QJsonObject& desc)` 和 `QJsonObject getDescriptor() const` 方法

**前置条件**: 确认所有使用 `setDescriptor(QJsonObject)` 的代码已迁移

**验证**: 编译通过

### 任务 4.6：移除 DAPyNodeFactory 中废弃的 convertDescriptorToMetaData

**文件**: `src/DAPyWorkFlow/DAPyNodeFactory.cpp`（修改）

**操作**: 删除 `static DAPyNodeMetaData convertDescriptorToMetaData(const pybind11::dict& descDict)` 函数

**前置条件**: 确认 `discoverNodes()` 不再使用此函数

**验证**: 编译通过

### 任务 4.7：更新文档

**文件**: `skills/develop-python-plugin/SKILL_cn.md`（修改）

**操作**: 
1. 将 `_node_descriptor` 字典描述改为 `DANodeDescriptor` 结构体描述
2. 将 `@NodeDef 装饰器参数说明` 中的 "自动生成 `_node_descriptor` 字典" 改为 "自动生成 `_node_descriptor` 结构体"
3. 更新代码示例，添加 `import da_py_workflow` 的 DANodeDescriptor 使用说明

**文件**: `docs/zh/dev-guide/workflow-python-node-dev.md`（修改）

**操作**: 
1. 将 `_node_descriptor` 字典格式说明改为 DANodeDescriptor 结构体字段说明
2. 更新节点定义示例代码

**文件**: `docs/zh/dev-guide/node-rendering-settings.md`（修改）

**操作**: 确认文档中关于 DANodeStyle 的描述与结构体方案一致（无需大幅修改，DANodeStyle 已使用结构体）

**验证**: 文档内容准确

### 任务 4.8：删除 DAGui/NodeSetting/ParameterDescriptor.h 转发头文件

**文件**: `src/DAGui/NodeSetting/ParameterDescriptor.h`（删除）

**操作**: 在确认所有代码已改为 `#include "DAUtils/ParameterDescriptor.h"` 后，删除此转发头文件

**前置条件**: 确认 DAGui 中无任何文件仍引用 `#include "ParameterDescriptor.h"`（不含路径前缀）

**验证**: 全量编译通过

### 阶段 4 完成标志

- 所有废弃代码已清理
- 文档已更新
- 全量编译通过
- 所有测试通过

---

## 全局验证清单

每个阶段完成后，执行以下验证：

### 编译验证

```powershell
.\scripts\build.ps1 -Full
```

### 功能验证（手动或自动化）

1. **节点发现**: 启动程序，确认节点列表面板显示所有已知节点
2. **节点创建**: 拖拽节点到场景，确认节点正常渲染
3. **参数面板**: 双击节点，确认参数面板正确显示所有参数编辑器
4. **节点执行**: 运行工作流，确认节点正常执行
5. **保存/加载**: 保存工程文件，重新加载，确认节点配置恢复正确
6. **旧插件兼容**: 如果有旧插件仍使用 dict 格式，确认 fallback 正常

---

## 风险与注意事项

| 风险 | 说明 | 缓解措施 |
|------|------|----------|
| `QVector<T>` pybind11 绑定 | pybind11 可能不直接支持 `QVector<DAPortDescriptor>` 的 `def_readwrite` | 如果编译失败，改用 `QList<DAPortDescriptor>`，同步修改 DANodeDescriptor 定义 |
| `QVariant::type()` Qt6 废弃 | ParameterDescriptor.defaultValue 使用 QVariant::type() 判断类型 | Qt6 中改用 `defaultValue.metaType().id()` 或添加 `Qt5Qt6Compat_*` 宏 |
| `pybind11::cast<DANodeDescriptor>` 类型检查 | Python 侧如果是 dict（旧插件）而非 DANodeDescriptor 结构体，cast 会抛异常 | 在 syncMetaFromPyNode 中添加 try/catch fallback，见任务 3.1 |
| `DAUtilsAPI.h` 导出宏 | ParameterDescriptor 移到 DAUtils 后需使用 DAUtils 的导出宏 | 确认 `DAUTILS_API` 宏名称（查看 `src/DAUtils/DAUtilsAPI.h`） |
| 旧插件 `_node_descriptor` 仍为 dict | 外部 pyplugins 目录的插件可能未更新 | DAPyNodeFactory::discoverNodes 和 syncMetaFromPyNode 都有 dict fallback |

---

## 不在本次重构范围内的内容

以下内容虽与本重构相关，但不纳入本次执行范围：

1. **DAPyNodeMetaData 与 DANodeDescriptor 合并**: 两者职责不同，保持独立
2. **DAPythonSignalHandler 传递 qualified_name**: `AgentNode._push_state()` 中使用 `_node_descriptor["qualified_name"]` — 这在阶段 2 完成后 `_node_descriptor` 变为结构体，Python 侧代码需改为 `_node_descriptor.qualifiedName`。但这属于 Python 侧节点的自行修改，不在框架重构范围
3. **DAParamTypeRegistry 改为接受 ParameterDescriptor 而非 QJsonObject**: 当前 DAParamTypeRegistry.createEditor 接受 `QJsonObject paramDescriptor`。长期可改为接受 `ParameterDescriptor`，但本次仅确保 ParameterDescriptor.rawDescriptor 提供 QJsonObject fallback