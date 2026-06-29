# Unified Property Form Design

**Date:** 2026-06-27

## Goal

用一套全新的属性表单内核替换项目内并行存在的两套属性窗口体系，满足以下目标：

- 彻底移除 `DACommonPropertySettingDialog` 与 `QtPropertyBrowser` 依赖
- 统一节点参数设置与通用 C++/Python 配置窗口的渲染内核
- 将当前节点专用的 `DAParamDef` 抽离为通用表单定义层，并去除其对 `DAPyNodeParameter` 的直接依赖
- 提供全新的 Python v2 类型化 API，替代 `property_config_builder.py`
- 首版仅支持声明式联动，不支持 Python 回调钩子

## Decisions

- 采用方案 A：公共 `FormSpec` 内核 + 公共渲染层 + 节点适配器
- 最终交付不保留 v1 builder、旧对话框、deprecated 标记或兼容入口
- Python v2 采用“类型化对象 + `dict`/JSON 桥接”的方式
- 本轮不补自动化测试，验收以软件内手工验证为准

## Current Problems

### 旧通用设置窗口

旧体系入口位于 `DAUIInterface::getConfigValues()` / `DAAppUI::getConfigValues()`，底层依赖 `DACommonPropertySettingDialog`。该实现：

- 基于 `QtPropertyBrowser`
- 使用面向旧窗口实现细节的 JSON schema
- 通过 Python `property_config_builder.py` 生成配置
- 与现有 `DAProperty*` 体系完全割裂

### 新属性面板

`DAPropertyItemWidget`、`DAPropertyPanelWidget`、`DAPropertyPanelContainerWidget` 已用于节点和图表属性面板，具备较好的嵌入性，但当前没有一套通用的 schema、编辑器注册系统和对话框壳层可供 Python/C++ 通用场景直接复用。

### 参数定义层耦合

`DAGui/NodeSetting/DAParamDef` 虽然已经是轻量定义结构，但仍放在节点设置模块中，并通过 `toParamDef(const DAPyNodeParameter&)` 直接绑定 Python 节点参数来源。这导致：

- 语义仍是“节点参数定义”
- 难以作为通用 C++ 设置表单模型复用
- 编辑器工厂 `DAParamTypeRegistry` 也被迫留在 `DAGui`

## Target Architecture

### Layer 1: DAUtils Form Core

在 `DAUtils` 新增一套纯定义、纯逻辑层，不依赖任何 `QWidget`：

- `DAFormSpec`
- `DAFormItemDef`
- `DAFormFieldDef`
- `DAFormGroupDef`
- `DAFormOption`
- `DAFormRule`
- `DAFormSchemaIO`
- `DAFormRuleEvaluator`

职责：

- 描述表单结构
- 负责 `QJsonObject <-> DAFormSpec`
- 负责声明式规则求值

这一层是统一 seam。任何来源的配置，只要能转成 `DAFormSpec`，就可以被统一渲染。

### Layer 2: DACommonWidgets Form Rendering

在 `DACommonWidgets` 基于现有 `DAProperty*` 体系构建统一渲染层：

- `DAFormEditorRegistry`
- `DAPropertyFormWidget`
- `DAPropertyFormDialog`

职责：

- 根据 `DAFormSpec` 动态创建属性项与编辑器
- 承载统一的值收集、值回写、规则刷新逻辑
- 提供可嵌入 widget 和模态 dialog 两种使用方式

其中 `DAPropertyFormWidget` 是主模块，负责：

- `field name -> property id / item / editor` 索引
- `QVariantMap` 批量写入
- 当前值收集
- 值变化后重新计算 `visible/enabled/required`

### Layer 3: DAGui Node Adapter

`DAGui/NodeSetting` 不再承载通用定义，只保留节点适配：

- 新增 `DANodeParameterFormAdapter`
- 改造 `DANodeParamSettingPanel`

职责：

- 将 `QList<DAPyNodeParameter>` 映射为 `DAFormSpec`
- 将节点实例当前值写入统一表单
- 监听统一表单值变化并回写 `DAPyNode`

### Layer 4: APP / DAInterface / Python Bridge

保留 `DAUIInterface::getConfigValues(...)` 作为统一入口，但其语义和实现全部切换到新体系：

- Python 可传 `FormSpec`
- Python 可传 `dict`
- Python 可传 JSON 字符串
- C++ 最终统一解析为 `DAFormSpec`
- UI 缓存对象由旧 dialog 切换为 `DAPropertyFormDialog`

## DAUtils Model Design

### DAFormSpec

顶层表单对象，包含：

- `version`
- `title`
- `items`

### DAFormItemDef

公共基类风格的数据结构，区分：

- `kind = group`
- `kind = field`

### DAFormGroupDef

包含：

- `name`
- `label`
- `description`
- `items`

### DAFormFieldDef

包含：

- `name`
- `type`
- `label`
- `description`
- `defaultValue`
- `placeholder`
- `readOnly`
- `layout`
- `height`
- `properties` 或显式字段集
- `rules`

### DAFormOption

枚举项结构：

- `value`
- `label`
- `description`

### DAFormRule

首版支持三类声明式规则：

- `visible_when`
- `enabled_when`
- `required_when`

规则表达式首版限定为简单表达式，围绕其他字段当前值求值，例如：

- `method == 'value'`
- `mode != 'custom'`
- `enable_filter == true`

不支持任意 Python 执行，不支持复杂脚本。

### DAFormSchemaIO

负责：

- 解析 v2 schema
- 生成 v2 schema
- 统一字段名与默认值策略

这层只支持 v2，不保留 v1 兼容逻辑。

### DAFormRuleEvaluator

负责：

- 基于当前 `QVariantMap` 计算字段规则结果
- 向渲染层返回字段的 `visible/enabled/required` 状态

后续若要增加新规则类型，仅在这里扩展。

## Python v2 API

### Public Modules

新增：

- `DAWorkbench.DAPyBase.form_spec`
- `DAWorkbench.DAPyBase.form_builder`

删除：

- `DAWorkbench.DAPyBase.property_config_builder`

### Public Types

Python 侧提供：

- `FormSpec`
- `GroupSpec`
- `FieldSpec`
- `OptionSpec`
- `FormBuilder`
- `option(...)`

### Supported Field Types

首版支持：

- `str`
- `int`
- `float`
- `bool`
- `enum`
- `file`
- `folder`
- `color`
- `font`
- `code`
- `list`

`dict` 仅在 schema 层预留，不提供首版编辑器。

### Example

```python
from DAWorkbench.DAPyBase.form_builder import FormBuilder, option

spec = (
    FormBuilder("Fill Missing Values")
    .group("fill", "Fill Method")
    .enum(
        "method",
        label="Fill Method",
        default="value",
        options=[
            option("value", "Fill with a specific value"),
            option("forward", "Use previous non-missing value"),
            option("backward", "Use next non-missing value"),
        ],
    )
    .end_group()
    .group("advanced", "Advanced")
    .int("limit", label="Limit", min=1, visible_when="method == 'value'")
    .bool("reindex", label="Reset Row Numbers", default=True)
    .end_group()
    .build()
)
```

### Python Bridge Behavior

`ui.getConfigValues(...)` 升级为接受：

- `FormSpec`
- `dict`
- JSON `str`

绑定层统一转为 `QJsonObject` 后交给 C++ 解析。

返回值保持为普通 Python `dict`，以降低业务脚本改造成本。

## v2 Schema

示例：

```json
{
  "version": 2,
  "title": "Fill Missing Values",
  "items": [
    {
      "kind": "group",
      "name": "fill",
      "label": "Fill Method",
      "items": [
        {
          "kind": "field",
          "name": "method",
          "type": "enum",
          "label": "Fill Method",
          "default": "value",
          "options": [
            { "value": "value", "label": "Fill with a specific value" },
            { "value": "forward", "label": "Use previous non-missing value" }
          ]
        }
      ]
    },
    {
      "kind": "field",
      "name": "limit",
      "type": "int",
      "label": "Limit",
      "min": 1,
      "visible_when": "method == 'value'"
    }
  ]
}
```

公共字段约定：

- `name`
- `label`
- `description`
- `default`
- `placeholder`
- `read_only`
- `layout`
- `height`

数值字段：

- `min`
- `max`
- `step`
- `decimals`

枚举字段：

- `options`

联动字段：

- `visible_when`
- `enabled_when`
- `required_when`

## Rendering Design

### DAFormEditorRegistry

从现有 `DAParamTypeRegistry` 演进并下沉到 `DACommonWidgets`。

每个字段类型通过统一适配接口承载四个动作：

- `createEditor`
- `setEditorValue`
- `editorValue`
- `connectValueChanged`

这样新增字段类型时，只需新增或注册一个字段适配器，而不必修改多个上层模块中的 `switch` 逻辑。

### DAPropertyFormWidget

内部基于 `DAPropertyPanelContainerWidget` 实现，负责：

- 按 spec 构建 group / field 层级
- 保存字段索引关系
- 应用当前值
- 收集当前值
- 响应值变化并刷新联动规则

对外 API 应至少提供：

- `setFormSpec(const DAFormSpec&)`
- `setValues(const QVariantMap&)`
- `QVariantMap values() const`
- `QVariant value(const QString& fieldName) const`
- `void setValue(const QString& fieldName, const QVariant&)`

### DAPropertyFormDialog

只是 `DAPropertyFormWidget` 的模态壳层，负责：

- 窗口标题
- 确认/取消
- 缓存复用

不承载字段逻辑。

## Node Reuse Design

### Remove DAParamDef

`DAParamDef` 及其 `toParamDef(const DAPyNodeParameter&)` 删除，不再作为公共定义层存在。

### DANodeParameterFormAdapter

新增薄适配器，将节点参数代理转换为 `DAFormSpec`。

其职责仅包括：

- 类型映射
- 默认值映射
- 属性映射
- label / description / layout / height / enum / min / max / step / filter 等转换

其职责不包括：

- 创建 QWidget
- 读取编辑器值
- 连接信号

### DANodeParamSettingPanel

改造为统一表单宿主：

1. `setNode()` 后读取 `QList<DAPyNodeParameter>`
2. 通过 `DANodeParameterFormAdapter` 生成 `DAFormSpec`
3. 从节点实例读取当前参数值写入 `DAPropertyFormWidget`
4. 监听字段值变化并回写 `DAPyNode`

这样节点 UI 外壳保留，但内部不再维护独立参数类型系统。

## APP / Interface Changes

### DAUIInterface

文档更新为面向“统一表单配置窗口”，而不是旧对话框 JSON。

### DAAppUI

缓存对象从 `DACommonPropertySettingDialog*` 改为 `DAPropertyFormDialog*`，逻辑保持：

- 指定 `cacheKey` 时缓存对话框
- 未指定时按需创建临时对话框

### DAInterfacePythonBinding

升级 `getConfigValues(...)` 绑定：

- 接收 Python `FormSpec`
- 接收 Python `dict`
- 接收 JSON 字符串

内部统一归一化为 `QJsonObject`。

## File Migration

### Remove

- `src/DACommonWidgets/DACommonPropertySettingDialog.h`
- `src/DACommonWidgets/DACommonPropertySettingDialog.cpp`
- `src/DACommonWidgets/DACommonPropertySettingDialog.ui`
- `src/PyScripts/DAWorkbench/DAPyBase/property_config_builder.py`
- `src/DAGui/NodeSetting/DAParamDef.h`
- `src/DAGui/NodeSetting/DAParamDef.cpp`

### Add

#### DAUtils

- `src/DAUtils/DAFormSpec.h`
- `src/DAUtils/DAFormSpec.cpp`
- `src/DAUtils/DAFormSchemaIO.h`
- `src/DAUtils/DAFormSchemaIO.cpp`
- `src/DAUtils/DAFormRuleEvaluator.h`
- `src/DAUtils/DAFormRuleEvaluator.cpp`

#### DACommonWidgets

- `src/DACommonWidgets/DAFormEditorRegistry.h`
- `src/DACommonWidgets/DAFormEditorRegistry.cpp`
- `src/DACommonWidgets/DAPropertyFormWidget.h`
- `src/DACommonWidgets/DAPropertyFormWidget.cpp`
- `src/DACommonWidgets/DAPropertyFormDialog.h`
- `src/DACommonWidgets/DAPropertyFormDialog.cpp`

#### DAGui / NodeSetting

- `src/DAGui/NodeSetting/DANodeParameterFormAdapter.h`
- `src/DAGui/NodeSetting/DANodeParameterFormAdapter.cpp`

#### Python

- `src/PyScripts/DAWorkbench/DAPyBase/form_spec.py`
- `src/PyScripts/DAWorkbench/DAPyBase/form_builder.py`

## Build System Changes

- 从 `src/DACommonWidgets/CMakeLists.txt` 删除 `damacro_import_QtPropertyBrowser(${DA_LIB_NAME})`
- 从相关 CMake 宏和依赖链中移除 `QtPropertyBrowser` 导入
- 更新 `DAUtils` / `DACommonWidgets` / `DAGui` / Python 导出文件的源列表

## Plugin Migration

`plugins/DataAnalysis/PyScripts/DADataAnalysisGui/dataframe_cleaner.py` 全量切换到新 v2 builder：

- 导入从 `property_config_builder` 改为 `form_builder`
- 所有 `enum_items + enum_descriptions` 改为 `options`
- 所有旧字段名切换到 v2

迁移完成后不保留任何 v1 builder 痕迹。

## Verification

本轮不补自动化测试，按用户要求采用软件内手工验证。

建议手工验证范围：

- DataAnalysis 插件中至少验证 3 类典型对话框
  - 纯基础字段
  - 分组 + 枚举
  - 声明式联动
- 节点参数设置面板验证
  - 常见基础类型
  - `below` 布局
  - 文件/颜色/字体/代码编辑器
- 通用配置入口验证
  - Python `FormSpec`
  - Python `dict`
  - JSON 字符串

## Non-Goals

本轮不做：

- Python 回调钩子
- `dict` 类型复杂编辑器
- v1 schema 兼容层
- deprecated 保留接口
- 自动化测试补齐

## Expected End State

最终仓库状态应满足：

- 不再存在 `DACommonPropertySettingDialog`
- 不再存在 `property_config_builder.py`
- 不再存在 `DAParamDef`
- `QtPropertyBrowser` 不再是编译或运行依赖
- 节点设置与通用配置窗口共享统一 `DAFormSpec + DAPropertyFormWidget` 内核
- Python 只暴露新的 v2 API
