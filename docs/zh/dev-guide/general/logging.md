# 日志系统

## 概述

DAWorkbench 使用基于 **spdlog** 的异步日志系统，通过 `qInstallMessageHandler` 拦截 Qt 的 `qDebug/qInfo/qWarning/qCritical` 消息，按 category 分流到不同的 spdlog logger，实现：

- **业务日志**（`da.*` category）→ 写入文件 + 控制台 + **UI 日志窗口**
- **系统日志**（Qt 自身、第三方库等无 category 的消息）→ 仅写入文件 + 控制台，**不污染 UI 窗口**

### 架构

```text
qDebug/qInfo/qWarning/qCritical
  │
  ▼
DALogger::daMessageHandler (qInstallMessageHandler 回调)
  │
  ├─ context.category 以 "da." 开头？
  │    ├─ 是 → mLogger (业务 logger，带 UI sink)
  │    │       ├─ stdout_color_sink  (控制台)
  │    │       ├─ rotating_file_sink (日志文件)
  │    │       └─ DAMessageLogSink   (UI 队列 → UI 窗口)
  │    │
  │    └─ 否 → mSystemLogger (系统 logger，无 UI sink)
  │              ├─ stdout_color_sink  (控制台)
  │              └─ rotating_file_sink (日志文件)
  │
  ▼
DAMessageLogQueue (线程安全环形缓冲，QTimer 惰性发射信号)
  │
  ▼
DAMessageLogsModel (QAbstractTableModel)
  │
  ▼
DAMessageLogViewWidget (UI 日志窗口)
```

### 核心模块

| 文件 | 职责 |
|------|------|
| `src/DAMessageHandler/DALogger.h/.cpp` | RAII 单例，管理 spdlog logger 生命周期、安装 qInstallMessageHandler、按 category 分流 |
| `src/DAMessageHandler/DALogCategory.h/.cpp` | 声明 `DA_USER` category + `daInfo`/`daWarning`/`daCritical`/`daDebug` 便捷宏 |
| `src/DAMessageHandler/DAMessageLogQueue.h/.cpp` | 线程安全日志队列（spdlog 后台线程 → UI 主线程的桥梁），QTimer 惰性信号发射 |
| `src/DAMessageHandler/DAMessageLogSink.h/.cpp` | spdlog 自定义 sink，从 `log_msg` 重建 `DAMessageLogItem` 推入队列 |
| `src/DAMessageHandler/DAMessageLogItem.h/.cpp` | 单条日志数据结构（类型/时间/文件/函数/行号/消息） |
| `src/DAGui/DAMessageLogViewWidget.h/.cpp` | UI 日志窗口 Widget |
| `src/DAGui/Models/DAMessageLogsModel.h/.cpp` | 日志 TableModel |

---

## 快速使用

### 1. CMake 链接

模块 CMakeLists.txt 中添加依赖：

```cmake
target_link_libraries(${DA_LIB_NAME} PRIVATE
    ${DA_PROJECT_NAME}::DAMessageHandler
    # ... 其他依赖
)
```

### 2. 包含头文件

```cpp
#include "DALogCategory.h"
```

### 3. 打印日志

```cpp
daDebug    << "debug message:" << value;        // 写文件，不进 UI（sink level=info）
daInfo     << "kernel initialized";              // 写文件 + 控制台 + UI 窗口
daWarning  << "config file not found:" << path;  // 写文件 + 控制台 + UI 窗口
daCritical << "failed to open project:" << err;  // 写文件 + 控制台 + UI 窗口
```

这些宏展开为 `qCInfo(DA_USER)` / `qCWarning(DA_USER)` / `qCCritical(DA_USER)` / `qCDebug(DA_USER)`，category 为 `"da.user"`，会被 `DALogger::dispatchMessage` 识别为业务日志，自动路由到带 UI sink 的 logger。

### 4. 多语言支持

日志内容通过 `tr()` 包裹以支持国际化，**每个 `tr()` 后必须有 `// cn:` 中文注释**：

```cpp
daInfo     << tr("Project saved successfully, path: %1").arg(path);  // cn:工程保存成功，路径：%1
daWarning  << tr("config file (%1) is missing the <configs> tag").arg(path);  // cn:配置文件(%1)缺失<configs>标签
daCritical << tr("Failed to save project! Path: %1").arg(path);  // cn:工程保存失败！路径：%1
```

---

## 宏 API 参考

### 便捷宏（推荐）

定义在 `src/DAMessageHandler/DALogCategory.h`：

| 宏 | 展开为 | category | 进 UI 窗口 | 级别 |
|----|--------|----------|-----------|------|
| `daDebug` | `qCDebug(DA_USER)` | da.user | ❌（sink level=info） | Debug |
| `daInfo` | `qCInfo(DA_USER)` | da.user | ✅ | Info |
| `daWarning` | `qCWarning(DA_USER)` | da.user | ✅ | Warning |
| `daCritical` | `qCCritical(DA_USER)` | da.user | ✅ | Critical |

用法与 `qDebug()` / `qInfo()` 完全一致，支持 `<<` 链式输出和 `.noquote()` 修饰：

```cpp
daInfo.noquote() << "raw string without quotes";  // 不加引号
daWarning << "value =" << x << "count =" << n;
```

### 何时用哪个级别

| 级别 | 使用场景 | 示例 |
|------|---------|------|
| `daDebug` | 开发调试信息，发布版默认不输出 | `daDebug << "argv[" << i << "]" << argv[i];` |
| `daInfo` | 正常运行的关键节点信息 | `daInfo << tr("Project saved successfully");` |
| `daWarning` | 非致命异常，用户应知晓 | `daWarning << tr("config file not found, using defaults");` |
| `daCritical` | 严重错误，影响功能 | `daCritical << tr("Failed to load project file: %1").arg(path);` |

---

## DALogger API

### 初始化

在 `main.cpp` 中，`QApplication` 构造前调用：

```cpp
// main.cpp
#include "DALogger.h"

int main(int argc, char* argv[]) {
    // 安装日志 handler + 配置 rotating file sink
    DA::DALogger::instance().setupRotatingFile(DA::DADir::getLogFilePath());
    // ... QApplication app(argc, argv);
}
```

### 三种初始化方式

```cpp
// 1. 旋转文件日志（推荐，生产环境）
DA::DALogger::instance().setupRotatingFile(
    logFilePath,           // 日志文件路径
    10 * 1048576,          // maxSize = 10MB
    5,                     // maxFiles = 5 个历史文件
    true,                  // outputStdout = 同时输出到控制台
    DA::DAOverflowPolicy::OverrunOldest  // 队列满时丢弃最旧消息
);

// 2. 按日期分割日志
DA::DALogger::instance().setupDailyFile(logFilePath, 5, true);

// 3. 仅控制台（调试用）
DA::DALogger::instance().setupConsole();
```

### 运行时配置

```cpp
// 设置全局日志级别（影响所有 sink）
DA::DALogger::instance().setLevel(DA::DALogLevel::Info);

// 设置 UI 队列的日志级别（仅影响业务 logger 的 UI sink）
// 例如设为 Warn，则只有 Warning 及以上消息进入 UI 窗口
DA::DALogger::instance().setQueueLevel(DA::DALogLevel::Warn);

// 启用/禁用 UI 队列捕获
DA::DALogger::instance().setQueueCaptureEnabled(false);  // 临时关闭 UI 日志
DA::DALogger::instance().setQueueCaptureEnabled(true);   // 恢复

// 设置 spdlog pattern
DA::DALogger::instance().setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");

// 设置 flush 间隔
DA::DALogger::instance().setFlushInterval(15);  // 15 秒
```

### DALogLevel 枚举

```cpp
enum class DALogLevel {
    Trace,     // 详细跟踪
    Debug,     // 调试信息
    Info,      // 一般信息
    Warn,      // 警告
    Error,     // 错误
    Critical,  // 严重错误
    Off        // 关闭日志
};
```

---

## Category 分流机制

### 为什么需要分流

`qInstallMessageHandler` 是全局拦截器，无法区分日志来源。如果不分流，Qt 自身和第三方库（SARibbon、qwt、ADS 等）的 `qWarning` 会大量涌入 UI 窗口，淹没业务日志。

### 工作原理

`DALogger::dispatchMessage` 通过 `QMessageLogContext::category` 判断日志来源：

```cpp
void DALogger::dispatchMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    bool isBusinessLog = context.category && QByteArray(context.category).startsWith("da.");
    auto& logger = isBusinessLog ? d->mLogger : d->mSystemLogger;
    // ...
}
```

- `da.*` 开头的 category → 业务 logger（带 UI sink）→ 进入 UI 日志窗口
- 其他 category（含默认的 `default`）→ 系统 logger（无 UI sink）→ 仅文件和控制台

### 扩展自定义 Category

如果某模块需要独立的 category 过滤（例如在 `QLoggingCategory::setFilterRules` 中单独控制），可以声明自己的 category：

```cpp
// 在模块的 .h 文件中
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(DA_MY_MODULE)

// 在模块的 .cpp 文件中
Q_LOGGING_CATEGORY(DA_MY_MODULE, "da.mymodule")

// 使用
qCInfo(DA_MY_MODULE) << "custom category log";
```

只要 category 以 `da.` 开头，就会自动进入 UI 窗口。但对于大多数场景，直接使用 `daInfo`/`daWarning`/`daCritical` 便捷宏（category 为 `da.user`）即可。

---

## UI 日志窗口

### 界面

UI 日志窗口（`DAMessageLogViewWidget`）位于主窗口的 Dock 停靠面板，显示业务日志，支持：

- 按级别过滤（Info/Warning/Critical 复选框）
- 自动滚动到底部
- 右键菜单：复制、清空、全选
- 快捷键：Ctrl+C 复制、Ctrl+A 全选

### 线程安全

日志可能来自任意线程（spdlog 后台线程、Python 工作线程等）。数据流：

1. `daInfo << "..."` → `qInstallMessageHandler` 回调（调用线程）
2. → spdlog async_logger → spdlog 后台线程池
3. → `DAMessageLogSink::log()` → `DAMessageLogQueue::push()`（线程安全，QMutex 保护）
4. → QTimer 1 秒间隔惰性发射 `messageQueueAppended` 信号（主线程）
5. → `DAMessageLogsModel::onMessageAppended()` 更新模型（主线程）
6. → UI 刷新

### QTimer 惰性发射

`DAMessageLogQueue` 使用 QTimer 以 1 秒间隔合并信号，避免高频日志导致 UI 卡顿。如果 1 秒内有 1000 条日志，只触发一次 `messageQueueAppended` 信号。

---

## 日志读取与故障排查

当用户报告运行异常、崩溃、行为不符合预期，或提到"日志"/"报错"/"看不到效果"时，应主动读取程序运行日志分析根因，不要凭猜测下结论。

### 日志文件位置

程序运行日志固定写入用户 AppData 目录（不随构建目录变化），路径解析逻辑见 `src/DAUtils/DADir.cpp`（`getAppDataPath` = `QStandardPaths::AppDataLocation + "/" + "DAWorkBench"`）：

| 文件 | Windows 路径 | 内容 |
|------|--------------|------|
| `da_log.log` | `%APPDATA%\DAWorkBench\log\da_log.log` | C++ 主程序当前日志（最新一次运行） |
| `da_log.1.log` ~ `da_log.5.log` | 同上目录 | 轮转历史日志（rotating 模式，默认单文件 10MB，保留 5 个） |
| `da_pyscript.log` | 同上目录 | 嵌入式 Python 脚本输出（工作流节点执行、agent_runner.py 等） |
| `dawork-config.xml` | `%APPDATA%\DAWorkBench\config\` | DAAppConfig XML 配置（非日志，但常用于诊断持久化问题） |
| `agent-config.ini` | `%APPDATA%\DAWorkBench\config\` | Agent LLM 配置（base_url/model/api_key/超时），QSettings IniFormat，api_key 为 DPAPI 加密的 QByteArray（@ByteArray 注解） |
| `recent-files.ini` | `%APPDATA%\DAWorkBench\config\` | 最近打开文件列表，QSettings IniFormat |

> Linux/macOS 上 `AppDataLocation` 解析为 `~/.local/share/DAWorkBench/` 或 `~/Library/Application Support/DAWorkBench/`。跨平台读取时优先用环境变量（`%APPDATA%` / `$XDG_DATA_HOME` / `~/Library/Application Support`）拼接 `DAWorkBench/log/`。

### 日志格式与级别

每行一条记录，格式：

```
[2026-08-04 16:58:10.977] [debug] [DAAppConfig.cpp:212] apply setting
 └────时间戳────┘ └─级别─┘ └──源文件:行号──┘ └─消息─┘
```

级别从低到高：`trace` / `debug` / `info` / `warning` / `error` / `critical` / `off`。默认配置下 `debug` 级别会写入文件（日志里能看到 `[debug]` 条目）。级别由 `DAAppConfig` 的 `LOG_LEVEL` / `LOG_QUEUE_LEVEL` 配置项控制（见 `main.cpp` 的 `setupLogger`）。

### 读取日志的推荐方式

1. **快速定位近期事件**：读 `da_log.log` 的**尾部**（offset 靠近文件总行数）。文件可能接近 10MB 上限，从头读会截断且浪费上下文。先跑 `dir /t:w "%APPDATA%\DAWorkBench\log\da_log.log"`（Windows）看修改时间确认用户刚运行过。
2. **按关键词检索**：在 `da_log.log` 里搜关键词定位：
   - 报错类：`Error|Exception|Traceback|failed|fatal|critical`
   - 崩溃类：`crash|dump|SIGSEGV|abnormal|terminate`
   - 模块类：`DAAgent|DAPyWorkFlow|DAAppConfig|DAAgentBridge|DAPluginManager`
   - 自定义诊断日志前缀（开发者临时加的 `[模块名]` 标记，如 `[DAAgentSettings]`）
3. **跨多次运行对照**：当前 `da_log.log` 是最新一次运行，`da_log.1.log` 是上一次。用户描述"之前能用现在不能"时，对比两个文件。
4. **Python 侧问题**：工作流节点执行异常、agent 子进程报错，先看 `da_pyscript.log`；C++ 主程序行为看 `da_log.log`。
5. **Agent 子进程 stderr**：`DAAgentBridge` 把子进程 stderr 转发到 `da_log.log`，标记为 `[DAAgentBridge.cpp:268] Agent stderr:`，Python traceback 在这里可见（注意 `\r\n` 被字面转义成字符串内容，不是真换行）。

### 常见调试场景对照表

| 现象 | 先看什么 | 搜什么关键词 |
|------|---------|-------------|
| 程序启动失败/闪退 | `da_log.log` 尾部 | `critical` / `error` / `dump` / `terminate` |
| 插件加载失败 | `da_log.log` | `loaded plugin` / `DAPluginManager` / `plugin directory` |
| 工作流节点执行报错 | `da_pyscript.log` + `da_log.log` | `DAPyWorkFlow` / `NodeDef` / `Traceback` |
| Agent 对话无响应/报错 | `da_log.log` → `da_pyscript.log` | `DAAgentBridge` / `Agent stderr` / `agent_runner` |
| 设置不持久化 | `da_log.log` + INI/XML | 相关模块的 `apply` / `saveConfig` 诊断日志；Agent 配置查 `agent-config.ini`；最近文件查 `recent-files.ini`；DAAppConfig 查 `dawork-config.xml`；旧版注册表残留查 `reg query "HKCU\Software\DA\DAWorkBench"` |
| 崩溃转储 | `dumps/dump*.dmp` + `.sysinfo` | dump 文件需用 WinDbg/VS 解析，`.sysinfo` 是文本可直读 |

### 注意事项

- 日志文件可能含敏感信息（API key 除外——业务代码应避免打印密钥明文，但第三方库或 Qt 自身可能意外泄露）。分析时不要把日志原文完整贴到对外渠道，只摘录与问题相关的行；遇到疑似密钥/token 的字符串要打码。
- `da_log.log` 是 rotating 的，如果用户报告"几天前"的问题，可能已被轮转覆盖，只能从 `da_log.N.log` 找。
- 程序退出时日志会 flush，但非正常退出（崩溃/强杀）最后几行可能未写入文件——这种场景看 `dumps/` 下的 dump 文件。
- 读日志前先确认修改时间（`dir /t:w`），避免读到陈旧日志误导判断。如果用户刚遇到问题但日志时间戳很旧，说明问题没触发到日志系统（可能是 UI 层死锁或日志系统本身没初始化）。

---

## 最佳实践

### ✅ 应该做的

1. **业务日志用便捷宏**：`daInfo`/`daWarning`/`daCritical`，日志自动进 UI 窗口
2. **tr() + 中文注释**：每个面向用户的日志用 `tr()` 包裹，后跟 `// cn:中文`
3. **带上下文信息**：日志应包含足够的诊断信息

```cpp
// ✅ 好
daWarning << tr("Failed to load plugin %1: %2").arg(name, error);  // cn:加载插件%1失败：%2

// ❌ 不好（缺少上下文）
daWarning << "error";
```

4. **异常信息记录**：catch 块中记录异常详情

```cpp
} catch (const std::exception& e) {
    daCritical << tr("Failed to process data: %1").arg(e.what());  // cn:处理数据失败：%1
}
```

### ❌ 不应该做的

1. **不要用 `qInfo()`/`qWarning()`/`qCritical()` 打印业务日志**

```cpp
// ❌ 不会进入 UI 窗口（无 category，被判定为系统日志）
qWarning() << tr("something went wrong");
```

2. **不要在 DALogger 内部使用 `da.*` category**：会导致日志循环
3. **不要在热路径大量打印 daDebug**：即使不进 UI，仍会写文件

### 第三方库日志

第三方库（SARibbon、qwt、ADS 等）的 `qWarning`/`qCritical` 会自动写入日志文件，但不会出现在 UI 窗口。如需查看，打开日志文件即可。

---

## 迁移指南

### 从旧版 `DA_LOG_*` 宏迁移

旧版宏 `DA_LOG_INFO`/`DA_LOG_ERROR` 等已废弃，替换规则：

| 旧宏 | 新宏 | 说明 |
|------|------|------|
| `DA_LOG_DEBUG(fmt, ...)` | `daDebug << ...` | 改为流式输出 |
| `DA_LOG_INFO(fmt, ...)` | `daInfo << ...` | 改为流式输出 |
| `DA_LOG_ERROR(fmt, ...)` | `daCritical << ...` | ERROR → Critical |
| `DA_LOG_SET_LEVEL(level)` | `DA::DALogger::instance().setLevel(...)` | 改用 DALogger API |

### 从 `qInfo()/qWarning()/qCritical() + tr()` 迁移

```cpp
// 旧
qInfo() << tr("Project saved successfully, path: %1").arg(path);

// 新
daInfo << tr("Project saved successfully, path: %1").arg(path);  // cn:工程保存成功，路径：%1
```

---

## 相关文件

| 文件 | 说明 |
|------|------|
| `src/DAMessageHandler/DALogger.h` | DALogger 单例类声明 |
| `src/DAMessageHandler/DALogCategory.h` | DA_USER category + 便捷宏 |
| `src/DAMessageHandler/DAMessageLogQueue.h` | 线程安全日志队列 |
| `src/DAMessageHandler/DAMessageLogSink.h` | spdlog 自定义 sink |
| `src/DAMessageHandler/DAMessageLogItem.h` | 日志数据结构 |
| `src/DAGui/DAMessageLogViewWidget.h` | UI 日志窗口 |
| `src/DAGui/Models/DAMessageLogsModel.h` | 日志 TableModel |
| `src/APP/main.cpp` | DALogger 初始化示例 |
