# C++ 层集成与桥接

本文档介绍 DAPyWorkFlow 的 C++ 层桥接机制，说明 C++ 如何将 Python-first 架构与 Qt GUI 集成，实现工作流的可视化渲染和执行调度。

## 导航

本系列文档包含以下章节：

- [DAPyWorkFlow 模块概述](workflow-overview.md)
- [Python 节点开发指南](workflow-python-node-dev.md)
- [工作流生命周期](workflow-lifecycle.md)
- [C++ 层集成与桥接](workflow-cpp-integration.md) ← 当前页
- [场景操作指南](workflow-scene-operation.md)

## 概述

DAPyWorkFlow 采用 Python-first 架构设计，节点逻辑完全由 Python 定义，C++ 层仅作为桥接层负责以下职责：

- **可视化渲染**：通过 `DAPyNodeGraphicsItem` 将 Python 节点渲染到 Qt 场景
- **交互编辑**：处理用户拖拽、连接、属性编辑等 GUI 操作
- **执行调度**：通过 `DAPyWorkFlowExecuter` 协调 Python 执行器，管理执行状态
- **数据桥接**：在 Qt 类型（`QJsonObject`）和 Python 类型（`py::dict`）之间转换

C++ 层不实现任何节点业务逻辑，所有节点执行都代理给 Python 层的 `DAWorkflowExecutor`。

## pybind11 桥接机制

### da_py_workflow Python 模块

C++ 层通过 pybind11 导出 `da_py_workflow` Python 模块，暴露以下类和函数供 Python 调用：

| Python 类/函数 | C++ 对应类型 | 说明 |
|---------------|-------------|------|
| `DAPyWorkFlowScene` | `DA::DAPyWorkFlowScene` | 工作流场景管理类 |
| `DAPyNodeProxy` | `DA::DAPyNodeProxy` | Python 节点的 C++ 代理 |
| `DAPyNodeState` | `DA::DAPyNodeState` | 节点状态枚举 |
| `DAPyLinkPoint` | `DA::DAPyLinkPoint` | 连接点描述结构 |
| `DAPyNodeMetaData` | `DA::DAPyNodeMetaData` | 节点元数据结构 |
| `DAPyPainterProxy` | `DA::DAPyPainterProxy` | QPainter 绘制代理 |
| `getNodeProxy()` | `DA::getNodeProxy()` | 获取节点代理的全局函数 |

### DAPyModuleWorkflow 单例

`DAPyModuleWorkflow` 是 `DAWorkFlowPy` Python 包的 C++ 封装，采用单例模式管理：

```cpp
// 获取单例实例
DA::DAPyModuleWorkflow& workflowMod = DA::DAPyModuleWorkflow::getInstance();

// 导入 Python 模块
if (workflowMod.import()) {
    // 获取缓存的 Python 类引用
    pybind11::object workflowClass = workflowMod.getWorkflowClass();
    pybind11::object registryClass = workflowMod.getNodeRegistryClass();
}
```

该类缓存了常用的 Python 类引用（`DAWorkflow`、`DANodeRegistry`、`NodeDef`），避免重复导入带来的性能开销。

### DAPythonSignalHandler 回调机制

`DAPythonSignalHandler` 实现 Python 线程到 Qt 主线程的安全回调：

```cpp
// Python 代码调用此函数，请求在主线程执行
void callInMainThread(std::function<void()> func);
```

当 Python 节点需要更新 C++ 层状态时（如执行进度、状态变更），通过此机制安全地回到 Qt 主线程执行回调，避免跨线程操作 GUI 导致的崩溃。

### DAPyJsonCast JSON 序列化

`DAPyJsonCast` 提供 `QJsonObject` 与 `py::dict` 的双向转换：

| 函数 | 功能 |
|------|------|
| `qjsonObjectToPyDict()` | `QJsonObject` → `py::dict` |
| `pyDictToQJsonObject()` | `py::dict` → `QJsonObject` |
| `qjsonArrayToPyList()` | `QJsonArray` → `py::list` |
| `pyListToQJsonArray()` | `py::list` → `QJsonArray` |

此转换用于节点配置参数的传递（C++ 属性面板 ↔ Python 节点配置）。

## 核心桥接类

### DAPyNodeProxy

`DAPyNodeProxy` 是 Python 节点在 C++ 层的代理，持有 `pybind11::object` 引用：

```cpp
class DAPyNodeProxy
{
public:
    // 执行节点（自动管理 GIL）
    bool exec();

    // Python 对象引用管理
    void setPyNodeRef(const pybind11::object& pyNode);
    pybind11::object getPyNodeRef() const;

    // 数据传递
    void setPyInputData(const QString& key, const pybind11::object& data);
    pybind11::object getPyOutputData(const QString& key) const;

    // 配置参数
    bool setConfig(const QJsonObject& config);
    QJsonObject getConfig() const;

    // 状态管理
    DAPyNodeState getNodeState() const;
    void setNodeState(DAPyNodeState state);

    // 节点元数据（从 Python 描述符获取）
    QString getNodeName() const;
    QString getNodePrototype() const;
    QList<QString> getInputKeys() const;
    QList<QString> getOutputKeys() const;
    QJsonObject getDescriptor() const;
};
```

**执行流程**：

1. `exec()` 被调用时，首先通过 `DAPyGILGuard` 获取 GIL
2. 调用 Python 节点的 `execute()` 方法
3. 捕获 `error_already_set` 异常并转换为错误信息
4. 释放 GIL，返回执行结果

**状态管理**：

- `DAPyNodeState` 枚举跟踪节点状态（`Idle`、`Waiting`、`Running`、`Success`、`Error`、`Skipped`）
- 状态变更通过 `DAPythonSignalHandler::callInMainThread` 同步到 C++ 层

### DAPyNodeFactory

`DAPyNodeFactory` 负责发现和创建 Python 节点：

```cpp
class DAPyNodeFactory : public QObject
{
    Q_OBJECT

public:
    // 发现 Python 节点（调用 DANodeRegistry.discover）
    bool discoverNodes(const QStringList& scanPaths, bool useEntryPoints);

    // 创建节点代理实例
    DAPyNodeProxy* createNodeProxy(const QString& qualifiedName);

    // 获取已发现节点的元数据
    QList<DAPyNodeMetaData> getNodeMetadataList() const;

Q_SIGNALS:
    // 节点发现完成信号
    void nodeDiscovered(const QList<DA::DAPyNodeMetaData>& metadataList);
};
```

**节点发现流程**：

1. `discoverNodes()` 调用 Python 层的 `DANodeRegistry.discover()`
2. 扫描指定目录和 entry_points 中的节点类
3. 将 `DANodeDescriptor` 转换为 C++ 的 `DAPyNodeMetaData`
4. 发射 `nodeDiscovered` 信号通知 UI 更新节点面板

## GIL 线程安全（重点）

### DAPyGILGuard RAII 守卫

`DAPyGILGuard` 是 GIL 获取的 RAII 封装：

```cpp
{
    DA::DAPyGILGuard gil;  // 构造时获取 GIL
    // 调用 Python API...
}  // 析构时自动释放 GIL
```

**核心方法**：

- `isAcquired()`：判断 GIL 是否已被获取
- `release()`：主动释放 GIL（析构时不再重复释放）

### DAPyGILRelease 临时释放

`DAPyGILRelease` 用于在持有 GIL 时临时释放，常用于发射 Qt 信号：

```cpp
{
    DA::DAPyGILGuard gil;  // 获取 GIL
    // 执行 Python 代码...
    {
        DA::DAPyGILRelease release;  // 临时释放 GIL
        emit someSignal();            // 安全发射 Qt 信号
    }  // 重新获取 GIL
    // 继续 Python 操作...
}
```

### 关键安全规则

!!! danger "死锁风险警告"
    以下规则必须严格遵守，否则会导致程序死锁。

**规则 1：禁止在持有 GIL 时调用 QThread::wait()**

```cpp
// 错误示例：会导致死锁
void someFunction() {
    DA::DAPyGILGuard gil;
    // ... Python 操作
    m_thread->wait();  // 死锁！GIL 未释放，Python 线程无法执行
}

// 正确做法：先释放 GIL
void someFunction() {
    {
        DA::DAPyGILGuard gil;
        // ... Python 操作
    }  // 释放 GIL
    m_thread->wait();  // 安全等待
}
```

**规则 2：error_already_set 必须在 GIL 作用域内处理**

```cpp
// 错误示例：异常析构时尝试获取 GIL 导致死锁
void callPython() {
    try {
        pybind11::gil_scoped_acquire gil;
        pyFunc();
    } catch (const pybind11::error_already_set& e) {
        // 离开作用域后 GIL 已释放
        // e.what() 尝试获取 GIL 输出 Python 异常信息 → 死锁
        QString err = QString::fromStdString(e.what());
    }
}

// 正确做法：在 GIL 作用域内消费异常
void callPython() {
    pybind11::gil_scoped_acquire gil;
    try {
        pyFunc();
    } catch (const pybind11::error_already_set& e) {
        QString err = QString::fromStdString(e.what());  // GIL 仍持有
    }
}
```

**规则 3：Python→C++ 回调必须通过 callInMainThread**

```cpp
// Python 代码调用此 C++ 函数
void pythonCallback() {
    // 此时处于 Python 线程，持有 GIL
    // 直接操作 Qt GUI 会导致崩溃

    // 正确做法：通过 SignalHandler 回到主线程
    m_signalHandler->callInMainThread([]() {
        // 现在在 Qt 主线程，可以安全操作 GUI
        m_label->setText("Updated");
    });
}
```

## 数据流桥接

### Python→C++ 数据路径

当 Python 节点执行完成后，输出数据通过以下路径传递到 C++ 层：

1. Python `execute()` 返回 `dict` 包含输出数据
2. C++ `DAPyNodeProxy::exec()` 获取返回值
3. `getPyOutputData()` 提取特定 key 的数据
4. 数据通过 `DASignalManager` 传播到下游节点

### C++→Python 数据路径

当 C++ 层设置节点输入数据时：

1. C++ 调用 `DAPyNodeProxy::setPyInputData(key, data)`
2. `DAPyGILGuard` 获取 GIL
3. 数据通过 `pybind11::object` 传递给 Python 节点
4. Python 节点在 `execute()` 中通过 `inputs[key]` 访问

### QJsonObject ↔ py::dict 转换

节点配置参数（如参数面板设置）通过 JSON 转换传递：

```cpp
// C++ 层设置配置
QJsonObject config;
config["column"] = "temperature";
config["threshold"] = 25.0;
proxy->setConfig(config);

// 内部转换为 Python dict
pybind11::dict pyConfig = DA::PY::qjsonObjectToPyDict(config);
pyNode.attr("set_config")(pyConfig);
```

## 执行器桥接

### DAPyWorkFlowExecuter 与 DAWorkflowExecutor 协作

C++ 层的 `DAPyWorkFlowExecuter` 与 Python 层的 `DAWorkflowExecutor` 协作执行工作流：

| 组件 | 职责 |
|------|------|
| `DAPyWorkFlowExecuter` | C++ 执行调度器，管理执行线程，发射 Qt 信号 |
| `DAWorkflowExecutor` | Python 执行引擎，拓扑排序，节点执行逻辑 |
| `DASignalManager` | 数据传播引擎，事件驱动下游节点触发 |

### 执行状态信号

`DAPyWorkFlowExecuter` 提供以下 Qt 信号：

```cpp
// 节点执行完成
void nodeExecuteFinished(std::shared_ptr<DA::DAPyNodeProxy> nodeProxy, bool success);

// 工作流执行完成
void finished(bool success);

// 执行状态变更
void execStateChanged(DA::DAPyWorkFlowExecuter::ExecState oldState,
                      DA::DAPyWorkFlowExecuter::ExecState newState);

// 执行进度
void progressChanged(int current, int total);
```

### ExecState 状态机

```cpp
enum ExecState {
    StateIdle = 0,      // 空闲，未开始执行
    StateRunning = 1,   // 运行中
    StatePaused = 2,    // 已暂停
    StateError = 3,     // 执行出错
    StateFinished = 4   // 执行完成
};
```

## 自定义绘制桥接

### DAPyPainterProxy

`DAPyPainterProxy` 将 Qt 的 `QPainter` 基本操作暴露给 Python，使节点可以自定义视觉渲染：

```cpp
class DAPyPainterProxy
{
public:
    // 基本绘制操作
    void drawRect(qreal x, qreal y, qreal w, qreal h);
    void drawText(qreal x, qreal y, const std::string& text);
    void drawLine(qreal x1, qreal y1, qreal x2, qreal y2);
    void drawEllipse(qreal x, qreal y, qreal w, qreal h);
    void fillRect(qreal x, qreal y, qreal w, qreal h, int r, int g, int b, int a = 255);

    // 样式设置
    void setPenColor(int r, int g, int b, int a = 255);
    void setPenWidth(qreal width);
    void setBrushColor(int r, int g, int b, int a = 255);
    void setFont(const std::string& family, qreal size);
    void setNoPen();
    void setNoBrush();
};
```

### Python 节点自定义绘制

Python 节点可通过 `DANodeDescriptor` 的 `paint_callback` 字段提供绘制函数：

```python
@NodeDef(name="自定义节点")
class CustomNode:
    def paint(self, painter, body_rect):
        # painter 是 DAPyPainterProxy 实例
        x, y, w, h = body_rect
        painter.fillRect(x, y, w, h, 240, 240, 240)
        painter.setPenColor(0, 0, 0)
        painter.drawText(x + 5, y + 20, "Custom Node")
```

!!! note "绘制限制"
    当前仅支持基本绘制操作（`drawRect`、`drawText`、`drawLine`、`fillRect` 等）。复杂绘制（如 SVG、图片）待后续实现。

!!! warning "性能提示"
    `paint_callback` 应在 50ms 内完成绘制，避免阻塞 GUI 线程。如果回调抛出异常，节点会回退到默认矩形模板渲染。

## 注意事项

1. **不要直接继承旧 DAWorkFlow 类**：`DAAbstractNode`、`DAAbstractNodeGraphicsItem` 等旧模块类已不再使用

2. **GIL 管理优先使用 RAII**：优先使用 `DAPyGILGuard` 和 `DAPyGILRelease`，避免手动调用 `PyGILState_Ensure/Release`

3. **异常处理必须在 GIL 作用域内**：`error_already_set.what()` 必须在持有 GIL 时调用

4. **跨线程 GUI 操作必须通过 SignalHandler**：Python 线程直接操作 Qt GUI 会导致崩溃

5. **DAPyNodeProxy 不是 QObject**：不能使用信号槽机制，状态变更通过显式函数调用

6. **DAPyPainterProxy 仅在绘制期间有效**：不要在 `paint_callback` 外保存引用

## 参考资料

- Python 模块源码：`src/PyScripts/DAWorkbench/DAWorkFlowPy/`
- C++ 桥接源码：
  - `src/DAPyWorkFlow/DAPyNodeProxy.h` — 节点代理
  - `src/DAPyWorkFlow/DAPyNodeFactory.h` — 节点工厂
  - `src/DAPyWorkFlow/DAPyGILGuard.h` — GIL 管理
  - `src/DAPyWorkFlow/DAPyModuleWorkflow.h` — Python 模块封装
  - `src/DAPyWorkFlow/DAPyWorkFlowExecuter.h` — 执行调度器
  - `src/DAPyWorkFlow/DAPyPainterProxy.h` — 绘制代理
  - `src/DAPyWorkFlow/PythonBinding/DAPyWorkFlowPythonBinding.h` — pybind11 绑定
  - `src/DAPyBindQt/DAPythonSignalHandler.h` — 跨线程回调
  - `src/DAPyBindQt/DAPyJsonCast.h` — JSON 转换
- 相关文档：
  - [模块概述](workflow-overview.md) — 架构总览
  - [Python 节点开发指南](workflow-python-node-dev.md) — Python 节点开发
  - [工作流生命周期](workflow-lifecycle.md) — 生命周期管理
  - [场景操作指南](workflow-scene-operation.md) — 可视化场景操作
  - [C++ 调用 Python](../python-binding/cpp-calling-python.md) — pybind11 使用指南
