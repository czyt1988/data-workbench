# DAPyWorkFlow 模块 AI Agent 开发指南

Python 工作流节点的 C++ 渲染代理层。C++ 类继承 `DAPyObjectWrapper`，通过 `attr()` 转发调用到 Python，不做本地数据缓存。详细架构设计见 [README.md](README.md)。

---

## 模块职责

| 职责 | 说明 |
|------|------|
| **代理 Python 对象** | `DAPyNode`、`DAPyWorkFlow`、`DAPyNodeFactory` 等持有 `pybind11::object`，通过 `attr()` 访问 Python 属性和方法 |
| **渲染工作流场景** | `DAPyNodeGraphicsItem`、`DAPyLinkGraphicsItem`、`DAPyWorkFlowScene` 在 QGraphicsView 中渲染 |
| **信号桥接** | `DAPySignalManager` 将 Python 回调转为 Qt 信号 |

## 依赖关系

```
DAPyWorkFlow → DAPyBindQt（Python↔Qt 绑定层）
             → DAGraphicsView（图形视图框架）
             → DAUtils
```

本模块**不应依赖** DAGui、DAData、DAFigure 等上层模块。

---

## ⚠️ 核心规则：pybind11 类型转换

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

## 代理类编码规范

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

## 文件组织

| 文件 | 职责 |
|------|------|
| `DAPyNode.h/.cpp` | 节点代理 — 元数据读取、配置读写、输入输出数据访问 |
| `DAPyWorkFlow.h/.cpp` | 工作流代理 — DAG 操作转发 |
| `DAPyNodeFactory.h/.cpp` | 节点工厂代理 — 发现/创建节点 |
| `DAPyNodeGraphicsItem.h/.cpp` | 节点渲染 — QGraphicsItem 实现 |
| `DAPyLinkGraphicsItem.h/.cpp` | 连线渲染 |
| `DAPyWorkFlowScene.h/.cpp` | 场景管理 — QGraphicsScene 编排 |
| `DAPySignalManager.h/.cpp` | 信号桥接 — Python 回调 → Qt 信号 |
| `DAPyWorkFlowExecutor.h/.cpp` | 执行器代理 |
| `DAPyWorkFlowSceneSerializer.h/.cpp` | 场景布局序列化 |
| `DAPyNodeMetaData.h/.cpp` | 节点元数据 C++ 结构体 |
| `DAPyNodeStyle.h/.cpp` | 节点样式 C++ 结构体 |
| `PythonBinding/` | pybind11 模块导出（供插件开发者使用） |

## 构建

```powershell
# 单独编译本模块
.\scripts\build.ps1 -Target DAPyWorkFlow

# 编译并运行测试
.\scripts\build.ps1 -Target DAPyWorkFlow -Test
```

## 反模式（本模块常见错误）

| 反模式 | 正确做法 |
|--------|---------|
| 在代理类中手写 `pyObjectToVariant()` 等转换函数 | 使用 `pybind11::cast<QVariant>()` / `pybind11::cast<QVariantHash>()` |
| 用 `QJsonObject` 做 C++↔Python 运行时数据传递 | 用 `QVariantHash`，仅在文件序列化时用 JSON |
| 遗漏 `#include "DAPybind11QtCaster.hpp"` | 凡是 `.cpp` 中用到 `cast<QString>` 等必须包含 |
| 代理类继承 `QObject` 或添加 `Q_SIGNALS` | 信号放在 `DAPySignalManager` 中 |
| 缓存 Python 属性到 C++ 成员变量 | 每次通过 `attr()` 实时读取 |
| 在本模块 `.h` 中定义类型转换函数 | 通用转换加到 `DAPybind11QtCaster.hpp`，私用转换放 `.cpp` 匿名命名空间 |
