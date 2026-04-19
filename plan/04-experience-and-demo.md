# Phase 4: 体验优化与 Demo 详细实施计划

## 阶段目标

优化用户体验，提供可演示的示例，完成文档更新和兼容性测试。

**周期**: 2 周  
**优先级**: 🔴 最高  
**前置条件**: Phase 3 100% 完成并通过验收

---

## 任务分解

### 任务 4.1: Agent 示例模板库开发

**目标**: 提供数据清洗、图表生成、报告撰写等常用 Agent 模板，开箱即用

**涉及文件**:
- `resources/templates/agents/` - Agent 模板目录
- `src/DAWorkFlow/DAAgentTemplateManager.h` - Agent 模板管理器
- `src/DAWorkFlow/DAAgentTemplateManager.cpp`
- `resources/templates/workflows/` - 工作流模板目录

**子步骤**:

#### Step 1: 设计模板目录结构
```
resources/templates/
├── agents/
│   ├── data_cleaning_agent.json      # 数据清洗 Agent
│   ├── data_analysis_agent.json      # 数据分析 Agent
│   ├── chart_generator_agent.json    # 图表生成 Agent
│   ├── report_writer_agent.json      # 报告撰写 Agent
│   └── code_executor_agent.json      # 代码执行 Agent
├── workflows/
│   ├── data_etl_workflow.json        # ETL 工作流
│   ├── auto_analysis_workflow.json   # 自动分析工作流
│   └── report_generation_workflow.json # 报告生成工作流
└── llm_configs/
    ├── openai_gpt4.json
    ├── anthropic_claude3.json
    └── local_llama2.json
```

**验收**: 目录结构创建完成

#### Step 2: 创建数据清洗 Agent 模板
```json
{
  "templateId": "data_cleaning_agent",
  "templateName": "数据清洗专家",
  "version": "1.0",
  "description": "自动检测并处理缺失值、异常值、重复数据",
  "category": "Data Processing",
  
  "agent": {
    "role": "数据清洗专家",
    "goal": "清洗数据，确保数据质量，处理缺失值、异常值和格式问题",
    "backstory": "你是一位经验丰富的数据科学家，擅长数据清洗和预处理工作。你能自动识别数据中的问题并采用合适的方法处理。",
    
    "llmProvider": "openai",
    "llmModel": "gpt-4",
    "maxIterations": 10,
    "timeoutSeconds": 300,
    "verbose": true,
    
    "tools": [
      "data_loader",
      "missing_value_detector",
      "outlier_detector",
      "duplicate_remover",
      "data_transformer"
    ],
    
    "parameters": {
      "temperature": 0.3,
      "maxTokens": 2000
    }
  },
  
  "instructions": [
    "1. 加载数据并检查基本信息",
    "2. 检测缺失值并报告",
    "3. 根据数据类型选择合适的填充策略",
    "4. 检测异常值并处理",
    "5. 检查并删除重复记录",
    "6. 输出清洗后的数据和质量报告"
  ],
  
  "exampleInput": {
    "file_path": "data.csv",
    "file_type": "csv"
  },
  
  "expectedOutput": {
    "cleanedData": "清洗后的 DataFrame",
    "qualityReport": "数据质量报告"
  }
}
```

**验收**: 模板文件创建完成

#### Step 3: 创建图表生成 Agent 模板
```json
{
  "templateId": "chart_generator_agent",
  "templateName": "可视化专家",
  "version": "1.0",
  "description": "根据数据特征自动生成合适的可视化图表",
  "category": "Visualization",
  
  "agent": {
    "role": "数据可视化专家",
    "goal": "分析数据特征，选择最合适的图表类型，生成 publication-ready 的图表",
    "backstory": "你是一位专业的数据可视化专家，深知如何通过图表有效传达数据信息。你熟悉各种图表类型的适用场景，能生成符合学术出版标准的图表。",
    
    "llmProvider": "openai",
    "llmModel": "gpt-4",
    "maxIterations": 8,
    "timeoutSeconds": 240,
    
    "tools": [
      "data_analyzer",
      "chart_selector",
      "chart_generator",
      "style_optimizer"
    ],
    
    "parameters": {
      "temperature": 0.5,
      "maxTokens": 2500
    }
  },
  
  "chartSelectionRules": {
    "timeSeries": "line",
    "comparison": "bar",
    "distribution": "histogram",
    "correlation": "scatter",
    "composition": "pie"
  },
  
  "stylePresets": {
    "academic": {
      "dpi": 300,
      "fontSize": 12,
      "colorScheme": "professional"
    },
    "presentation": {
      "dpi": 150,
      "fontSize": 14,
      "colorScheme": "vibrant"
    }
  }
}
```

**验收**: 模板文件创建完成

#### Step 4: 创建完整工作流模板
```json
{
  "templateId": "auto_analysis_workflow",
  "templateName": "自动数据分析工作流",
  "version": "1.0",
  "description": "从数据导入到报告生成的完整自动化流程",
  
  "nodes": [
    {
      "id": "loader",
      "type": "DAStandardNode",
      "name": "数据加载",
      "config": {
        "operation": "load_csv",
        "outputKey": "raw_data"
      }
    },
    {
      "id": "cleaner",
      "type": "DAAgentNode",
      "name": "数据清洗",
      "template": "data_cleaning_agent",
      "inputs": ["raw_data"],
      "outputs": ["cleaned_data", "quality_report"]
    },
    {
      "id": "analyzer",
      "type": "DAAgentNode",
      "name": "数据分析",
      "template": "data_analysis_agent",
      "inputs": ["cleaned_data"],
      "outputs": ["analysis_results", "statistics"]
    },
    {
      "id": "visualizer",
      "type": "DAAgentNode",
      "name": "可视化",
      "template": "chart_generator_agent",
      "inputs": ["cleaned_data", "analysis_results"],
      "outputs": ["charts", "figures"]
    },
    {
      "id": "reporter",
      "type": "DAAgentNode",
      "name": "报告生成",
      "template": "report_writer_agent",
      "inputs": ["quality_report", "analysis_results", "charts"],
      "outputs": ["final_report"]
    }
  ],
  
  "edges": [
    {"from": "loader", "to": "cleaner"},
    {"from": "cleaner", "to": "analyzer"},
    {"from": "analyzer", "to": "visualizer"},
    {"from": "cleaner", "to": "reporter"},
    {"from": "analyzer", "to": "reporter"},
    {"from": "visualizer", "to": "reporter"}
  ],
  
  "metadata": {
    "estimatedTime": "5-10 minutes",
    "difficulty": "beginner",
    "tags": ["automation", "analysis", "report"]
  }
}
```

**验收**: 工作流模板创建完成

#### Step 5: 实现模板管理器
```cpp
// src/DAWorkFlow/DAAgentTemplateManager.h
#pragma once
#include <QObject>
#include <QMap>

class DAWorkFlow_EXPORT DAAgentTemplateManager : public QObject {
    Q_OBJECT
    
public:
    static DAAgentTemplateManager* instance();
    
    // 加载所有模板
    void loadTemplates(const QString& templateDir);
    
    // 获取模板
    QVariantMap getTemplate(const QString& templateId) const;
    
    // 获取所有模板列表
    QStringList getTemplateList() const;
    
    // 获取模板分类
    QStringList getCategories() const;
    
    // 根据模板创建 Agent
    DAAgentNode* createAgentFromTemplate(const QString& templateId,
                                          const QString& name = QString());
    
    // 根据模板创建工作流
    DAWorkFlow* createWorkflowFromTemplate(const QString& templateId);
    
    // 保存自定义模板
    bool saveCustomTemplate(const QString& templateId, const QVariantMap& template);
    
Q_SIGNALS:
    void templatesLoaded();
    void templateCreated(const QString& templateId);
    
private:
    explicit DAAgentTemplateManager(QObject* parent = nullptr);
    void loadTemplateFile(const QString& filePath);
    
    QMap<QString, QVariantMap> m_templates;
    QString m_templateDir;
};
```

**验收**: 模板管理器可加载和创建模板

---

### 任务 4.2: 全流程 Demo 开发

**目标**: 完成「数据导入→AI 自动清洗→自动分析→生成报告」完整 Agent 工作流 Demo

**涉及文件**:
- `demos/auto_analysis/` - Demo 目录
- `demos/auto_analysis/main.cpp` - Demo 主程序
- `demos/auto_analysis/sample_data.csv` - 示例数据
- `demos/auto_analysis/README.md` - Demo 说明

**子步骤**:

#### Step 1: 准备示例数据
```csv
# sample_data.csv - 带问题的示例数据
date,temperature,humidity,pressure,wind_speed
2024-01-01,23.5,65,1013.2,5.2
2024-01-02,,68,1012.8,4.8
2024-01-03,22.1,70,1011.5,
2024-01-04,21.8,72,1010.2,6.1
2024-01-05,999.9,65,1013.5,5.5  # 异常值
2024-01-06,24.2,63,1014.1,4.9
2024-01-06,24.2,63,1014.1,4.9  # 重复记录
2024-01-07,25.1,61,1015.3,5.0
```

**验收**: 示例数据包含缺失值、异常值、重复记录

#### Step 2: 创建 Demo 工作流
```cpp
// demos/auto_analysis/main.cpp
#include <QApplication>
#include "DAWorkFlow/DAWorkFlow.h"
#include "DAWorkFlow/DAWorkFlowExecuter.h"
#include "DAWorkFlow/DAAgentTemplateManager.h"
#include "DAData/DADataManager.h"
#include "DAFigure/DAFigureWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 1. 初始化
    auto* templateManager = DAAgentTemplateManager::instance();
    templateManager->loadTemplates(":/templates");
    
    // 2. 从模板创建工作流
    auto* workflow = templateManager->createWorkflowFromTemplate(
        "auto_analysis_workflow"
    );
    
    // 3. 设置输入数据
    auto* loaderNode = workflow->findNode("loader");
    loaderNode->setProperty("file_path", "sample_data.csv");
    
    // 4. 创建主窗口
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("AI Agent 自动数据分析 Demo");
    mainWindow.resize(1200, 800);
    
    // 5. 创建工作流编辑器
    auto* workflowEditor = createWorkflowEditor(workflow);
    mainWindow.setCentralWidget(workflowEditor);
    
    // 6. 创建监控面板
    auto* monitorWidget = new DAAgentMonitorWidget(&mainWindow);
    monitorWidget->setExecuter(new DAWorkFlowExecuter());
    
    // 7. 显示窗口
    mainWindow.show();
    
    // 8. 自动执行（可选）
    QTimer::singleShot(1000, [&]() {
        auto* executer = new DAWorkFlowExecuter();
        executer->executeWorkflowAsync(workflow);
    });
    
    return app.exec();
}
```

**验收**: Demo 程序可编译运行

#### Step 3: 实现 Demo 自动化流程
```cpp
// 自动执行各个步骤
void runAutoAnalysisDemo()
{
    // Step 1: 加载数据
    auto* dataManager = DADataManager::instance();
    auto rawData = loadCSV("sample_data.csv");
    dataManager->addData(rawData, "raw_data");
    
    // Step 2: AI 自动清洗
    auto* cleanerAgent = createDataCleaningAgent();
    auto cleanedData = cleanerAgent->execute(rawData);
    dataManager->addData(cleanedData, "cleaned_data");
    
    // Step 3: AI 自动分析
    auto* analyzerAgent = createDataAnalysisAgent();
    auto analysisResults = analyzerAgent->execute(cleanedData);
    
    // Step 4: 自动生成图表
    auto* chartAgent = createChartGeneratorAgent();
    auto charts = chartAgent->execute(cleanedData, analysisResults);
    
    // Step 5: 生成报告
    auto* reportAgent = createReportWriterAgent();
    auto report = reportAgent->execute(
        cleanedData,
        analysisResults,
        charts
    );
    
    // Step 6: 显示结果
    displayResults(report, charts);
}
```

**验收**: Demo 可自动执行完整流程

#### Step 4: 编写 Demo 说明文档
```markdown
# AI Agent 自动数据分析 Demo

## 功能演示

本 Demo 展示如何使用 AI Agent 工作流实现自动化数据分析：

1. **数据导入**: 自动读取 CSV 文件
2. **AI 清洗**: 自动检测并处理缺失值、异常值、重复记录
3. **AI 分析**: 自动进行描述性统计、相关性分析
4. **AI 可视化**: 自动生成合适的图表
5. **AI 报告**: 自动生成分析报告

## 运行方法

```bash
# 编译
cmake --build build --target auto_analysis_demo

# 运行
./build/bin/auto_analysis_demo
```

## 预期结果

- 清洗后的数据集
- 统计数据表
- 3-5 个可视化图表
- 完整的分析报告（Markdown 格式）
```

**验收**: 说明文档完整

---

### 任务 4.3: 文档更新

**目标**: 完成 AI Agent 功能使用文档、开发文档、安全指南编写

**涉及文件**:
- `docs/zh/user-guide/ai-agent/` - 用户指南
- `docs/zh/dev-guide/ai-agent/` - 开发指南
- `docs/zh/security-guide.md` - 安全指南

**子步骤**:

#### Step 1: 编写用户指南
```markdown
# AI Agent 用户指南

## 快速开始

### 创建第一个 Agent

1. 打开工作流编辑器
2. 从节点面板拖拽 "Agent" 节点到画布
3. 双击节点打开配置面板
4. 填写 Agent 信息:
   - 名称：我的数据分析 Agent
   - 角色：数据分析专家
   - 目标：分析数据并生成洞察
5. 选择 LLM 提供商和模型
6. 点击"运行"按钮

### 使用模板

1. 点击"模板库"按钮
2. 选择合适的模板（如"数据清洗专家"）
3. 点击"使用模板"
4. 根据实际情况调整配置

## Agent 配置说明

### 基本配置
- **角色**: Agent 的专业身份
- **目标**: Agent 要完成的任务
- **背景故事**: Agent 的背景描述

### LLM 配置
- **提供商**: OpenAI / Anthropic / 本地模型
- **模型**: 选择具体的 LLM 模型
- **Temperature**: 控制输出随机性 (0-1)
- **最大迭代次数**: 防止无限循环

### 工具配置
- 数据加载工具
- 图表生成工具
- 代码执行工具

## 工作流编排

### 连接 Agent

1. 拖拽多个 Agent 节点
2. 从一个节点的输出端口拖拽到另一个节点的输入端口
3. 设置数据传递关系

### 执行控制

- **运行**: 开始执行工作流
- **暂停**: 暂停当前执行
- **停止**: 终止执行
- **重试**: 重新执行失败的节点

## 监控与调试

### 查看执行状态

- 节点颜色表示状态（蓝色=运行，绿色=完成，红色=失败）
- 监控面板显示详细日志
- 调用链路图展示执行顺序

### 调试技巧

1. 启用"详细日志"选项
2. 查看每个迭代的思考过程
3. 检查工具调用输入输出
```

**验收**: 用户指南完成

#### Step 2: 编写开发指南
```markdown
# AI Agent 开发指南

## 架构概述

### 核心组件

- **DAAgentNode**: Agent 节点基类
- **DAAgentConfig**: Agent 配置类
- **DAWorkFlowExecuter**: 异步执行引擎
- **DAToolRegistry**: 工具注册表

### 类图

```
DAAgentNode
├── DACrewAINode (crewAI 集成)
├── DALangChainNode (LangChain 集成)
└── DALangGraphNode (LangGraph 集成)
```

## 自定义 Agent 开发

### Step 1: 继承 DAAgentNode

```cpp
class MyCustomAgent : public DAAgentNode {
    Q_OBJECT
    
public:
    QFuture<QVariant> execAsyncImpl() override;
    
private:
    QVariant executeCustomLogic();
};
```

### Step 2: 实现执行逻辑

```cpp
QFuture<QVariant> MyCustomAgent::execAsyncImpl()
{
    return QtConcurrent::run([this]() {
        // 自定义 Agent 逻辑
        return executeCustomLogic();
    });
}
```

### Step 3: 注册节点

```cpp
DANodeRegistry::instance()->registerNode(
    "MyCustomAgent",
    []() { return new MyCustomAgent("MyAgent"); }
);
```

## 自定义工具开发

### Step 1: 继承 DAToolInterface

```cpp
class MyCustomTool : public DAToolInterface {
    Q_OBJECT
    
public:
    QVariant execute(const QVariantMap& inputs) override;
};
```

### Step 2: 实现工具逻辑

```cpp
QVariant MyCustomTool::execute(const QVariantMap& inputs)
{
    // 工具实现
    QString inputData = inputs["data"].toString();
    
    // 处理...
    
    return result;
}
```

### Step 3: 注册工具

```cpp
DAToolRegistry::instance()->registerTool(new MyCustomTool());
```

## Python API

### 导入模块

```python
import da
from da import Agent, Workflow, Tool
```

### 创建 Agent

```python
agent = da.Agent(
    name="My Agent",
    role="Data Analyst",
    goal="Analyze data",
    llm_provider="openai",
    llm_model="gpt-4"
)
```

### 执行工作流

```python
workflow = da.Workflow()
workflow.add_node(agent)
result = workflow.execute()
```

## 最佳实践

1. **错误处理**: 始终在 Agent 中捕获异常
2. **超时设置**: 为长时间操作设置超时
3. **日志记录**: 使用 Q_EMIT 发送详细日志
4. **资源管理**: 及时释放 Python GIL
```

**验收**: 开发指南完成

#### Step 3: 编写安全指南
```markdown
# AI Agent 安全指南

## 密钥安全

### 存储方式

- ✅ **推荐**: 使用操作系统密钥管理器
  - Windows: Credential Manager
  - macOS: Keychain
  - Linux: Secret Service API

- ❌ **禁止**: 
  - 明文存储在配置文件中
  - 硬编码在代码中
  - 提交到版本控制

### 使用方法

```cpp
// 正确方式
auto* secretManager = DASecretManager::instance();
secretManager->storeSecret("openai_api_key", "sk-...");

// 在 Agent 中引用
agentConfig->setApiKeyRef("openai_api_key");
```

## 执行沙箱

### 限制的危险操作

- `os.system()` - 系统命令执行
- `subprocess.*` - 子进程
- `eval()` / `exec()` - 动态代码执行
- `__import__()` - 动态导入

### 白名单机制

仅允许导入安全模块：
- `pandas`, `numpy`, `scipy` - 数据处理
- `math`, `json`, `re` - 基础工具
- `crewai`, `langchain` - AI 框架

## 资源限制

### 默认配置

| 限制项 | 默认值 | 说明 |
|--------|--------|------|
| 最大迭代次数 | 10 | 防止无限循环 |
| 执行超时 | 300 秒 | 防止长时间占用 |
| API 请求频率 | 60 次/分钟 | 防止滥用 |
| 最大输出长度 | 10000 字符 | 防止输出爆炸 |

### 自定义限制

```cpp
agentConfig->setMaxIterations(20);
agentConfig->setTimeoutSeconds(600);
```

## 数据脱敏

### 自动脱敏模式

日志和输出中自动过滤：
- API 密钥 (`sk-*`, `key-*`)
- Bearer Token
- 密码字段

### 手动脱敏

```cpp
DALogFilter::instance()->addSensitivePattern("custom-pattern");
```

## 网络安全

### 允许的网络请求

- 配置的 LLM API 端点
- 用户明确授权的 URL

### 禁止的网络请求

- 任意 HTTP 请求
- 内网地址访问
- 未知端点连接

## 审计日志

### 记录内容

- Agent 执行时间
- 工具调用记录
- API 调用次数
- 错误和异常

### 日志位置

```
logs/
├── agent_execution.log
├── tool_calls.log
└── api_usage.log
```

## 安全清单

在使用 AI Agent 前，请确认：

- [ ] API 密钥已存储在密钥管理器
- [ ] 执行沙箱已启用
- [ ] 资源限制已配置
- [ ] 日志脱敏已开启
- [ ] 网络访问已限制
```

**验收**: 安全指南完成

---

### 任务 4.4: 兼容性测试与优化

**目标**: 支持 Qt5/Qt6 兼容，性能优化，解决已知 Bug

**涉及文件**:
- `tst/auto_analysis/` - 自动化测试
- `.github/workflows/` - CI 配置
- `CMakeLists.txt` - 构建配置

**子步骤**:

#### Step 1: Qt5/Qt6 兼容性检查
```cpp
// 使用宏处理版本差异
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5 代码
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("Files (*)"));
#else
    // Qt6 代码
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save File"), QString(), tr("Files (*)"));
#endif

// 检查所有新增代码的兼容性
grep -r "QT_VERSION_CHECK" src/DAWorkFlow/
```

**验收**: 所有 Qt 版本差异已处理

#### Step 2: 性能优化
```cpp
// 优化信号槽连接
connect(agentNode, &DAAgentNode::statusChanged,
        monitorWidget, &DAAgentMonitorWidget::updateAgentStatus,
        Qt::QueuedConnection);  // 避免频繁阻塞

// 优化日志输出
class BufferedLogger {
public:
    void append(const QString& message) {
        m_buffer.append(message);
        if (m_buffer.size() >= 100) {
            flush();
        }
    }
    
private:
    QStringList m_buffer;
    void flush();
};

// 优化 Agent 执行缓存
class AgentExecutionCache {
    QCache<QString, QVariant> m_cache;
    
    QVariant getOrExecute(const QString& key, std::function<QVariant()> executor) {
        if (m_cache.contains(key)) {
            return *m_cache[key];
        }
        auto result = executor();
        m_cache.insert(key, new QVariant(result));
        return result;
    }
};
```

**验收**: 性能测试通过

#### Step 3: 编写自动化测试
```cpp
// tst/auto_analysis/test_agent_workflow.cpp
#include <QTest>
#include "DAWorkFlow/DAAgentNode.h"
#include "DAWorkFlow/DAWorkFlowExecuter.h"

class TestAgentWorkflow : public QObject {
    Q_OBJECT
    
private slots:
    void testAgentCreation();
    void testAgentExecution();
    void testWorkflowExecution();
    void testAsyncExecution();
    void testToolCalling();
};

void TestAgentWorkflow::testAgentCreation()
{
    auto* agent = new DAAgentNode("TestAgent");
    agent->config()->setRole("Tester");
    agent->config()->setGoal("Test the system");
    
    QCOMPARE(agent->config()->role(), "Tester");
    QCOMPARE(agent->config()->goal(), "Test the system");
}

void TestAgentWorkflow::testAgentExecution()
{
    auto* agent = new DAAgentNode("TestAgent");
    // 设置 mock LLM
    
    auto future = agent->execAsync();
    future.waitForFinished();
    
    QVERIFY(!future.result().isNull());
}
```

**验收**: 所有测试通过

#### Step 4: 配置 CI 工作流
```yaml
# .github/workflows/ai-agent-tests.yml
name: AI Agent Tests

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [windows-latest, macos-latest, ubuntu-latest]
        qt-version: [5.15, 6.5]
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Setup Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.10'
    
    - name: Install AI dependencies
      run: |
        pip install -r requirements.txt
        pip install pytest
    
    - name: Configure Qt
      uses: jurplel/install-qt-action@v3
      with:
        version: ${{ matrix.qt-version }}
    
    - name: Build
      run: |
        cmake -S . -B build
        cmake --build build --parallel
    
    - name: Run Tests
      run: |
        ctest --test-dir build --output-on-failure
```

**验收**: CI 工作流配置完成

---

## 验收标准

### Phase 4 完成条件（全部必须满足）:
- [ ] Agent 模板库包含至少 5 个模板
- [ ] 全流程 Demo 可正常运行
- [ ] 用户指南、开发指南、安全指南完成
- [ ] Qt5/Qt6 兼容性测试通过
- [ ] 所有自动化测试通过
- [ ] CI 工作流配置完成

### 交付物:
1. `resources/templates/agents/` - Agent 模板（5+ 个）
2. `resources/templates/workflows/` - 工作流模板（3+ 个）
3. `demos/auto_analysis/` - 完整 Demo
4. `docs/zh/user-guide/ai-agent/` - 用户指南
5. `docs/zh/dev-guide/ai-agent/` - 开发指南
6. `docs/zh/security-guide.md` - 安全指南
7. `tst/auto_analysis/` - 自动化测试

---

## 技术风险

| 风险 ID | 风险描述 | 缓解方案 | 验证方法 |
|---------|----------|----------|----------|
| R1 | 模板加载失败 | 添加错误处理和日志 | 异常测试 |
| R2 | Demo 依赖环境问题 | 提供 requirements.txt 和 Docker | 环境隔离测试 |
| R3 | Qt 版本兼容问题 | 充分测试，使用条件编译 | 双版本 CI |
| R4 | 性能瓶颈 | Profiling 分析，优化热点 | 性能测试 |
| R5 | 文档过时 | 文档与代码同步更新 | 文档审查 |

---

## 依赖关系

```
Phase 3 ──> 任务 4.1 (模板库)
              │
              ├──> 任务 4.2 (Demo)
              │
              ├──> 任务 4.3 (文档)
              │
              └──> 任务 4.4 (测试优化)
```

**前置依赖**: Phase 3 必须完成  
**后置依赖**: 项目发布依赖 Phase 4 完成

---

## 时间规划

| 周次 | 任务 | 预计工时 |
|------|------|----------|
| Week 1 | 任务 4.1 + 任务 4.2 | 40 小时 |
| Week 2 | 任务 4.3 + 任务 4.4 | 40 小时 |

**里程碑**: Week 2 结束前完成所有 Phase 4 任务，项目可发布演示
