# Qt事件循环与Python asyncio桥接 + DAWorkFlowExecuter异步重构 架构设计文档

> **文档版本:** v1.0  
> **创建日期:** 2026-04-19  
> **关联任务:** Task 0.3 (Phase 1 前置架构设计)  
> **目标:** 为AI Agent集成提供Qt↔Python异步桥接、节点异步扩展、工作流引擎混合调度的完整架构方案

---

## 目录

1. [问题背景与关键阻塞点](#1-问题背景与关键阻塞点)
2. [Qt主线程与Python asyncio线程的桥接方案](#2-qt主线程与python-asyncio线程的桥接方案)
3. [DAAbstractNode异步扩展方案](#3-daabstractnode异步扩展方案)
4. [DAWorkFlowExecuter同步/异步混合调度引擎重构方案](#4-daworkflowexecuter同步异步混合调度引擎重构方案)
5. [crewAI同步调用模式集成方案](#5-crewai同步调用模式集成方案)
6. [LangGraph异步调用模式集成方案](#6-langgraph异步调用模式集成方案)
7. [DAPythonSignalHandler扩展方案](#7-dapythonsignalhandler扩展方案)
8. [线程安全与GIL管理策略](#8-线程安全与gil管理策略)
9. [类接口设计汇总](#9-类接口设计汇总)
10. [序列图](#10-序列图)

---

## 1. 问题背景与关键阻塞点

### 1.1 当前架构现状

项目当前的Python-C++交互架构基于 **pybind11嵌入模式**，核心组件如下：

| 组件 | 文件 | 当前行为 |
|------|------|----------|
| `DAAbstractNode` | `DAAbstractNode.h:209` | `virtual bool exec() = 0` — 纯同步，阻塞调用 |
| `DAWorkFlowExecuter` | `DAWorkFlowExecuter.h:12` | 在独立QThread中运行，`moveToThread`模式，DAG遍历+同步`exec()` |
| `DAPythonSignalHandler` | `DAPythonSignalHandler.h:18` | `callInMainThread()` — Python→Qt主线程安全回调，`Qt::QueuedConnection` |
| `DAPyInterpreter` | `DAPyInterpreter.h:27` | `pybind11::scoped_interpreter` — 全局Python解释器生命周期管理 |
| `DAPybind11InQt.h` | `DAPybind11InQt.h:6` | `#undef slots` → `#include pybind11` → `#define slots Q_SLOTS` — slots宏冲突解决 |
| `DAPybind11QtCaster.hpp` | `DAPybind11QtCaster.hpp` | QString/QDateTime/QList/QVariant等Qt-Python类型双向转换器 |

### 1.2 三大关键阻塞点 (Metis识别)

| 编号 | 阻塞点 | 核心矛盾 | 设计对策章节 |
|------|--------|----------|-------------|
| **R1** | GIL + asyncio线程安全 | Python GIL与asyncio事件循环在Qt主线程中无法共存 — asyncio要求独占线程的事件循环 | §2 双事件循环桥接 |
| **R3** | sync `exec()` vs async Agent | `DAAbstractNode::exec()` 是同步纯虚方法，Agent节点(crewAI/LangGraph)需要异步执行 | §3 异步节点扩展 |
| **R6** | async执行UI阻塞 | 异步Agent执行可能耗时数分钟，若在Qt主线程执行将冻结UI | §4 混合调度引擎 + §8 线程隔离 |

---

## 2. Qt主线程与Python asyncio线程的桥接方案

### 2.1 核心矛盾分析

**问题：** Python的`asyncio`事件循环必须在其所在线程中运行，且该线程在循环运行期间会被独占。Qt的主线程运行Qt事件循环(`QCoreApplication::exec()`)，两者互斥——不能在Qt主线程中运行`asyncio.run()`，否则Qt事件循环被阻塞，UI冻结。

**参考现有代码：** `DAPythonSignalHandler::callInMainThread()`（`DAPythonSignalHandler.cpp:24-62`）已建立了Python→Qt的安全通道，使用`Qt::QueuedConnection`跨线程投递。但反向通道(Qt→Python asyncio)尚未建立。

### 2.2 双事件循环桥接架构

设计一个专用的 **DAAsyncioBridge** 类，在独立的Python线程中运行asyncio事件循环，同时提供双向通信通道：

```
┌─────────────────────────────────┐     ┌──────────────────────────────────┐
│        Qt Main Thread           │     │     Python Asyncio Thread        │
│                                 │     │                                  │
│  QCoreApplication::exec()       │     │  asyncio.run(loop.run_forever()) │
│  ┌─────────────────────┐        │     │  ┌──────────────────────┐        │
│  │ DAWorkFlowExecuter  │        │     │  │ asyncio.EventLoop    │        │
│  │ (QThread worker)    │────┐   │     │  │                      │        │
│  └─────────────────────┘    │   │     │  │  - call_soon()       │        │
│                             │   │     │  │  - run_until_complete│        │
│  ┌─────────────────────┐    │   │     │  │  - create_task()     │        │
│  │ DAPythonSignalHandler│◄──┼───│─────│──│  bridge_callback     │        │
│  │ (Qt→Python回调)      │    │   │     │  └──────────────────────┘        │
│  └─────────────────────┘    │   │     │                                  │
│                             │   │     │  ┌──────────────────────┐        │
│  ┌─────────────────────┐    │   │     │  │ GIL Management       │        │
│  │ DAAsyncioBridge     │◄──┘───│─────│──│  (pybind11 gil_scoped)│        │
│  │ (双向桥接核心)      │        │     │  └──────────────────────┘        │
│  └─────────────────────┘        │     └──────────────────────────────────┘
└─────────────────────────────────┘
```

### 2.3 DAAsyncioBridge 类设计

```cpp
// 文件: src/DAPyBindQt/DAAsyncioBridge.h
namespace DA
{
/**
 * @brief Qt事件循环与Python asyncio事件循环的双向桥接器
 *
 * 在独立的Python线程中运行asyncio事件循环，
 * 提供Qt→asyncio和asyncio→Qt的双向安全通信通道。
 * 解决R1阻塞点：GIL+asyncio与Qt主线程的冲突。
 *
 * @code
 * // 使用示例
 * DAAsyncioBridge* bridge = DAAsyncioBridge::instance();
 * bridge->start();  // 启动asyncio线程
 * 
 * // 从Qt线程提交异步任务
 * bridge->submitAsyncTask(pythonCallable, args);
 * 
 * // 结果通过信号返回Qt线程
 * connect(bridge, &DAAsyncioBridge::asyncTaskFinished, ...);
 * @endcode
 *
 * @see DAPythonSignalHandler
 */
class DAPYBINDQT_API DAAsyncioBridge : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAsyncioBridge)
public:
    // 获取全局单例（生命周期由DAAppController管理）
    static DAAsyncioBridge* instance();

    explicit DAAsyncioBridge(QObject* parent = nullptr);
    ~DAAsyncioBridge();

    // 启动asyncio事件循环线程
    bool start();
    // 停止asyncio事件循环线程
    void stop();
    // 是否已启动
    bool isRunning() const;

    // 提交异步Python协程到asyncio事件循环（从Qt线程调用，线程安全）
    // pythonCallable可以是async函数或普通可调用对象
    int submitAsyncTask(const pybind11::object& pythonCallable,
                        const pybind11::args& args = pybind11::args());

    // 提交同步Python函数到asyncio线程（通过run_in_executor包装为协程）
    int submitSyncTask(const pybind11::object& pythonCallable,
                       const pybind11::args& args = pybind11::args());

    // 取消任务
    bool cancelTask(int taskId);

    // 在Python asyncio线程中执行回调（从Python线程调用，内部使用）
    // 此函数不需要GIL，因为调用者已在Python线程中
    void callInAsyncioThread(std::function<void()> func);

Q_SIGNALS:
    /**
     * @brief 异步任务完成信号（在Qt主线程中发射）
     * @param taskId 任务ID
     * @param result Python返回值（通过pybind11::object封装）
     * @param success 是否成功
     */
    void asyncTaskFinished(int taskId, const QVariant& result, bool success);

    /**
     * @brief 异步任务进度更新信号
     * @param taskId 任务ID
     * @param progress 进度百分比 (0-100)
     * @param message 进度消息
     */
    void asyncTaskProgress(int taskId, int progress, const QString& message);

    /**
     * @brief asyncio线程启动完成信号
     */
    void bridgeStarted();

    /**
     * @brief asyncio线程停止信号
     */
    void bridgeStopped();
};
}  // namespace DA
```

### 2.4 桥接实现原理

**Qt→asyncio方向：** 
- Qt线程通过`submitAsyncTask()`将Python callable和参数序列化
- 使用`QMetaObject::invokeMethod()`或`std::function`+`std::mutex`条件变量，将任务投递到asyncio线程
- asyncio线程中的桥接回调通过`asyncio.call_soon_threadsafe()`将任务注入事件循环
- 任务完成后，通过`DAPythonSignalHandler::callInMainThread()`将结果投递回Qt主线程

**asyncio→Qt方向：**
- 已有基础设施：`DAPythonSignalHandler::callInMainThread()`（`DAPythonSignalHandler.cpp:24-62`）
- asyncio线程中的Python代码需要操作Qt对象时，调用此函数
- 该函数使用`Qt::QueuedConnection`信号安全地将操作投递到Qt主线程

**关键设计决策：**
- **不将asyncio运行在Qt主线程** — 这会导致Qt事件循环阻塞（已明确禁止）
- **asyncio线程是独立线程** — 与当前`DAWorkFlowExecuter`使用的`moveToThread`模式一致
- **所有Python操作在同一线程** — 避免GIL争用，asyncio线程持有GIL时执行Python代码

---

## 3. DAAbstractNode异步扩展方案

### 3.1 核心矛盾分析

**问题：** `DAAbstractNode::exec()` 是纯虚同步方法（`DAAbstractNode.h:209`），返回`bool`。Agent节点需要异步执行：
- crewAI的`crew.kickoff()`虽然同步，但可能耗时数分钟
- LangGraph的`graph.astream()`是async协程，必须运行在asyncio环境中

**不能修改`exec()`签名**的原因：
1. 现有大量节点已实现`exec()`（DAStandardNodeConstValue、DAStandardNodeInputOutput等）
2. 修改签名将破坏所有现有节点的兼容性
3. 工作流引擎`DAWorkFlowExecuter::executeNode()`（`DAWorkFlowExecuter.cpp:319-330`）依赖`exec()`的同步语义

### 3.2 DAAbstractNodeAsync 扩展类设计

采用**继承扩展**而非**修改基类**的策略，引入新的异步节点基类：

```cpp
// 文件: src/DAWorkFlow/DAAbstractNodeAsync.h
namespace DA
{
/**
 * @brief 异步节点执行状态枚举
 */
enum class DAAsyncNodeState
{
    Idle,        ///< 空闲，等待执行
    Running,     ///< 正在异步执行中
    Completed,   ///< 执行完成（成功）
    Failed,      ///< 执行失败
    Cancelled    ///< 被取消
};

/**
 * @brief 支持异步执行的节点基类
 *
 * 继承DAAbstractNode，通过状态机管理异步执行生命周期，
 * 解决R3阻塞点：sync exec() vs async Agent。
 *
 * 设计原则：
 * 1. exec()保持同步语义 — 快速启动异步任务后返回true
 * 2. 异步结果通过信号通知 — DAWorkFlowExecuter不再阻塞等待
 * 3. 状态机管理执行生命周期 — Running→Completed/Failed/Cancelled
 *
 * @code
 * class DACrewAINode : public DAAbstractNodeAsync
 * {
 * protected:
 *     bool exec() override
 *     {
 *         // 快速启动异步任务，不阻塞
 *         startAsyncExecution();
 *         return true;  // 启动成功即返回true
 *     }
 *     
 *     void execAsync() override
 *     {
 *         // 在asyncio线程中执行crew.kickoff()
 *         // 结果通过setAsyncResult()返回
 *     }
 * };
 * @endcode
 *
 * @see DAAbstractNode, DAWorkFlowExecuter
 */
class DAWORKFLOW_API DAAbstractNodeAsync : public DAAbstractNode
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAbstractNodeAsync)
public:
    DAAbstractNodeAsync();
    virtual ~DAAbstractNodeAsync();

    // 获取当前异步执行状态
    DAAsyncNodeState asyncState() const;

    // 是否为异步节点（默认true，可覆盖）
    virtual bool isAsyncNode() const;

    // 同步exec()实现 — 启动异步任务后立即返回
    // 子类不应重写此方法，应重写execAsync()
    bool exec() override;

    // 异步执行的核心虚方法（在asyncio线程中执行）
    // 子类必须实现此方法完成异步逻辑
    virtual void execAsync() = 0;

    // 取消异步执行
    virtual void cancelAsync();

Q_SIGNALS:
    /**
     * @brief 异步执行完成信号（在Qt主线程中发射）
     * @param node 异步节点指针
     * @param success 是否成功
     * @param result 异步执行结果（QVariant封装）
     */
    void asyncFinished(DAAbstractNodeAsync* node, bool success, const QVariant& result);

    /**
     * @brief 异步执行进度信号
     * @param node 异步节点指针
     * @param progress 进度百分比 (0-100)
     * @param message 进度描述消息
     */
    void asyncProgress(DAAbstractNodeAsync* node, int progress, const QString& message);
};
}  // namespace DA
```

### 3.3 异步状态机设计

```
                    ┌───────┐
         exec()调用 │ Idle  │
                    └───┬───┘
                        │ startAsyncExecution()
                        ▼
                    ┌───────┐
                    │Running│  ←── execAsync()在asyncio线程执行
                    └───┬───┘
                        │
            ┌───────────┼───────────┐
            │           │           │
            ▼           ▼           ▼
        ┌────────┐  ┌────────┐  ┌──────────┐
        │Completed│ │ Failed │  │Cancelled │
        └─────────┘ └────────┘  └──────────┘
            │           │           │
            └───────────┼───────────┘
                        │
                        ▼
                  emit asyncFinished()
                  → DAWorkFlowExecuter继续DAG遍历
```

**关键设计决策：**
- `exec()`快速返回`true`（表示"启动成功"），不等待异步结果
- `DAWorkFlowExecuter`在遇到异步节点时暂停DAG遍历，等待`asyncFinished`信号
- 异步完成后恢复DAG遍历，继续执行下游节点

### 3.4 exec()默认实现逻辑

```cpp
// DAAbstractNodeAsync.cpp (示意)
bool DAAbstractNodeAsync::exec()
{
    DA_D(d);
    if (d->mAsyncState != DAAsyncNodeState::Idle) {
        return false;  // 防止重复执行
    }
    d->mAsyncState = DAAsyncNodeState::Running;
    
    // 将execAsync()投递到asyncio线程
    DAAsyncioBridge* bridge = DAAsyncioBridge::instance();
    if (!bridge || !bridge->isRunning()) {
        d->mAsyncState = DAAsyncNodeState::Failed;
        Q_EMIT asyncFinished(this, false, QVariant());
        return false;
    }
    
    // 通过bridge投递异步任务
    // execAsync()将在asyncio线程中执行
    bridge->submitNodeAsyncTask(this);
    
    return true;  // 启动成功，立即返回
}
```

---

## 4. DAWorkFlowExecuter同步/异步混合调度引擎重构方案

### 4.1 当前执行流程分析

基于`DAWorkFlowExecuter.cpp`（行210-280）的执行流程：

```
startExecute()
  → prepareStartExec()  // 查找GlobalNodes/IsolatedNodes/BeginNodes
  → 回调: CallbackPrepareStartExecute
  → 执行GlobalNodes (executeNodeNotTransmit)
  → 执行IsolatedNodes (executeNode)
  → 执行BeginNodes (executeNode)
  → 回调: CallbackPrepareEndExecute
  → emit finished(true)
```

`executeNode()`（行319-330）的同步逻辑：
```
executeNode(n)
  → n->exec()          // 同步阻塞
  → emit nodeExecuteFinished(n, state)
  → sendParam(n, outInfo)   // 传递输出参数到下游节点
  → transmit(outInfo)       // 检查下游节点入度，满足则执行
```

**核心矛盾：** `executeNode()`是完全同步的串行执行。当遇到`DAAbstractNodeAsync`节点时，`exec()`会立即返回`true`但异步结果尚未就绪，不能直接传递参数到下游。

### 4.2 混合调度引擎重构设计

重构`DAWorkFlowExecuter`为同步/异步混合调度引擎：

```cpp
// 文件: src/DAWorkFlow/DAWorkFlowExecuter.h (重构后)
namespace DA
{
class DAWORKFLOW_API DAWorkFlowExecuter : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAWorkFlowExecuter)
public:
    DAWorkFlowExecuter(QObject* p = nullptr);
    ~DAWorkFlowExecuter();

    // 设置查询的开始点
    void setStartNode(DAAbstractNode::SharedPointer n);
    // 设置workflow
    void setWorkFlow(DAWorkFlow* wf);
    // 获取全局节点
    QList<DAAbstractNode::SharedPointer> getGlobalNodes() const;
    // 获取孤立节点
    QList<DAAbstractNode::SharedPointer> getIsolatedNodesNodes() const;
    // 判断是否在请求结束
    bool isTerminateRequest() const;

public Q_SLOTS:
    // 开始执行（兼容原有接口）
    void startExecute();
    // 请求终止
    void terminateRequest();
    // 单独执行某个节点
    void executeNode(DAAbstractNode::SharedPointer n);

private Q_SLOTS:
    // 执行节点但不传递（全局节点）
    void executeNodeNotTransmit(DAAbstractNode::SharedPointer n);
    // 异步节点完成回调
    void onAsyncNodeFinished(DAAbstractNodeAsync* node, bool success, const QVariant& result);

Q_SIGNALS:
    /**
     * @brief 节点执行完成返回的结果
     * @param n 节点指针
     * @param state 执行状态
     */
    void nodeExecuteFinished(DAAbstractNode::SharedPointer n, bool state);
    /**
     * @brief 完成执行发射此信号
     */
    void finished(bool success);

private:
    // 内部执行节点（区分同步/异步）
    void executeNodeInternal(DAAbstractNode::SharedPointer n);
    // 处理同步节点执行
    void executeSyncNode(DAAbstractNode::SharedPointer n);
    // 处理异步节点执行（不阻塞，等待信号回调）
    void executeAsyncNode(DAAbstractNode::SharedPointer n);
    // 等待的异步节点集合（DAG遍历暂停点）
    // QMap<节点指针, 等待的下游节点列表>
};
}  // namespace DA
```

### 4.3 混合调度执行流程

**重构后的executeNodeInternal()逻辑：**

```cpp
void DAWorkFlowExecuter::executeNodeInternal(DAAbstractNode::SharedPointer n)
{
    // 判断是否为异步节点
    DAAbstractNodeAsync* asyncNode = dynamic_cast<DAAbstractNodeAsync*>(n.get());
    if (asyncNode && asyncNode->isAsyncNode()) {
        executeAsyncNode(n);  // 不阻塞，等待信号
    } else {
        executeSyncNode(n);   // 原有同步逻辑
    }
}

void DAWorkFlowExecuter::executeSyncNode(DAAbstractNode::SharedPointer n)
{
    // 与原逻辑相同
    bool state = n->exec();
    Q_EMIT nodeExecuteFinished(n, state);
    QList<DAAbstractNode::LinkInfo> outInfo = n->getAllOutputLinkInfo();
    d_ptr->sendParam(n, outInfo);
    d_ptr->transmit(outInfo);
}

void DAWorkFlowExecuter::executeAsyncNode(DAAbstractNode::SharedPointer n)
{
    // 异步节点：exec()启动异步任务后立即返回
    bool state = n->exec();
    Q_EMIT nodeExecuteFinished(n, state);  // "启动成功"信号
    
    // 不立即传递参数和触发下游
    // 将此节点加入等待集合
    d_ptr->mPendingAsyncNodes.insert(n);
}

void DAWorkFlowExecuter::onAsyncNodeFinished(DAAbstractNodeAsync* node, bool success, const QVariant& result)
{
    DAAbstractNode::SharedPointer n = node->pointer();
    
    // 从等待集合中移除
    d_ptr->mPendingAsyncNodes.remove(n);
    
    // 设置节点输出数据
    if (success && result.isValid()) {
        // 将asyncResult写入节点的输出端口
        // 子类在execAsync()中通过setOutputData()设置
    }
    
    // 现在可以传递参数和触发下游节点（与同步节点相同逻辑）
    QList<DAAbstractNode::LinkInfo> outInfo = n->getAllOutputLinkInfo();
    d_ptr->sendParam(n, outInfo);
    d_ptr->transmit(outInfo);
    
    // 检查是否所有异步节点都已完成
    if (d_ptr->mPendingAsyncNodes.isEmpty()) {
        // 继续执行后续逻辑或发射finished
        continueExecutionAfterAsyncNodes();
    }
}
```

### 4.4 DAG遍历的暂停与恢复机制

```
startExecute()
  → prepareStartExec()
  → 执行GlobalNodes (同步)
  → 执行IsolatedNodes
      → 遍历每个节点: executeNodeInternal(n)
          → 同步节点: 立即执行 + 传递
          → 异步节点: 启动 + 加入等待集合 → 暂停下游遍历
  → 执行BeginNodes
      → 同上
  → 如果存在等待中的异步节点: 不发射finished，等待onAsyncNodeFinished
  → 所有异步节点完成后:
      → 回调: CallbackPrepareEndExecute
      → emit finished(true)
```

**关键设计决策：**
- 不改变`startExecute()`的入口接口，保持与`DAWorkFlow::exec()`的兼容性
- 异步节点的下游遍历暂停通过`mPendingAsyncNodes`集合管理
- 信号连接：`DAAbstractNodeAsync::asyncFinished` → `DAWorkFlowExecuter::onAsyncNodeFinished`
- 需要跨线程信号连接（异步节点在asyncio线程发射信号，DAWorkFlowExecuter在worker线程接收）

### 4.5 跨线程信号连接策略

```cpp
// DAWorkFlowExecuter设置异步节点信号连接
void DAWorkFlowExecuter::executeAsyncNode(DAAbstractNode::SharedPointer n)
{
    DAAbstractNodeAsync* asyncNode = dynamic_cast<DAAbstractNodeAsync*>(n.get());
    
    // 使用Qt::QueuedConnection确保跨线程安全
    // asyncio线程 → Qt worker线程
    connect(asyncNode, &DAAbstractNodeAsync::asyncFinished,
            this, &DAWorkFlowExecuter::onAsyncNodeFinished,
            Qt::QueuedConnection);
    
    // ... 启动异步执行 ...
}
```

**参考现有代码：** `DAPythonSignalHandler`使用同样的`Qt::QueuedConnection`模式（`DAPythonSignalHandler.cpp:13`），此方案与之完全一致。

---

## 5. crewAI同步调用模式集成方案

### 5.1 crewAI调用特性

**crewAI的`crew.kickoff()`是同步方法** — 直接返回`CrewOutput`对象，不需要asyncio环境。但有以下挑战：
1. 执行时间可能很长（数分钟到数十分钟）
2. 需要Python ≥ 3.10环境
3. 需要GIL持有期间执行Python代码
4. 需要将`CrewOutput`结果转换回C++数据结构

### 5.2 DACrewAINode 设计

```cpp
// 文件: src/DAWorkFlow/StandardNodes/DACrewAINode.h (规划)
namespace DA
{
/**
 * @brief crewAI集成节点
 *
 * 封装crewAI的crew.kickoff()调用，将其包装为异步节点。
 * 虽然crew.kickoff()是同步的，但执行时间较长，
 * 因此通过DAAsyncioBridge的submitSyncTask()包装，
 * 在asyncio线程的run_in_executor中执行，避免阻塞Qt线程。
 *
 * @code
 * // Python侧配置
 * from crewai import Crew, Agent, Task
 * crew = Crew(agents=[...], tasks=[...])
 * 
 * // C++节点配置
 * DACrewAINode* node = new DACrewAINode();
 * node->setCrewConfig(crewConfig);  // 通过pybind11传递Python对象
 * @endcode
 *
 * @see DAAbstractNodeAsync, DAAsyncioBridge
 */
class DACrewAINode : public DAAbstractNodeAsync
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DACrewAINode)
public:
    DACrewAINode();
    virtual ~DACrewAINode();

    // 设置crew配置（Python dict或JSON字符串）
    void setCrewConfig(const QString& jsonConfig);
    // 获取crew配置
    QString getCrewConfig() const;

    // 获取crew执行结果
    QVariant getCrewResult() const;

    // 异步节点标识
    bool isAsyncNode() const override;

protected:
    // execAsync实现 — 在asyncio线程的run_in_executor中调用crew.kickoff()
    void execAsync() override;

    // 创建GraphicsItem
    DAAbstractNodeGraphicsItem* createGraphicsItem() override;
};
}  // namespace DA
```

### 5.3 crewAI执行序列

```
DACrewAINode::exec()
  │
  ├─ 设置asyncState = Running
  ├─ 通过DAAsyncioBridge::submitSyncTask()投递
  │   │
  │   └─ asyncio线程: loop.run_in_executor(None, crew.kickoff_wrapper)
  │       │
  │       ├─ 获取GIL (pybind11::gil_scoped_acquire)
  │       ├─ 调用 crew.kickoff() ←── 同步阻塞，但在asyncio线程中
  │       ├─ 获取CrewOutput结果
  │       ├─ 将结果转换为QVariant (通过DAPybind11QtCaster)
  │       └─ 释放GIL
  │   │
  │   └─ 完成回调 → DAPythonSignalHandler::callInMainThread()
  │       │
  │       └─ Qt主线程: emit asyncFinished(node, success, result)
  │           │
  │           └─ DAWorkFlowExecuter::onAsyncNodeFinished() 恢复DAG遍历
```

**关键设计决策：**
- `crew.kickoff()`是同步的，但包装为`submitSyncTask()`通过asyncio的`run_in_executor`在独立线程池执行
- 这样asyncio事件循环不会被阻塞 — `run_in_executor`使用默认线程池
- GIL管理：crew.kickoff()需要持有GIL，通过`pybind11::gil_scoped_acquire`确保

---

## 6. LangGraph异步调用模式集成方案

### 6.1 LangGraph调用特性

**LangGraph的`graph.astream()`是异步协程** — 必须运行在asyncio事件循环中，且支持流式输出（逐步返回中间结果）。这是最复杂的集成场景：
1. 必须在asyncio环境中执行
2. 需要流式处理中间结果（streaming）
3. 每个步骤的输出需要实时反馈到Qt UI
4. Python ≥ 3.10要求

### 6.2 DALangGraphNode 设计

```cpp
// 文件: src/DAWorkFlow/StandardNodes/DALangGraphNode.h (规划)
namespace DA
{
/**
 * @brief LangGraph集成节点
 *
 * 封装LangGraph的graph.astream()异步调用，
 * 支持流式输出逐步反馈到Qt UI。
 *
 * LangGraph的异步特性需要通过DAAsyncioBridge直接
 * 提交async协程，而非通过run_in_executor包装。
 *
 * @code
 * // Python侧配置
 * from langgraph.graph import StateGraph
 * graph = StateGraph(...)
 * compiled = graph.compile()
 * 
 * // C++节点配置
 * DALangGraphNode* node = new DALangGraphNode();
 * node->setGraphConfig(graphConfig);
 * @endcode
 *
 * @see DAAbstractNodeAsync, DAAsyncioBridge
 */
class DALangGraphNode : public DAAbstractNodeAsync
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DALangGraphNode)
public:
    DALangGraphNode();
    virtual ~DALangGraphNode();

    // 设置LangGraph配置
    void setGraphConfig(const QString& jsonConfig);
    // 获取配置
    QString getGraphConfig() const;

    // 获取最终结果
    QVariant getGraphResult() const;

    // 获取流式步骤结果列表
    QList<QVariant> getStreamSteps() const;

    // 异步节点标识
    bool isAsyncNode() const override;

protected:
    // execAsync实现 — 直接提交async协程到asyncio事件循环
    void execAsync() override;

    // 创建GraphicsItem
    DAAbstractNodeGraphicsItem* createGraphicsItem() override;
};
}  // namespace DA
```

### 6.3 LangGraph流式输出处理

LangGraph的`astream()`是async generator，每一步产出中间状态。需要在asyncio线程中逐步消费，并通过桥接传递到Qt线程：

```
DALangGraphNode::execAsync()
  │
  └─ asyncio线程: 
      │
      ├─ 获取GIL (pybind11::gil_scoped_acquire)
      ├─ import langgraph
      ├─ 构建 graph 并 compile
      ├─ for step in await compiled.astream(input_state):
      │   │
      │   ├─ 解析step结果 → QVariant
      │   ├─ 通过DAAsyncioBridge::callInMainThread()投递进度
      │   │   └─ Qt主线程: emit asyncProgress(node, stepIndex, stepDesc)
      │   │       └─ UI更新进度显示
      │   │
      │   ├─ 继续消费下一个step
      │   └─ 释放GIL（短暂释放，让其他Python操作有机会执行）
      │
      └─ 最终结果 → QVariant
      └─ 通过DAAsyncioBridge投递完成信号
          └─ Qt主线程: emit asyncFinished(node, success, finalResult)
```

### 6.4 流式回调桥接实现

需要在DAAsyncioBridge中增加流式回调支持：

```cpp
// DAAsyncioBridge新增方法
/**
 * @brief 注册流式回调，用于LangGraph等async generator的逐步输出
 * @param taskId 任务ID
 * @param callback 每步完成时的回调函数（在Qt主线程执行）
 */
void registerStreamCallback(int taskId, std::function<void(const QVariant&)> callback);
```

**Python侧桥接函数**（将添加到`da_app`或新建`da_agent` PYBIND11_EMBEDDED_MODULE）：

```python
# _da_asyncio_bridge.py (嵌入到Python环境中)
import asyncio
from typing import Any, Callable

_da_bridge_callback = None  # 由C++侧设置

def set_bridge_callback(cb: Callable):
    """设置C++侧的回调函数"""
    global _da_bridge_callback
    _da_bridge_callback = cb

async def run_langgraph_stream(graph_callable, input_state, step_callback=None):
    """
    执行LangGraph的astream()，逐步通过回调传递结果到C++侧
    """
    compiled = graph_callable()
    results = []
    
    async for step in compiled.astream(input_state):
        # 将step转换为可传递到C++的数据
        step_data = dict(step) if hasattr(step, '__iter__') else str(step)
        
        # 通过C++桥接回调传递进度
        if step_callback:
            step_callback(step_data)
        
        # 如果有C++侧回调
        if _da_bridge_callback:
            _da_bridge_callback(step_data)
        
        results.append(step_data)
    
    return results  # 最终结果

async def run_crewai_sync(crew_callable, inputs):
    """
    在asyncio的run_in_executor中执行crewAI的同步kickoff()
    """
    result = await asyncio.get_event_loop().run_in_executor(
        None,  # 使用默认线程池
        lambda: crew_callable().kickoff(inputs)
    )
    return result
```

---

## 7. DAPythonSignalHandler扩展方案

### 7.1 当前能力分析

`DAPythonSignalHandler`（`DAPythonSignalHandler.h:18`）当前提供：
- `callInMainThread(std::function<void()>)` — Python→Qt主线程的单向回调
- `Qt::QueuedConnection`信号机制 — 线程安全投递
- `FunctionWrapper` — 函数包装器+唯一ID映射
- `std::mutex` — 线程安全保护
- 生命周期管理 — `m_destroying`标志+`clearPendingFunctions()`

**不足之处：**
1. 只支持`std::function<void()>` — 无参数传递，无返回值
2. 只支持Python→Qt方向 — 缺少Qt→Python方向的回调
3. 不支持async回调 — 无法将async函数结果传递到Qt线程

### 7.2 扩展设计

保持`DAPythonSignalHandler`的核心职责不变（Python→Qt主线程回调），扩展为支持有参回调：

```cpp
// 文件: src/DAPyBindQt/DAPythonSignalHandler.h (扩展后)
namespace DA
{
class DAPYBINDQT_API DAPythonSignalHandler : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPythonSignalHandler)
public:
    explicit DAPythonSignalHandler(QObject* parent = nullptr);
    virtual ~DAPythonSignalHandler();

    DAPythonSignalHandler(const DAPythonSignalHandler&) = delete;
    DAPythonSignalHandler& operator=(const DAPythonSignalHandler&) = delete;

    // 原有：无参回调（保持向后兼容）
    void callInMainThread(std::function<void()> func);

    // 新增：带QVariant参数的回调
    void callInMainThreadWithData(std::function<void(const QVariant&)> func, const QVariant& data);

    // 新增：带QVariant结果返回的回调（同步等待）
    // 注意：此方法会阻塞调用线程直到主线程执行完成
    // 仅在非Qt主线程中调用，且需谨慎使用避免死锁
    QVariant callInMainThreadAndWait(std::function<QVariant()> func);

    // 清理所有待执行的函数
    void clearPendingFunctions();

Q_SIGNALS:
    // 原有信号
    void executeRequested(int funcWrapperId);

    // 新增：带数据的回调信号
    void executeWithDataRequested(int funcWrapperId, const QVariant& data);

private Q_SLOTS:
    void onExecuteRequested(int funcWrapperId);
    void onExecuteWithDataRequested(int funcWrapperId, const QVariant& data);
};
}  // namespace DA
```

### 7.3 async回调桥接策略

对于async回调（如LangGraph的`astream`逐步结果），不通过`DAPythonSignalHandler`直接处理，而是通过`DAAsyncioBridge`：

- **asyncio→Qt主线程**：`DAAsyncioBridge`内部使用`DAPythonSignalHandler::callInMainThreadWithData()`将async结果投递到Qt
- **Qt→asyncio**：通过`DAAsyncioBridge::submitAsyncTask()`将Qt侧的Python调用投递到asyncio线程

这样`DAPythonSignalHandler`保持简洁的职责边界，复杂async桥接逻辑在`DAAsyncioBridge`中处理。

---

## 8. 线程安全与GIL管理策略

### 8.1 线程模型总览

```
┌──────────────────────────────────────────────────────────────────┐
│                        线程架构                                   │
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐│
│  │ Qt主线程      │  │ Qt Worker线程 │  │ Python Asyncio线程       ││
│  │              │  │              │  │                          ││
│  │ UI事件循环    │  │ DAWorkFlow   │  │ asyncio事件循环          ││
│  │ 渲染更新      │  │ Executer     │  │                          ││
│  │ 用户交互      │  │ (DAG遍历)    │  │ crewAI/LangGraph执行     ││
│  │              │  │              │  │ Python GIL持有            ││
│  └──────────────┘  └──────────────┘  └──────────────────────────┘│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │ 线程间通信通道                                                │ │
│  │                                                              │ │
│  │  Qt主线程 ←── DAPythonSignalHandler ←── Asyncio线程          │ │
│  │  Qt主线程 ←── DAWorkFlowExecuter信号 ←── Worker线程          │ │
│  │  Worker线程 ←── DAAsyncioBridge ←── Asyncio线程              │ │
│  │  Asyncio线程 ←── submitAsyncTask() ←── Qt任何线程            │ │
│  └──────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────┘
```

### 8.2 GIL管理规则

| 操作场景 | GIL策略 | 说明 |
|----------|---------|------|
| asyncio线程执行Python代码 | **持有GIL** | asyncio线程是Python主执行线程 |
| Qt线程调用Python（pybind11） | `gil_scoped_acquire` | 短暂获取GIL，完成后释放 |
| asyncio线程等待I/O | **释放GIL** | `Py_BEGIN_ALLOW_THREADS` / `gil_scoped_release` |
| asyncio线程调用Qt回调 | **释放GIL** | 跨线程回调时释放GIL，避免死锁 |
| crewAI在run_in_executor中执行 | 线程池线程持有GIL | 默认线程池中独立获取GIL |
| LangGraph astream消费 | 持有GIL消费，步骤间释放 | 允许其他线程在步骤间隔中使用GIL |

**关键规则：**
1. **asyncio线程是Python的主线程** — 长时间持有GIL，其他线程通过`gil_scoped_acquire`短暂获取
2. **跨线程回调时必须释放GIL** — 调用`DAPythonSignalHandler::callInMainThread()`前释放GIL
3. **pybind11 .cpp文件必须包含"DAPybind11InQt.h"** — 解决slots宏冲突（现有规范，`DAPybind11InQt.h:6`）
4. **不在Qt主线程中长时间持有GIL** — 避免UI冻结

### 8.3 死锁预防策略

| 潜在死锁场景 | 预防措施 |
|-------------|----------|
| asyncio线程持有GIL + Qt主线程等待GIL + Qt主线程需要处理asyncio投递的信号 | asyncio线程在投递Qt回调前释放GIL |
| Qt Worker线程等待async节点完成 + asyncio线程等待Qt主线程处理回调 | 使用`Qt::QueuedConnection`异步投递，不使用同步等待 |
| crewAI线程池线程持有GIL + asyncio事件循环需要GIL处理回调 | crewAI通过`run_in_executor`在独立线程池线程中执行，不干扰asyncio线程的GIL |

**规则：永远不在asyncio线程中同步等待Qt主线程的操作。** 所有asyncio→Qt的通信必须通过`Qt::QueuedConnection`异步投递。

### 8.4 现有代码的GIL管理参考

`DAPythonSignalHandler.cpp`（行24-62）中的GIL管理：
```cpp
void DAPythonSignalHandler::callInMainThread(std::function<void()> func)
{
    // 检查是否在主线程 — 如果在，直接执行（此时调用者已持有GIL）
    if (QThread::currentThread() == app->thread()) {
        func();
        return;
    }
    // 不在主线程 — 创建FunctionWrapper并通过信号投递
    // 注意：调用者（Python线程）应在投递前释放GIL
    Q_EMIT executeRequested(funcId);
}
```

`DAPybind11InQt.h`（行1-24）的宏冲突解决：
```cpp
#undef slots
#include "pybind11/pybind11.h"  // ... 所有pybind11头文件
#define slots Q_SLOTS  // 恢复Qt的slots宏
```

所有新的pybind11 .cpp文件必须遵守此规范。

---

## 9. 类接口设计汇总

### 9.1 新增类清单

| 类名 | 文件位置 | 负责职责 | 继承关系 |
|------|---------|---------|---------|
| `DAAsyncioBridge` | `src/DAPyBindQt/DAAsyncioBridge.h/.cpp` | Qt↔asyncio双向桥接 | `QObject` |
| `DAAbstractNodeAsync` | `src/DAWorkFlow/DAAbstractNodeAsync.h/.cpp` | 异步节点基类+状态机 | `DAAbstractNode` + `QObject` |
| `DACrewAINode` | `plugins/.../DACrewAINode.h/.cpp` (规划) | crewAI集成节点 | `DAAbstractNodeAsync` |
| `DALangGraphNode` | `plugins/.../DALangGraphNode.h/.cpp` (规划) | LangGraph集成节点 | `DAAbstractNodeAsync` |

### 9.2 修改类清单

| 类名 | 文件位置 | 修改内容 |
|------|---------|---------|
| `DAWorkFlowExecuter` | `src/DAWorkFlow/DAWorkFlowExecuter.h/.cpp` | 增加异步节点调度逻辑，增加`mPendingAsyncNodes`集合 |
| `DAPythonSignalHandler` | `src/DAPyBindQt/DAPythonSignalHandler.h/.cpp` | 增加`callInMainThreadWithData`方法 |

### 9.3 PIMPL模式使用

所有新类遵循项目PIMPL规范（`DAGlobals.h`定义）：

```cpp
// DAAsyncioBridge.h
class DAAsyncioBridge : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAsyncioBridge)  // ← PIMPL宏
    ...
};

// DAAsyncioBridge.cpp
class DAAsyncioBridge::PrivateData
{
    DA_DECLARE_PUBLIC(DAAsyncioBridge)  // ← PIMPL反向宏
public:
    PrivateData(DAAsyncioBridge* p);
    
    QThread* mAsyncioThread { nullptr };  // asyncio线程
    // ... 其他私有数据
};

DAAsyncioBridge::DAAsyncioBridge(QObject* parent) 
    : QObject(parent), DA_PIMPL_CONSTRUCT  // ← PIMPL构造宏
{
    DA_D(d);  // ← PIMPL获取私有指针
    ...
}
```

### 9.4 Q_PROPERTY使用

异步节点暴露属性遵循项目规范：

```cpp
// DACrewAINode.h
class DACrewAINode : public DAAbstractNodeAsync
{
    Q_OBJECT
    // Q_PROPERTY暴露属性（遵循项目规范）
    Q_PROPERTY(QString crewConfig READ getCrewConfig WRITE setCrewConfig NOTIFY crewConfigChanged)
    Q_PROPERTY(QVariant crewResult READ getCrewResult NOTIFY crewResultChanged)
    ...
Q_SIGNALS:
    void crewConfigChanged(const QString& config);
    void crewResultChanged(const QVariant& result);
};
```

### 9.5 Qt5/Qt6兼容性

新类需要兼容Qt5和Qt6，遵循项目现有模式：

```cpp
// 在需要Qt5/6差异处理的地方
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5的方法
#else
    // Qt6的方法
#endif
```

**特别注意：** `DAAbstractNode.h`（行230-234）已有qHash的Qt5/6兼容处理，新类如需qHash也应遵循同样模式。

---

## 10. 序列图

### 10.1 同步节点执行序列（原有流程，不变）

```
DAWorkFlow          DAWorkFlowExecuter      DAAbstractNode
   │                      │                      │
   │──exec()─────────────►│                      │
   │                      │──startExecute()──────►│
   │                      │                      │──prepareStartExec()
   │                      │                      │
   │                      │──executeNode(n)──────►│
   │                      │                      │──n->exec() [同步阻塞]
   │                      │                      │◄──bool result
   │                      │◄──nodeExecuteFinished │
   │                      │──sendParam()─────────►│ [传递参数到下游]
   │                      │──transmit()──────────►│ [触发下游执行]
   │                      │                      │
   │                      │──... (遍历所有节点) ──►│
   │                      │◄──finished(true)──────│
   │◄──finished(true)─────│                      │
```

### 10.2 异步节点执行序列（新增流程）

```
DAWorkFlow    DAWorkFlowExecuter   DAAbstractNodeAsync   DAAsyncioBridge   AsyncioThread
   │                │                    │                    │                │
   │──exec()───────►│                    │                    │                │
   │                │──startExecute()────►│                    │                │
   │                │                    │──prepareStartExec() │                │
   │                │                    │                    │                │
   │                │──executeNodeInternal(n)►│               │                │
   │                │                    │──检测isAsyncNode() │                │
   │                │──executeAsyncNode(n)►│                  │                │
   │                │                    │──n->exec()         │                │
   │                │                    │  [启动异步,立即返回]│                │
   │                │                    │►──submitAsyncTask──►│                │
   │                │◄──nodeExecuteFin.──│                    │──call_soon────►│
   │                │                    │                    │                │
   │                │  [暂停DAG遍历]     │                    │                │
   │                │  mPendingAsync.add │                    │                │
   │                │                    │                    │                │
   │                │                    │                    │  [asyncio线程]  │
   │                │                    │                    │                │
   │                │                    │                    │                │──获取GIL
   │                │                    │                    │                │──execAsync()
   │                │                    │                    │                │──调用Agent
   │                │                    │                    │                │──获取结果
   │                │                    │                    │                │──释放GIL
   │                │                    │                    │                │
   │                │                    │                    │◄──result───────│
   │                │                    │                    │                │
   │                │                    │◄──callInMainThread─│ [Qt::Queued]  │
   │                │                    │                    │                │
   │                │                    │──asyncFinished────►│                │
   │                │◄──onAsyncNodeFin.──│                    │                │
   │                │  [恢复DAG遍历]     │                    │                │
   │                │──sendParam()──────►│                    │                │
   │                │──transmit()────────►│                   │                │
   │                │                    │                    │                │
   │                │◄──finished(true)───│                    │                │
   │◄──finished(true)─│                 │                    │                │
```

### 10.3 crewAI节点执行序列

```
DACrewAINode        DAAsyncioBridge         AsyncioThread         Python (crewAI)
   │                    │                       │                      │
   │──exec()────────────│                       │                      │
   │  [启动异步]        │                       │                      │
   │──submitSyncTask()─►│                       │                      │
   │                    │──call_soon_threadsafe─►│                      │
   │                    │                       │                      │
   │                    │                       │──run_in_executor()──►│
   │                    │                       │   [线程池线程]        │──获取GIL
   │                    │                       │                      │──crew.kickoff()
   │                    │                       │                      │  [同步阻塞数分钟]
   │                    │                       │                      │──CrewOutput结果
   │                    │                       │                      │──释放GIL
   │                    │                       │◄──result──────────────│
   │                    │                       │                      │
   │                    │──callInMainThread─────►│ [投递到Qt主线程]     │
   │◄──asyncFinished────│                       │                      │
   │  [恢复DAG遍历]     │                       │                      │
```

### 10.4 LangGraph节点流式执行序列

```
DALangGraphNode     DAAsyncioBridge         AsyncioThread         Python (LangGraph)
   │                    │                       │                      │
   │──exec()────────────│                       │                      │
   │──submitAsyncTask()►│                       │                      │
   │                    │──call_soon_threadsafe─►│                      │
   │                    │                       │                      │
   │                    │                       │──获取GIL              │
   │                    │                       │──await graph.astream()►│
   │                    │                       │                      │──step 1
   │                    │                       │◄──step_1_data─────────│
   │                    │                       │──释放GIL(短暂)       │
   │                    │──callInMainThread─────►│ [投递step_1进度]     │
   │◄──asyncProgress───│                       │                      │
   │                    │                       │──获取GIL              │
   │                    │                       │                      │──step 2
   │                    │                       │◄──step_2_data─────────│
   │                    │──callInMainThread─────►│ [投递step_2进度]     │
   │◄──asyncProgress───│                       │                      │
   │                    │                       │──...                  │──... steps
   │                    │                       │──最终结果             │──final result
   │                    │──callInMainThread─────►│                      │
   │◄──asyncFinished────│                       │                      │
   │  [恢复DAG遍历]     │                       │                      │
```

---

## 附录A: Python嵌入模块扩展

### 当前模块（三个）

| 模块名 | 文件 | 功能 |
|--------|------|------|
| `da_app` | `src/DAPyBindQt/DAPyModule.h` | 应用级API |
| `da_interface` | 现有 | 界面交互API |
| `da_data` | 现有 | 数据处理API |

### 新增模块（规划）

| 模块名 | 规划文件 | 功能 |
|--------|---------|------|
| `da_agent` | `src/DAPyBindQt/DAPyModuleAgent.h` | Agent框架桥接API |

```cpp
// src/DAPyBindQt/DAPyModuleAgent.h (规划)
// 在.cpp中必须 #include "DAPybind11InQt.h" 第一个

PYBIND11_EMBEDDED_MODULE(da_agent, m)
{
    m.def("submit_async_task", &submitAsyncTaskPy);
    m.def("submit_sync_task", &submitSyncTaskPy);
    m.def("run_crewai", &runCrewAIPy);
    m.def("run_langgraph_stream", &runLangGraphStreamPy);
    m.def("set_bridge_callback", &setBridgeCallbackPy);
    
    // 暴露DAAsyncioBridge的关键方法
    pybind11::class_<DAAsyncioBridge>(m, "AsyncioBridge")
        .def_static("instance", &DAAsyncioBridge::instance,
                    pybind11::return_value_policy::reference)
        .def("submit_async_task", &DAAsyncioBridge::submitAsyncTask)
        .def("submit_sync_task", &DAAsyncioBridge::submitSyncTask)
        .def("cancel_task", &DAAsyncioBridge::cancelTask)
        .def("is_running", &DAAsyncioBridge::isRunning);
}
```

---

## 附录B: DAAbstractNodeAsync与QObject继承的特殊考虑

### 问题：DAAbstractNode不继承QObject

`DAAbstractNode`（`DAAbstractNode.h:49`）继承`std::enable_shared_from_this<DAAbstractNode>`，**不继承QObject**。因此`DAAbstractNodeAsync`需要特殊处理：

```cpp
// DAAbstractNodeAsync必须继承QObject才能使用信号槽
class DAAbstractNodeAsync : public DAAbstractNode  // 继承非QObject基类
                            , public QObject        // 同时继承QObject
{
    Q_OBJECT  // ← 可以使用，因为同时继承了QObject
    ...
};
```

**注意：** 这种多重继承方式在Qt中是允许的（QObject必须是最右边的基类），但需要注意：
1. `dynamic_cast<DAAbstractNodeAsync*>` 从 `DAAbstractNode*` 需要跨基类转换
2. `QObject`的生命周期管理需与`std::shared_ptr<DAAbstractNode>`协调
3. `DAAbstractNodeAsync*` 可以同时作为 `DAAbstractNode::SharedPointer` 和 `QObject*` 使用

**与QwtPlotItem的区别：** QwtPlotItem不继承QObject，因此**不能使用Q_OBJECT**（已在AGENTS.md中明确禁止）。但`DAAbstractNodeAsync`是新增类，同时继承了QObject，可以使用Q_OBJECT和信号槽。

---

## 附录C: 构建系统集成

### CMake修改点

```cmake
# src/DAPyBindQt/CMakeLists.txt 新增源文件
target_sources(DAPyBindQt PRIVATE
    DAAsyncioBridge.h
    DAAsyncioBridge.cpp
    DAPyModuleAgent.h
    DAPyModuleAgent.cpp
)

# src/DAWorkFlow/CMakeLists.txt 新增源文件
target_sources(DAWorkFlow PRIVATE
    DAAbstractNodeAsync.h
    DAAbstractNodeAsync.cpp
)
```

### 元类型注册

```cpp
// DAAbstractNodeAsync.cpp (命名空间外)
DA_AUTO_REGISTER_META_TYPE(DA::DAAbstractNodeAsync)
DA_AUTO_REGISTER_META_TYPE(DA::DAAsyncNodeState)
```

---

## 附录D: 与现有代码规范的兼容性检查表

| 规范要求 | 方案是否遵守 | 具体体现 |
|----------|-------------|---------|
| DA前缀命名 | ✅ | DAAsyncioBridge, DAAbstractNodeAsync, DACrewAINode, DALangGraphNode |
| PIMPL模式 | ✅ | 所有新类使用DA_DECLARE_PRIVATE/DA_D/DA_DC宏 |
| Q_PROPERTY | ✅ | DACrewAINode/DALangGraphNode暴露属性 |
| Qt5/Qt6兼容 | ✅ | qHash等需要兼容的地方使用QT_VERSION_CHECK宏 |
| Doxygen注释 | ✅ | .cpp文件详细注释，.h文件简要//注释 |
| pybind11 slots宏 | ✅ | 所有.pybind11.cpp文件#include "DAPybind11InQt.h" |
| QwtPlotItem不使用Q_OBJECT | ✅ | 不涉及QwtPlotItem，DAAbstractNodeAsync继承QObject |
| 禁止小写slots/signal宏 | ✅ | 使用Q_SIGNALS/Q_SLOTS |

---

## 附录E: 风险评估与缓解措施

| 风险 | 严重度 | 缓解措施 |
|------|--------|---------|
| asyncio线程与GIL争用导致UI卡顿 | 高 | asyncio线程步骤间释放GIL；crewAI使用run_in_executor独立线程 |
| DAAbstractNodeAsync双重继承(QObject+非QObject)的dynamic_cast问题 | 中 | 使用`dynamic_cast`跨基类转换时确保RTTI启用；提供`castTo<DAAbstractNodeAsync>()`模板方法 |
| 异步节点信号跨3个线程(asyncio→Qt主→Qt worker)的投递延迟 | 中 | 所有跨线程通信使用Qt::QueuedConnection；避免同步等待 |
| crewAI/LangGraph依赖Python≥3.10与现有Python 3.8+的冲突 | 高 | DAPyInterpreter增加Python版本检查；不符合版本时拒绝启动asyncio桥接 |
| 大量异步节点并发导致mPendingAsyncNodes集合过大 | 低 | 支持cancelTask()取消超时任务；设置最大并发限制 |

---

## 附录F: 术语表

| 术语 | 定义 |
|------|------|
| GIL | Python全局解释器锁 (Global Interpreter Lock)，同一时刻只有一个线程执行Python代码 |
| asyncio | Python异步I/O框架，基于事件循环和协程 |
| DAG | 有向无环图 (Directed Acyclic Graph)，工作流的数学模型 |
| PIMPL | Pointer to Implementation，C++中将私有实现隐藏到独立类的模式 |
| moveToThread | Qt中将QObject移动到指定线程的模式，对象的所有事件处理在该线程执行 |
| QueuedConnection | Qt信号槽连接类型，接收者的槽函数在接收者所在线程的事件循环中执行 |
| run_in_executor | asyncio方法，将同步函数在独立线程池中执行，避免阻塞事件循环 |
| astream | LangGraph的异步流式方法，逐步返回async generator的中间结果 |
| kickoff | crewAI的同步执行方法，返回CrewOutput |