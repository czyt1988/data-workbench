# Phase 2: Agent 编排能力建设详细实施计划

## 阶段目标

实现 Agent 可视化拖拽编排、运行状态实时监控、安全机制，为用户提供直观的 Agent 工作流编辑和调试能力。

**周期**: 4-5 周  
**优先级**: 🔴 最高  
**前置条件**: Phase 1 100% 完成并通过验收

---

## 任务分解

### 任务 2.1: Agent 节点 UI 组件开发

**目标**: 工作流编辑器支持 Agent 类型节点，展示名称、角色、状态、输入输出

**涉及文件**:
- `src/DAWorkFlow/DAAgentNodeGraphicsItem.h` - Agent 节点图形项
- `src/DAWorkFlow/DAAgentNodeGraphicsItem.cpp`
- `src/DAWorkFlow/DANodeGraphicsScene.h` - 节点场景
- `src/DAGraphicsView/` - 图形视图基础类
- `src/DAWorkFlow/DAAbstractNodeGraphicsItem.h` - 现有节点图形项参考

**子步骤**:

#### Step 1: 设计 Agent 节点视觉样式
```cpp
// Agent 节点视觉特征
class DAAgentNodeGraphicsItem : public DAAbstractNodeGraphicsItem {
public:
    // Agent 节点特有视觉元素
    - 顶部：Agent 名称 + 角色图标（🤖）
    - 中部：当前状态指示器（运行中/完成/失败）
    - 底部：迭代次数进度条
    - 边框：根据状态变色（运行=蓝色，完成=绿色，失败=红色）
};
```

**验收**: UI 设计稿完成，包含：
- 正常状态样式
- 运行中动画效果
- 状态指示器设计
- 输入输出端口布局

#### Step 2: 实现 Agent 节点图形项
```cpp
// src/DAWorkFlow/DAAgentNodeGraphicsItem.h
#pragma once
#include "DAAbstractNodeGraphicsItem.h"
#include "DAAgentNode.h"

class DAWorkFlow_EXPORT DAAgentNodeGraphicsItem : public DAAbstractNodeGraphicsItem {
    Q_OBJECT
    
public:
    explicit DAAgentNodeGraphicsItem(DAAgentNode* node, QGraphicsItem* parent = nullptr);
    ~DAAgentNodeGraphicsItem() override;
    
    // 重绘方法
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;
    
    // 获取节点尺寸
    QSizeF sizeHint() const override;
    
protected:
    // 绘制 Agent 特有元素
    void drawAgentHeader(QPainter* painter);
    void drawStatusIndicator(QPainter* painter);
    void drawIterationProgress(QPainter* painter);
    
    // 状态变化时更新
    void onStatusChanged(const QString& status);
    void onIterationChanged(int iteration);
    
private:
    DAAgentNode* m_agentNode;
    QColor m_statusColor;
    int m_maxDisplayWidth = 200;
};
```

**验收**: 编译通过，节点可正确显示

#### Step 3: 实现状态指示器动画
```cpp
// src/DAWorkFlow/DAAgentNodeGraphicsItem.cpp
void DAAgentNodeGraphicsItem::drawStatusIndicator(QPainter* painter)
{
    if (m_agentNode->status() == "Running") {
        // 运行中：旋转动画
        painter->save();
        painter->setPen(QPen(Qt::blue, 2));
        painter->setBrush(Qt::NoBrush);
        
        // 绘制旋转的圆环
        static int angle = 0;
        angle = (angle + 5) % 360;
        painter->rotate(angle);
        painter->drawEllipse(QPoint(0, 0), 8, 8);
        painter->restore();
    } else if (m_agentNode->status() == "Completed") {
        // 完成：绿色对勾
        painter->setPen(QPen(Qt::green, 2));
        painter->drawLine(-5, 0, -2, 3);
        painter->drawLine(-2, 3, 5, -4);
    } else if (m_agentNode->status() == "Failed") {
        // 失败：红色叉号
        painter->setPen(QPen(Qt::red, 2));
        painter->drawLine(-5, -5, 5, 5);
        painter->drawLine(-5, 5, 5, -5);
    }
}
```

**验收**: 状态指示器动画流畅，状态变化响应及时

#### Step 4: 注册 Agent 节点到图形工厂
```cpp
// src/DAWorkFlow/DANodeGraphicsItemFactory.cpp
QGraphicsItem* DANodeGraphicsItemFactory::create(DAAbstractNode* node)
{
    if (auto* agentNode = qobject_cast<DAAgentNode*>(node)) {
        return new DAAgentNodeGraphicsItem(agentNode);
    }
    // 其他节点类型...
    return new DAAbstractNodeGraphicsItem(node);
}
```

**验收**: Agent 节点可在编辑器中创建并显示

---

### 任务 2.2: Agent 属性配置面板开发

**目标**: 支持配置 Agent 角色、目标、工具、LLM 参数、最大迭代次数、执行超时

**涉及文件**:
- `src/DAWorkFlow/DAAgentConfigWidget.h` - Agent 配置面板
- `src/DAWorkFlow/DAAgentConfigWidget.cpp`
- `src/DAWorkFlow/DAAgentConfig.h` - Phase 1 已创建
- `src/DAGui/DAPropertyBrowser/` - 属性浏览器参考

**子步骤**:

#### Step 1: 设计配置面板 UI 布局
```cpp
// 配置面板结构
class DAAgentConfigWidget : public QWidget {
    Q_OBJECT
    
    // UI 组件
    - QLineEdit* m_nameEdit;           // Agent 名称
    - QTextEdit* m_roleEdit;           // 角色描述
    - QTextEdit* m_goalEdit;           // 目标
    - QTextEdit* m_backstoryEdit;      // 背景故事
    - QComboBox* m_llmProviderCombo;   // LLM 提供商 (OpenAI/Claude/Local)
    - QComboBox* m_llmModelCombo;      // 模型选择
    - QDoubleSpinBox* m_temperatureSpin;  // Temperature
    - QSpinBox* m_maxTokensSpin;       // Max Tokens
    - QSpinBox* m_maxIterationsSpin;   // 最大迭代次数
    - QSpinBox* m_timeoutSpin;         // 超时时间 (秒)
    - QListWidget* m_toolsList;        // 可用工具列表
    - QCheckBox* m_verboseCheck;       // 详细日志
};
```

**验收**: UI 布局设计完成

#### Step 2: 实现配置面板
```cpp
// src/DAWorkFlow/DAAgentConfigWidget.h
#pragma once
#include <QWidget>
#include "DAAgentConfig.h"

class DAWorkFlow_EXPORT DAAgentConfigWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit DAAgentConfigWidget(QWidget* parent = nullptr);
    ~DAAgentConfigWidget() override;
    
    // 设置/获取配置
    void setConfig(DAAgentConfig* config);
    DAAgentConfig* config() const { return m_config; }
    
    // 从 UI 更新配置
    void updateConfigFromUI();
    
    // 从配置更新 UI
    void updateUIFromConfig();
    
Q_SIGNALS:
    void configChanged();
    
private Q_SLOTS:
    void onProviderChanged(const QString& provider);
    void onToolAdded(const QString& tool);
    void onToolRemoved(const QString& tool);
    
private:
    void setupUI();
    void connectSignals();
    
    DAAgentConfig* m_config;
    
    // UI 组件
    QLineEdit* m_nameEdit;
    QTextEdit* m_roleEdit;
    QTextEdit* m_goalEdit;
    QTextEdit* m_backstoryEdit;
    QComboBox* m_llmProviderCombo;
    QComboBox* m_llmModelCombo;
    QDoubleSpinBox* m_temperatureSpin;
    QSpinBox* m_maxTokensSpin;
    QSpinBox* m_maxIterationsSpin;
    QSpinBox* m_timeoutSpin;
    QListWidget* m_toolsList;
    QCheckBox* m_verboseCheck;
};
```

**验收**: 编译通过

#### Step 3: 实现 LLM 配置联动
```cpp
// src/DAWorkFlow/DAAgentConfigWidget.cpp
void DAAgentConfigWidget::onProviderChanged(const QString& provider)
{
    m_llmModelCombo->clear();
    
    if (provider == "OpenAI") {
        m_llmModelCombo->addItems({"gpt-4", "gpt-4-turbo", "gpt-3.5-turbo"});
    } else if (provider == "Anthropic") {
        m_llmModelCombo->addItems({"claude-3-opus", "claude-3-sonnet", "claude-3-haiku"});
    } else if (provider == "Local") {
        m_llmModelCombo->addItems({"llama-2-70b", "mistral-7b", "custom"});
    }
    
    m_config->setLlmProvider(provider);
    Q_EMIT configChanged();
}
```

**验收**: LLM 提供商切换时模型列表正确更新

#### Step 4: 集成到节点属性面板
```cpp
// src/DAWorkFlow/DANodePropertyPanel.cpp
void DANodePropertyPanel::setNode(DAAbstractNode* node)
{
    if (auto* agentNode = qobject_cast<DAAgentNode*>(node)) {
        // 显示 Agent 专用配置面板
        auto* configWidget = new DAAgentConfigWidget(this);
        configWidget->setConfig(agentNode->config());
        setMainWidget(configWidget);
    } else {
        // 使用通用属性面板
        setMainWidget(createDefaultPropertyWidget(node));
    }
}
```

**验收**: 选中 Agent 节点时显示专用配置面板

---

### 任务 2.3: 异步调度引擎开发

**目标**: 支持按有向图执行异步 Agent 工作流，处理输入输出传递，UI 无阻塞

**涉及文件**:
- `src/DAWorkFlow/DAWorkFlowExecuter.h` - Phase 1 已扩展异步接口
- `src/DAWorkFlow/DAWorkFlowExecuter.cpp`
- `src/DAWorkFlow/DAAgentNode.h` - Phase 1 已创建
- `src/DAPyBindQt/DAPythonSignalHandler.h` - Python 信号处理

**子步骤**:

#### Step 1: 实现异步工作流调度器
```cpp
// src/DAWorkFlow/DAWorkFlowExecuter.cpp
QFuture<bool> DAWorkFlowExecuter::executeWorkflowAsyncImpl(DAWorkFlow* workflow)
{
    return QtConcurrent::run([this, workflow]() {
        Q_EMIT executionStarted();
        
        // 1. 拓扑排序获取执行顺序
        auto nodes = workflow->sortedNodes();
        
        // 2. 按顺序执行节点
        for (auto* node : nodes) {
            if (m_stopRequested) {
                Q_EMIT executionFinished(false);
                return false;
            }
            
            Q_EMIT nodeExecutionStarted(node);
            
            QVariant result;
            try {
                if (node->isAsyncSupported()) {
                    // Agent 节点：异步执行
                    auto future = node->execAsync();
                    result = future.result(); // 等待完成
                } else {
                    // 普通节点：同步执行
                    result = node->exec();
                }
                
                Q_EMIT nodeExecutionFinished(node, result);
                
                // 3. 传递输出到下游节点
                propagateOutput(node, result);
                
            } catch (const std::exception& e) {
                Q_EMIT errorOccurred(QString::fromUtf8(e.what()));
                Q_EMIT executionFinished(false);
                return false;
            }
        }
        
        Q_EMIT executionFinished(true);
        return true;
    });
}
```

**验收**: 异步工作流可正常执行

#### Step 2: 实现输入输出传递
```cpp
void DAWorkFlowExecuter::propagateOutput(DAAbstractNode* sourceNode, const QVariant& output)
{
    // 获取所有下游节点
    auto downstreamNodes = sourceNode->downstreamNodes();
    
    for (auto* targetNode : downstreamNodes) {
        // 找到连接边
        auto links = sourceNode->linksTo(targetNode);
        
        for (const auto& link : links) {
            // 设置目标节点的输入
            targetNode->setInputData(link.targetKey, output);
            
            // 检查目标节点所有输入是否已就绪
            if (targetNode->allInputsReady()) {
                // 触发目标节点执行
                executeNode(targetNode);
            }
        }
    }
}
```

**验收**: 数据正确传递到下游节点

#### Step 3: 实现 UI 无阻塞
```cpp
// 确保执行在独立线程，UI 通过信号槽更新
class DAWorkFlowExecuter : public QObject {
    Q_OBJECT
    
public:
    void startAsyncExecution(DAWorkFlow* workflow)
    {
        // 在工作者线程执行
        auto future = executeWorkflowAsyncImpl(workflow);
        
        // 连接完成信号到 UI 更新
        connect(this, &DAWorkFlowExecuter::executionFinished,
                this, &DAWorkFlowExecuter::onExecutionFinished,
                Qt::QueuedConnection); // 确保在 UI 线程执行
    }
    
private Q_SLOTS:
    void onExecutionFinished(bool success)
    {
        // UI 更新代码（在 UI 线程执行）
        updateStatusBar(success ? "执行完成" : "执行失败");
        updateNodeStatusIndicators();
    }
};
```

**验收**: 执行期间 UI 保持响应，可拖动窗口、点击按钮

#### Step 4: 实现并行 Agent 执行（可选优化）
```cpp
// 对于无依赖关系的节点，可并行执行
QFuture<bool> DAWorkFlowExecuter::executeWorkflowAsyncImpl(DAWorkFlow* workflow)
{
    return QtConcurrent::run([this, workflow]() {
        auto nodes = workflow->sortedNodes();
        QList<QFuture<QVariant>> pendingFutures;
        
        for (auto* node : nodes) {
            if (node->allInputsReady() && node->isAsyncSupported()) {
                // 启动异步执行
                pendingFutures.append(node->execAsync());
            }
        }
        
        // 等待所有并行任务完成
        for (auto& future : pendingFutures) {
            future.waitForFinished();
        }
        
        return true;
    });
}
```

**验收**: 无依赖的 Agent 节点并行执行

---

### 任务 2.4: 运行状态监控面板开发

**目标**: 实时展示 Agent 执行进度、输出日志、调用链路，支持暂停、终止、重试

**涉及文件**:
- `src/DAWorkFlow/DAAgentMonitorWidget.h` - Agent 监控面板
- `src/DAWorkFlow/DAAgentMonitorWidget.cpp`
- `src/DAWorkFlow/DAAgentNode.h` - Agent 节点
- `src/DAWorkFlow/DAWorkFlowExecuter.h` - 执行引擎

**子步骤**:

#### Step 1: 设计监控面板 UI
```cpp
// 监控面板布局
class DAAgentMonitorWidget : public QWidget {
    Q_OBJECT
    
    // UI 组件
    - QTreeWidget* m_callTree;         // 调用链路树
    - QTextEdit* m_logOutput;          // 日志输出
    - QProgressBar* m_progressBar;     // 进度条
    - QLabel* m_statusLabel;           // 状态标签
    - QPushButton* m_pauseButton;      // 暂停按钮
    - QPushButton* m_stopButton;       // 终止按钮
    - QPushButton* m_retryButton;      // 重试按钮
    - QTableWidget* m_iterationTable;  // 迭代详情表
};
```

**验收**: UI 设计稿完成

#### Step 2: 实现监控面板
```cpp
// src/DAWorkFlow/DAAgentMonitorWidget.h
#pragma once
#include <QWidget>
#include "DAWorkFlowExecuter.h"
#include "DAAgentNode.h"

class DAWorkFlow_EXPORT DAAgentMonitorWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit DAAgentMonitorWidget(QWidget* parent = nullptr);
    ~DAAgentMonitorWidget() override;
    
    // 设置监控的执行器
    void setExecuter(DAWorkFlowExecuter* executer);
    
    // 更新 Agent 状态
    void updateAgentStatus(DAAgentNode* agent, const QString& status);
    
    // 添加日志
    void appendLog(const QString& message, LogLevel level = Info);
    
    // 添加迭代记录
    void addIterationRecord(DAAgentNode* agent, int iteration,
                           const QString& thought, const QString& action);
    
Q_SIGNALS:
    void pauseRequested();
    void stopRequested();
    void retryRequested();
    
private Q_SLOTS:
    void onNodeExecutionStarted(DAAbstractNode* node);
    void onNodeExecutionFinished(DAAbstractNode* node, const QVariant& result);
    void onAgentThought(const QString& thought);
    void onAgentAction(const QString& action, const QVariant& actionInput);
    void onAgentObservation(const QVariant& observation);
    void onExecutionFinished(bool success);
    
private:
    void setupUI();
    void updateCallTree();
    void updateProgress();
    
    DAWorkFlowExecuter* m_executer;
    QTreeWidget* m_callTree;
    QTextEdit* m_logOutput;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QPushButton* m_pauseButton;
    QPushButton* m_stopButton;
    QPushButton* m_retryButton;
    QTableWidget* m_iterationTable;
    
    enum LogLevel { Info, Warning, Error };
};
```

**验收**: 编译通过

#### Step 3: 实现调用链路可视化
```cpp
void DAAgentMonitorWidget::updateCallTree()
{
    m_callTree->clear();
    
    auto workflow = m_executer->currentWorkflow();
    auto nodes = workflow->nodes();
    
    // 构建树结构
    QMap<DAAbstractNode*, QTreeWidgetItem*> nodeItems;
    
    for (auto* node : nodes) {
        auto* item = new QTreeWidgetItem(m_callTree);
        item->setText(0, node->nodeName());
        
        if (auto* agent = qobject_cast<DAAgentNode*>(node)) {
            item->setText(1, agent->status());
            item->setIcon(0, QIcon(":/icons/agent.png"));
        }
        
        nodeItems[node] = item;
    }
    
    // 添加连接关系
    for (auto* node : nodes) {
        for (auto* downstream : node->downstreamNodes()) {
            // 添加箭头或连线指示
            nodeItems[node]->addChild(nodeItems[downstream]);
        }
    }
}
```

**验收**: 调用链路树正确显示节点依赖关系

#### Step 4: 实现控制按钮功能
```cpp
void DAAgentMonitorWidget::setupUI()
{
    // 连接控制按钮
    connect(m_pauseButton, &QPushButton::clicked,
            this, &DAAgentMonitorWidget::pauseRequested);
    
    connect(m_stopButton, &QPushButton::clicked,
            this, &DAAgentMonitorWidget::stopRequested);
    
    connect(m_retryButton, &QPushButton::clicked,
            this, &DAAgentMonitorWidget::retryRequested);
}

// 在执行器中实现对应功能
void DAWorkFlowExecuter::pauseExecution()
{
    m_paused = true;
    // 等待当前节点完成
}

void DAWorkFlowExecuter::stopExecution()
{
    m_stopRequested = true;
    // 取消所有进行中的任务
    for (auto& future : m_pendingFutures) {
        future.cancel();
    }
}

void DAWorkFlowExecuter::retryExecution()
{
    // 重置状态并重新执行
    resetExecutionState();
    executeWorkflowAsync(m_workflow);
}
```

**验收**: 暂停、终止、重试功能正常工作

---

### 任务 2.5: 安全机制开发

**目标**: API 密钥使用 OS 密钥管理器存储，Python 执行沙箱限制危险操作，自动脱敏敏感信息

**涉及文件**:
- `src/DAUtils/DASecretManager.h` - 密钥管理器
- `src/DAUtils/DASecretManager.cpp`
- `src/DAPyScripts/DAPythonSandbox.h` - Python 沙箱
- `src/DAPyScripts/DAPythonSandbox.cpp`
- `src/DAPyScripts/DALogFilter.h` - 日志过滤器

**子步骤**:

#### Step 1: 实现密钥管理器（Windows）
```cpp
// src/DAUtils/DASecretManager.h
#pragma once
#include <QObject>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#endif

class DAUtils_EXPORT DASecretManager : public QObject {
    Q_OBJECT
    
public:
    static DASecretManager* instance();
    
    // 存储密钥
    bool storeSecret(const QString& keyName, const QString& secret);
    
    // 读取密钥
    QString getSecret(const QString& keyName);
    
    // 删除密钥
    bool deleteSecret(const QString& keyName);
    
    // 检查密钥是否存在
    bool hasSecret(const QString& keyName);
    
Q_SIGNALS:
    void secretStored(const QString& keyName);
    void secretDeleted(const QString& keyName);
    
private:
    explicit DASecretManager(QObject* parent = nullptr);
    
#ifdef Q_OS_WIN
    // Windows Credential Manager
    bool storeSecretWin(const QString& keyName, const QString& secret);
    QString getSecretWin(const QString& keyName);
    bool deleteSecretWin(const QString& keyName);
#endif
    
#ifdef Q_OS_MAC
    // macOS Keychain
    bool storeSecretMac(const QString& keyName, const QString& secret);
    QString getSecretMac(const QString& keyName);
    bool deleteSecretMac(const QString& keyName);
#endif
};
```

**验收**: 密钥可安全存储和读取

#### Step 2: 实现密钥存储 Windows 版本
```cpp
// src/DAUtils/DASecretManager.cpp (Windows)
bool DASecretManager::storeSecretWin(const QString& keyName, const QString& secret)
{
    CREDENTIALW cred = {0};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = (wchar_t*)keyName.utf16();
    cred.CredentialBlobSize = secret.size() * sizeof(wchar_t);
    cred.CredentialBlob = (LPBYTE)secret.utf16();
    cred.Persist = CRED_PERSIST_CURRENT_USER;
    
    BOOL result = CredWriteW(&cred, 0);
    return result != FALSE;
}

QString DASecretManager::getSecretWin(const QString& keyName)
{
    CREDENTIALW* cred = nullptr;
    if (!CredReadW(keyName.utf16(), CRED_TYPE_GENERIC, 0, &cred)) {
        return QString();
    }
    
    QString secret = QString::fromUtf16((ushort*)cred->CredentialBlob,
                                        cred->CredentialBlobSize / sizeof(wchar_t));
    CredFree(cred);
    return secret;
}
```

**验收**: 密钥在 Windows Credential Manager 中可查看

#### Step 3: 实现 Python 沙箱
```cpp
// src/DAPyScripts/DAPythonSandbox.h
#pragma once
#include <QObject>

class DAPyScripts_EXPORT DAPythonSandbox : public QObject {
    Q_OBJECT
    
public:
    explicit DAPythonSandbox(QObject* parent = nullptr);
    
    // 初始化沙箱环境
    void initialize();
    
    // 执行受限 Python 代码
    QVariant executeRestricted(const QString& code);
    
    // 添加允许的模块
    void allowModule(const QString& moduleName);
    
    // 添加禁止的函数
    void blockFunction(const QString& functionName);
    
Q_SIGNALS:
    void executionTimeout();
    void dangerousOperationBlocked(const QString& operation);
    
private:
    // 危险函数黑名单
    QStringList m_blockedFunctions = {
        "os.system", "os.popen", "subprocess.call",
        "eval", "exec", "compile",
        "__import__", "importlib.import_module"
    };
    
    // 允许模块白名单
    QStringList m_allowedModules = {
        "pandas", "numpy", "scipy",
        "math", "json", "re",
        "crewai", "langchain", "langgraph"
    };
    
    int m_executionTimeout = 30000; // 30 秒
};
```

**验收**: 危险函数调用被阻止

#### Step 4: 实现沙箱限制逻辑
```python
# Python 沙箱代码（在 Python 端执行）
def setup_sandbox():
    import sys
    import builtins
    
    # 保存原始 __import__
    original_import = builtins.__import__
    
    # 白名单模块
    ALLOWED_MODULES = {'pandas', 'numpy', 'scipy', 'math', 'json', 're'}
    
    def restricted_import(name, *args, **kwargs):
        if name not in ALLOWED_MODULES:
            raise ImportError(f"Module '{name}' is not allowed in sandbox")
        return original_import(name, *args, **kwargs)
    
    # 替换 __import__
    builtins.__import__ = restricted_import
    
    # 禁用危险函数
    dangerous_funcs = ['system', 'popen', 'exec', 'eval']
    for func in dangerous_funcs:
        if hasattr(builtins, func):
            delattr(builtins, func)
```

**验收**: 尝试导入未授权模块时抛出异常

#### Step 5: 实现日志脱敏
```cpp
// src/DAPyScripts/DALogFilter.h
#pragma once
#include <QObject>
#include <QRegularExpression>

class DAPyScripts_EXPORT DALogFilter : public QObject {
    Q_OBJECT
    
public:
    explicit DALogFilter(QObject* parent = nullptr);
    
    // 过滤敏感信息
    QString filter(const QString& logMessage);
    
    // 添加敏感模式
    void addSensitivePattern(const QString& pattern);
    
Q_SIGNALS:
    void sensitiveDataFiltered(const QString& pattern);
    
private:
    QList<QRegularExpression> m_sensitivePatterns;
    
    void setupDefaultPatterns();
};

// 实现
void DALogFilter::setupDefaultPatterns()
{
    // API 密钥模式
    m_sensitivePatterns << QRegularExpression("sk-[a-zA-Z0-9]{32,}");
    m_sensitivePatterns << QRegularExpression("key-[a-zA-Z0-9]{32,}");
    
    // Bearer Token
    m_sensitivePatterns << QRegularExpression("Bearer\\s+[a-zA-Z0-9\\-_\\.]+");
    
    // 密码模式
    m_sensitivePatterns << QRegularExpression("password[\"']?\\s*[:=]\\s*[\"']?[^\"'\\s]+");
}

QString DALogFilter::filter(const QString& logMessage)
{
    QString filtered = logMessage;
    
    for (const auto& pattern : m_sensitivePatterns) {
        filtered.replace(pattern, "[REDACTED]");
    }
    
    return filtered;
}
```

**验收**: 日志中敏感信息被替换为 `[REDACTED]`

---

### 任务 2.6: Agent 工作流保存/加载功能

**目标**: Agent 工作流可保存为项目文件，密钥不明文存储，加载后可正常执行

**涉及文件**:
- `src/DAWorkFlow/DAWorkFlow.h` - 工作流类
- `src/DAWorkFlow/DAAgentNode.h` - Agent 节点
- `src/DAProject/DAProjectFile.h` - 项目文件
- `src/DAUtils/DASecretManager.h` - 密钥管理器

**子步骤**:

#### Step 1: 设计项目文件格式
```xml
<!-- 项目文件 XML 结构 -->
<DAWorkFlow version="2.0">
    <Nodes>
        <Node id="agent1" type="DAAgentNode" name="Data Analyst">
            <Properties>
                <Property name="role">数据分析专家</Property>
                <Property name="goal">分析数据并生成报告</Property>
                <Property name="llmProvider">OpenAI</Property>
                <Property name="llmModel">gpt-4</Property>
                <Property name="maxIterations">10</Property>
                <Property name="timeoutSeconds">300</Property>
            </Properties>
            <LLMConfig>
                <Property name="temperature">0.7</Property>
                <Property name="maxTokens">2000</Property>
            </LLMConfig>
            <Tools>
                <Tool>data_loader</Tool>
                <Tool>chart_generator</Tool>
            </Tools>
            <SecretKeyRef>openai_api_key</SecretKeyRef>
            <!-- 注意：不存储实际密钥，只存储引用 -->
        </Node>
        
        <Node id="node2" type="DAStandardNode">
            <!-- 普通节点 -->
        </Node>
    </Nodes>
    
    <Links>
        <Link from="agent1" to="node2" outputKey="result" inputKey="input"/>
    </Links>
</DAWorkFlow>
```

**验收**: XML 格式设计完成

#### Step 2: 实现 Agent 节点序列化
```cpp
// src/DAWorkFlow/DAAgentNode.cpp
void DAAgentNode::saveToXml(QXmlStreamWriter& writer) const
{
    DAAbstractNode::saveToXml(writer);
    
    // 保存 Agent 配置
    writer.writeTextElement("role", m_config->role());
    writer.writeTextElement("goal", m_config->goal());
    writer.writeTextElement("backstory", m_config->backstory());
    writer.writeTextElement("llmProvider", m_config->llmProvider());
    writer.writeTextElement("llmModel", m_config->llmModel());
    writer.writeTextElement("maxIterations", QString::number(m_config->maxIterations()));
    writer.writeTextElement("timeoutSeconds", QString::number(m_config->timeoutSeconds()));
    
    // 保存 LLM 参数
    writer.writeStartElement("llmParams");
    for (auto it = m_config->llmParams().begin();
         it != m_config->llmParams().end(); ++it) {
        writer.writeTextElement(it.key(), it.value().toString());
    }
    writer.writeEndElement();
    
    // 保存工具列表
    writer.writeStartElement("tools");
    for (const auto& tool : m_tools) {
        writer.writeTextElement("tool", tool);
    }
    writer.writeEndElement();
    
    // 保存密钥引用（不是实际密钥）
    if (!m_config->llmProvider().isEmpty()) {
        QString keyRef = m_config->llmProvider().toLower() + "_api_key";
        writer.writeTextElement("secretKeyRef", keyRef);
    }
}
```

**验收**: Agent 节点可正确保存到 XML

#### Step 3: 实现 Agent 节点反序列化
```cpp
bool DAAgentNode::loadFromXml(QXmlStreamReader& reader)
{
    if (!DAAbstractNode::loadFromXml(reader)) {
        return false;
    }
    
    // 加载 Agent 配置
    while (!reader.atEnd()) {
        reader.readNext();
        
        if (reader.isEndElement() && reader.name() == "Node") {
            break;
        }
        
        if (reader.isStartElement()) {
            QString name = reader.name().toString();
            
            if (name == "role") {
                m_config->setRole(reader.readElementText());
            } else if (name == "goal") {
                m_config->setGoal(reader.readElementText());
            } else if (name == "backstory") {
                m_config->setBackstory(reader.readElementText());
            } else if (name == "llmProvider") {
                m_config->setLlmProvider(reader.readElementText());
            } else if (name == "llmModel") {
                m_config->setLlmModel(reader.readElementText());
            } else if (name == "maxIterations") {
                m_config->setMaxIterations(reader.readElementText().toInt());
            } else if (name == "timeoutSeconds") {
                m_config->setTimeoutSeconds(reader.readElementText().toInt());
            } else if (name == "llmParams") {
                // 加载 LLM 参数
                while (!reader.atEnd()) {
                    reader.readNext();
                    if (reader.isEndElement() && reader.name() == "llmParams") {
                        break;
                    }
                    if (reader.isStartElement()) {
                        m_config->llmParams()[reader.name().toString()] = reader.readElementText();
                    }
                }
            } else if (name == "tools") {
                // 加载工具列表
                while (!reader.atEnd()) {
                    reader.readNext();
                    if (reader.isEndElement() && reader.name() == "tools") {
                        break;
                    }
                    if (reader.isStartElement() && reader.name() == "tool") {
                        m_tools.append(reader.readElementText());
                    }
                }
            }
        }
    }
    
    return true;
}
```

**验收**: Agent 节点可从 XML 正确加载

#### Step 4: 实现密钥加载时从密钥管理器读取
```cpp
// src/DAWorkFlow/DAWorkFlow.cpp
bool DAWorkFlow::loadProject(const QString& filePath)
{
    // 加载 XML
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QXmlStreamReader reader(&file);
    
    // 解析节点
    while (!reader.atEnd()) {
        reader.readNext();
        
        if (reader.isStartElement() && reader.name() == "Node") {
            auto* node = createNodeFromXml(reader);
            
            // 如果是 Agent 节点，从密钥管理器加载密钥
            if (auto* agentNode = qobject_cast<DAAgentNode*>(node)) {
                QString keyRef = agentNode->secretKeyRef();
                if (!keyRef.isEmpty()) {
                    // 从密钥管理器获取实际密钥
                    QString secret = DASecretManager::instance()->getSecret(keyRef);
                    if (!secret.isEmpty()) {
                        // 设置到 Agent 配置中
                        agentNode->config()->llmParams()["api_key"] = secret;
                    }
                }
            }
        }
    }
    
    return true;
}
```

**验收**: 加载项目后 Agent 可正常执行（密钥从密钥管理器读取）

#### Step 5: 实现密钥验证
```cpp
// 加载时验证密钥是否存在
bool DAWorkFlow::validateAgentSecrets() const
{
    for (auto* node : m_nodes) {
        if (auto* agentNode = qobject_cast<DAAgentNode*>(node)) {
            QString keyRef = agentNode->secretKeyRef();
            if (!keyRef.isEmpty()) {
                if (!DASecretManager::instance()->hasSecret(keyRef)) {
                    Q_EMIT secretMissing(keyRef);
                    return false;
                }
            }
        }
    }
    return true;
}
```

**验收**: 密钥缺失时给出明确提示

---

## 验收标准

### Phase 2 完成条件（全部必须满足）:
- [ ] Agent 节点可在编辑器中拖拽创建
- [ ] Agent 属性面板可配置所有参数
- [ ] 异步工作流可正常执行，UI 无阻塞
- [ ] 监控面板实时显示执行状态
- [ ] 暂停、终止、重试功能正常
- [ ] API 密钥存储在 OS 密钥管理器
- [ ] Python 沙箱阻止危险操作
- [ ] 日志自动脱敏敏感信息
- [ ] 工作流可保存/加载，密钥不明文存储

### 交付物:
1. `src/DAWorkFlow/DAAgentNodeGraphicsItem.h/cpp` - Agent 节点 UI
2. `src/DAWorkFlow/DAAgentConfigWidget.h/cpp` - Agent 配置面板
3. `src/DAWorkFlow/DAAgentMonitorWidget.h/cpp` - 监控面板
4. `src/DAUtils/DASecretManager.h/cpp` - 密钥管理器
5. `src/DAPyScripts/DAPythonSandbox.h/cpp` - Python 沙箱
6. `src/DAPyScripts/DALogFilter.h/cpp` - 日志过滤器

---

## 技术风险

| 风险 ID | 风险描述 | 缓解方案 | 验证方法 |
|---------|----------|----------|----------|
| R1 | 异步执行导致内存泄漏 | 使用智能指针，RAII 管理资源 | 长时间运行测试 |
| R2 | 密钥管理器跨平台兼容性 | 分别实现 Windows/macOS/Linux 版本 | 跨平台测试 |
| R3 | Python 沙箱被绕过 | 多层防护，定期安全审计 | 渗透测试 |
| R4 | 监控面板更新频繁导致 UI 卡顿 | 节流更新，批量处理信号 | 性能分析 |
| R5 | 工作流文件版本兼容性 | 实现版本迁移逻辑 | 版本升级测试 |

---

## 依赖关系

```
Phase 1 ──> 任务 2.1 ──> 任务 2.2
              │
              └──> 任务 2.3 ──> 任务 2.4
                                   │
              任务 2.5 ────────────┘
              任务 2.6 (最后)
```

**前置依赖**: Phase 1 必须完成  
**后置依赖**: Phase 3 依赖 Phase 2 完成

---

## 时间规划

| 周次 | 任务 | 预计工时 |
|------|------|----------|
| Week 1 | 任务 2.1 + 任务 2.2 | 40 小时 |
| Week 2 | 任务 2.3 | 40 小时 |
| Week 3 | 任务 2.4 | 40 小时 |
| Week 4 | 任务 2.5 | 40 小时 |
| Week 5 | 任务 2.6 + 集成测试 | 40 小时 |

**里程碑**: Week 5 结束前完成所有 Phase 2 任务，Agent 编排功能可演示
