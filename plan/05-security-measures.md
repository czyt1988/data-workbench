# Phase 5: 安全措施详细实施计划

## 阶段目标

实现完整的安全防护体系，包括密钥安全、执行沙箱、资源限制、数据脱敏、网络安全和审计日志。

**周期**: 2 周（与 Phase 2 并行进行）  
**优先级**: 🔴 最高（阻断性）  
**前置条件**: Phase 1 完成（Python 环境就绪）

---

## 任务分解

### 任务 5.1: 密钥安全存储

**目标**: API 密钥存储在 Windows Credential Manager/macOS Keychain/Linux Secret Service，项目文件仅存储引用 ID

**涉及文件**:
- `src/DAUtils/DASecretManager.h` - Phase 2 已创建
- `src/DAUtils/DASecretManager.cpp` - Phase 2 已创建
- `src/DAUtils/DASecretManager_win.cpp` - Windows 实现
- `src/DAUtils/DASecretManager_mac.cpp` - macOS 实现
- `src/DAUtils/DASecretManager_linux.cpp` - Linux 实现

**子步骤**:

#### Step 1: 实现 Windows Credential Manager 集成
```cpp
// src/DAUtils/DASecretManager_win.cpp
#include <windows.h>
#include <wincred.h>

bool DASecretManager::storeSecretWin(const QString& keyName, const QString& secret)
{
    CREDENTIALW cred = {0};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = (wchar_t*)keyName.utf16();
    cred.CredentialBlobSize = secret.size() * sizeof(wchar_t);
    cred.CredentialBlob = (LPBYTE)secret.utf16();
    cred.Persist = CRED_PERSIST_CURRENT_USER;
    cred.UserName = NULL;
    
    // 设置属性
    cred.AttributeCount = 2;
    CREDENTIAL_ATTRIBUTEW attrs[2];
    
    // 属性 1: 应用标识
    attrs[0].Keyword = L"Application";
    attrs[0].Value = (LPBYTE)L"DataWorkbench";
    attrs[0].ValueSize = sizeof(L"DataWorkbench");
    
    // 属性 2: 创建时间
    attrs[1].Keyword = L"CreatedAt";
    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    attrs[1].Value = (LPBYTE)timestamp.utf16();
    attrs[1].ValueSize = timestamp.size() * sizeof(wchar_t);
    
    cred.Attributes = attrs;
    
    BOOL result = CredWriteW(&cred, 0);
    if (!result) {
        qWarning() << "Failed to store secret:" << GetLastError();
        return false;
    }
    
    return true;
}

QString DASecretManager::getSecretWin(const QString& keyName)
{
    CREDENTIALW* cred = nullptr;
    
    BOOL result = CredReadW(
        keyName.utf16(),
        CRED_TYPE_GENERIC,
        0,
        &cred
    );
    
    if (!result) {
        DWORD error = GetLastError();
        if (error == ERROR_NOT_FOUND) {
            qWarning() << "Secret not found:" << keyName;
        } else {
            qWarning() << "Failed to read secret:" << error;
        }
        return QString();
    }
    
    // 解密密钥
    QString secret = QString::fromUtf16(
        (ushort*)cred->CredentialBlob,
        cred->CredentialBlobSize / sizeof(wchar_t)
    );
    
    CredFree(cred);
    return secret;
}

bool DASecretManager::deleteSecretWin(const QString& keyName)
{
    BOOL result = CredDeleteW(
        keyName.utf16(),
        CRED_TYPE_GENERIC,
        0
    );
    
    if (!result) {
        qWarning() << "Failed to delete secret:" << GetLastError();
        return false;
    }
    
    return true;
}
```

**验收**: 密钥可在 Windows Credential Manager 中查看和管理

#### Step 2: 实现 macOS Keychain 集成
```cpp
// src/DAUtils/DASecretManager_mac.cpp
#include <Security/Security.h>

bool DASecretManager::storeSecretMac(const QString& keyName, const QString& secret)
{
    // 删除旧密钥（如果存在）
    SecKeychainItemRef item = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        0, nullptr,
        keyName.size(), keyName.utf8().constData(),
        nullptr,
        &item
    );
    
    if (status == errSecSuccess && item) {
        SecKeychainItemDelete(item);
        CFRelease(item);
    }
    
    // 添加新密钥
    const char* serviceName = "DataWorkbench";
    NSData* secretData = [NSData dataWithBytes:secret.utf8().constData()
                                        length:secret.size()];
    
    status = SecKeychainAddGenericPassword(
        nullptr,
        strlen(serviceName), serviceName,
        keyName.size(), keyName.utf8().constData(),
        secretData.length, [secretData bytes],
        &item
    );
    
    if (status != errSecSuccess) {
        qWarning() << "Failed to store secret in Keychain:" << status;
        return false;
    }
    
    if (item) {
        CFRelease(item);
    }
    
    return true;
}

QString DASecretManager::getSecretMac(const QString& keyName)
{
    void* data = nullptr;
    UInt32 length = 0;
    
    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        0, nullptr,
        keyName.size(), keyName.utf8().constData(),
        &length,
        &data,
        nullptr
    );
    
    if (status != errSecSuccess) {
        qWarning() << "Failed to read secret from Keychain:" << status;
        return QString();
    }
    
    QString secret = QString::fromUtf8((const char*)data, length);
    
    // 释放内存（SecKeychain 分配）
    SecKeychainItemFreeContent(nullptr, data);
    
    return secret;
}
```

**验收**: 密钥可在 macOS Keychain 中查看

#### Step 3: 实现 Linux Secret Service 集成
```cpp
// src/DAUtils/DASecretManager_linux.cpp
#include <libsecret/secret.h>

// 定义 Secret Service schema
#define SECRET_SCHEMA_TYPE_DATA_WORKBENCH "data_workbench_secret"
#define SECRET_SCHEMA_DATA_WORKBENCH { \
    SECRET_SCHEMA_TYPE_DATA_WORKBENCH, \
    SECRET_SCHEMA_NONE }

bool DASecretManager::storeSecretLinux(const QString& keyName, const QString& secret)
{
    GError* error = nullptr;
    
    // 存储密钥
    secret_password_store_sync(
        &SECRET_SCHEMA_DATA_WORKBENCH,
        SECRET_COLLECTION_DEFAULT,
        keyName.utf8().constData(),
        secret.utf8().constData(),
        nullptr,  // 取消属性
        nullptr,
        &error
    );
    
    if (error) {
        qWarning() << "Failed to store secret:" << error->message;
        g_error_free(error);
        return false;
    }
    
    return true;
}

QString DASecretManager::getSecretLinux(const QString& keyName)
{
    GError* error = nullptr;
    
    gchar* password = secret_password_lookup_sync(
        &SECRET_SCHEMA_DATA_WORKBENCH,
        nullptr,
        &error,
        keyName.utf8().constData(),
        SECRET_SCHEMA_NONE
    );
    
    if (error) {
        qWarning() << "Failed to read secret:" << error->message;
        g_error_free(error);
        return QString();
    }
    
    if (!password) {
        return QString();
    }
    
    QString secret = QString::fromUtf8(password);
    secret_password_free(password);
    
    return secret;
}
```

**验收**: 密钥可在 Linux Secret Service 中查看

#### Step 4: 实现密钥引用验证
```cpp
// src/DAWorkFlow/DAWorkFlow.cpp
bool DAWorkFlow::validateSecretReferences() const
{
    QStringList missingSecrets;
    
    for (auto* node : m_nodes) {
        if (auto* agentNode = qobject_cast<DAAgentNode*>(node)) {
            QString keyRef = agentNode->secretKeyRef();
            
            if (!keyRef.isEmpty()) {
                if (!DASecretManager::instance()->hasSecret(keyRef)) {
                    missingSecrets.append(keyRef);
                }
            }
        }
    }
    
    if (!missingSecrets.isEmpty()) {
        Q_EMIT secretsMissing(missingSecrets);
        return false;
    }
    
    return true;
}

// 在加载项目后调用
bool DAWorkFlow::loadProject(const QString& filePath)
{
    // ... 加载 XML ...
    
    // 验证密钥引用
    if (!validateSecretReferences()) {
        // 弹出对话框提示用户添加缺失的密钥
        showMissingSecretsDialog();
        return false;
    }
    
    return true;
}
```

**验收**: 加载项目时自动验证密钥是否存在

---

### 任务 5.2: Python 执行沙箱

**目标**: 限制危险系统调用，设置执行超时、最大迭代次数

**涉及文件**:
- `src/DAPyScripts/DAPythonSandbox.h` - Phase 2 已创建
- `src/DAPyScripts/DAPythonSandbox.cpp` - Phase 2 已创建
- `src/DAPyScripts/sandbox_restricted.py` - Python 沙箱代码

**子步骤**:

#### Step 1: 实现 Python 沙箱初始化
```python
# src/DAPyScripts/sandbox_restricted.py
"""
Python 沙箱环境 - 限制危险操作
"""

import sys
import builtins
import types

# 白名单模块
ALLOWED_MODULES = {
    # 核心库
    'math', 're', 'json', 'datetime', 'collections',
    'itertools', 'functools', 'operator', 'statistics',
    
    # 数据处理
    'pandas', 'numpy', 'scipy',
    
    # AI 框架
    'crewai', 'langchain', 'langgraph', 'openai',
    
    # 内部模块
    'da', 'da_data', 'da_figure', 'da_workflow'
}

# 黑名单函数
BLOCKED_FUNCTIONS = {
    # 系统命令
    'os.system', 'os.popen', 'os.spawn', 'os.exec',
    'subprocess.call', 'subprocess.run', 'subprocess.Popen',
    
    # 代码执行
    'eval', 'exec', 'compile',
    
    # 导入
    '__import__', 'importlib.import_module',
    
    # 其他危险函数
    'input', 'open'  # 文件访问
}

def setup_sandbox():
    """设置沙箱环境"""
    
    # 1. 限制 __import__
    original_import = builtins.__import__
    
    def restricted_import(name, globals=None, locals=None, fromlist=(), level=0):
        # 检查模块是否在白名单
        base_module = name.split('.')[0]
        if base_module not in ALLOWED_MODULES:
            raise ImportError(
                f"Module '{name}' is not allowed in sandbox. "
                f"Allowed modules: {', '.join(ALLOWED_MODULES)}"
            )
        return original_import(name, globals, locals, fromlist, level)
    
    builtins.__import__ = restricted_import
    
    # 2. 禁用危险函数
    import os
    import subprocess
    
    for func_path in BLOCKED_FUNCTIONS:
        try:
            parts = func_path.split('.')
            if len(parts) == 2:
                module_name, func_name = parts
                module = getattr(sys.modules.get(module_name), None)
                if module and hasattr(module, func_name):
                    setattr(module, func_name, None)
            elif len(parts) == 1:
                if hasattr(builtins, parts[0]):
                    delattr(builtins, parts[0])
        except (KeyError, AttributeError):
            pass  # 函数不存在，跳过
    
    # 3. 限制内置危险函数
    builtins.eval = None
    builtins.exec = None
    
    print("Sandbox initialized successfully")

# 自动执行
setup_sandbox()
```

**验收**: 尝试导入未授权模块时抛出异常

#### Step 2: 实现执行超时
```cpp
// src/DAPyScripts/DAPythonSandbox.cpp
QVariant DAPythonSandbox::executeRestricted(const QString& code)
{
    py::gil_scoped_acquire acquire;
    
    try {
        // 创建执行上下文
        py::dict globals;
        py::dict locals;
        
        // 加载沙箱环境
        py::exec(R"(
import sys
sys.path.insert(0, 'src/DAPyScripts')
import sandbox_restricted
)", globals, locals);
        
        // 设置超时
        QFutureWatcher<QVariant> watcher;
        QEventLoop loop;
        connect(&watcher, &QFutureWatcher<QVariant>::finished,
                &loop, &QEventLoop::quit);
        
        // 在独立线程执行
        auto future = QtConcurrent::run([this, code, globals, locals]() {
            try {
                py::object result = py::exec(code.toStdString(), globals, locals);
                return py::object(result).cast<QVariant>();
            } catch (const py::error_already_set& e) {
                throw std::runtime_error(e.what());
            }
        });
        
        watcher.setFuture(future);
        
        // 等待完成或超时
        if (!watcher.waitForFinished(m_executionTimeout)) {
            Q_EMIT executionTimeout();
            throw std::runtime_error("Execution timeout");
        }
        
        return future.result();
        
    } catch (const std::exception& e) {
        Q_EMIT errorOccurred(QString::fromUtf8(e.what()));
        throw;
    }
}
```

**验收**: 超时执行被中断并抛出异常

#### Step 3: 实现最大迭代次数限制
```cpp
// src/DAWorkFlow/DAAgentNode.cpp
QVariant DAAgentNode::executeAgentLoop()
{
    for (m_currentIteration = 0;
         m_currentIteration < m_config->maxIterations();
         ++m_currentIteration) {
        
        // 检查取消请求
        if (m_cancelRequested) {
            setStatus("Cancelled");
            Q_EMIT executionCancelled();
            return QVariant();
        }
        
        // 检查超时
        if (m_executionTimer.elapsed() > m_config->timeoutSeconds() * 1000) {
            setStatus("Timeout");
            Q_EMIT errorOccurred("Execution timeout");
            throw std::runtime_error("Agent execution timeout");
        }
        
        // 执行迭代
        try {
            QString thought = generateThought();
            Q_EMIT agentThought(thought);
            
            auto [action, actionInput] = decideAction();
            Q_EMIT agentAction(action, actionInput);
            
            QVariant observation = executeAction(action, actionInput);
            Q_EMIT agentObservation(observation);
            
            // 检查是否完成
            if (isGoalAchieved(observation)) {
                return generateFinalResponse(observation);
            }
            
        } catch (const std::exception& e) {
            setStatus("Error");
            Q_EMIT errorOccurred(QString::fromUtf8(e.what()));
            throw;
        }
    }
    
    // 超过最大迭代次数
    setStatus("MaxIterationsExceeded");
    Q_EMIT errorOccurred(
        QString("Agent exceeded maximum iterations (%1)")
        .arg(m_config->maxIterations())
    );
    throw std::runtime_error("Max iterations exceeded");
}
```

**验收**: 超过最大迭代次数时自动停止

---

### 任务 5.3: 数据脱敏

**目标**: 日志和输出自动过滤敏感信息

**涉及文件**:
- `src/DAPyScripts/DALogFilter.h` - Phase 2 已创建
- `src/DAPyScripts/DALogFilter.cpp` - Phase 2 已创建
- `src/DAUtils/DASensitiveDataDetector.h` - 敏感数据检测器
- `src/DAUtils/DASensitiveDataDetector.cpp`

**子步骤**:

#### Step 1: 实现敏感数据检测器
```cpp
// src/DAUtils/DASensitiveDataDetector.h
#pragma once
#include <QObject>
#include <QRegularExpression>

class DAUtils_EXPORT DASensitiveDataDetector : public QObject {
    Q_OBJECT
    
public:
    static DASensitiveDataDetector* instance();
    
    // 检测并脱敏
    QString redact(const QString& text);
    
    // 检测是否包含敏感数据
    bool containsSensitiveData(const QString& text);
    
    // 获取检测到的敏感数据类型
    QStringList detectSensitiveTypes(const QString& text);
    
    // 添加自定义模式
    void addPattern(const QString& name, const QRegularExpression& pattern);
    
Q_SIGNALS:
    void sensitiveDataDetected(const QString& type);
    
private:
    explicit DASensitiveDataDetector(QObject* parent = nullptr);
    void setupDefaultPatterns();
    
    struct PatternInfo {
        QString name;
        QRegularExpression pattern;
        QString replacement;
    };
    
    QList<PatternInfo> m_patterns;
};
```

**验收**: 编译通过

#### Step 2: 实现默认敏感模式
```cpp
// src/DAUtils/DASensitiveDataDetector.cpp
void DASensitiveDataDetector::setupDefaultPatterns()
{
    // API 密钥模式
    addPattern("OpenAI API Key",
               QRegularExpression("sk-[a-zA-Z0-9]{32,}"),
               "[OPENAI_API_KEY]");
    
    addPattern("Anthropic API Key",
               QRegularExpression("sk-ant-[a-zA-Z0-9-]{32,}"),
               "[ANTHROPIC_API_KEY]");
    
    // Bearer Token
    addPattern("Bearer Token",
               QRegularExpression("Bearer\\s+[a-zA-Z0-9\\-_\\.]+"),
               "Bearer [TOKEN]");
    
    // 密码
    addPattern("Password",
               QRegularExpression(
                   "(?i)(password|passwd|pwd)[\"']?\\s*[:=]\\s*[\"']?[^\"'\\s,;]+"),
               "password=[REDACTED]");
    
    // 邮箱
    addPattern("Email",
               QRegularExpression("[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}"),
               "[EMAIL]");
    
    // 信用卡号
    addPattern("Credit Card",
               QRegularExpression("\\b[0-9]{4}[- ]?[0-9]{4}[- ]?[0-9]{4}[- ]?[0-9]{4}\\b"),
               "[CREDIT_CARD]");
    
    // 身份证号
    addPattern("Chinese ID",
               QRegularExpression("\\b[1-9]\\d{5}(18|19|20)\\d{2}(0[1-9]|1[0-2])(0[1-9]|[12]\\d|3[01])\\d{3}[\\dXx]\\b"),
               "[ID_CARD]");
}

QString DASensitiveDataDetector::redact(const QString& text)
{
    QString result = text;
    
    for (const auto& pattern : m_patterns) {
        result.replace(pattern.pattern, pattern.replacement);
    }
    
    return result;
}
```

**验收**: 敏感信息被正确脱敏

#### Step 3: 集成到日志系统
```cpp
// src/DAMessageHandler/DALogger.cpp
void DALogger::logMessage(QtMsgType type, const QMessageLogContext& context,
                          const QString& msg)
{
    // 脱敏处理
    QString safeMessage = DASensitiveDataDetector::instance()->redact(msg);
    
    // 写入日志
    QString logEntry = formatLogEntry(type, context, safeMessage);
    m_logFile->write(logEntry.toUtf8());
    
    // 输出到控制台
    std::cerr << logEntry.toStdString() << std::endl;
}

// 在 Agent 日志中也应用脱敏
void DAAgentNode::logMessage(const QString& message)
{
    QString safeMessage = DASensitiveDataDetector::instance()->redact(message);
    Q_EMIT logMessageGenerated(safeMessage);
}
```

**验收**: 所有日志输出已脱敏

---

### 任务 5.4: 网络安全

**目标**: 仅允许访问配置的 LLM API 端点，禁止任意网络请求

**涉及文件**:
- `src/DAPyScripts/DANetworkSandbox.h` - 网络沙箱
- `src/DAPyScripts/DANetworkSandbox.cpp`
- `src/DAUtils/DAWhitelistManager.h` - 白名单管理器

**子步骤**:

#### Step 1: 实现网络请求拦截
```python
# src/DAPyScripts/network_sandbox.py
"""
网络请求沙箱 - 限制网络访问
"""

import urllib.request
import urllib.parse
import socket

# 白名单域名
ALLOWED_HOSTS = {
    'api.openai.com',
    'api.anthropic.com',
    'api.cohere.ai',
    'localhost',
    '127.0.0.1'
}

# 白名单 URL 模式
ALLOWED_URL_PATTERNS = [
    'https://api.openai.com/v1/*',
    'https://api.anthropic.com/v1/*',
]

def is_allowed_url(url):
    """检查 URL 是否在白名单中"""
    from fnmatch import fnmatch
    
    parsed = urllib.parse.urlparse(url)
    host = parsed.hostname
    
    # 检查域名
    if host not in ALLOWED_HOSTS:
        return False
    
    # 检查 URL 模式
    for pattern in ALLOWED_URL_PATTERNS:
        if fnmatch(url, pattern):
            return True
    
    return False

# 拦截 urllib 请求
original_urlopen = urllib.request.urlopen

def restricted_urlopen(url, data=None, timeout=None):
    if isinstance(url, str):
        if not is_allowed_url(url):
            raise PermissionError(
                f"Network access to '{url}' is not allowed. "
                f"Allowed hosts: {', '.join(ALLOWED_HOSTS)}"
            )
    return original_urlopen(url, data, timeout)

urllib.request.urlopen = restricted_urlopen

# 拦截 socket 连接
original_socket_connect = socket.socket.connect

def restricted_socket_connect(self, address):
    host = address[0] if isinstance(address, tuple) else address
    if host not in ALLOWED_HOSTS:
        raise PermissionError(
            f"Connection to '{host}' is not allowed"
        )
    return original_socket_connect(self, address)

socket.socket.connect = restricted_socket_connect

print("Network sandbox initialized")
```

**验收**: 尝试访问未授权域名时抛出异常

#### Step 2: 实现 C++ 网络白名单
```cpp
// src/DAUtils/DAWhitelistManager.h
#pragma once
#include <QObject>
#include <QSet>
#include <QRegularExpression>

class DAUtils_EXPORT DAWhitelistManager : public QObject {
    Q_OBJECT
    
public:
    static DAWhitelistManager* instance();
    
    // URL 白名单
    void addAllowedUrlPattern(const QString& pattern);
    bool isUrlAllowed(const QString& url) const;
    
    // 主机白名单
    void addAllowedHost(const QString& host);
    bool isHostAllowed(const QString& host) const;
    
    // 端口白名单
    void addAllowedPort(int port);
    bool isPortAllowed(int port) const;
    
    // 加载配置
    void loadFromConfig(const QString& configPath);
    
Q_SIGNALS:
    void blockedAttempt(const QString& url, const QString& reason);
    
private:
    explicit DAWhitelistManager(QObject* parent = nullptr);
    
    QSet<QString> m_allowedHosts;
    QSet<int> m_allowedPorts;
    QList<QRegularExpression> m_allowedUrlPatterns;
};
```

**验收**: 编译通过

---

### 任务 5.5: 审计日志

**目标**: 记录所有 Agent 执行、工具调用、API 请求，用于安全审计

**涉及文件**:
- `src/DAUtils/DAAuditLogger.h` - 审计日志器
- `src/DAUtils/DAAuditLogger.cpp`

**子步骤**:

#### Step 1: 设计审计日志格式
```cpp
// src/DAUtils/DAAuditLogger.h
#pragma once
#include <QObject>
#include <QFile>
#include <QJsonDocument>

class DAUtils_EXPORT DAAuditLogger : public QObject {
    Q_OBJECT
    
public:
    static DAAuditLogger* instance();
    
    // 初始化
    void initialize(const QString& logDir);
    
    // 记录事件
    void logAgentExecution(const QString& agentId,
                           const QString& status,
                           int durationMs,
                           const QVariantMap& details);
    
    void logToolCall(const QString& toolId,
                     const QString& agentId,
                     const QVariantMap& inputs,
                     const QVariant& output,
                     int durationMs);
    
    void logApiRequest(const QString& provider,
                       const QString& endpoint,
                       int statusCode,
                       int tokensUsed,
                       int durationMs);
    
    void logSecurityEvent(const QString& eventType,
                          const QString& description,
                          const QString& severity);
    
    // 刷新日志
    void flush();
    
    // 获取日志文件路径
    QString getLogFilePath() const { return m_logFilePath; }
    
Q_SIGNALS:
    void logWritten(const QString& entry);
    
private:
    explicit DAAuditLogger(QObject* parent = nullptr);
    void writeLog(const QString& category, const QJsonObject& entry);
    
    QFile* m_logFile;
    QTextStream* m_logStream;
    QString m_logFilePath;
    QMutex m_mutex;
};
```

**验收**: 编译通过

#### Step 2: 实现审计日志
```cpp
// src/DAUtils/DAAuditLogger.cpp
void DAAuditLogger::logAgentExecution(const QString& agentId,
                                       const QString& status,
                                       int durationMs,
                                       const QVariantMap& details)
{
    QJsonObject entry;
    entry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    entry["category"] = "agent_execution";
    entry["agent_id"] = agentId;
    entry["status"] = status;
    entry["duration_ms"] = durationMs;
    
    // 添加详细信息（脱敏后）
    QJsonObject detailsJson;
    for (auto it = details.begin(); it != details.end(); ++it) {
        QString value = it.value().toString();
        // 脱敏处理
        value = DASensitiveDataDetector::instance()->redact(value);
        detailsJson[it.key()] = value;
    }
    entry["details"] = detailsJson;
    
    writeLog("agent", entry);
}

void DAAuditLogger::logToolCall(const QString& toolId,
                                 const QString& agentId,
                                 const QVariantMap& inputs,
                                 const QVariant& output,
                                 int durationMs)
{
    QJsonObject entry;
    entry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    entry["category"] = "tool_call";
    entry["tool_id"] = toolId;
    entry["agent_id"] = agentId;
    entry["duration_ms"] = durationMs;
    
    // 输入输出（脱敏）
    QJsonObject inputsJson;
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        inputsJson[it.key()] = 
            DASensitiveDataDetector::instance()->redact(it.value().toString());
    }
    entry["inputs"] = inputsJson;
    
    entry["output"] = 
        DASensitiveDataDetector::instance()->redact(output.toString());
    
    writeLog("tools", entry);
}

void DAAuditLogger::writeLog(const QString& category, const QJsonObject& entry)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_logStream) {
        qWarning() << "Audit logger not initialized";
        return;
    }
    
    // 写入 JSON 行格式
    QString line = QString::fromUtf8(
        QJsonDocument(entry).toJson(QJsonDocument::Compact)
    );
    
    *m_logStream << line << "\n";
    m_logStream->flush();
    
    Q_EMIT logWritten(line);
}
```

**验收**: 审计日志正确记录

#### Step 3: 集成到 Agent 执行流程
```cpp
// src/DAWorkFlow/DAAgentNode.cpp
QVariant DAAgentNode::executeAgentLoop()
{
    QElapsedTimer timer;
    timer.start();
    
    try {
        // 记录开始
        DAAuditLogger::instance()->logAgentExecution(
            objectName(),
            "started",
            0,
            {{"role", m_config->role()}}
        );
        
        QVariant result = executeAgentLoopInternal();
        
        // 记录完成
        DAAuditLogger::instance()->logAgentExecution(
            objectName(),
            "completed",
            timer.elapsed(),
            {{"result_size", result.toString().size()}}
        );
        
        return result;
        
    } catch (const std::exception& e) {
        // 记录错误
        DAAuditLogger::instance()->logAgentExecution(
            objectName(),
            "failed",
            timer.elapsed(),
            {{"error", e.what()}}
        );
        
        DAAuditLogger::instance()->logSecurityEvent(
            "agent_failure",
            QString("Agent %1 failed: %2").arg(objectName(), e.what()),
            "medium"
        );
        
        throw;
    }
}
```

**验收**: Agent 执行被完整记录

---

## 验收标准

### Phase 5 完成条件（全部必须满足）:
- [ ] Windows Credential Manager 集成完成
- [ ] macOS Keychain 集成完成
- [ ] Linux Secret Service 集成完成
- [ ] Python 沙箱阻止所有危险操作
- [ ] 执行超时正常工作
- [ ] 最大迭代次数限制有效
- [ ] 日志脱敏覆盖所有敏感模式
- [ ] 网络访问限制生效
- [ ] 审计日志记录所有关键事件

### 交付物:
1. `src/DAUtils/DASecretManager_*.cpp` - 跨平台密钥管理
2. `src/DAPyScripts/sandbox_restricted.py` - Python 沙箱
3. `src/DAPyScripts/network_sandbox.py` - 网络沙箱
4. `src/DAUtils/DASensitiveDataDetector.h/cpp` - 敏感数据检测
5. `src/DAUtils/DAWhitelistManager.h/cpp` - 网络白名单
6. `src/DAUtils/DAAuditLogger.h/cpp` - 审计日志器

---

## 技术风险

| 风险 ID | 风险描述 | 缓解方案 | 验证方法 |
|---------|----------|----------|----------|
| R1 | 密钥管理器 API 变更 | 使用稳定 API，封装隔离层 | 版本测试 |
| R2 | 沙箱被绕过 | 多层防护，定期审计 | 渗透测试 |
| R3 | 脱敏模式遗漏 | 持续更新模式库 | 正则测试 |
| R4 | 审计日志性能影响 | 异步写入，批量处理 | 性能分析 |
| R5 | 网络白名单配置错误 | 默认拒绝，明确允许 | 配置审查 |

---

## 依赖关系

```
Phase 1 (Python 环境) ──> 任务 5.2 (沙箱)
Phase 2 (密钥管理) ────> 任务 5.1 (密钥存储)
任务 5.1 ───────────────> 任务 5.3 (脱敏)
任务 5.2 ───────────────> 任务 5.4 (网络安全)
任务 5.3 + 5.4 ─────────> 任务 5.5 (审计日志)
```

**前置依赖**: Phase 1 必须完成  
**后置依赖**: Phase 4 依赖 Phase 5 完成

---

## 时间规划

| 周次 | 任务 | 预计工时 |
|------|------|----------|
| Week 1 | 任务 5.1 + 任务 5.2 + 任务 5.3 | 40 小时 |
| Week 2 | 任务 5.4 + 任务 5.5 + 安全测试 | 40 小时 |

**里程碑**: Week 2 结束前完成所有 Phase 5 任务，安全机制可投入使用
