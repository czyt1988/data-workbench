# DAPyWorkFlow 模块开发指南

Python 工作流节点的 C++ 渲染代理层。C++ 类继承 `DAPyObjectWrapper`，通过 `attr()` 转发调用到 Python，不做本地数据缓存。

DAWorkFlowPy 类似于 `Apache Hamilton`，是一个工作流节点的制定和运行模块。与 Hamilton 的"函数=节点"单输出模型不同，DAWorkFlowPy 采用类似 Unreal/Blender 节点编辑器的**多输入多输出端口模型**。与 NodeGraphQt（UI+数据一体化）不同，DAWorkFlowPy 是纯数据模型层，UI 由 C++ QGraphicsView 渲染，实现更干净的分离。

---

## 一、架构分层

```
┌─────────────────────────────────────────────────────┐
│                 Plugin Layer                         │
│  Python-only:  @NodeDef nodes in PyScripts          │
│  C++-optional: NodeFactory subclass + custom UI     │
│       ↕ registration ↕                              │
├─────────────────────────────────────────────────────┤
│         Python Logic Layer                           │
│  (src/PyScripts/DAWorkbench/DAWorkFlowPy)           │
│  DAWorkflow       — DAG model (topology, lifecycle) │
│  DAWorkflowNode   — node definition + execute()     │
│  NodeDef          — decorator for node declaration  │
│  DAConnection     — node-to-node data link          │
│  DANodeRegistry   — discover & register node types  │
│  DANodeFactory    — create node instances           │
│  DAWorkflowExecutor — topological execution engine   │
│  DAWorkflowSerializer — JSON save/load              │
│       ↕ attr() proxy ↕                              │
├─────────────────────────────────────────────────────┤
│         C++ Rendering Proxy Layer                    │
│  (src/DAPyWorkFlow)                                 │
│  DAPyWorkFlow      — proxy for DAWorkflow           │
│  DAPyNode          — proxy for DAWorkflowNode       │
│  DAPyNodeFactory   — proxy for DANodeFactory        │
│  DAPyWorkFlowManager — signal manager (QObject)     │
│  DAPyNodeGraphicsItem — renders node in scene       │
│  DAPyLinkGraphicsItem — renders connection          │
│  DAPyWorkFlowScene — QGraphicsScene orchestration   │
│  DAPyWorkFlowSceneSerializer — scene layout save    │
│  PythonBinding     — pybind11 module exposure       │
└─────────────────────────────────────────────────────┘
```

### 核心设计思路：Thin Proxy + Python-First

- Python 拥有 ALL 逻辑（DAG、节点定义、执行、序列化）
- C++ 代理类是纯 thin proxy — 持有 `pybind11::object`，通过 `DAPyObjectWrapper::attr()` 转发调用，不做本地数据缓存
- 与 DAPyDataFrame 模式完全一致
- Qt 信号仅在 `DAPyWorkFlowManager`（QObject）中存在，代理类不继承 QObject

## 二、模块职责

| 职责 | 说明 |
|------|------|
| **代理 Python 对象** | `DAPyNode`、`DAPyWorkFlow`、`DAPyNodeFactory` 等持有 `pybind11::object`，通过 `attr()` 访问 Python 属性和方法 |
| **渲染工作流场景** | `DAPyNodeGraphicsItem`、`DAPyLinkGraphicsItem`、`DAPyWorkFlowScene` 在 QGraphicsView 中渲染 |
| **信号桥接** | `DAPySignalManager` 将 Python 回调转为 Qt 信号 |

## 三、依赖关系

```
DAPyWorkFlow → DAPyBindQt（Python↔Qt 绑定层）
             → DAGraphicsView（图形视图框架）
             → DAUtils
```

本模块**不应依赖** DAGui、DAData、DAFigure 等上层模块。

---

## 四、设计原则

### P1: Python 逻辑独立性

DAWorkFlowPy 模块可以脱离 C++ 端独立运行和测试。不导入任何 C++ 通过 pybind11 导出的模块。

- **Why**: 参考 Blender 和 KNIME 的做法——逻辑层与渲染层完全解耦，使得 Python 单元测试不需要 Qt 环境
- **How**: 所有 Python 模块的 import 只使用标准库和第三方库（pandas, numpy 等），永远不出现 `import da_xxx`（C++ 导出模块）

### P2: C++ 纯代理原则

C++ 代理类只继承 `DAPyObjectWrapper`，不继承 `QObject`，不做本地数据缓存，所有操作通过 `attr()` 转发到 Python。

- **Why**: 与 DAPyDataFrame 模式一致，代理类保持极简，避免双端数据不一致
- **How**: 代理类的方法实现只有 `attr("method_name")(args)` 调用，不做本地存储或计算

### P3: 信号与代理分离

Qt 信号槽仅在 `DAPyWorkFlowManager`（QObject）中存在，代理类（DAPyObjectWrapper 子类）不包含任何 Qt 信号。

- **Why**: 避免在 Python 代理中引入 Qt 依赖，保持代理纯净性，信号管理集中化便于维护
- **How**: 所有 `Q_SIGNALS` 和 `Q_SLOTS` 只在 Manager/Scene 等纯 C++ Qt 类中定义

### P4: 渲染与逻辑分层

NodeDisplay（icon, render_template, style）是 Python 节点类的附加属性，仅供 C++ 渲染读取，Python 逻辑层不使用 NodeDisplay。

- **Why**: 参考 Blender DNA/RNA 模式——节点定义（逻辑）和节点显示（渲染）是两个独立关注点
- **How**: Python `execute()` 方法永远不读取 `_node_display`，C++ 渲染层只读取 `_node_display` 不干预 `execute()`

### P5: 序列化由 Python 主导

工作流序列化统一由 Python DAWorkflowSerializer 处理（JSON格式），C++ 仅保存场景布局（节点位置等 UI 元数据）。

- **Why**: Python owns all logic data（节点拓扑、参数、连接），序列化应由数据所有者负责
- **How**: 保存文件时：Python 生成逻辑 JSON + C++ 生成布局 JSON → 合并存储。加载时：Python 恢复 DAG → C++ 恢复布局

### P6: 插件 Python-first

插件开发的主流路径是纯 Python（@NodeDef），C++ NodeFactory 子类是可选的高级扩展点，仅用于渲染/UI定制。

- **Why**: 参考 ComfyUI 的 custom_nodes 模式——降低门槛，开发者只需定义 Python 类即可创建新节点
- **How**: 所有新节点建议用 `@NodeDef` 定义，C++ NodeFactory 子类只在需要自定义 QGraphicsItem 或 QDialog 时才使用

### P7: 单文件单职责

每个模块文件只承担一个明确的职责。混合多种职责的文件应拆分。

### P8: 代理类不传播异常

C++ 代理类的异常处理遵循 DAPyDataFrame 模式——try/catch + 安全默认值返回，不向 Qt 应用层传播 Python 异常。

- **Why**: 保持 Qt UI 稳定性，Python 异常不应导致 C++ 程序崩溃
- **How**: 代理类每个方法用 try/catch 包裹 `attr()` 调用，失败时返回空值/false/none，通过 `qCritical()` 记录错误

---

## 五、⚠️ 核心规则：pybind11 类型转换

### 铁律：复用 DAPybind11QtCaster，禁止重复造轮子

所有 Python ↔ Qt 类型转换**必须**使用 `src/DAPyBindQt/DAPybind11QtCaster.hpp` 中已注册的 `type_caster` 和辅助函数。**严禁**在本模块中手写转换函数（如 `pyObjectToVariant`、`variantToPyObject` 等）。

#### 已支持的类型（直接使用 `pybind11::cast`）

| Qt 类型 | Python 类型 | 用法 |
|---------|------------|------|
| `QString` | `str` | `pybind11::cast<QString>(handle)` / `pybind11::cast(qstring)` |
| `QByteArray` | `bytes` | 同上 |
| `QDate` | `datetime.date` | 同上 |
| `QTime` | `datetime.time` | 同上 |
| `QDateTime` | `datetime.datetime` / `pandas.Timestamp` / `numpy.datetime64` | 同上 |
| `QList<T>` | `list` | `pybind11::cast<QList<int>>(handle)` |
| `QVector<T>` | `list` | 同上（Qt5） |
| `QSet<T>` | `set` / `list` | 同上 |
| `QHash<K, V>` | `dict` | `pybind11::cast<QHash<QString, QVariant>>(handle)` |
| `QMap<K, V>` | `dict` | 同上 |
| `QVariant` | 任意（自动推断） | `pybind11::cast<QVariant>(handle)` |
| `QColor` | `tuple(r,g,b,a)` / `str("#RRGGBB")` / `QColor` | 同上 |
| `QPointF` | `tuple(x, y)` | 同上 |
| **`QVariantHash`** = `QHash<QString, QVariant>` | `dict` | `pybind11::cast<QVariantHash>(handle)` ✅ |
| **`QVariantList`** = `QList<QVariant>` | `list` | `pybind11::cast<QVariantList>(handle)` ✅ |

> `QVariantHash` 和 `QVariantList` 是复合类型，内部递归使用各元素的 `type_caster`，支持任意嵌套。

#### 辅助函数（`DA::PY` 命名空间）

当需要显式构造 `pybind11::object` 时使用：

```cpp
#include "DAPyBindQt/DAPybind11QtCaster.hpp"

// Qt → Python
pybind11::object obj = DA::PY::toPyObject(qstring);
pybind11::object obj = DA::PY::toPyObject(qvariant);
pybind11::object obj = DA::PY::toPyObject(qhash);  // QHash<K,V>
pybind11::object obj = DA::PY::toPyObject(qlist);   // QList<T>

// Python → Qt
QString s = DA::PY::fromPyString(handle);
QDate d   = DA::PY::fromPyDate(handle);
```

#### JSON 转换（`DA::PY` 命名空间，`DAPyJsonCast.h`）

仅在需要 JSON **序列化**时使用（如文件读写、网络传输）。如果只是在 C++ 和 Python 之间传递数据，用 `QVariantHash` 而非 `QJsonObject`。

```cpp
#include "DAPyBindQt/DAPyJsonCast.h"

pybind11::dict  d = DA::PY::qjsonObjectToPyDict(jsonObj);  // QJsonObject → dict
QJsonObject     j = DA::PY::pyDictToQJsonObject(pyDict);   // dict → QJsonObject
```

### 正确用法示例

```cpp
#include "DAPybind11QtCaster.hpp"

// ✅ 正确：直接用 type_caster 转换
QVariantHash config = attr("_input_data").cast<QVariantHash>();
pybind11::dict pyConfig = pybind11::cast(config);

// ✅ 正确：QVariant 自动推断类型
QVariant val = pybind11::cast<QVariant>(some_python_object);
pybind11::object pyVal = DA::PY::toPyObject(qvariant);

// ❌ 错误：手写 pyObjectToVariant / variantToPyObject
// ❌ 错误：手写 isinstance<bool_> / isinstance<int_> 链来判断类型再逐个 cast
// ❌ 错误：用 QJsonObject 做 C++↔Python 数据传递（仅序列化场景才用 JSON）
```

### bool/int 判断顺序

`type_caster<QVariant>` 内部已正确处理 `bool` 在 `int` 之前的判断顺序（Python 中 `bool` 是 `int` 的子类）。如果你需要手动判断类型（极少数场景），**必须**先判断 `bool` 再判断 `int`：

```cpp
// ✅ 正确顺序
if (pybind11::isinstance<pybind11::bool_>(obj))  // 先 bool
    ...
else if (pybind11::isinstance<pybind11::int_>(obj))  // 后 int
    ...

// ❌ 错误：isinstance<int_>(True) 返回 true，bool 被误判为 int
```

### 缺少类型转换时的处理流程

当 `pybind11::cast<TargetType>(handle)` 编译报错（找不到 `type_caster` 特化）或运行时抛异常时：

```
1. 确认 TargetType 是否已在上表中 → 检查 include 是否遗漏
2. 未找到 → 判断该转换是否通用（其他模块也可能用到）
   ├─ 是（通用） → 在 DAPybind11QtCaster.hpp 中新增 type_caster 特化
   └─ 否（仅本模块） → 在本模块 .cpp 文件的匿名命名空间中实现
3. 绝不在 .h 文件中定义转换函数（避免链接冲突）
```

**判断"通用性"的标准**：如果该 Qt 类型在 Qt 基础库（Core/Gui/Widgets）中存在，则视为通用类型，应添加到 `DAPybind11QtCaster.hpp`。仅当类型来自 DAPyWorkFlow 内部（如 `DAPyNodeState` 枚举）时才在本模块实现。

---

## 六、代理类编码规范

### 1. 继承 DAPyObjectWrapper，不继承 QObject

```cpp
// ✅ 正确
class DAPYWORKFLOW_API DAPyNode : public DAPyObjectWrapper { ... };

// ❌ 错误：代理类不应包含 Qt 信号槽
class DAPYWORKFLOW_API DAPyNode : public QObject, public DAPyObjectWrapper { ... };
```

Qt 信号仅在 `DAPySignalManager`、`DAPyWorkFlowScene` 等纯 C++ Qt 类中使用。

### 2. attr() 代理 + try/catch 安全默认值

每个代理方法必须用 try/catch 包裹，异常时返回安全默认值：

```cpp
QString DAPyNode::getNodeName() const
{
    if (isNone())
        return {};
    try {
        return attr("name").cast<QString>();
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return {};
}
```

### 3. 不缓存 Python 数据

代理类不存储从 Python 读取的数据（如名称、参数列表）。每次调用都通过 `attr()` 实时读取，避免双端数据不一致。

### 4. Include 检查清单

编写 `.cpp` 文件时，若用到 `pybind11::cast<QString>`、`pybind11::cast<QVariant>` 等 Qt 类型转换：

```
✅ 必须包含: #include "DAPybind11QtCaster.hpp"
❌ 不要遗漏（遗漏会导致编译通过但运行时 cast 失败）
```

---

## 七、Python 逻辑层

> 位于 `src/PyScripts/DAWorkbench/DAWorkFlowPy`，Python 端代码应独立于 C++ 端运行。

### 模块划分

| 模块 | 文件 | 职责 |
|------|------|------|
| 类型定义 | `types.py` | `Input`, `Output`, `Parameter` 声明类 |
| 节点定义 | `node_def.py` | `NodeDef` 装饰器，`DAWorkflowNode` 基类，`NodeDisplay` 渲染属性 |
| 连接关系 | `connection.py` | `DAConnection` — 节点间数据连接 |
| 工作流模型 | `workflow.py` | `DAWorkflow` — DAG 模型，管理节点和连接，拓扑排序 |
| 节点注册 | `node_registry.py` | `DANodeRegistry` — 发现并注册 `@NodeDef` 节点类 |
| 节点工厂 | `node_factory.py` | `DANodeFactory` — 发现+创建节点实例 |
| 执行引擎 | `executor.py` | `DAWorkflowExecutor` — 拓扑排序执行，异步支持 |
| 信号管理 | `signal_manager.py` | `DASignalManager` — 节点间数据传播 |
| 序列化 | `serializer.py` | `DAWorkflowSerializer` — JSON 序列化/反序列化 |
| 语法糖 | `syntax.py` | `NodeProxy` / `>>` 操作符语法 |

### NodeDisplay 与元数据分离

`NodeDef` 装饰器生成两类属性：

- **逻辑属性**: `qualified_name`, `name`, `category`, `inputs`, `outputs`, `parameters` — 节点执行所需
- **渲染属性**: `_node_display`（NodeDisplay dataclass）— 仅 C++ 渲染所需，Python 逻辑层不使用

### 序列化格式

```json
{
  "name": "My Workflow",
  "version": "1.0",
  "nodes": [
    {
      "node_id": "pkg.DataFilter_1",
      "qualified_name": "pkg.DataFilter",
      "parameters": { "column": "value" }
    }
  ],
  "connections": [
    {
      "source_node_id": "pkg.DataFilter_1",
      "source_output_channel": "filtered",
      "target_node_id": "pkg.DataSort_1",
      "target_input_channel": "data"
    }
  ]
}
```

场景布局由 C++ 单独保存（节点位置、缩放级别等 UI 元数据），合并到同一文件的不同 section 中。

---

## 八、插件扩展机制

### Python-only 路径（主流）

```python
from DAWorkFlowPy import NodeDef, Input, Output, Parameter

@NodeDef(name="Data Filter", category="Data Processing", icon=":icons/filter.svg")
class DataFilter:
    column = Parameter(str, default="value", description="列名")

    class Inputs:
        data = Input("DataFrame", required=True)

    class Outputs:
        filtered = Output("DataFrame")

    def execute(self, inputs, params):
        df = inputs["data"]
        return {"filtered": df[df[params["column"]] > 0]}
```

节点发现：`DANodeRegistry.discover(scan_paths=[plugin_py_path])` 自动扫描 `@NodeDef` 装饰的类。

### C++-optional 路径（高级扩展）

C++ NodeFactory 子类（如 DataAnalysisNodeFactory）仅扩展渲染/UI 层面：

- 自定义 QDialog（参数编辑对话框）
- 自定义 QGraphicsItem 渲染（如特殊表格显示）
- 节点加入 workflow 的回调
- 序列化扩展信息

节点逻辑（execute）始终在 Python DAWorkflowNode 中定义，C++ 不干预。

### 插件接口契约

Python 插件包需要提供：

1. 包含 `@NodeDef` 装饰的节点类
2. `qualified_name` 自动生成（模块名.类名）
3. `NodeDisplay` 提供 icon/render_template/style 供 C++ 渲染
4. `execute()` 方法定义节点逻辑

可选增强：支持 `pyproject.toml` 的 `entry_points` 声明，便于 pip 安装的插件自动被发现。

---

## 九、C++ 渲染代理层详细设计

### 类结构

| 类 | 职责 | 继承 | Qt信号 |
|----|------|------|---------|
| DAPyWorkFlow | DAWorkflow 的纯代理，attr() 转发 DAG 操作 | DAPyObjectWrapper | 无 |
| DAPyNode | 单个节点的纯代理，读取元数据和 NodeDisplay | DAPyObjectWrapper | 无 |
| DAPyNodeFactory | DANodeFactory 的纯代理，转发 discover/create | DAPyObjectWrapper | 无 |
| DAPyNodeMetaData | 从 Python dict 转换的节点元数据 C++ 结构体 | 纯数据 | 无 |
| DAPyWorkFlowManager | 信号管理类，持有 DAPyWorkFlow，封装操作并发射 Qt 信号 | QObject | 有 |
| DAPyNodeGraphicsItem | 节点渲染 QGraphicsItem | QGraphicsItem | 无 |
| DAPyLinkGraphicsItem | 连线渲染 QGraphicsItem | QGraphicsItem | 无 |
| DAPyWorkFlowScene | QGraphicsScene，从 Manager 监听信号更新图形项 | QGraphicsScene | 无 |
| DAPyWorkFlowSceneSerializer | 场景布局序列化 | 纯序列化 | 无 |

### 信号流转模式

```
用户操作 → DAPyWorkFlowManager.addNode(proxy)
                    ↓ 内部调用 DAPyWorkFlow.attr("add_node")(proxy)
                    ↓ Python DAWorkflow 处理，返回 node_id
                    ↓ DAPyWorkFlowManager 发射 nodeAdded(node_id, proxy)
                    ↓ DAPyWorkFlowScene 接收信号，创建 DAPyNodeGraphicsItem
```

DAPyWorkFlowManager 信号列表：

- `nodeAdded(QString nodeId, DAPyNodeProxy* proxy)` — 节点添加
- `nodeRemoved(QString nodeId)` — 节点移除
- `connectionAdded(...)` — 连接添加
- `connectionRemoved(QString connId)` — 连接移除
- `executionStarted()` — 执行开始
- `executionFinished(bool success)` — 执行完成
- `nodeExecuted(QString nodeId, bool success)` — 单个节点执行完成
- `executorStateChanged(QString oldState, QString newState)` — 执行器状态变更

---

## 十、文件组织

| 文件 | 职责 |
|------|------|
| `DAPyNode.h/.cpp` | 节点代理 — 元数据读取、配置读写、输入输出数据访问 |
| `DAPyWorkFlow.h/.cpp` | 工作流代理 — DAG 操作转发 |
| `DAPyNodeFactory.h/.cpp` | 节点工厂代理 — 发现/创建节点 |
| `DAPyNodeGraphicsItem.h/.cpp` | 节点渲染 — QGraphicsItem 实现 |
| `DAPyLinkGraphicsItem.h/.cpp` | 连线渲染 |
| `DAPyLinkPoint.h/.cpp` | 连接点 |
| `DAPyLinkPointStyle.h/.cpp` | 连接点样式 |
| `DAPyNodeConnection.h/.cpp` | 节点连接关系 |
| `DAPyNodeParameter.h/.cpp` | 节点参数 |
| `DAPyWorkFlowScene.h/.cpp` | 场景管理 — QGraphicsScene 编排 |
| `DAPyWorkFlowManager.h/.cpp` | 信号管理 — Qt 信号发射（QObject） |
| `DAPySignalManager.h/.cpp` | 信号桥接 — Python 回调 → Qt 信号 |
| `DAPyWorkFlowExecutor.h/.cpp` | 执行器代理 |
| `DAPyWorkFlowSceneSerializer.h/.cpp` | 场景布局序列化 |
| `DAPyWorkFlowSerializer.h/.cpp` | 工作流序列化 |
| `DAPyNodeMetaData.h/.cpp` | 节点元数据 C++ 结构体 |
| `DAPyNodeStyle.h/.cpp` | 节点样式 C++ 结构体 |
| `DAPyNodePalette.h/.cpp` | 节点颜色管理 |
| `DAPyNodeState.h` | 节点状态枚举 |
| `DAPyExecutorState.h` | 执行器状态枚举 |
| `DAPyWorkFlowState.h` | 工作流状态枚举 |
| `DAPyWorkFlowUndoCommands.h/.cpp` | 撤销/重做命令 |
| `DAPyWorkFlowCommandsFactory.h/.cpp` | 撤销命令创建工厂 |
| `DAPyPainterProxy.h/.cpp` | 代理 Python 侧绘图功能 |
| `DAPyWorkFlowEnumStringUtils.h/.cpp` | 枚举↔字符串映射 |
| `DAPyModuleWorkflow.h/.cpp` | 模块包装器 |
| `DAPyWorkFlowAPI.h` | API 导出宏定义 |
| `PythonBinding/` | pybind11 模块导出（供插件开发者使用） |

## 十一、构建

```powershell
# 单独编译本模块
.\scripts\build.ps1 -Target DAPyWorkFlow

# 编译并运行测试
.\scripts\build.ps1 -Target DAPyWorkFlow -Test
```

## 十二、反模式（本模块常见错误）

| 反模式 | 正确做法 |
|--------|---------|
| 在代理类中手写 `pyObjectToVariant()` 等转换函数 | 使用 `pybind11::cast<QVariant>()` / `pybind11::cast<QVariantHash>()` |
| 用 `QJsonObject` 做 C++↔Python 运行时数据传递 | 用 `QVariantHash`，仅在文件序列化时用 JSON |
| 遗漏 `#include "DAPybind11QtCaster.hpp"` | 凡是 `.cpp` 中用到 `cast<QString>` 等必须包含 |
| 代理类继承 `QObject` 或添加 `Q_SIGNALS` | 信号放在 `DAPyWorkFlowManager` / `DAPySignalManager` 中 |
| 缓存 Python 属性到 C++ 成员变量 | 每次通过 `attr()` 实时读取 |
| 在本模块 `.h` 中定义类型转换函数 | 通用转换加到 `DAPybind11QtCaster.hpp`，私用转换放 `.cpp` 匿名命名空间 |
| Python 端引用 C++ 导出的 `da_xxx` 模块 | Python 逻辑层仅依赖标准库和第三方库 |
| C++ 端导出内容与 DAWorkFlowPy 耦合 | C++ 导出内容独立于 DAWorkFlowPy，供插件二次开发使用 |

---

## 十三、参考项目

| 项目 | 核心借鉴点 |
|------|-----------|
| KNIME | Node Triad（Model/View/Dialog 分离）；Proxy Pattern 跨语言桥接；Extension Point 插件机制 |
| Blender | DNA/RNA 代理架构；Python 定义节点，C++ 纯执行；序列化由数据所有者负责 |
| ComfyUI | `@NodeDef` 装饰器模式；Registry Pattern 自动发现；custom_nodes drop-in 插件 |
| NodeGraphQt | 多端口节点编辑器参考（UI+数据一体化 vs 本项目的分离架构） |
| Qt NodeEditor | C++ QGraphicsScene 渲染 + Python 代理；pybind11 proxy pattern |
| DAPyDataFrame（本项目） | `DAPyObjectWrapper::attr()` 纯代理模式；try/catch 安全默认值；不继承 QObject |
