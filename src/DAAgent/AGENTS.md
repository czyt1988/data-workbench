# DAAgent 模块开发指南

DAWorkbench 的 AI Agent 助手模块：内嵌 LLM 聊天 + 数据分析工具调用。**工具执行在 C++ 主进程**，**LLM 推理在 Python 子进程**（langchain-openai + langgraph），两者通过 stdin/stdout 管道以 **JSON Lines 协议**通信。聊天界面（DAGui/Agent）基于 QWebEngine + QWebChannel 渲染。

> ⚠️ 本文件是 AI 开发 Agent 相关功能（新增工具、修改聊天 UI、调协议、改 Python 脚本）的必读指南。核心设计约束见 § 十一（铁律），改动前先对照。

---

## 一、模块定位与构建

| 项 | 值 |
|----|----|
| 模块名 | `DAWorkbench::DAAgent`（SHARED 库） |
| 源码目录 | `src/DAAgent/`（含 `tools/` 子目录） |
| 依赖（PUBLIC） | DAInterface, DAData, DAFigure, DAPyBindQt, DAPyScripts, DAGui + Qt Core/Gui/Widgets |
| 依赖（PRIVATE） | Qt PrintSupport/Svg（PDF/SVG 导出）、DAAxOfficeWrapper（Win, docx 导出） |
| 编译条件 | **仅 `DA_ENABLE_PYTHON=ON` 时编译**（`src/CMakeLists.txt:62`） |
| 导出宏 | `DAAgent_API`（`DAAgentAPI.h`，`DAAGENT_BUILD` 定义于 `CMakeLists.txt:48`） |

**层级**：DAAgent 位于接口层（Layer 4，与 DAInterface/DAPluginSupport 同级）。它向上提供 `DAAgentInterface` 供插件（Layer 5 及插件）注册工具/提示词、控制 UI 显隐与 LLM 配置；向下依赖 DAGui（Layer 3）等。创建新类时**禁止**让 DAAgent 依赖 APP。

**构建**：`.\scripts\build.ps1 -Target DAAgent`。`tools/*.cpp` 用 `file(GLOB ... CONFIGURE_DEPENDS)` 收集，新增工具文件无需改 CMake。

---

## 二、架构总览（设计方案）

```
┌───────────────────────────────────────────────────────────────┐
│  C++ 主进程 (Host)                                             │
│  src/DAAgent/                                                  │
│   DAAgentInterface   — 公共接口（插件注册工具/提示词）           │
│   DAAgentModule      — 接口实现：工具注册、系统提示词组装、      │
│                        懒启动、LLM 配置读写                     │
│   DAAgentBridge      — QProcess 子进程管理 + JSON Lines 协议解析 │
│   tools/             — 15 个平台内置工具（DAAgentToolBase 基类） │
│  src/DAGui/Agent/（UI 在 DAGui 模块）                           │
│   DAAgentDockWidget  — 聊天面板（QWebEngine + 输入框）           │
│   DAAgentWebChannel  — C++↔JS 桥（registerObject "chatBridge"） │
│   DAAgentSettingsWidget — LLM 设置页（QSettings + DPAPI 加密）   │
└──────────────────────┬────────────────────────────────────────┘
                       │  QProcess 匿名管道
                       │  stdin/stdout: JSON Lines（每行一条 JSON）
┌──────────────────────▼────────────────────────────────────────┐
│  Python 子进程                                                 │
│  src/PyScripts/DAWorkbench/agent/agent_runner.py               │
│   StdioProtocol — 后台线程读 stdin → asyncio.Queue              │
│   AgentRunner   — langgraph StateGraph（agent/tools/ask_user）  │
│   ToolFactory   — C++ tool schema → bind_tools（含注入 ask_user）│
└───────────────────────────────────────────────────────────────┘
```

### 核心设计决策

| # | 决策 | 说明 |
|---|------|------|
| D1 | **子进程模型** | agent 推理在独立 Python 进程（QProcess），C++ 主进程崩溃/无响应不影响子进程；协议为 stdin/stdout JSON Lines |
| D2 | **工具执行在 C++** | LLM 只拿到工具 schema（OpenAI function schema 字典，无 Pydantic 转换）；真实执行由 `DAAgentBridge::executeTool()` 在主进程完成，结果经 stdin 回传 |
| D3 | **懒启动** | 首次 `sendMessage()` 才启动子进程（`startAgentInternal()`），启动时一次性下发 init（LLM 配置 + 工具规格 + 系统提示词） |
| D4 | **HITL 提问** | langgraph `interrupt()/resume()` + 注入的 `ask_user` 工具实现人机交互提问；resume 后继续同一 thread 的图执行 |
| D5 | **流式输出** | `llm.astream()` 逐 chunk 把 token 发给 C++ → 经 WebChannel 推给 chat.js 实时渲染（防抖 50ms） |
| D6 | **Windows asyncio 兼容** | 强制 `WindowsSelectorEventLoopPolicy`；stdin 用后台线程 `read1()` 阻塞读取（asyncio pipe transport 在 Windows/QProcess 下不可用） |

---

## 三、目录结构与关键文件

### 3.1 `src/DAAgent/`（核心库）

| 文件 | 职责 |
|------|------|
| `DAAgentInterface.h` | 公共接口：`registerTool` / `registerSystemPrompt` / `showDockWidget` / `hideDockWidget` / `sendMessage` / `isRunning` / `getLLMConfig` / `setLLMConfig` |
| `DAAgentModule.h/.cpp` | 接口实现：工具注册表 `m_tools`、系统提示词 `m_systemPrompts`、懒启动、`connectSignals()` 信号链、Python/脚本路径探测 |
| `DAAgentBridge.h/.cpp` | QProcess 生命周期（start/stop/超时）、stdin/stdout 读写、JSON Lines 解析分发、工具执行兜底 |
| `DAAbstractAgentTool.h` | 工具抽象基类（纯虚）：`getToolSpec` / `execute` / `getOwnerModule` |
| `DAAgentAPI.h` | `DAAgent_API` 导出宏 |
| `tools/` | 平台内置工具实现（见 § 七） |

### 3.2 `src/DAGui/Agent/`（聊天 UI，属 DAGui 模块）

| 文件 | 职责 |
|------|------|
| `DAAgentDockWidget.h/.cpp` | 聊天面板 QWidget（**非 QDockWidget**，见 § 八）；持有 WebEngine 视图、输入框、发送按钮、状态标签 |
| `DAAgentWebChannel.h/.cpp` | QWebChannel 桥对象（注册名 `chatBridge`）；JS 调 `onUserSelect`/`onUserMessage`，C++ 调 `callJS()` 驱动 JS 渲染函数 |
| `DAAgentSettingsWidget.h/.cpp` | LLM 设置页（继承 `DAAbstractSettingPage`）：base_url/api_key/model/超时 + 连接测试；DPAPI 加解密静态方法 |
| `resources/` | `chat.html` + `chat.js` + `chat.css` + `markdown-it.min.js` + `highlight.min.js` + `chat.qrc` |

### 3.3 `src/PyScripts/DAWorkbench/agent/`（Python 子进程）

| 文件 | 职责 |
|------|------|
| `agent_runner.py` | 唯一入口脚本：协议收发、LLM 配置、langgraph 图构建、agent 循环 |

### 3.4 `src/APP/`（集成点）

| 文件 | 职责 |
|------|------|
| `DAAppCore.cpp` | `new DAAgentModule(this, this)` + `initialize()`，多态持有 `DAAgentInterface*` |
| `DAAppDockingArea.cpp:228-236` | `new DAAgentDockWidget` + `createDockWidgetAsTab`（左侧管理区标签页） |
| `DAAppController.cpp:242-248` | 设置 dock 的 toggle action（`actionShowAgentArea`）+ **`setDockWidget()` 注入**（触发信号链连接） |
| `DAAppActions.cpp:232` / `DAAppRibbonArea.cpp:367` | Ribbon 大按钮「Agent 助手」 |

---

## 四、Python 脚本位置（重点）

| 项 | 路径 |
|----|------|
| **源码** | `src/PyScripts/DAWorkbench/agent/agent_runner.py`（开发时编辑此处） |
| **运行时** | `<exe目录>/PyScripts/DAWorkbench/agent/agent_runner.py`（`applicationDirPath() + "/PyScripts"`，见 `DACoreInterface::getPythonScriptsPath()` `DACoreInterface.cpp:107`） |
| **部署机制** | 根 `CMakeLists.txt:254-261` `file(COPY src/PyScripts → bin/PyScripts)` |

路径探测：`DAAgentModule::detectAgentScriptPath()` = `getPythonScriptsPath() + "/DAWorkbench/agent/agent_runner.py"`；Python 解释器用 `DAPyInterpreter::getPythonInterpreterPath()`（`DAPyInterpreter.cpp:327`），兜底 `QStandardPaths::findExecutable("python")`。

> ⚠️ **修改 Python 文件后必须同步到运行时目录**（重跑 cmake 配置或手动 copy），且**必须重启程序**——Python 在启动时导入，不支持热重载。cmake 的 `file(COPY)` 只在 configure 阶段执行，不会跟踪文件变更。

**Python 依赖**（`requirements.txt`）：`langgraph`、`langchain-openai`、`pydantic>=2.0`（langgraph 的 `interrupt()` 需要）。

---

## 五、JSON Lines 协议

stdout 专用于协议，**绝对禁止在 stdout 打印日志**（污染协议流会导致 C++ 端解析失败）。所有调试输出写 stderr。

### 5.1 C++ → Python（stdin）

| type | 载荷 | 说明 |
|------|------|------|
| `init` | `config`{base_url, api_key, model} + `tools`(schema 数组) + `system_prompt` | 启动时一次性下发；config 缺 base_url/api_key/model 任一则报错退出 |
| `user_msg` | `content` | 用户消息，触发一轮 agent 推理 |
| `tool_result` | `call_id` + `result` | 工具执行结果回传（RPC 应答） |
| `user_answer` | `answer` | 用户对 HITL 问题的回答，触发 `resume()` |
| `stop` | — | 优雅停止，子进程退出主循环 |

### 5.2 Python → C++（stdout）

| type | 载荷 | 说明 |
|------|------|------|
| `booting` | — | **必须在导入 langchain 之前发送**；C++ 收到后重置 ready 超时计时器（冷启动 ~16s） |
| `ready` | `model` | 初始化完成，C++ 停止 ready 计时器 |
| `token` | `content` | 流式 token |
| `message_end` | `content` | 本轮最终回复（agent_node 在无 tool_calls 时发送） |
| `tool_call` | `call_id` + `tool` + `arguments` | 请求 C++ 执行工具；C++ 回传 `tool_result` |
| `question` | `text` + `options` | HITL 提问（**只发一次**，见铁律 T8） |
| `error` | `message` | 子进程侧错误 |
| `done` | — | 本轮处理结束（暂停于 interrupt 时不发） |

### 5.3 协议细节

- Windows 下 Python 文本模式输出 `\r\n`，C++ 端 `onReadyReadStandardOutput` 解析行前必须 `chop(1)` 裁掉 `'\r'`（`DAAgentBridge.cpp:183-188`）。
- `tool_call` 的执行通过 `QTimer::singleShot(0)` 投递回主线程事件循环，避免在 stdout 读取回调里长时间阻塞管道（管道阻塞会死锁子进程）。
- Python 端 `_wait_for_result` **严格匹配 `call_id`**，迟到的/错配的 `tool_result` 记日志后忽略。

---

## 六、信号链（C++ 内部）

```
DAAgentBridge (9 个信号)
  ├─ agentToken / agentMessageComplete / agentToolCall / agentToolResult
  ├─ agentQuestion / agentError / agentReady / agentBusy / agentDone
  └──► DAAgentModule::connectSignals()（DAAgentModule.cpp:180）
        └─► DAAgentDockWidget::onAgent* 槽
              └─► DAAgentWebChannel::append* → callJS() → chat.js 渲染函数
```

**用户消息反向路径**：
`onSendClicked()` → `m_channel->appendUserMessage(text)`（先渲染，勿重复调用）+ `emit sendMessageRequested` → `DAAgentModule` lambda → `sendMessage` → `m_bridge->sendMessage` → stdin。

**用户回答路径**：
chat.js 选项按钮 → `chatBridge.onUserSelect(answer)` → `DAAgentWebChannel::userAnswerSelected` → `DAAgentDockWidget::onUserAnswer` → `userAnswerSelected` → `DAAgentModule::connectSignals` → `m_bridge->sendUserAnswer` → stdin `user_answer`。

> ⚠️ 信号链的**两次连接**：`initialize()` 时 Dock 尚未创建（`connectSignals` 对空 dock 提前返回）；`DAAppController` 调 `setDockWidget()` 注入后再次 `connectSignals()` 完成连接。不要试图在别处 `new DAAgentDockWidget`（见铁律 T3）。

---

## 七、工具系统

### 7.1 抽象与基类

- `DAAbstractAgentTool`（纯虚）：`getToolSpec()` 返回 OpenAI function schema，`execute(params)` 返回结果 JSON，`getOwnerModule()` 返回归属模块。
- `DAAgentToolBase`（`tools/DAAgentToolBase.h`）：平台内置工具基类，提供数据管理器/图表窗口的便捷访问：
  - `dataMgr()` / `findData(name)` / `allDatas()`
  - `chartOperateWidget()` → `currentFigure()` / `currentChart()` / `findChart(chartId)`（链路：`DACoreInterface::getUiInterface` → `getDockingArea` → `getChartOperateWidget`）
  - `errorResponse(msg)` / `successResponse(data|message)`

### 7.2 平台内置工具（16 个，`DAAgentModule::registerBuiltinTools`）

| 类别 | 工具（name） | 文件 |
|------|-------------|------|
| 数据 (5) | `list_data` / `get_data_info` / `query_data` / `get_column_stats` / `export_data` | `DAAgentToolListData` / `DAAgentToolDataInfo` / `DAAgentToolQueryData` / `DAAgentToolColumnStats` / `DAAgentToolExportData` |
| 绘图 (8) | `create_chart` / `add_curve` / `set_chart_style` / `add_annotation` / `add_region` / `create_subplots` / `save_chart_image` / `list_figures` | `DAAgentToolCreateChart` / `DAAgentToolAddCurve` / `DAAgentToolSetChartStyle` / `DAAgentToolAddAnnotation` / `DAAgentToolAddRegion` / `DAAgentToolCreateSubplots` / `DAAgentToolSaveChartImage` / `DAAgentToolListFigures` |
| 文件/报告 (3) | `read_file` / `write_file` / `save_report` | `DAAgentToolReadFile` / `DAAgentToolWriteFile` / `DAAgentToolSaveReport` |

#### 绘图工具关键设计

- **`create_chart` / `create_subplots` 每次调用创建新 figure**：通过 `DAAgentToolBase::createFigure(name)` 创建新 figure（标签页），再在其内部创建 chart。不会复用已有 figure/chart，避免多张图叠加到同一绘图。
- **`figure_name` 参数**：所有绘图工具（`add_curve` / `set_chart_style` / `add_annotation` / `add_region` / `save_chart_image`）均支持可选 `figure_name` 参数，通过 `findChart(chartId, figureName)` 在指定 figure 中定位 chart。`create_chart` / `create_subplots` 的 `figure_name` 用于命名新 figure（标签页标题）。
- **`list_figures` 工具**：列出所有 figure 及其内部 chart 的名称/索引/标题，供 agent 检索已有绘图后通过 `figure_name` + `chart_id` 精确定位修改。
- **坐标轴自动缩放**：`DAAgentToolBase::enableAutoScale(chart)` 在添加数据后调用 `setAxisAutoScale(xBottom/yLeft, true)`，因为 `DAFigureWidget::createChart()` 会通过 `setAxisScale(0,800)/(0,500)` 锁定坐标轴范围（禁用 Qwt auto-scale），不恢复会导致数据落在可见范围外而显示空白。

> 新增内置工具：在 `tools/` 新建 `DAAgentToolXxx.h/.cpp`（继承 `DAAgentToolBase`），在 `registerBuiltinTools()` 中 `registerTool(new DAAgentToolXxx(m_core, this))` 即可。CMake GLOB 自动收集。

### 7.3 注册与执行

- 注册：`DAAgentModule::registerTool` → `m_tools[name]` → `m_bridge->setTools(m_tools)`。
- 执行：`DAAgentBridge::executeTool()` 查表 → **try/catch 兜底**（工具抛异常时返回 `{success:false, error:...}`，避免 Bridge 崩溃导致子进程永久挂起）→ 回传 `tool_result` → 同时 emit `agentToolResult` 供 UI 展示。工具未设置 `success` 字段时自动补 `true`。
- **插件扩展**：插件可经 `DAAgentInterface::registerTool` 注入领域工具。注意：跨 DLL 派生 `DAAbstractAgentTool` 需要 `DAAgent_API` 导出宏（`DAAGENT_BUILD` 只在编译 DAAgent 库时定义，见 `CMakeLists.txt:44-48` 注释）。

---

## 八、DAGui 模块涉及的界面

> 聊天 UI 全部在 **DAGui 模块**（`src/DAGui/Agent/`），DAAgent 库只持有信号与逻辑。改动 UI 时到 DAGui/Agent 下找。

### 8.1 DAAgentDockWidget（聊天面板）

- 继承 **QWidget**，**不是 QDockWidget**——ADS 的 `DAAppDockingArea::createDockWidget(QWidget*, ...)` 会包装成 `ads::CDockWidget`；若继承 QDockWidget 会出现双标题栏/拖拽冲突。
- 布局：`QWebEngineView`（主区）+ 状态标签 + `QTextEdit`（输入）+ 发送按钮；`Ctrl+Enter` 发送快捷键。
- 加载 `qrc:///DAAgent/chat.html`；启用开发者工具（Qt6，`DeveloperToolsEnabled`；Qt5 用环境变量 `QTWEBENGINE_CHROMIUM_FLAGS=--remote-debugging-port=9222`）。
- 槽函数直接透传 Bridge 信号给 `DAAgentWebChannel` 的 append* 方法。
- **Dock 集成**：`DAAppDockingArea::buildDockingArea()` 创建，作为左侧管理区标签页（与工作流节点列表 `mWorkflowNodeListDock` 同组），Ribbon 大按钮 `actionShowAgentArea` 控制显隐（`DAAppRibbonArea.cpp:367`）。

### 8.2 DAAgentWebChannel（C++↔JS 桥）

- 是 **QObject**（不是 QWebChannel）；`QWebChannel` 实例单独 `new`，通过 `registerObject("chatBridge", m_channel)` 暴露，**名字必须与 chat.js 的 `channel.objects.chatBridge` 一致**。
- JS → C++：`onUserSelect(answer)`（选项点击）、`onUserMessage(text)`。
- C++ → JS：`callJS("funcName(...)")` 调 `page()->runJavaScript`；`toJsString()` 做字符串转义（引号/反斜杠/控制字符 → `\uXXXX`），工具参数/结果用 JSON 直接内嵌。
- 对应 chat.js 渲染函数：`appendUserMessage` / `appendToken`（防抖 50ms 渲染） / `finalizeAgentMessage` / `appendToolCall` / `appendToolResult` / `appendQuestion`（选项按钮 → `onUserSelect`） / `clearChat`。

### 8.3 DAAgentSettingsWidget（LLM 设置页）

- 继承 `DAAbstractSettingPage`（非 QWidget），注册到平台设置系统（参考 `src/APP/SettingPages/DASettingPagePython.h`）。
- 字段：Base URL / API Key（Password 模式）/ Model / Ready Timeout / Stop Timeout / 测试连接按钮。
- **必须连接 `settingChanged()`** 到字段变更信号，否则平台的脏页机制不会触发 `apply()`（配置永不保存）。
- `apply()` → `saveConfig()` → QSettings；`getLLMConfig()`（DAAgentModule）与设置页共用同一组 QSettings key。

### 8.4 前端资源

`chat.html` 引入 `qrc:///qtwebchannel/qwebchannel.js` + markdown-it + highlight.js；`chat.js` 维护 `currentAgentMsg` 与防抖渲染。改 UI 样式在 `chat.css`。

---

## 九、配置持久化（QSettings）

> DAAgent 库**无法链接 APP 的 DAAppConfig**，因此用 QSettings 持久化（`DAAgentModule::getLLMConfig` 与 `DAAgentSettingsWidget` 共用同一存储源）。

| Key | 含义 | 默认值 |
|-----|------|--------|
| `agent/llm_base_url` | LLM Base URL | — |
| `agent/llm_api_key` | **DPAPI 加密**后 Base64（Windows 当前用户作用域；非 Windows 仅为 base64，开发用） | — |
| `agent/llm_model` | 模型名 | — |
| `agent/ready_timeout_sec` | 子进程就绪超时（覆盖 langchain 冷启动 ~17s） | 60 |
| `agent/stop_timeout_sec` | stopAgent 等待退出超时 | 5 |

加密：Windows 用 `CryptProtectData`/`CryptUnprotectData`（`DAAgentSettingsWidget.cpp:214-264`），静态方法 `encryptApiKey`/`decryptApiKey` 供 `DAAgentModule` 调用。**日志/诊断禁止打印 api_key 明文**（只打加密 blob 大小）。

---

## 十、APP 层集成（生命周期）

1. `DAAppCore::initialize()` → `new DAAgentModule(this, this)` + `initialize()`（创建 Bridge、注册 15 个内置工具、预连接信号；**不创建 Dock**）。
2. `DAAppDockingArea::buildDockingArea()` → `new DAAgentDockWidget` + `createDockWidgetAsTab`（左侧标签页）。
3. `DAAppController`（initConnection 附近）→ `agentMod->setDockWidget(mDock->getAgentDockWidget())`（触发 `connectSignals` 完成 Bridge↔Dock 信号链）+ 绑定 `actionShowAgentArea` toggle action。
4. **懒启动**：首次 `sendMessage()` → `startAgentInternal()` → 读 QSettings LLM 配置 + 探测 Python/脚本路径 → `m_bridge->startAgent(...)`。
5. **退出**：`DAAgentBridge` 析构自动 `stopAgent()`（写 `stop` 消息 → `waitForFinished(stopTimeout)` → 必要时 `kill()`）。

---

## 十一、常见陷阱（铁律）

> 改 Agent 代码前必读。多数陷阱是已发生过的线上事故（见 `da_log.log` 与提交历史）。

### T1. stdout = 协议通道，日志禁止进 stdout
Python 端任何 `print`/调试输出必须走 stderr，否则污染 JSON Lines 流，C++ 端解析失败。`agent_runner.py:41-47` 的 logging 已配置到 stderr。

### T2. booting 心跳必须在导入 langchain 之前发送
`agent_runner.py:38` 在 import langchain 前 `sys.stdout.write('{"type": "booting"}\n')`。若不发，C++ 的 ready 超时计时器（默认 60s）会在冷启动导入（~16s）期间 `kill()` 子进程；`TerminateProcess` 不 flush Python stderr 块缓冲 → 表现为**静默崩溃**（exitCode=62097，无任何 stderr 输出）。

### T3. Dock 只能由 DAAppDockingArea 创建并注入
`initialize()` 里 `new DAAgentDockWidget` 会导致**两个 Dock 实例**，信号链连到隐藏的（未注册的）那个。Dock 就绪后必须经 `setDockWidget()` 注入。

### T4. 不要在 DAAgentModule::sendMessage 里 emit agentBusy
`DAAgentInterface` 无 `agentBusy` 信号（无 signals: 段），emit 无法编译。busy 状态由 `DAAgentBridge` 统一发射（`sendMessage`/`tool_call` 时 true，`done`/进程退出时 false）。

### T5. 工具执行必须 try/catch
工具抛异常 → Bridge 崩溃 → 子进程收不到 `tool_result` **永久挂起**。`executeTool()` 已兜底；新工具内部也要自行捕获并返回 `errorResponse`。

### T6. ToolMessage.content 必须是字符串
`agent_runner.py:309-312`：`content=json.dumps(result, ensure_ascii=False)`。langgraph 的 `ToolMessage.content` 只接受 str/list，传 dict 会报错。

### T7. token 流必须累积后再读 tool_calls
`agent_node` 逐 chunk 读 `.tool_calls` 拿到的是增量 delta（多为 None/部分）。必须把 `AIMessageChunk` 累加（`collected_chunks = collected_chunks + chunk`）后再从完整消息上读 `tool_calls`。

### T8. question 只发送一次，不能在 ask_user_node 内发
`ask_user_node` 只构造 `interrupt()` 值并暂停图；`resume` 后节点会**重新执行**，若在节点内发 question 会重复。统一由 `_send_question_if_paused()`（run() 与 resume() 共用）检测 interrupt 状态后发送**一次**，且暂停期间**不发送 done**。

### T9. Windows asyncio 约束
- 强制 `WindowsSelectorEventLoopPolicy`（`agent_runner.py:530-531`）：规避 ProactorEventLoop 的 `_empty_waiter` bug（cpython#103631，3.11 未修复）、QProcess 管道句柄 IOCP WinError 6。
- stdin 用后台线程 `read1()` 阻塞读取（`StdioProtocol.init_reader`）：`read(n)` 会等满 n 字节，对 QProcess 管道永久卡死。

### T10. C++ 端解析 stdout 行要裁 `\r`
Windows 文本模式行尾是 `\r\n`，`indexOf('\n')` 会留下 `'\r'` 导致 `QJsonDocument::fromJson` 失败。`DAAgentBridge.cpp:186-188` 已处理；进程退出时残留缓冲也要排空解析（`onProcessFinished`）。

### T11. init 失败时不要回显原始消息
`agent_runner.py:464`：若 C++ 误发含 api_key 的 init，回显原始内容会把密钥泄漏到 UI。用固定错误消息。

### T12. i18n 规范
- 界面字符串：`tr("English")` + `//cn:中文` 行内注释（C++）；禁止源码直接写中文 UI 文本。
- 日志：`daDebug` / `qInfo` 纯英文不翻译；`daCritical` / `daWarning` 进 UI 消息队列**必须翻译**（本项目启动失败的报错用 `daCritical`）。
- `DAAgentDockWidget` 中 `tr("Ready")` / `tr("Agent thinking...")` 等均为英文源 + cn 注释。

### T13. 日志与调试分流
- 子进程 stderr 经 `DAAgentBridge::onReadyReadStandardError` 转发到 `da_log.log`，标记 `Agent stderr:`（`\r\n` 被字面转义成字符串，不是真换行）。
- 诊断前缀 `[DAAgentSettings]`（loadConfig/saveConfig/apply）用于排查配置持久化问题。
- 注意区分：`da_pyscript.log` 是**嵌入式 Python**（工作流节点等）的输出，**agent 子进程的日志在 `da_log.log`**（子进程 logging 走 stderr → QProcess stderr → da_log.log）。

---

## 十二、调试指南

| 现象 | 排查路径 |
|------|---------|
| 子进程启动后 UI 干等/被杀 | 看 `da_log.log` 是否有 "Agent 子进程启动后 X 毫秒内未就绪"；检查 Python 依赖是否装齐（langgraph/langchain-openai） |
| 静默崩溃 exitCode=62097 无 stderr | 几乎都是 T2 问题（booting 心跳缺失/超时被杀，TerminateProcess 不 flush stderr） |
| 中文乱码/协议解析失败 | stdout 编码（`reconfigure(encoding="utf-8")`）与 `\r\n`（T10） |
| 工具结果不显示在对话流 | 检查 `agentToolResult` 信号是否连到 `onAgentToolResult`（`connectSignals` 是否在 `setDockWidget` 后完成） |
| 配置不保存 | `da_log.log` 搜 `[DAAgentSettings]`；确认字段变更连接了 `settingChanged()`（§ 8.3）；Windows 查注册表 `HKCU\Software\DAWorkBench`（QSettings） |
| 提问重复/卡在提问 | T8：question 是否在 ask_user_node 内误发；thread_id 是否一致（`agent_session_1`） |
| API key 相关 | 日志禁止打印明文；排查 `agent/llm_api_key` 是否为空（`api_key_enc_size=0`） |

**日志位置**：`%APPDATA%\DAWorkBench\log\da_log.log`（含 Agent stderr 转发）、`da_log.1.log`（上一次运行）。详见根 AGENTS.md「运行日志与调试」。

---

## 十三、WHERE TO LOOK（速查）

| 任务 | 位置 |
|------|------|
| 新增工具 | `src/DAAgent/tools/`（继承 `DAAgentToolBase`）+ `DAAgentModule::registerBuiltinTools` |
| 改系统提示词 | `DAAgentModule::assembleSystemPrompt()`；插件注册走 `registerSystemPrompt` |
| 改聊天渲染 | `src/DAGui/Agent/resources/chat.js` / `chat.html` / `chat.css` |
| 改聊天 UI 结构 | `src/DAGui/Agent/DAAgentDockWidget.cpp` |
| 改 C++↔JS 桥 | `src/DAGui/Agent/DAAgentWebChannel.cpp`（注意 `toJsString` 转义与 `chatBridge` 注册名） |
| 改协议 | `DAAgentBridge.cpp`（C++ 侧）+ `agent_runner.py` `StdioProtocol`/`main()`（Python 侧）——**两端必须同步** |
| 改 agent 推理逻辑 | `agent_runner.py` `AgentRunner`（图构建/节点/路由） |
| LLM 配置 | `DAAgentSettingsWidget.cpp`（设置页）+ `DAAgentModule.cpp` `getLLMConfig`/`setLLMConfig`（QSettings） |
| Dock/Ribbon 集成 | `src/APP/DAAppDockingArea.cpp:228` / `DAAppController.cpp:242` / `DAAppRibbonArea.cpp:367` |

---

## 十四、开发 Checklist

新增/修改 Agent 功能时逐项确认：

- [ ] 新类归属模块正确（工具→DAAgent，UI→DAGui，不违反依赖方向）
- [ ] Python 改动已同步到运行时 `bin/PyScripts/` 并重启程序验证
- [ ] 协议改动 C++/Python 两端同步，消息 type 大小写一致
- [ ] 新工具：`getToolSpec` schema 完整（含 required）、`execute` 内部 try/catch、错误用 `errorResponse`、已注册到 `registerBuiltinTools`
- [ ] UI 字符串英文源 + `//cn:` 注释；`daCritical`/`daWarning` 已翻译
- [ ] 不打印 api_key 明文日志
- [ ] 符合根 AGENTS.md 铁律（stdout 协议、booting 心跳、单 Dock 实例、ToolMessage str 等）
