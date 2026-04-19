# Phase 3: AI 框架集成详细实施计划

## 阶段目标

完成 crewAI、LangChain/LangGraph 双框架对接，实现 LLM 配置中心，工具调用能力对接，统一同步/异步执行模式。

**周期**: 3-4 周  
**优先级**: 🔴 最高  
**前置条件**: Phase 2 100% 完成并通过验收

---

## 任务分解

### 任务 3.1: crewAI 框架集成

**目标**: 支持创建 crewAI Agent/Task/Crew，通过工作流可视化编排，同步执行返回结果

**涉及文件**:
- `src/DAWorkFlow/DACrewAINode.h` - crewAI 专用节点
- `src/DAWorkFlow/DACrewAINode.cpp`
- `src/DAPyScripts/DACrewAIWrapper.h` - crewAI Python 封装
- `src/DAPyScripts/DACrewAIWrapper.cpp`

**子步骤**:

#### Step 1: 设计 crewAI 节点类
```cpp
// src/DAWorkFlow/DACrewAINode.h
#pragma once
#include "DAAgentNode.h"
#include <QVariantList>

class DAWorkFlow_EXPORT DACrewAINode : public DAAgentNode {
    Q_OBJECT
    Q_PROPERTY(QString crewDescription READ crewDescription WRITE setCrewDescription NOTIFY crewDescriptionChanged)
    Q_PROPERTY(QVariantList tasks READ tasks WRITE setTasks NOTIFY tasksChanged)
    Q_PROPERTY(bool sequential READ isSequential WRITE setSequential NOTIFY sequentialChanged)
    
public:
    explicit DACrewAINode(const QString& name, QObject* parent = nullptr);
    ~DACrewAINode() override;
    
    // DAAgentNode 接口
    QFuture<QVariant> execAsyncImpl() override;
    
    // crewAI 特有属性
    QString crewDescription() const { return m_crewDescription; }
    void setCrewDescription(const QString& desc);
    
    QVariantList tasks() const { return m_tasks; }
    void setTasks(const QVariantList& tasks);
    
    bool isSequential() const { return m_sequential; }
    void setSequential(bool sequential);
    
    // 执行 crewAI
    QVariant executeCrew();
    
Q_SIGNALS:
    void crewDescriptionChanged(const QString& desc);
    void tasksChanged(const QVariantList& tasks);
    void sequentialChanged(bool sequential);
    void crewStarted();
    void crewFinished(const QVariant& result);
    void taskCompleted(const QString& taskName, const QVariant& output);
    
private:
    QString m_crewDescription;
    QVariantList m_tasks;  // List of task configs
    bool m_sequential = true;
    
    // Python 调用
    QVariant callCrewAI(const QString& agents, const QString& tasks, bool sequential);
};
```

**验收**: 编译通过

#### Step 2: 实现 crewAI Python 封装
```python
# src/DAPyScripts/crewai_wrapper.py
from crewai import Agent, Task, Crew, Process
import json

def create_agent(name, role, goal, backstory, llm_config=None, tools=None):
    """创建 crewAI Agent"""
    return Agent(
        name=name,
        role=role,
        goal=goal,
        backstory=backstory,
        llm=llm_config,
        tools=tools or [],
        verbose=True
    )

def create_task(description, expected_output, agent=None, tools=None):
    """创建 crewAI Task"""
    return Task(
        description=description,
        expected_output=expected_output,
        agent=agent,
        tools=tools or []
    )

def execute_crew(agents, tasks, sequential=True, verbose=False):
    """执行 crewAI Crew"""
    crew = Crew(
        agents=agents,
        tasks=tasks,
        verbose=verbose,
        process=Process.sequential if sequential else Process.hierarchical
    )
    
    result = crew.kickoff()
    return result
```

**验收**: Python 脚本可独立运行

#### Step 3: 实现 C++ 调用 crewAI
```cpp
// src/DAPyScripts/DACrewAIWrapper.cpp
QVariant DACrewAIWrapper::executeCrew(const QVariantList& agents,
                                       const QVariantList& tasks,
                                       bool sequential)
{
    py::gil_scoped_acquire acquire;
    
    try {
        // 导入 crewAI 包装模块
        py::object crewai_module = py::module_::import("crewai_wrapper");
        
        // 创建 Agents
        py::list py_agents;
        for (const auto& agent_config : agents) {
            auto config = agent_config.toMap();
            py::object agent = crewai_module.attr("create_agent")(
                config["name"].toString(),
                config["role"].toString(),
                config["goal"].toString(),
                config["backstory"].toString()
            );
            py_agents.append(agent);
        }
        
        // 创建 Tasks
        py::list py_tasks;
        for (const auto& task_config : tasks) {
            auto config = task_config.toMap();
            py::object task = crewai_module.attr("create_task")(
                config["description"].toString(),
                config["expected_output"].toString()
            );
            py_tasks.append(task);
        }
        
        // 执行 Crew
        py::object result = crewai_module.attr("execute_crew")(
            py_agents, py_tasks, sequential
        );
        
        return py::object(result).cast<QVariant>();
        
    } catch (const py::error_already_set& e) {
        throw std::runtime_error(e.what());
    }
}
```

**验收**: C++ 可成功调用 crewAI

#### Step 4: 实现 crewAI 节点执行逻辑
```cpp
// src/DAWorkFlow/DACrewAINode.cpp
QFuture<QVariant> DACrewAINode::execAsyncImpl()
{
    return QtConcurrent::run([this]() {
        Q_EMIT crewStarted();
        
        try {
            // 1. 准备 Agent 配置
            QVariantList agents;
            agents.append(createAgentConfig());
            
            // 2. 准备 Task 配置
            QVariantList tasks = m_tasks;
            
            // 3. 执行 Crew
            QVariant result = callCrewAI(agents, tasks, m_sequential);
            
            Q_EMIT crewFinished(result);
            return result;
            
        } catch (const std::exception& e) {
            Q_EMIT errorOccurred(QString::fromUtf8(e.what()));
            throw;
        }
    });
}

QVariant DACrewAINode::callCrewAI(const QString& agents,
                                   const QString& tasks,
                                   bool sequential)
{
    // 调用 Python crewAI 封装
    py::gil_scoped_acquire acquire;
    
    py::object crewai_module = py::module_::import("crewai_wrapper");
    py::object result = crewai_module.attr("execute_crew")(
        py::eval(agents), py::eval(tasks), sequential
    );
    
    return py::object(result).cast<QVariant>();
}
```

**验收**: crewAI 节点可执行并返回结果

---

### 任务 3.2: LangChain & LangGraph 集成

**目标**: 支持 LangChain 组件编排，LangGraph 有向图映射为软件工作流，异步执行正常

**涉及文件**:
- `src/DAWorkFlow/DALangChainNode.h` - LangChain 节点
- `src/DAWorkFlow/DALangChainNode.cpp`
- `src/DAWorkFlow/DALangGraphNode.h` - LangGraph 节点
- `src/DAWorkFlow/DALangGraphNode.cpp`
- `src/DAPyScripts/DALangChainWrapper.py` - LangChain Python 封装
- `src/DAPyScripts/DALangGraphWrapper.py` - LangGraph Python 封装

**子步骤**:

#### Step 1: 设计 LangChain 节点
```cpp
// src/DAWorkFlow/DALangChainNode.h
#pragma once
#include "DAAgentNode.h"

class DAWorkFlow_EXPORT DALangChainNode : public DAAgentNode {
    Q_OBJECT
    Q_PROPERTY(QString chainType READ chainType WRITE setChainType NOTIFY chainTypeChanged)
    Q_PROPERTY(QVariantMap chainConfig READ chainConfig WRITE setChainConfig NOTIFY chainConfigChanged)
    
public:
    enum ChainType {
        SimpleChain,
        SequentialChain,
        TransformChain,
        RouterChain,
        CustomChain
    };
    Q_ENUM(ChainType)
    
    explicit DALangChainNode(const QString& name, QObject* parent = nullptr);
    ~DALangChainNode() override;
    
    QFuture<QVariant> execAsyncImpl() override;
    
    QString chainType() const { return m_chainType; }
    void setChainType(const QString& type);
    
    QVariantMap chainConfig() const { return m_chainConfig; }
    void setChainConfig(const QVariantMap& config);
    
Q_SIGNALS:
    void chainTypeChanged(const QString& type);
    void chainConfigChanged(const QVariantMap& config);
    
private:
    QString m_chainType;
    QVariantMap m_chainConfig;
    
    QVariant executeLangChain();
};
```

**验收**: 编译通过

#### Step 2: 设计 LangGraph 节点（支持有向图）
```cpp
// src/DAWorkFlow/DALangGraphNode.h
#pragma once
#include "DAAgentNode.h"
#include <QMap>

class DAWorkFlow_EXPORT DALangGraphNode : public DAAgentNode {
    Q_OBJECT
    Q_PROPERTY(QMap<QString, QVariant> graphNodes READ graphNodes WRITE setGraphNodes NOTIFY graphNodesChanged)
    Q_PROPERTY(QMap<QString, QStringList> graphEdges READ graphEdges WRITE setGraphEdges NOTIFY graphEdgesChanged)
    Q_PROPERTY(QString entryNode READ entryNode WRITE setEntryNode NOTIFY entryNodeChanged)
    
public:
    explicit DALangGraphNode(const QString& name, QObject* parent = nullptr);
    ~DALangGraphNode() override;
    
    QFuture<QVariant> execAsyncImpl() override;
    
    // 图结构
    QMap<QString, QVariant> graphNodes() const { return m_graphNodes; }
    void setGraphNodes(const QMap<QString, QVariant>& nodes);
    
    QMap<QString, QStringList> graphEdges() const { return m_graphEdges; }
    void setGraphEdges(const QMap<QString, QStringList>& edges);
    
    QString entryNode() const { return m_entryNode; }
    void setEntryNode(const QString& node);
    
    // 将软件工作流映射为 LangGraph
    QVariantMap buildLangGraph();
    
Q_SIGNALS:
    void graphNodesChanged(const QMap<QString, QVariant>& nodes);
    void graphEdgesChanged(const QMap<QString, QStringList>& edges);
    void entryNodeChanged(const QString& node);
    
private:
    QMap<QString, QVariant> m_graphNodes;  // node_id -> node_config
    QMap<QString, QStringList> m_graphEdges;  // node_id -> [target_ids]
    QString m_entryNode;
    
    QVariant executeLangGraph();
};
```

**验收**: 编译通过

#### Step 3: 实现 LangGraph 工作流映射
```cpp
// src/DAWorkFlow/DALangGraphNode.cpp
QVariantMap DALangGraphNode::buildLangGraph()
{
    QVariantMap graph;
    
    // 1. 添加节点
    QVariantList nodes;
    for (auto it = m_graphNodes.begin(); it != m_graphNodes.end(); ++it) {
        QVariantMap nodeConfig;
        nodeConfig["id"] = it.key();
        nodeConfig["config"] = it.value();
        nodes.append(nodeConfig);
    }
    graph["nodes"] = nodes;
    
    // 2. 添加边
    QVariantList edges;
    for (auto it = m_graphEdges.begin(); it != m_graphEdges.end(); ++it) {
        for (const auto& target : it.value()) {
            QVariantMap edge;
            edge["source"] = it.key();
            edge["target"] = target;
            edges.append(edge);
        }
    }
    graph["edges"] = edges;
    
    // 3. 设置入口节点
    graph["entry_point"] = m_entryNode;
    
    return graph;
}

QFuture<QVariant> DALangGraphNode::execAsyncImpl()
{
    return QtConcurrent::run([this]() {
        // 构建 LangGraph
        QVariantMap graph = buildLangGraph();
        
        // 转换为 Python dict
        py::dict py_graph = qvariant_to_pydict(graph);
        
        // 调用 LangGraph 执行
        py::gil_scoped_acquire acquire;
        py::object langgraph_module = py::module_::import("langgraph_wrapper");
        py::object result = langgraph_module.attr("execute_graph")(py_graph);
        
        return py::object(result).cast<QVariant>();
    });
}
```

**验收**: 工作流可映射为 LangGraph 并执行

#### Step 4: 实现 LangChain Python 封装
```python
# src/DAPyScripts/langchain_wrapper.py
from langchain.chains import SequentialChain, TransformChain
from langchain.prompts import PromptTemplate
from langchain.llms import OpenAI

def create_simple_chain(prompt_template, llm_config):
    """创建简单 Chain"""
    llm = OpenAI(**llm_config)
    prompt = PromptTemplate(template=prompt_template, input_variables=["input"])
    chain = prompt | llm
    return chain

def create_sequential_chain(chains, input_variables, output_variables):
    """创建 SequentialChain"""
    return SequentialChain(
        chains=chains,
        input_variables=input_variables,
        output_variables=output_variables,
        verbose=True
    )

def execute_chain(chain, inputs):
    """执行 Chain"""
    result = chain(inputs)
    return result
```

**验收**: Python 脚本可独立运行

---

### 任务 3.3: LLM 配置中心开发

**目标**: 统一管理多 LLM 平台 API 密钥、参数配置，支持 OpenAI/Claude/本地大模型

**涉及文件**:
- `src/DAUtils/DALLMConfigCenter.h` - LLM 配置中心
- `src/DAUtils/DALLMConfigCenter.cpp`
- `src/DAUtils/DALLMProvider.h` - LLM 提供商配置
- `src/DAUtils/DALLMProvider.cpp`

**子步骤**:

#### Step 1: 设计 LLM 配置中心
```cpp
// src/DAUtils/DALLMConfigCenter.h
#pragma once
#include <QObject>
#include <QMap>
#include "DASecretManager.h"

class DAUtils_EXPORT DALLMProvider : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString providerId READ providerId WRITE setProviderId NOTIFY providerIdChanged)
    Q_PROPERTY(QString providerName READ providerName WRITE setProviderName NOTIFY providerNameChanged)
    Q_PROPERTY(QString apiKeyRef READ apiKeyRef WRITE setApiKeyRef NOTIFY apiKeyRefChanged)
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QVariantMap defaultParams READ defaultParams WRITE setDefaultParams NOTIFY defaultParamsChanged)
    Q_PROPERTY(QStringList models READ models WRITE setModels NOTIFY modelsChanged)
    
public:
    explicit DALLMProvider(QObject* parent = nullptr);
    
    QString providerId() const { return m_providerId; }
    void setProviderId(const QString& id);
    
    QString providerName() const { return m_providerName; }
    void setProviderName(const QString& name);
    
    QString apiKeyRef() const { return m_apiKeyRef; }
    void setApiKeyRef(const QString& ref);
    
    // 获取实际 API 密钥（从密钥管理器）
    QString getApiKey() const;
    
    QString baseUrl() const { return m_baseUrl; }
    void setBaseUrl(const QString& url);
    
    QVariantMap defaultParams() const { return m_defaultParams; }
    void setDefaultParams(const QVariantMap& params);
    
    QStringList models() const { return m_models; }
    void setModels(const QStringList& models);
    
Q_SIGNALS:
    void providerIdChanged(const QString& id);
    void providerNameChanged(const QString& name);
    void apiKeyRefChanged(const QString& ref);
    void baseUrlChanged(const QString& url);
    void defaultParamsChanged(const QVariantMap& params);
    void modelsChanged(const QStringList& models);
    
private:
    QString m_providerId;      // "openai", "anthropic", "local"
    QString m_providerName;    // "OpenAI", "Anthropic", "Local LLM"
    QString m_apiKeyRef;       // 密钥管理器引用
    QString m_baseUrl;         // API 基础 URL
    QVariantMap m_defaultParams;
    QStringList m_models;
};

class DAUtils_EXPORT DALLMConfigCenter : public QObject {
    Q_OBJECT
    
public:
    static DALLMConfigCenter* instance();
    
    // 注册提供商
    void registerProvider(DALLMProvider* provider);
    
    // 获取提供商
    DALLMProvider* getProvider(const QString& providerId) const;
    
    // 获取所有提供商
    QList<DALLMProvider*> providers() const { return m_providers.values(); }
    
    // 设置默认提供商
    void setDefaultProvider(const QString& providerId);
    DALLMProvider* defaultProvider() const { return m_defaultProvider; }
    
    // 测试连接
    QFuture<bool> testConnection(const QString& providerId);
    
Q_SIGNALS:
    void providerAdded(const QString& providerId);
    void providerRemoved(const QString& providerId);
    void connectionTested(const QString& providerId, bool success);
    
private:
    explicit DALLMConfigCenter(QObject* parent = nullptr);
    
    QMap<QString, DALLMProvider*> m_providers;
    DALLMProvider* m_defaultProvider = nullptr;
};
```

**验收**: 编译通过

#### Step 2: 实现 LLM 配置中心
```cpp
// src/DAUtils/DALLMConfigCenter.cpp
DALLMConfigCenter* DALLMConfigCenter::instance()
{
    static DALLMConfigCenter instance;
    return &instance;
}

void DALLMConfigCenter::registerProvider(DALLMProvider* provider)
{
    m_providers[provider->providerId()] = provider;
    Q_EMIT providerAdded(provider->providerId());
    
    if (!m_defaultProvider) {
        m_defaultProvider = provider;
    }
}

QString DALLMProvider::getApiKey() const
{
    return DASecretManager::instance()->getSecret(m_apiKeyRef);
}

QFuture<bool> DALLMConfigCenter::testConnection(const QString& providerId)
{
    return QtConcurrent::run([this, providerId]() {
        auto* provider = getProvider(providerId);
        if (!provider) return false;
        
        try {
            // 调用 LLM API 测试连接
            // 实际实现需要调用对应 LLM 的 API
            QString apiKey = provider->getApiKey();
            QString baseUrl = provider->baseUrl();
            
            // 发送测试请求...
            return true;
            
        } catch (...) {
            return false;
        }
    });
}
```

**验收**: 可注册和获取 LLM 提供商配置

#### Step 3: 实现配置 UI
```cpp
// src/DAGui/DALLMConfigDialog.h
class DALLMConfigDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit DALLMConfigDialog(QWidget* parent = nullptr);
    
private:
    void setupUI();
    void loadProviders();
    
    QTabWidget* m_tabWidget;
    QListWidget* m_providerList;
    
    // 每个提供商的配置页面
    QMap<QString, QWidget*> m_providerPages;
};
```

**验收**: 配置对话框可显示和编辑

---

### 任务 3.4: 工具调用能力对接

**目标**: Agent 可直接调用软件内置数据处理、可视化工具，支持自定义工具扩展

**涉及文件**:
- `src/DAWorkFlow/DAToolRegistry.h` - 工具注册表
- `src/DAWorkFlow/DAToolRegistry.cpp`
- `src/DAWorkFlow/DAToolInterface.h` - 工具接口
- `src/DAWorkFlow/DAToolDataLoader.h` - 数据加载工具
- `src/DAWorkFlow/DAToolChartGenerator.h` - 图表生成工具

**子步骤**:

#### Step 1: 定义工具接口
```cpp
// src/DAWorkFlow/DAToolInterface.h
#pragma once
#include <QObject>
#include <QVariantMap>

class DAWorkFlow_EXPORT DAToolInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString toolId READ toolId WRITE setToolId NOTIFY toolIdChanged)
    Q_PROPERTY(QString toolName READ toolName WRITE setToolName NOTIFY toolNameChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QVariantMap parameters READ parameters WRITE setParameters NOTIFY parametersChanged)
    
public:
    explicit DAToolInterface(QObject* parent = nullptr);
    virtual ~DAToolInterface() = default;
    
    QString toolId() const { return m_toolId; }
    void setToolId(const QString& id);
    
    QString toolName() const { return m_toolName; }
    void setToolName(const QString& name);
    
    QString description() const { return m_description; }
    void setDescription(const QString& desc);
    
    QVariantMap parameters() const { return m_parameters; }
    void setParameters(const QVariantMap& params);
    
    // 执行工具（纯虚）
    virtual QVariant execute(const QVariantMap& inputs) = 0;
    
Q_SIGNALS:
    void toolIdChanged(const QString& id);
    void toolNameChanged(const QString& name);
    void descriptionChanged(const QString& desc);
    void parametersChanged(const QVariantMap& params);
    
protected:
    QString m_toolId;
    QString m_toolName;
    QString m_description;
    QVariantMap m_parameters;
};
```

**验收**: 编译通过

#### Step 2: 实现工具注册表
```cpp
// src/DAWorkFlow/DAToolRegistry.h
#pragma once
#include <QObject>
#include <QMap>
#include "DAToolInterface.h"

class DAWorkFlow_EXPORT DAToolRegistry : public QObject {
    Q_OBJECT
    
public:
    static DAToolRegistry* instance();
    
    // 注册工具
    void registerTool(DAToolInterface* tool);
    
    // 获取工具
    DAToolInterface* getTool(const QString& toolId) const;
    
    // 获取所有工具
    QList<DAToolInterface*> tools() const { return m_tools.values(); }
    
    // 取消注册
    void unregisterTool(const QString& toolId);
    
    // 执行工具
    QVariant executeTool(const QString& toolId, const QVariantMap& inputs);
    
Q_SIGNALS:
    void toolRegistered(const QString& toolId);
    void toolUnregistered(const QString& toolId);
    
private:
    explicit DAToolRegistry(QObject* parent = nullptr);
    
    QMap<QString, DAToolInterface*> m_tools;
};
```

**验收**: 工具可注册和获取

#### Step 3: 实现数据加载工具
```cpp
// src/DAWorkFlow/DAToolDataLoader.h
#pragma once
#include "DAToolInterface.h"
#include "DAData/DADataManager.h"

class DAWorkFlow_EXPORT DAToolDataLoader : public DAToolInterface {
    Q_OBJECT
    
public:
    explicit DAToolDataLoader(QObject* parent = nullptr);
    
    QVariant execute(const QVariantMap& inputs) override;
    
private:
    DADataManager* m_dataManager;
};

// 实现
QVariant DAToolDataLoader::execute(const QVariantMap& inputs)
{
    QString filePath = inputs["file_path"].toString();
    QString fileType = inputs["file_type"].toString();  // "csv", "excel", "json"
    
    if (fileType == "csv") {
        py::gil_scoped_acquire acquire;
        py::object pd = py::module_::import("pandas");
        py::object df = pd.attr("read_csv")(filePath.toStdString());
        
        // 转换为 DAData
        auto* daData = new DADataPyDataFrame();
        daData->setPyObject(df);
        return QVariant::fromValue(daData);
    }
    
    return QVariant();
}
```

**验收**: 数据加载工具可读取 CSV 文件

#### Step 4: 实现图表生成工具
```cpp
// src/DAWorkFlow/DAToolChartGenerator.h
#pragma once
#include "DAToolInterface.h"
#include "DAFigure/DAFigureWidget.h"

class DAWorkFlow_EXPORT DAToolChartGenerator : public DAToolInterface {
    Q_OBJECT
    
public:
    explicit DAToolChartGenerator(QObject* parent = nullptr);
    
    QVariant execute(const QVariantMap& inputs) override;
    
private:
    DAFigureWidget* m_figureWidget;
};

// 实现
QVariant DAToolChartGenerator::execute(const QVariantMap& inputs)
{
    QString chartType = inputs["chart_type"].toString();  // "line", "scatter", "bar"
    auto* data = inputs["data"].value<DAData*>();
    QString title = inputs["title"].toString();
    
    if (!m_figureWidget) {
        m_figureWidget = new DAFigureWidget();
    }
    
    auto* chart = m_figureWidget->createChart();
    
    if (chartType == "line") {
        // 添加折线图
        chart->addCurve(data->toDataFrame());
    } else if (chartType == "scatter") {
        // 添加散点图
        chart->addScatter(data->toDataFrame());
    }
    
    chart->setChartTitle(title);
    
    // 返回图表路径或引用
    return QVariant::fromValue(m_figureWidget);
}
```

**验收**: 图表生成工具可创建图表

---

### 任务 3.5: 双模式执行桥接开发

**目标**: 同步 crewAI 和异步 LangGraph 执行模式统一适配到工作流引擎，调度透明

**涉及文件**:
- `src/DAWorkFlow/DAWorkflowExecutorAsync.h` - 异步执行器扩展
- `src/DAWorkFlow/DAWorkflowExecutorAsync.cpp`
- `src/DAWorkFlow/DAExecutionMode.h` - 执行模式枚举

**子步骤**:

#### Step 1: 定义执行模式
```cpp
// src/DAWorkFlow/DAExecutionMode.h
#pragma once

namespace DA {

enum class ExecutionMode {
    Synchronous,    // 同步执行（默认）
    Asynchronous,   // 异步执行
    Hybrid          // 混合模式（自动判断）
};

enum class ExecutionPolicy {
    Sequential,     // 顺序执行
    Parallel,       // 并行执行（无依赖节点）
    Optimized       // 优化执行（自动判断）
};

}
```

**验收**: 编译通过

#### Step 2: 扩展执行器支持双模式
```cpp
// src/DAWorkFlow/DAWorkFlowExecuter.h
class DAWorkFlow_EXPORT DAWorkFlowExecuter : public QObject {
    Q_OBJECT
    Q_PROPERTY(DA::ExecutionMode executionMode READ executionMode WRITE setExecutionMode NOTIFY executionModeChanged)
    Q_PROPERTY(DA::ExecutionPolicy executionPolicy READ executionPolicy WRITE setExecutionPolicy NOTIFY executionPolicyChanged)
    
public:
    // 设置执行模式
    void setExecutionMode(DA::ExecutionMode mode);
    DA::ExecutionMode executionMode() const { return m_executionMode; }
    
    void setExecutionPolicy(DA::ExecutionPolicy policy);
    DA::ExecutionPolicy executionPolicy() const { return m_executionPolicy; }
    
    // 统一执行接口（自动判断模式）
    QFuture<bool> execute(DAWorkFlow* workflow);
    
private:
    // 根据节点类型自动判断执行模式
    DA::ExecutionMode determineExecutionMode(DAWorkFlow* workflow) const;
    
    // 同步执行
    bool executeSynchronous(DAWorkFlow* workflow);
    
    // 异步执行
    QFuture<bool> executeAsynchronous(DAWorkFlow* workflow);
    
    // 混合执行
    QFuture<bool> executeHybrid(DAWorkFlow* workflow);
    
    DA::ExecutionMode m_executionMode = DA::ExecutionMode::Hybrid;
    DA::ExecutionPolicy m_executionPolicy = DA::ExecutionPolicy::Optimized;
};
```

**验收**: 编译通过

#### Step 3: 实现执行模式自动判断
```cpp
// src/DAWorkFlow/DAWorkFlowExecuter.cpp
DA::ExecutionMode DAWorkFlowExecuter::determineExecutionMode(DAWorkFlow* workflow) const
{
    if (m_executionMode != DA::ExecutionMode::Hybrid) {
        return m_executionMode;
    }
    
    // 检查是否有异步节点
    bool hasAsyncNodes = false;
    for (auto* node : workflow->nodes()) {
        if (node->isAsyncSupported()) {
            hasAsyncNodes = true;
            break;
        }
    }
    
    return hasAsyncNodes ? DA::ExecutionMode::Asynchronous
                         : DA::ExecutionMode::Synchronous;
}

QFuture<bool> DAWorkFlowExecuter::execute(DAWorkFlow* workflow)
{
    DA::ExecutionMode mode = determineExecutionMode(workflow);
    
    switch (mode) {
        case DA::ExecutionMode::Synchronous:
            return QtConcurrent::run([this, workflow]() {
                return executeSynchronous(workflow);
            });
            
        case DA::ExecutionMode::Asynchronous:
            return executeAsynchronous(workflow);
            
        case DA::ExecutionMode::Hybrid:
            return executeHybrid(workflow);
    }
    
    return QtConcurrent::run([this, workflow]() {
        return executeSynchronous(workflow);
    });
}
```

**验收**: 执行模式自动判断正确

#### Step 4: 实现混合执行
```cpp
QFuture<bool> DAWorkFlowExecuter::executeHybrid(DAWorkFlow* workflow)
{
    return QtConcurrent::run([this, workflow]() {
        auto nodes = workflow->sortedNodes();
        QList<QFuture<QVariant>> pendingFutures;
        
        for (auto* node : nodes) {
            if (m_stopRequested) {
                return false;
            }
            
            Q_EMIT nodeExecutionStarted(node);
            
            if (node->isAsyncSupported()) {
                // 异步节点：启动任务，不等待
                pendingFutures.append(node->execAsync());
            } else {
                // 同步节点：等待所有 pending 任务完成
                for (auto& future : pendingFutures) {
                    future.waitForFinished();
                }
                pendingFutures.clear();
                
                // 执行同步节点
                QVariant result = node->exec();
                Q_EMIT nodeExecutionFinished(node, result);
                propagateOutput(node, result);
            }
        }
        
        // 等待所有异步节点完成
        for (auto& future : pendingFutures) {
            future.waitForFinished();
        }
        
        return true;
    });
}
```

**验收**: 混合模式下同步/异步节点可共存执行

---

## 验收标准

### Phase 3 完成条件（全部必须满足）:
- [ ] crewAI 节点可创建并执行
- [ ] LangChain 节点可创建并执行
- [ ] LangGraph 节点可映射工作流并执行
- [ ] LLM 配置中心可管理多提供商
- [ ] 工具调用可执行数据处理和可视化
- [ ] 同步/异步/混合执行模式可正常切换
- [ ] 所有 AI 框架集成测试通过

### 交付物:
1. `src/DAWorkFlow/DACrewAINode.h/cpp` - crewAI 节点
2. `src/DAWorkFlow/DALangChainNode.h/cpp` - LangChain 节点
3. `src/DAWorkFlow/DALangGraphNode.h/cpp` - LangGraph 节点
4. `src/DAUtils/DALLMConfigCenter.h/cpp` - LLM 配置中心
5. `src/DAWorkFlow/DAToolRegistry.h/cpp` - 工具注册表
6. `src/DAWorkFlow/DAToolDataLoader.h/cpp` - 数据加载工具
7. `src/DAWorkFlow/DAToolChartGenerator.h/cpp` - 图表生成工具

---

## 技术风险

| 风险 ID | 风险描述 | 缓解方案 | 验证方法 |
|---------|----------|----------|----------|
| R1 | crewAI 版本更新导致 API 变更 | 锁定版本，封装隔离层 | 版本升级测试 |
| R2 | LangGraph 异步与 Qt 事件循环冲突 | 独立 Python 线程运行 | 压力测试 |
| R3 | 工具调用导致循环依赖 | 依赖图检测，禁止循环 | 静态分析 |
| R4 | LLM API 密钥泄露 | 严格使用密钥管理器 | 安全审计 |
| R5 | 混合执行模式调度错误 | 详细日志，状态追踪 | 集成测试 |

---

## 依赖关系

```
Phase 2 ──> 任务 3.1 (crewAI)
              │
              ├──> 任务 3.2 (LangChain/LangGraph)
              │
              ├──> 任务 3.3 (LLM 配置中心)
              │
              ├──> 任务 3.4 (工具调用)
              │
              └──> 任务 3.5 (双模式桥接)
```

**前置依赖**: Phase 2 必须完成  
**后置依赖**: Phase 4 依赖 Phase 3 完成

---

## 时间规划

| 周次 | 任务 | 预计工时 |
|------|------|----------|
| Week 1 | 任务 3.1 | 40 小时 |
| Week 2 | 任务 3.2 | 40 小时 |
| Week 3 | 任务 3.3 + 任务 3.4 | 40 小时 |
| Week 4 | 任务 3.5 + 集成测试 | 40 小时 |

**里程碑**: Week 4 结束前完成所有 Phase 3 任务，AI 框架集成可演示
