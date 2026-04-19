# Phase 1: 基础能力适配阶段详细实施计划

## 阶段目标

完成 Python API 全量暴露、工作流引擎异步重构、Agent 基础模型适配，为 Agent 编排能力建设打下基础。

**周期**: 4 周  
**优先级**: 🔴 最高  
**前置条件**: Phase 0（前置预研）100% 完成并通过验收

---

## 任务分解

### 任务 1.1: Python API 接口设计

**目标**: 梳理现有软件核心功能，设计 Python API 接口规范

**涉及文件**:
- `src/DAWorkFlow/DAAbstractNode.h` - 节点基类
- `src/DAWorkFlow/DAWorkFlowExecuter.h` - 执行引擎
- `src/DAFigure/` - 图表模块头文件
- `src/DAData/` - 数据模块头文件
- 新文件：`docs/design/python-api-spec.md`

**子步骤**:

#### Step 1: 梳理 DAWorkFlow 核心接口
```cpp
// 需要暴露的接口清单
class DAWorkFlowExecuter {
    Q_INVOKABLE DAWorkFlow* createWorkflow();
    Q_INVOKABLE bool executeWorkflow(DAWorkFlow* workflow);
    Q_INVOKABLE void stopExecution();
};

class DAAbstractNode {
    Q_PROPERTY(QString nodeName READ nodeName WRITE setNodeName NOTIFY nodeNameChanged)
    Q_PROPERTY(QVariant nodeData READ nodeData WRITE setNodeData)
    // 需要暴露的属性和方法
};
```

**验收**: 输出《DAWorkFlow Python API 清单》

#### Step 2: 梳理 DAFigure 核心接口
```cpp
// 图表模块需要暴露的接口
class DAFigureWindow {
    Q_INVOKABLE void addScatterPlot(const QString& name, const QVariantList& x, const QVariantList& y);
    Q_INVOKABLE void addLinePlot(const QString& name, const QVariantList& x, const QVariantList& y);
    Q_INVOKABLE void saveAsImage(const QString& path, int dpi = 300);
    Q_INVOKABLE void setXLabel(const QString& label);
    Q_INVOKABLE void setYLabel(const QString& label);
    Q_INVOKABLE void setTitle(const QString& title);
};
```

**验收**: 输出《DAFigure Python API 清单》

#### Step 3: 梳理 DAData 核心接口
```cpp
// 数据处理模块需要暴露的接口
class DADataFrame {
    Q_INVOKABLE QVariantList columns() const;
    Q_INVOKABLE int rowCount() const;
    Q_INVOKABLE void filter(const QString& condition);
    Q_INVOKABLE void groupBy(const QStringList& columns);
    Q_INVOKABLE DADataFrame* aggregate(const QString& method);
};
```

**验收**: 输出《DAData Python API 清单》

#### Step 4: 设计 Python API 规范文档
**规范内容**:
- 命名规范：Python 端使用 snake_case，C++ 端保持 Qt 规范
- 类型映射：QVariant → Python object, QStringList → list[str]
- 异常处理：C++ 异常映射为 Python 异常
- 文档规范：所有 API 必须有 docstring

**验收**: 完成 `docs/design/python-api-spec.md`

---

### 任务 1.2: pybind11 Python API 绑定实现

**目标**: 基于 pybind11 实现核心 Python API 绑定

**涉及文件**:
- `src/DAPyBindQt/` - pybind11 绑定代码目录
- 新文件：`src/DAPyBindQt/daworkflow_module.cpp`
- 新文件：`src/DAPyBindQt/dafigure_module.cpp`
- 新文件：`src/DAPyBindQt/dadata_module.cpp`

**子步骤**:

#### Step 1: 创建 DAWorkFlow Python 模块
```cpp
// src/DAPyBindQt/daworkflow_module.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/qt.h>
#include "DAWorkFlow/DAWorkFlowExecuter.h"
#include "DAWorkFlow/DAAbstractNode.h"

namespace py = pybind11;

PYBIND11_MODULE(daworkflow, m) {
    m.doc() = "DAWorkFlow Python bindings";
    
    py::class_<DAAbstractNode>(m, "AbstractNode")
        .def_property("name", &DAAbstractNode::nodeName, &DAAbstractNode::setNodeName)
        .def_property("data", &DAAbstractNode::nodeData, &DAAbstractNode::setNodeData)
        .def("exec", &DAAbstractNode::exec)
        .def("exec_async", &DAAbstractNode::execAsync);
    
    py::class_<DAWorkFlowExecuter>(m, "WorkflowExecuter")
        .def(py::init<>())
        .def("create_workflow", &DAWorkFlowExecuter::createWorkflow)
        .def("execute", &DAWorkFlowExecuter::executeWorkflow)
        .def("stop", &DAWorkFlowExecuter::stopExecution);
}
```

**验收**: 
```python
# Python 端测试
import daworkflow
executer = daworkflow.WorkflowExecuter()
workflow = executer.create_workflow()
assert workflow is not None
print("DAWorkFlow bindings OK")
```

#### Step 2: 创建 DAFigure Python 模块
```cpp
// src/DAPyBindQt/dafigure_module.cpp
#include <pybind11/pybind11.h>
#include "DAFigure/DAFigureWindow.h"

namespace py = pybind11;

PYBIND11_MODULE(dafigure, m) {
    m.doc() = "DAFigure Python bindings";
    
    py::class_<DAFigureWindow>(m, "FigureWindow")
        .def(py::init<>())
        .def("add_scatter", &DAFigureWindow::addScatterPlot)
        .def("add_line", &DAFigureWindow::addLinePlot)
        .def("save_image", &DAFigureWindow::saveAsImage)
        .def("set_x_label", &DAFigureWindow::setXLabel)
        .def("set_y_label", &DAFigureWindow::setYLabel)
        .def("set_title", &DAFigureWindow::setTitle);
}
```

**验收**:
```python
import dafigure
fig = dafigure.FigureWindow()
fig.add_scatter("data1", [1,2,3], [4,5,6])
fig.save_image("test.png", 300)
print("DAFigure bindings OK")
```

#### Step 3: 创建 DAData Python 模块
```cpp
// src/DAPyBindQt/dadata_module.cpp
#include <pybind11/pybind11.h>
#include "DAData/DADateFrame.h"

namespace py = pybind11;

PYBIND11_MODULE(dadata, m) {
    m.doc() = "DAData Python bindings";
    
    py::class_<DADateFrame>(m, "DataFrame")
        .def(py::init<>())
        .def("columns", &DADateFrame::columns)
        .def("row_count", &DADateFrame::rowCount)
        .def("filter", &DADateFrame::filter)
        .def("group_by", &DADateFrame::groupBy)
        .def("aggregate", &DADateFrame::aggregate);
}
```

**验收**:
```python
import dadata
df = dadata.DataFrame()
cols = df.columns()
print(f"Columns: {cols}")
print("DAData bindings OK")
```

#### Step 4: 创建统一 DA 包入口
```python
# src/DAPyBindQt/da/__init__.py
from .daworkflow import WorkflowExecuter, AbstractNode
from .dafigure import FigureWindow
from .dadata import DataFrame

__version__ = "1.0.0"
__all__ = ['WorkflowExecuter', 'AbstractNode', 'FigureWindow', 'DataFrame']
```

**验收**:
```python
import da
wf = da.createWorkflow()
fig = da.FigureWindow()
df = da.DataFrame()
print(f"DA package version: {da.__version__}")
```

---

### 任务 1.3: 工作流引擎异步重构

**目标**: DAAbstractNode 扩展 `execAsync()` 方法，DAWorkFlowExecuter 支持同步/异步节点混合调度

**涉及文件**:
- `src/DAWorkFlow/DAAbstractNode.h` - 节点基类
- `src/DAWorkFlow/DAAbstractNode.cpp`
- `src/DAWorkFlow/DAWorkFlowExecuter.h` - 执行引擎
- `src/DAWorkFlow/DAWorkFlowExecuter.cpp`
- `src/DAWorkFlow/DANodeRegistry.h` - 节点注册表

**子步骤**:

#### Step 1: 扩展 DAAbstractNode 支持异步接口
```cpp
// src/DAWorkFlow/DAAbstractNode.h
class DAWorkFlow_EXPORT DAAbstractNode : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString nodeName READ nodeName WRITE setNodeName NOTIFY nodeNameChanged)
    Q_PROPERTY(QVariant nodeData READ nodeData WRITE setNodeData NOTIFY nodeDataChanged)
    Q_PROPERTY(bool isAsyncSupported READ isAsyncSupported CONSTANT)
    
public:
    explicit DAAbstractNode(const QString& name, QObject* parent = nullptr);
    ~DAAbstractNode() override;
    
    // 同步执行（现有接口）
    virtual QVariant exec();
    
    // 新增：异步执行接口
    virtual QFuture<QVariant> execAsync();
    
    // 判断节点是否支持异步
    virtual bool isAsyncSupported() const { return false; }
    
    // 取消异步执行
    virtual void cancelExecution();
    
Q_SIGNALS:
    void executionStarted();
    void executionFinished(const QVariant& result);
    void executionCancelled();
    void progressChanged(int percent, const QString& message);
    
protected:
    // 子类实现的异步执行方法
    virtual QFuture<QVariant> execAsyncImpl() = 0;
    
private:
    Q_DISABLE_COPY(DAAbstractNode)
    DA_DECLARE_PRIVATE(DAAbstractNode)
};
```

**验收**: 编译通过，接口定义完整

#### Step 2: 实现 DAAbstractNode 异步基类逻辑
```cpp
// src/DAWorkFlow/DAAbstractNode.cpp
QFuture<QVariant> DAAbstractNode::execAsync()
{
    if (!isAsyncSupported()) {
        // 不支持异步的节点，降级为同步执行
        return QtConcurrent::run([this]() { return exec(); });
    }
    return execAsyncImpl();
}

void DAAbstractNode::cancelExecution()
{
    // 默认实现，子类可重写
    Q_EMIT executionCancelled();
}
```

**验收**: 单元测试通过

#### Step 3: 扩展 DAWorkFlowExecuter 支持异步调度
```cpp
// src/DAWorkFlow/DAWorkFlowExecuter.h
class DAWorkFlow_EXPORT DAWorkFlowExecuter : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(int currentProgress READ currentProgress NOTIFY progressChanged)
    
public:
    explicit DAWorkFlowExecuter(QObject* parent = nullptr);
    ~DAWorkFlowExecuter() override;
    
    // 同步执行（现有接口）
    virtual bool executeWorkflow(DAWorkFlow* workflow);
    
    // 新增：异步执行接口
    virtual QFuture<bool> executeWorkflowAsync(DAWorkFlow* workflow);
    
    // 停止执行
    virtual void stopExecution();
    
    // 判断是否正在运行
    bool isRunning() const;
    int currentProgress() const;
    
Q_SIGNALS:
    void executionStarted();
    void executionFinished(bool success);
    void nodeExecutionStarted(DAAbstractNode* node);
    void nodeExecutionFinished(DAAbstractNode* node, const QVariant& result);
    void progressChanged(int percent, const QString& message);
    void errorOccurred(const QString& errorMessage);
    
private:
    // 异步执行内部实现
    QFuture<bool> executeWorkflowAsyncImpl(DAWorkFlow* workflow);
    
    Q_DISABLE_COPY(DAWorkFlowExecuter)
    DA_DECLARE_PRIVATE(DAWorkFlowExecuter)
};
```

**验收**: 编译通过

#### Step 4: 实现异步执行引擎核心逻辑
```cpp
// src/DAWorkFlow/DAWorkFlowExecuter.cpp
QFuture<bool> DAWorkFlowExecuter::executeWorkflowAsyncImpl(DAWorkFlow* workflow)
{
    return QtConcurrent::run([this, workflow]() {
        auto nodes = workflow->sortedNodes(); // 拓扑排序
        
        for (auto* node : nodes) {
            if (m_stopRequested) {
                return false;
            }
            
            Q_EMIT nodeExecutionStarted(node);
            
            QVariant result;
            if (node->isAsyncSupported()) {
                // 异步节点：等待完成
                auto future = node->execAsync();
                result = future.result(); // 阻塞等待
            } else {
                // 同步节点
                result = node->exec();
            }
            
            Q_EMIT nodeExecutionFinished(node, result);
            
            // 传递输出到下游节点
            propagateOutput(node, result);
        }
        
        return true;
    });
}
```

**验收**:
```cpp
// 测试代码
auto executer = new DAWorkFlowExecuter();
auto workflow = createTestWorkflow();
auto future = executer->executeWorkflowAsync(workflow);
QSignalSpy spy(executer, &DAWorkFlowExecuter::executionFinished);
future.waitForFinished();
QVERIFY(spy.wait());
```

#### Step 5: 实现混合调度（同步/异步节点共存）
```cpp
// 在 executeWorkflowAsyncImpl 中处理混合调度
for (auto* node : nodes) {
    if (node->isAsyncSupported()) {
        // 异步节点：创建 QFuture 并加入等待队列
        m_pendingFutures.append(node->execAsync());
    } else {
        // 同步节点：等待所有 pending futures 完成后执行
        waitForAllPending();
        node->exec();
    }
}

// 等待所有异步节点完成
for (auto& future : m_pendingFutures) {
    future.waitForFinished();
}
```

**验收**: 混合工作流执行成功

---

### 任务 1.4: Agent 节点基础模型开发

**目标**: Agent 节点继承 DAAbstractNode，支持角色、目标、LLM 配置、运行状态等属性

**涉及文件**:
- 新文件：`src/DAWorkFlow/DAAgentNode.h`
- 新文件：`src/DAWorkFlow/DAAgentNode.cpp`
- 新文件：`src/DAWorkFlow/DAAgentConfig.h`
- `src/DAWorkFlow/DAWorkFlow.pri` - 添加新文件到项目

**子步骤**:

#### Step 1: 定义 Agent 配置类
```cpp
// src/DAWorkFlow/DAAgentConfig.h
#pragma once
#include <QObject>
#include <QVariantMap>
#include "DAWorkFlow_EXPORT.h"

class DAWorkFlow_EXPORT DAAgentConfig : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString agentName READ agentName WRITE setAgentName NOTIFY agentNameChanged)
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged)
    Q_PROPERTY(QString goal READ goal WRITE setGoal NOTIFY goalChanged)
    Q_PROPERTY(QString backstory READ backstory WRITE setBackstory NOTIFY backstoryChanged)
    Q_PROPERTY(QString llmProvider READ llmProvider WRITE setLlmProvider NOTIFY llmProviderChanged)
    Q_PROPERTY(QString llmModel READ llmModel WRITE setLlmModel NOTIFY llmModelChanged)
    Q_PROPERTY(QVariantMap llmParams READ llmParams WRITE setLlmParams NOTIFY llmParamsChanged)
    Q_PROPERTY(int maxIterations READ maxIterations WRITE setMaxIterations NOTIFY maxIterationsChanged)
    Q_PROPERTY(int timeoutSeconds READ timeoutSeconds WRITE setTimeoutSeconds NOTIFY timeoutSecondsChanged)
    Q_PROPERTY(bool verbose READ verbose WRITE setVerbose NOTIFY verboseChanged)
    
public:
    explicit DAAgentConfig(QObject* parent = nullptr);
    ~DAAgentConfig() override;
    
    // Getters
    QString agentName() const { return m_agentName; }
    QString role() const { return m_role; }
    QString goal() const { return m_goal; }
    QString backstory() const { return m_backstory; }
    QString llmProvider() const { return m_llmProvider; }
    QString llmModel() const { return m_llmModel; }
    QVariantMap llmParams() const { return m_llmParams; }
    int maxIterations() const { return m_maxIterations; }
    int timeoutSeconds() const { return m_timeoutSeconds; }
    bool verbose() const { return m_verbose; }
    
    // Setters
    void setAgentName(const QString& name);
    void setRole(const QString& role);
    void setGoal(const QString& goal);
    void setBackstory(const QString& backstory);
    void setLlmProvider(const QString& provider);
    void setLlmModel(const QString& model);
    void setLlmParams(const QVariantMap& params);
    void setMaxIterations(int iterations);
    void setTimeoutSeconds(int seconds);
    void setVerbose(bool verbose);
    
Q_SIGNALS:
    void agentNameChanged(const QString& name);
    void roleChanged(const QString& role);
    void goalChanged(const QString& goal);
    void backstoryChanged(const QString& backstory);
    void llmProviderChanged(const QString& provider);
    void llmModelChanged(const QString& model);
    void llmParamsChanged(const QVariantMap& params);
    void maxIterationsChanged(int iterations);
    void timeoutSecondsChanged(int seconds);
    void verboseChanged(bool verbose);
    
private:
    QString m_agentName;
    QString m_role;
    QString m_goal;
    QString m_backstory;
    QString m_llmProvider;      // "openai", "anthropic", "local"
    QString m_llmModel;         // "gpt-4", "claude-3", etc.
    QVariantMap m_llmParams;    // temperature, max_tokens, etc.
    int m_maxIterations = 10;
    int m_timeoutSeconds = 300;
    bool m_verbose = false;
};
```

**验收**: 编译通过，Q_PROPERTY 全部可用

#### Step 2: 定义 Agent 节点类
```cpp
// src/DAWorkFlow/DAAgentNode.h
#pragma once
#include "DAAbstractNode.h"
#include "DAAgentConfig.h"
#include <QFuture>

class DAWorkFlow_EXPORT DAAgentNode : public DAAbstractNode {
    Q_OBJECT
    Q_PROPERTY(DAAgentConfig* config READ config WRITE setConfig NOTIFY configChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int currentIteration READ currentIteration NOTIFY iterationChanged)
    Q_PROPERTY(QStringList tools READ tools WRITE setTools NOTIFY toolsChanged)
    
    // Agent 执行状态枚举
    Q_ENUMS(AgentStatus)
    
public:
    enum AgentStatus {
        StatusIdle = 0,
        StatusRunning,
        StatusWaitingForInput,
        StatusCompleted,
        StatusFailed,
        StatusCancelled
    };
    
    explicit DAAgentNode(const QString& name, QObject* parent = nullptr);
    ~DAAgentNode() override;
    
    // DAAbstractNode 接口
    QVariant exec() override;
    QFuture<QVariant> execAsyncImpl() override;
    bool isAsyncSupported() const override { return true; }
    void cancelExecution() override;
    
    // 属性访问
    DAAgentConfig* config() const { return m_config; }
    void setConfig(DAAgentConfig* config);
    
    QString status() const { return m_status; }
    int currentIteration() const { return m_currentIteration; }
    QStringList tools() const { return m_tools; }
    void setTools(const QStringList& tools);
    
    // 设置 Agent 输入
    void setInput(const QVariant& input);
    
Q_SIGNALS:
    void configChanged(DAAgentConfig* config);
    void statusChanged(const QString& status);
    void iterationChanged(int iteration);
    void toolsChanged(const QStringList& tools);
    void agentThought(const QString& thought);
    void agentAction(const QString& action, const QVariant& actionInput);
    void agentObservation(const QVariant& observation);
    
protected:
    void setStatus(const QString& status);
    void setCurrentIteration(int iteration);
    
private:
    // 内部执行逻辑
    QVariant executeAgentLoop();
    
    DAAgentConfig* m_config;
    QString m_status;
    int m_currentIteration = 0;
    QStringList m_tools;
    QVariant m_input;
    bool m_cancelRequested = false;
};
```

**验收**: 编译通过

#### Step 3: 实现 Agent 节点核心逻辑
```cpp
// src/DAWorkFlow/DAAgentNode.cpp
QFuture<QVariant> DAAgentNode::execAsyncImpl()
{
    return QtConcurrent::run([this]() {
        setStatus("Running");
        Q_EMIT executionStarted();
        
        try {
            QVariant result = executeAgentLoop();
            setStatus("Completed");
            Q_EMIT executionFinished(result);
            return result;
        } catch (const std::exception& e) {
            setStatus("Failed");
            Q_EMIT errorOccurred(QString::fromUtf8(e.what()));
            throw;
        }
    });
}

QVariant DAAgentNode::executeAgentLoop()
{
    // Agent 执行主循环
    for (m_currentIteration = 0; m_currentIteration < m_config->maxIterations(); ++m_currentIteration) {
        if (m_cancelRequested) {
            setStatus("Cancelled");
            Q_EMIT executionCancelled();
            return QVariant();
        }
        
        // 1. 思考（Thought）
        QString thought = generateThought();
        Q_EMIT agentThought(thought);
        
        // 2. 决定行动（Action）
        auto [action, actionInput] = decideAction();
        Q_EMIT agentAction(action, actionInput);
        
        // 3. 执行行动（工具调用或 LLM 调用）
        QVariant observation = executeAction(action, actionInput);
        Q_EMIT agentObservation(observation);
        
        // 4. 判断是否完成
        if (isGoalAchieved(observation)) {
            return generateFinalResponse(observation);
        }
    }
    
    // 超过最大迭代次数
    throw std::runtime_error("Agent exceeded maximum iterations");
}
```

**验收**: 单元测试通过

#### Step 4: 注册 Agent 节点到节点工厂
```cpp
// src/DAWorkFlow/DANodeRegistry.cpp
void DANodeRegistry::registerBuiltinNodes()
{
    // 注册现有节点...
    
    // 新增：注册 Agent 节点
    registerNode("Agent", []() { return new DAAgentNode("Agent"); });
}
```

**验收**: Agent 节点可在节点面板中找到

---

### 任务 1.5: 嵌入式 Python 环境优化

**目标**: 预装 crewAI/LangChain 等依赖，支持 pip 扩展，环境隔离

**涉及文件**:
- `src/DAPyScripts/PythonEnvironment.h`
- `src/DAPyScripts/PythonEnvironment.cpp`
- `requirements.txt`
- `src/DAPyScripts/PythonPackageInstaller.h`
- `src/DAPyScripts/PythonPackageInstaller.cpp`

**子步骤**:

#### Step 1: 实现 Python 环境管理器
```cpp
// src/DAPyScripts/PythonEnvironment.h
class DAWorkFlow_EXPORT PythonEnvironment : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString pythonPath READ pythonPath CONSTANT)
    Q_PROPERTY(QString virtualEnvPath READ virtualEnvPath CONSTANT)
    Q_PROPERTY(bool isInitialized READ isInitialized NOTIFY initializedChanged)
    
public:
    static PythonEnvironment* instance();
    
    QString pythonPath() const { return m_pythonPath; }
    QString virtualEnvPath() const { return m_virtualEnvPath; }
    bool isInitialized() const { return m_initialized; }
    
    // 初始化 Python 环境
    bool initialize();
    
    // 执行 Python 代码
    QVariant executeScript(const QString& script);
    
    // 安装包
    bool installPackage(const QString& packageName, const QString& version = QString());
    
    // 检查包是否已安装
    bool isPackageInstalled(const QString& packageName);
    
Q_SIGNALS:
    void initializedChanged(bool initialized);
    void packageInstalled(const QString& packageName);
    void errorOccurred(const QString& error);
    
private:
    explicit PythonEnvironment(QObject* parent = nullptr);
    bool createVirtualEnv();
    bool installRequirements();
    
    QString m_pythonPath;
    QString m_virtualEnvPath;
    bool m_initialized = false;
};
```

**验收**: 环境初始化成功

#### Step 2: 实现 pip 安装器
```cpp
// src/DAPyScripts/PythonPackageInstaller.cpp
bool PythonPackageInstaller::installPackage(const QString& packageName, const QString& version)
{
    QString spec = packageName;
    if (!version.isEmpty()) {
        spec += "==" + version;
    }
    
    // 调用 pip install
    QProcess process;
    process.start(m_pipPath, QStringList() << "install" << spec);
    
    if (!process.waitForFinished(300000)) { // 5 分钟超时
        Q_EMIT errorOccurred("Package installation timeout");
        return false;
    }
    
    if (process.exitCode() != 0) {
        Q_EMIT errorOccurred(QString::fromUtf8(process.readAllStandardError()));
        return false;
    }
    
    Q_EMIT packageInstalled(packageName);
    return true;
}
```

**验收**: 可成功安装 Python 包

#### Step 3: 预装 AI 依赖
```python
# requirements.txt 更新
# 核心依赖
pandas>=1.3.0
numpy>=1.20.0
scipy>=1.7.0

# AI 框架
crewai>=0.20.0
langchain>=0.1.0
langgraph>=0.0.1
openai>=1.0.0

# 工具
python-dotenv>=1.0.0
pydantic>=2.0.0
```

**验收**:
```bash
pip install -r requirements.txt
# 无冲突，所有包安装成功
```

#### Step 4: 实现环境隔离
```cpp
// 每个项目使用独立的虚拟环境
void PythonEnvironment::initializeProject(const QString& projectPath)
{
    m_virtualEnvPath = projectPath + "/.venv";
    
    if (!QFileInfo::exists(m_virtualEnvPath)) {
        createVirtualEnv();
    }
    
    // 设置 Python 路径
    #ifdef Q_OS_WIN
    m_pythonPath = m_virtualEnvPath + "/Scripts/python.exe";
    #else
    m_pythonPath = m_virtualEnvPath + "/bin/python";
    #endif
    
    // 初始化 Python 解释器
    Py_Initialize();
}
```

**验收**: 项目间环境隔离，互不影响

---

## 验收标准

### Phase 1 完成条件（全部必须满足）:
- [ ] Python 可导入 `da` 包并调用核心功能
- [ ] DAAbstractNode 支持 `execAsync()` 方法
- [ ] DAWorkFlowExecuter 支持异步工作流执行
- [ ] DAAgentNode 可创建并执行
- [ ] Python 虚拟环境可正常安装 AI 依赖

### 交付物:
1. `src/DAPyBindQt/daworkflow_module.cpp` - WorkFlow Python 绑定
2. `src/DAPyBindQt/dafigure_module.cpp` - Figure Python 绑定
3. `src/DAPyBindQt/dadata_module.cpp` - Data Python 绑定
4. `src/DAWorkFlow/DAAgentNode.h/cpp` - Agent 节点实现
5. `src/DAWorkFlow/DAAgentConfig.h` - Agent 配置类
6. `docs/design/python-api-spec.md` - Python API 规范文档

---

## 技术风险

| 风险 ID | 风险描述 | 缓解方案 | 验证方法 |
|---------|----------|----------|----------|
| R1 | pybind11 类型转换异常 | 逐类型测试，添加异常处理 | 单元测试覆盖所有类型 |
| R3 | 异步执行导致 UI 阻塞 | 确保异步节点在独立线程运行 | 压力测试，UI 响应测试 |
| R6 | Agent 无限循环 | 严格限制 maxIterations 和 timeout | 超时测试，边界测试 |
| R7 | GIL 导致性能问题 | 释放 GIL 执行长时间操作 | 性能分析，profiling |

---

## 依赖关系

```
Phase 0 (预研) ──────> 任务 1.1 ──> 任务 1.2
                         │
                         └──> 任务 1.3 ──> 任务 1.4
                                              │
                         任务 1.5 ─────────────┘
```

**前置依赖**: Phase 0 必须完成  
**后置依赖**: Phase 2 依赖 Phase 1 完成

---

## 时间规划

| 周次 | 任务 | 预计工时 |
|------|------|----------|
| Week 1 | 任务 1.1 + 任务 1.2 | 40 小时 |
| Week 2 | 任务 1.3 | 40 小时 |
| Week 3 | 任务 1.4 | 40 小时 |
| Week 4 | 任务 1.5 + 集成测试 | 40 小时 |

**里程碑**: Week 4 结束前完成所有 Phase 1 任务，Python API 可正常使用
