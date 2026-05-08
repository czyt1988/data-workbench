# 节点描述符架构重构评估

**日期**: 2026-05-07
**议题**: 将 Python→C++ 节点描述符传递方式从 JSON (QJsonObject) 改为直接 pybind11 结构体绑定
**决策**: 渐进式替换，保留纯 Python dict 方式向后兼容

---

## 1. 当前架构

### 1.1 数据流

```
Python @NodeDef 装饰器
  → 构建 _node_descriptor dict（纯 Python dict）
  → cls.get_descriptor() 返回 dict

C++ DAPyNodeProxy::getDescriptor()
  → 获取 GIL（互斥锁）
  → 调用 pyNode.get_descriptor()
  → 接收 py::dict
  → DA::PY::pyDictToQJsonObject() 递归转换
  → 返回 QJsonObject

消费方通过 QJsonObject["key"] 读取各个字段
```

### 1.2 `_node_descriptor` 字典结构

```python
{
    "name": str,               # 节点显示名称
    "category": str,           # 节点分类
    "icon": str,               # 图标标识
    "qualified_name": str,     # 模块名.类名（唯一标识）
    "inputs": [                # 输入端口列表
        {"name": str, "data_type": str, "required": bool, "description": str}
    ],
    "outputs": [               # 输出端口列表
        {"name": str, "data_type": str, "description": str}
    ],
    "parameters": [            # 参数列表
        {"name": str, "type": str, "default": Any, "description": str}
    ],
    "style": dict,             # DANodeStyle.toJson() 返回的样式字典，可选
    "render_template": str,    # "nodestyle" | "widget"
}
```

### 1.3 现存的两套数据获取路径

`DAPyNodeProxy` 中存在**技术债务** — 同一个 descriptor 信息有两种获取方式：

| 路径 | 方法 | 特点 |
|------|------|------|
| **快速路径** | `syncMetaFromPyNode()` | 直接读 Python 属性 (`pyNode.attr("qualified_name")` 等)，不走 JSON |
| **慢速路径** | `getDescriptor()` | 读完整 dict → JSON 转换，用于 parameters、render_template 等 |

两种路径可能产生不一致，且增加了维护成本。

### 1.4 JSON 消费方一览

| 消费方 | 读取方式 | 用途 |
|--------|----------|------|
| `DAPyNodeProxy::getDescriptor()` | `pyNode.get_descriptor()` → `pyDictToQJsonObject()` | 全量获取 |
| `DAPyNodeProxy::syncMetaFromPyNode()` | `pyNode.attr("qualified_name")` 等逐属性读取 | 缓存元信息 |
| `DAPyWorkFlowScene::createPyNode()` | 从 `QJsonObject["qualified_name"]` 提取 | 创建节点 |
| `DAPyWorkFlowGraphicsView::createNode_()` | 从 `DAPyNodeMetaData` 构建 `QJsonObject` | 拖拽创建 |
| `DAPyNodeGraphicsItem::setDescriptor()` | 存储 `QJsonObject` | 渲染/连接点 |
| `DANodeParamSettingPanel` | `ParameterDescriptor::fromJsonArray()` | 参数面板渲染 |
| `DAParamTypeRegistry` | 从 `ParameterDescriptor.type` 字段创建编辑器 | 11 种类型编辑器 |
| 保存/加载 | `saveToXml()` / `loadFromXml()` | 工程文件序列化 |

---

## 2. 重构方案：C++ `DANodeDescriptor` 结构体

### 2.1 核心思路

将 Python 端的 `node_descriptor.py` 中 `DANodeDescriptor` 类替换为 C++ 导出的结构体：

```cpp
// 预期 C++ 侧
struct DAPYWORKFLOW_API DANodeDescriptor
{
    QString name;
    QString category;
    QString qualifiedName;
    QString icon;
    QVector<DAPortDescriptor> inputs;
    QVector<DAPortDescriptor> outputs;
    QVector<ParameterDescriptor> parameters;
    RenderTemplate renderTemplate;
    DANodeStyle style;
};

struct DAPYWORKFLOW_API DAPortDescriptor
{
    QString name;
    QString dataType;
    bool required = true;
    QString description;
};
```

通过 pybind11 导出到 Python，Python 侧直接创建并填充：

```python
import da_py_workflow

desc = da_py_workflow.DANodeDescriptor()
desc.name = "Data Filter"
desc.category = "Data Processing"
# ...
```

### 2.2 与 `DANodeStyle` 先例对比

项目中 `DANodeStyle` 已经采用了类似模式：

| 维度 | DANodeStyle (现有) | DANodeDescriptor (提议) |
|------|-------------------|------------------------|
| C++ 定义 | `struct DANodeStyle` (DAPyNodeStyle.h) | `struct DANodeDescriptor` (新) |
| pybind11 绑定 | `py::class_<DANodeStyle>` + `def_readwrite` | `py::class_<DANodeDescriptor>` + `def_readwrite` |
| Python 使用 | `style = da_py_workflow.DANodeStyle()` | `desc = da_py_workflow.DANodeDescriptor()` |
| 序列化 | `toJson()` / `fromJson()` 稀疏策略 | 需新增类似机制 |

**结论**: `DANodeStyle` 模式已被验证可行，`DANodeDescriptor` 可直接复用此模式。

---

## 3. 优势与劣势分析

### 3.1 优势

| 优势 | 详细说明 |
|------|----------|
| **类型安全** | 编译期检查，杜绝 `descriptor["naem"]` 这类拼写错误。当前 JSON key 是字符串，运行时才能发现错误 |
| **性能提升** | 省去 `py::dict → QJsonObject` 递归转换开销。`getDescriptor()` 每次调用都获取 GIL + 转换整个 dict，改结构体后变为一次 GIL 获取 → 直接字段读取 |
| **IDE 自动补全** | Python 侧 `desc.name`、C++ 侧 `desc.name` 均可自动补全，而 `descriptor["name"]` 无法补全 |
| **代码可维护性** | 字段定义集中在 `DANodeDescriptor` 结构体中，而非散落在各消费方的字符串 key 中 |
| **统一数据路径** | 消除 `syncMetaFromPyNode()` vs `getDescriptor()` 的两套获取路径，所有元信息统一从结构体读取 |
| **与现有风格一致** | 与 `DANodeStyle` 绑定模式一致，团队已熟悉此模式 |

### 3.2 劣势

| 劣势 | 详细说明 | 缓解措施 |
|------|----------|----------|
| **Python 插件开发者需导入 C++ 模块** | 当前只需 `from DAWorkbench.DAWorkFlowPy import NodeDef`，改为需要 `import da_py_workflow` | **保留 dict 方式**：@NodeDef 同时支持 dict 和结构体，Python 开发者可选择 |
| **序列化需额外实现** | JSON 天然可序列化，结构体需手动实现 `toJson()`/`fromJson()` | 参考 `DANodeStyleToJson()`/`DANodeStyleFromJson()` 的稀疏策略，直接复用 |
| **迁移成本** | 约 20 个消费点需要逐步替换 | 渐进式 3 阶段替换，每个阶段独立测试 |
| **Python 侧调试依赖 C++ 模块加载** | 纯 Python 测试环境中 `da_py_workflow` 模块不可用 | 保留 dict 路径作为 fallback，纯 Python 测试不受影响 |
| **嵌套结构体绑定复杂度** | `inputs`/`outputs`/`parameters` 是列表嵌套结构体 | pybind11 支持 `def_readwrite` 对 `QVector<T>` 的绑定，参考 `DAPyNodeMetaData` |

### 3.3 综合对比表

| 维度 | JSON 方案（现状） | 结构体方案（目标） | 混合方案（渐进式） |
|------|:--:|:--:|:--:|
| **类型安全** | ❌ 运行时 | ✅ 编译期 | ✅ 新路径编译期 |
| **性能** | ⚠️ 中等（GIL+序列化） | ✅ 高（直接字段访问） | ✅ 新路径高性能 |
| **IDE 自动补全** | ❌ 字符串 key | ✅ 结构体字段 | ✅ 新路径可补全 |
| **代码可维护性** | ⚠️ key 散落各处 | ✅ 集中定义 | ✅ 新路径集中定义 |
| **Python 插件开发体验** | ✅ 纯 Python dict | ⚠️ 需导入 C++ 模块 | ✅ 双轨可选 |
| **序列化（保存/恢复）** | ✅ 天然可序列化 | ⚠️ 需额外实现 | ✅ 结构体 toJson/fromJson |
| **向后兼容** | ✅ 现有代码不变 | ❌ 需迁移所有插件 | ✅ 保留 dict 方式 |
| **跨语言调试** | ⚠️ JSON 可读性好 | ✅ 结构体可直接 inspect | ✅ 两种方式均可 |
| **迁移成本** | — | ⚠️ 高（一次性） | ⚠️ 中等（分阶段） |

---

## 4. 推进策略：渐进式 3 阶段替换

基于用户决策（渐进式替换 + 保留纯 dict 方式），推荐以下 3 阶段方案：

### 阶段 1：创建 C++ `DANodeDescriptor` 结构体

**目标**: 定义结构体 + pybind11 绑定 + 序列化函数

| 任务 | 文件位置 | 说明 |
|------|----------|------|
| 定义 `DANodeDescriptor` 结构体 | `src/DAPyWorkFlow/DANodeDescriptor.h` | 包含 name、category、qualifiedName、icon、inputs、outputs、parameters、renderTemplate、style |
| 定义 `DAPortDescriptor` 子结构体 | `src/DAPyWorkFlow/DANodeDescriptor.h` | name、dataType、required、description |
| 定义 `DAParameterDescriptor` 子结构体 | 已存在于 `src/DAGui/NodeSetting/ParameterDescriptor.h` | 评估是否复用或扩展 |
| 实现 `toJson()`/`fromJson()` | `src/DAPyWorkFlow/DANodeDescriptor.cpp` | 参考 `DANodeStyleToJson()` 稀疏策略 |
| pybind11 绑定 | `src/DAPyWorkFlow/PythonBinding/DAPyWorkFlowPythonBinding.cpp` | `py::class_<DANodeDescriptor>` + 全字段 `def_readwrite` |
| CMake 注册 | `src/DAPyWorkFlow/CMakeLists.txt` | 新增源文件 |

**验证**: 编写单元测试，验证结构体创建、字段读写、toJson/fromJson 正确性

### 阶段 2：Python 侧双轨运行

**目标**: `@NodeDef` 同时支持 dict 和结构体，C++ 新增结构体读取路径

| 任务 | 文件位置 | 说明 |
|------|----------|------|
| 修改 `@NodeDef` 装饰器 | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_def.py` | 在构建 dict 的同时，构建 `da_py_workflow.DANodeDescriptor` 结构体并存为 `_node_descriptor_struct` 属性 |
| 修改 `DANodeDescriptor` Python 类 | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_descriptor.py` | 保留 `to_dict()` 方法，新增 `to_struct()` 方法返回 C++ 结构体 |
| 修改 `DAPyNodeProxy::getDescriptor()` | `src/DAPyWorkFlow/DAPyNodeProxy.h/.cpp` | 新增 `getDescriptorStruct()` 返回 `DANodeDescriptor`，旧方法保持不变 |
| 修改 `syncMetaFromPyNode()` | `src/DAPyWorkFlow/DAPyNodeProxy.cpp` | 优先从 `_node_descriptor_struct` 读取（快速路径），fallback 到 `_node_descriptor` dict |

**验证**: 确认 dict 方式和结构体方式均能正确传递所有字段，现有插件无需修改即可运行

### 阶段 3：逐步替换消费方

**目标**: 新代码统一使用结构体，保留 JSON 序列化用于保存/加载

| 优先级 | 消费方 | 替换方式 |
|--------|--------|----------|
| **P0** | `DAPyNodeProxy::syncMetaFromPyNode()` | 直接从结构体字段读取，消除两套路径 |
| **P0** | `DAPyNodeGraphicsItem::setDescriptor()` | 新增 `setDescriptorStruct(const DANodeDescriptor&)`，内部统一使用结构体 |
| **P1** | `DAPyWorkFlowScene::createPyNode()` | 从结构体提取 `qualifiedName`，而非 JSON key |
| **P1** | `DAPyWorkFlowGraphicsView::createNode_()` | 从 `DAPyNodeMetaData` 构建 `DANodeDescriptor` |
| **P2** | `DANodeParamSettingPanel` | 从 `DANodeDescriptor.parameters` 直接获取 `QVector<ParameterDescriptor>` |
| **P2** | 保存/加载 (`saveToXml`/`loadFromXml`) | 内部调用 `DANodeDescriptorToJson()`/`DANodeDescriptorFromJson()` 序列化 |
| **P3** | 移除旧 JSON 路径 | 废弃 `getDescriptor()` 返回 QJsonObject 的方法 |

**验证**: 每个替换点独立验证，确认功能无变化

---

## 5. Python 侧双轨兼容设计

保留纯 Python dict 方式意味着：

- `@NodeDef` 装饰器**同时生成** `_node_descriptor` dict 和 `_node_descriptor_struct` C++ 结构体
- Python 开发者可以继续只使用 `from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter`
- 如果 Python 开发者需要类型安全，可以选择性地使用 `import da_py_workflow` 直接创建结构体
- 纯 Python 测试环境中（C++ 模块不可用时），仅 dict 路径生效

`@NodeDef` 装饰器修改后的逻辑：

```python
def NodeDef(name, category="", render_template="rect", style=None):
    normalized_template = _normalize_render_template(render_template)

    def decorator(cls):
        # ...收集 inputs/outputs/parameters（不变）...

        # 路径 A：构建 dict（向后兼容）
        descriptor_dict = {
            "name": name,
            "category": category,
            ...
        }
        cls._node_descriptor = descriptor_dict

        # 路径 B：构建 C++ 结构体（性能优先）
        try:
            import da_py_workflow
            descriptor_struct = da_py_workflow.DANodeDescriptor()
            descriptor_struct.name = name
            descriptor_struct.category = category
            ...
            cls._node_descriptor_struct = descriptor_struct
        except ImportError:
            # 纯 Python 环境中 C++ 模块不可用，仅 dict 路径生效
            cls._node_descriptor_struct = None

        return cls
    return decorator
```

C++ 侧读取优先级：

```
优先读取 _node_descriptor_struct（结构体路径，高性能）
  ↓ 如果不可用
回退到 _node_descriptor dict → pyDictToQJsonObject（旧路径，兼容性）
```

---

## 6. 风险与注意事项

| 风险 | 缓解措施 |
|------|----------|
| pybind11 `QVector<T>` 绑定可能需要自定义转换器 | 参考 `DAPyNodeMetaData` 中 `inputKeys`/`outputKeys` 的 `QList<QString>` 绑定模式 |
| `ParameterDescriptor` 存在于两个位置（`DAGui/NodeSetting/` 和提议的 `DAPyWorkFlow/`） | 评估是否复用 `DAGui::ParameterDescriptor`，避免类型不一致 |
| Python 端 `DANodeDescriptor` 类与 C++ 导出的同名类型冲突 | 统一使用 C++ 导出的 `da_py_workflow.DANodeDescriptor`，废弃 Python 侧 `node_descriptor.py` 中的同名类 |
| 保存/加载格式需要保持向后兼容 | 工程文件继续使用 JSON 格式，`DANodeDescriptorToJson()` 输出与现有格式一致 |
| `DAPyNodeMetaData` 和 `DANodeDescriptor` 的关系需要厘清 | `DAPyNodeMetaData` 是发现阶段的轻量摘要，`DANodeDescriptor` 是完整描述，两者职责不同但字段重叠 |

---

## 7. 与同类项目对比

| 项目 | 方案 | 评价 |
|------|------|------|
| 本项目现状 | py::dict → QJsonObject | 灵活但无类型安全 |
| 本项目 DANodeStyle | C++ struct + toJson/fromJson | 类型安全 + 可序列化 ✅ |
| Qt for Python (PySide) | 直接 C++ 类绑定 | 零开销，Qt 官方做法 |
| nanobind/pybind11 推荐 | `py::class_<>` 直接绑定 | 推荐类型绑定而非 dict |

本项目的 `DANodeStyle` 方案是行业推荐做法，将此模式推广到 `DANodeDescriptor` 是合理的演进方向。

---

## 8. 结论

**JSON 方案的核心问题**: 字符串 key 无编译期检查、每次调用都要获取 GIL + 递归 JSON 转换、key 散落各消费方导致维护困难。

**结构体方案的核心价值**: 编译期类型安全、消除 JSON 序列化开销、IDE 自动补全、集中定义、统一数据路径。

**推荐策略**: 渐进式 3 阶段替换，保留纯 Python dict 向后兼容。每个阶段独立验证、独立可回滚。最终目标：新代码统一使用 `DANodeDescriptor` 结构体，JSON 仅用于工程文件序列化。