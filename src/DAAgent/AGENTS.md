# DAAgent 模块开发指南

DAWorkbench 的 AI Agent 助手模块：内嵌 LLM 聊天 + 数据分析工具调用。**工具执行在 C++ 主进程**，**LLM 推理在 Python 子进程**（langchain-openai + langgraph），两者通过 stdin/stdout 管道以 **JSON Lines 协议**通信。聊天界面（DAGui/Agent）基于 QWebEngine + QWebChannel 渲染。

> ⚠️ 本文件是 AI 开发 Agent 相关功能（新增工具、修改聊天 UI、调协议、改 Python 脚本）的必读指南。核心设计约束见 § 十一（铁律），改动前先对照。

---

## 一、模块定位与构建

| 项 | 值 |
|----|----|
| 模块名 | `DAWorkbench::DAAgent`（SHARED 库） |
| 源码目录 | `src/DAAgent/`（模块根含 `DAAgentToolBase.h/.cpp`；`tools/` 子目录已删除） |
| 依赖（PUBLIC） | DAInterface, DAData, DAPyBindQt, DAPyScripts + Qt Core/Gui/Widgets（**无 DAGui / DAFigure / qwt / ADS**） |
| 依赖（PRIVATE） | Qt PrintSupport/Svg（PDF/SVG 导出）、DAAxOfficeWrapper（Win, docx 导出）、Crypt32（Win, DPAPI 加密 api_key） |
| 编译条件 | Python 为强制依赖，始终参与编译（`src/CMakeLists.txt:62`） |
| 导出宏 | `DAAgent_API`（`DAAgentAPI.h`，`DAAGENT_BUILD` 定义于 `CMakeLists.txt:48`） |

**层级**：DAAgent 位于接口层（Layer 4，与 DAInterface/DAPluginSupport 同级）。它向上提供 `DAAgentInterface` 供插件（Layer 5 及插件）注册工具/提示词、控制 UI 显隐与 LLM 配置；向下只依赖 DAInterface/DAData/DAPyBindQt/DAPyScripts（L1/L2）。**DAAgent 是纯 agent 框架库，不依赖任何 GUI 模块**（无 DAGui/DAFigure/qwt/ADS）——工具注册、UI 接线、设置页持久化全部由插件 / APP 层决定，DAGui 与 DAAgent 为兄弟模块互不依赖。创建新类时**禁止**让 DAAgent 依赖 APP，也禁止重新引入 DAGui 依赖（见铁律 T3）。

**构建**：`.\scripts\build.ps1 -Target DAAgent`。模块根 `*.h/*.cpp` 用 `file(GLOB ... CONFIGURE_DEPENDS)` 收集（`DAAgentToolBase.h/.cpp` 在根目录）。工具不再在本模块内，新增工具在 `plugins/DAAgentTools/`（见 § 七）。

---

## 二、架构总览（设计方案）

```
┌───────────────────────────────────────────────────────────────┐
│  C++ 主进程 (Host)                                             │
│  src/DAAgent/                                                  │
│   DAAgentInterface   — 公共接口（14 信号 + 工具/提示词注册）   │
│   DAAgentModule      — 接口实现：工具注册表、系统提示词组装、    │
│                        懒启动、LLM 配置读写（agent-config.ini）、│
│                        会话生命周期、cleanupSessions（读 ini 的  │
│                        max_sessions/session_retention_days）    │
│   DAAgentToolBase    — 瘦工具基类（DAAgent_API，数据/响应方法）  │
│   DAAgentBridge      — QProcess 子进程管理 + JSON Lines 协议解析 │
│                        （sendLoadSession 下发历史重建 state）   │
│   DAAgentSessionStore — 会话持久化层：JSONL 读写 / 索引 / 清理 / │
│                        标题 / last_active 指针（非 QObject）     │
│  sessions/（运行时，DADir::getAppDataPath("sessions")）         │
│   <id>.jsonl         — 每轮对话 append-only 追加                │
│   sessions_index.json — 全局索引（原子写 tmp+rename）            │
│   last_active.json    — 上次活跃会话指针（sessionId+projectPath）│
│  src/DAGui/Agent/（UI 在 DAGui 模块，与 DAAgent 互不依赖）       │
│   DAAgentDockWidget  — 聊天面板（QWebEngine + 输入框 + 会话下拉）│
│   DAAgentWebChannel  — C++↔JS 桥（registerObject "chatBridge"） │
│  src/APP/SettingPages/（设置页在 APP 层）                       │
│   DAAgentSettingsWidget — LLM 设置页（setAgentInterface 注入，  │
│                        走 get/setLLMConfig 持久化，见 § 九）    │
│  plugins/DAAgentTools/（工具插件）                              │
│   DAAgentChartToolBase + tools/ — 20 个内置工具，插件注册       │
└──────────────────────┬────────────────────────────────────────┘
                       │  QProcess 匿名管道
                       │  stdin/stdout: JSON Lines（每行一条 JSON）
┌──────────────────────▼────────────────────────────────────────┐
│  Python 子进程                                                 │
│  src/PyScripts/DAWorkbench/agent/agent_runner.py               │
│   StdioProtocol — 后台线程读 stdin → asyncio.Queue              │
│   AgentRunner   — langgraph StateGraph（agent/tools/ask_user）  │
│   ToolFactory   — C++ tool schema → bind_tools（含注入 ask_user）│
│   _stream_llm   — 读 collected_chunks.usage_metadata 回传 usage │
└───────────────────────────────────────────────────────────────┘
```


### 信号与依赖架构（重构后）

- `DAAgentInterface` 暴露 **14 个 agent 生命周期/会话信号**：Bridge 转发的 10 个（`agentToken` / `agentMessageComplete` / `agentToolCall` / `agentToolResult` / `agentQuestion` / `agentError` / `agentReady` / `agentBusy` / `agentDone` / `agentSessionLoaded`）+ Module 上移的 4 个（`tokenUsageUpdated` / `sessionSwitched` / `sessionCreated` / `sessionListChanged`），以及 `sendUserAnswer` / `newSession` 两个纯虚方法。
- `DAAgentModule` 转发 `DAAgentBridge` 的 10 个信号到接口（`connectSignals` 内 Bridge→接口信号连接），自身 emit 4 个会话/用量信号（如 `tokenUsageUpdated` 由 Bridge `agentUsage` 经 lambda 补 `context_window` 后发射）。
- **Dock 的信号↔槽由 `DAAppController`（APP 层）直接 connect**（决策 D3b）：接口 14 信号中 13 条 → Dock 槽、Dock 8 信号中 7 条 → 接口方法（信号→方法 PMF）。Module 不再持有 Dock，`setDockWidget` 已废弃删除，`showDockWidget`/`hideDockWidget` 为 no-op（仅留接口签名兼容）。
- **DAGui 与 DAAgent 互不依赖**：DAGui 不 link DAAgent，DAAgent 不 link DAGui；二者由 APP 层 `connect` 协调（详见 § 六）。

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
| `DAAgentInterface.h` | 公共接口：14 个信号 + `registerTool` / `registerSystemPrompt` / `showDockWidget` / `hideDockWidget`（no-op）/ `sendMessage` / `stop` / `sendUserAnswer` / `newSession` / `isRunning` / `getLLMConfig` / `setLLMConfig` |
| `DAAgentModule.h/.cpp` | 接口实现：工具注册表 `m_tools`、系统提示词 `m_systemPrompts`、懒启动、`connectSignals()`（仅 Bridge→Module 持久化/状态 lambda，**不连 Dock**）、LLM 配置读写（agent-config.ini + api_key DPAPI 加解密）、Python/脚本路径探测 |
| `DAAgentBridge.h/.cpp` | QProcess 生命周期（start/stop/超时）、stdin/stdout 读写、JSON Lines 解析分发、工具执行兜底；`sendLoadSession` 下发历史 messages 重建 state |
| `DAAgentSessionStore.h/.cpp` | 会话持久化层（非 QObject，PIMPL）：JSONL append-only 读写、全局索引（原子写 tmp+rename）、`cleanupOldSessions`（数量+时间双限）、`setLastActive`/`lastActiveSession`（按工程过滤的精确匹配）、自动标题、工程导入导出 |
| `DAAbstractAgentTool.h` | 工具抽象基类（纯虚）：`getToolSpec` / `execute` / `getOwnerModule` |
| `DAAgentToolBase.h/.cpp` | **瘦**工具基类（`class DAAgent_API DAAgentToolBase`，模块根目录）：`dataMgr`/`findData`/`allDatas` + `errorResponse`/`successResponse`；图表方法已移入 `DAAgentChartToolBase`（见 § 七） |
| `DAAgentAPI.h` | `DAAgent_API` 导出宏 |

### 3.2 `src/DAGui/Agent/`（聊天 UI，属 DAGui 模块）

| 文件 | 职责 |
|------|------|
| `DAAgentDockWidget.h/.cpp` | 聊天面板 QWidget（**非 QDockWidget**，见 § 八）；持有 WebEngine 视图、输入框、发送按钮、状态标签 |
| `DAAgentWebChannel.h/.cpp` | QWebChannel 桥对象（注册名 `chatBridge`）；JS 调 `onUserSelect`/`onUserMessage`，C++ 调 `callJS()` 驱动 JS 渲染函数 |
| `resources/` | `chat.html` + `chat.js` + `chat.css` + `markdown-it.min.js` + `highlight.min.js` + `chat.qrc` |

> LLM 设置页 `DAAgentSettingsWidget` 已迁至 `src/APP/SettingPages/`（APP 层），不再属 DAGui/Agent，见 § 九。

### 3.3 `src/PyScripts/DAWorkbench/agent/`（Python 子进程）

| 文件 | 职责 |
|------|------|
| `agent_runner.py` | 唯一入口脚本：协议收发、LLM 配置、langgraph 图构建、agent 循环；权限层（permission-layer）：存储 §8 权限字段、`tool_node` 发起 `tool_call` 前对代码执行工具调 `permission_judge` 产出 `safety` 裁决、gated_tools 长超时 |
| `permission_judge.py` | 代码内容判定管线（咨询方）：静态危险模式（deny/escalate，清单由 `code_patterns` 配置注入）+ 可选判官模型（复用当前供应商凭据），产出 `{verdict, reason, source}`；`run_script` 按 `workspace_root` 解析入口文件后走同一管线（判定边界=入口文件，不递归） |
| `context_manager.py` / `error_classifier.py` / `retry_wrapper.py` | 上下文压缩/截断、错误分类、退避重试（agent_runner 的基础设施模块） |

### 3.4 `src/APP/`（集成点）

| 文件 | 职责 |
|------|------|
| `DAAppCore.cpp` | `new DAAgentModule(this, this)` + `initialize()`，多态持有 `DAAgentInterface*` |
| `DAAppDockingArea.cpp` | `new DAAgentDockWidget` + `createDockWidgetAsTab`（左侧管理区标签页） |
| `DAAppController.cpp` | 初始化中直接 `connect` Dock↔`DAAgentInterface` 信号链（13 条接口信号→Dock 槽 + 7 条 Dock 信号→接口方法，见 § 六）+ 绑定 `actionShowAgentArea` toggle action |
| `DAAppActions.cpp` / `DAAppRibbonArea.cpp` | Ribbon 大按钮「Agent 助手」 |

---

## 四、Python 脚本位置（重点）

| 项 | 路径 |
|----|------|
| **源码** | `src/PyScripts/DAWorkbench/agent/agent_runner.py`（开发时编辑此处） |
| **运行时** | `<exe目录>/PyScripts/DAWorkbench/agent/agent_runner.py`（`applicationDirPath() + "/PyScripts"`，见 `DACoreInterface::getPythonScriptsPath()` `DACoreInterface.cpp:107`） |
| **部署机制** | 根 `CMakeLists.txt:254-261` `file(COPY src/PyScripts → bin/PyScripts)` |

路径探测：`DAAgentModule::detectAgentScriptPath()` = `getPythonScriptsPath() + "/DAWorkbench/agent/agent_runner.py"`；Python 解释器用 `DAPyInterpreter::getPythonInterpreterPath()`（`DAPyInterpreter.cpp:327`），兜底 `QStandardPaths::findExecutable("python")`。

> ⚠️ **修改 Python 文件后必须同步到运行时目录**（重跑 cmake 配置或手动 copy），且**必须重启程序**——Python 在启动时导入，不支持热重载。cmake 的 `file(COPY)` 只在 configure 阶段执行，不会跟踪文件变更。

### 4.1 系统提示词 markdown（外部可编辑）

| 项 | 路径 |
|----|------|
| **源码** | `src/DAAgent/system_prompt.md`（开发时编辑此处） |
| **运行时** | `<exe目录>/PyScripts/DAWorkbench/agent/system_prompt.md`（与 `agent_runner.py` 同目录） |
| **部署机制** | `src/DAAgent/CMakeLists.txt`：`install(FILES ...)` + `file(COPY ...)` 双写（镜像 PyScripts 模式） |

平台基础系统提示词不再硬编码在 C++ 中，而是由 `DAAgentModule::assembleSystemPrompt()` 在调用时（首次 `sendMessage()` 触发懒启动）从上述 markdown 文件读取。**修改 markdown 后无需重启程序，下一轮 agent 会话即生效**。文件缺失或为空时回退到内置默认提示词（`kDefaultPrompt` 常量）并写 `daWarning` 到 `da_log.log`，保证异常部署下 agent 仍可用。插件经 `registerSystemPrompt` 注入的提示词仍在基础提示词之后拼接，机制不变。

**Python 依赖**（`requirements.txt`）：`langgraph`、`langchain-openai`、`pydantic>=2.0`（langgraph 的 `interrupt()` 需要）。

---

## 五、JSON Lines 协议

stdout 专用于协议，**绝对禁止在 stdout 打印日志**（污染协议流会导致 C++ 端解析失败）。所有调试输出写 stderr。

### 5.1 C++ → Python（stdin）

| type | 载荷 | 说明 |
|------|------|------|
| `init` | `config`{base_url, api_key, model, context_window, compaction_threshold, max_recent_messages, tool_result_max_chars, tool_result_preview_chars, **permission_mode, workspace_root, gated_tools[], tool_approval_timeout_sec, code_patterns{deny[],escalate[]}, judge{model,timeout_sec}**} + `tools`(schema 数组) + `system_prompt` | 启动时一次性下发；config 缺 base_url/api_key/model 任一则报错退出；上下文管理参数有默认值兜底；权限层字段由 `DAAgentBridge::buildPermissionConfig()` 组装（母文档 §8，Python 侧存储并在 auto 模式消费） |
| `user_msg` | `content` | 用户消息，触发一轮 agent 推理 |
| `tool_result` | `call_id` + `result` | 工具执行结果回传（RPC 应答） |
| `user_answer` | `answer` | 用户对 HITL 问题的回答，触发 `resume()` |
| `load_session` | `session_id` + `messages`(JSON 数组，T6 记录的 message 字段) | **切换/恢复会话**时下发历史 messages 重建 langgraph state（不重启子进程）；Python 端 `graph.aupdate_state` 注入后回 `session_loaded` 确认 |
| `reconfigure` | `config`{base_url, api_key, model, max_output_tokens, context_window, ..., 权限层字段同 `init`} | **热替换 LLM 配置**（不重启子进程、不丢 MemorySaver 会话状态）：Python 端 `AgentRunner.reconfigure()` 热替换 ChatOpenAI + compactor/token_estimator，图与 state 不动，回 `ready` 确认。消息在 stdin 排队，当前轮跑完后主循环处理，下一轮用新模型（见 §15.3）。权限模式切换/设置页保存后经此同步权限层字段（Python 仅存储 + 按模式决定是否判定） |
| `stop` | — | 优雅停止，子进程退出主循环 |

### 5.2 Python → C++（stdout）

| type | 载荷 | 说明 |
|------|------|------|
| `booting` | — | **必须在导入 langchain 之前发送**；C++ 收到后重置 ready 超时计时器（冷启动 ~16s） |
| `ready` | `model` | 初始化完成（或 `reconfigure` 热替换完成），C++ 停止 ready 计时器。Module 的 `agentReady` 处理器在非恢复路径（`isRecovering()==false` 且无 pending load_session）时早返回，故 `reconfigure` 复用本信号安全无副作用 |
| `token` | `content` | 流式 token |
| `message_end` | `content` | 本轮最终回复（agent_node 在无 tool_calls 时发送） |
| `tool_call` | `call_id` + `tool` + `arguments` + 可选 `safety`{verdict: allow/deny/uncertain, reason: str, source: rules/model/none} | 请求 C++ 执行工具；C++ 回传 `tool_result`。`safety` 为 Python 侧代码裁决，**仅 auto 模式 + code_exec 工具**（`run_code`/`run_script`）产出，其余缺省不带；C++ 权限门消费：deny→拒绝（脱敏文案）、allow→放行（判官已配置时）、uncertain/缺失→ask（判官未配置时 allow 也降级 ask，D1） |
| `question` | `text` + `options` | HITL 提问（**只发一次**，见铁律 T8） |
| `usage` | `input_tokens` + `output_tokens` + `total_tokens` + `source` | LLM `usage_metadata` 权威 token 统计回传（`_stream_llm` 读 `collected_chunks.usage_metadata`）；C++ 收到后发 `agentUsage` 信号供 UI 显示占比 |
| `session_loaded` | `session_id` | `load_session` 后 Python 重建 state 完成的确认；C++ 收到才允许下一轮 `sendMessage`（见铁律 T15） |
| `error` | `message` | 子进程侧错误 |
| `done` | — | 本轮处理结束（暂停于 interrupt 时不发） |

### 5.3 协议细节

- Windows 下 Python 文本模式输出 `\r\n`，C++ 端 `onReadyReadStandardOutput` 解析行前必须 `chop(1)` 裁掉 `'\r'`（`DAAgentBridge.cpp:183-188`）。
- `tool_call` 的执行通过 `QTimer::singleShot(0)` 投递回主线程事件循环，避免在 stdout 读取回调里长时间阻塞管道（管道阻塞会死锁子进程）。
- Python 端 `_wait_for_result` **严格匹配 `call_id`**，迟到的/错配的 `tool_result` 记日志后忽略。

---

## 六、信号链（C++ 内部）

> 决策 D3b 落地后的结构（plan-01/02）：**DAAgentInterface 暴露 14 个信号**；`DAAgentModule::connectSignals` 只保留 Bridge→Module 的持久化/状态 lambda，**不再连接 Dock**；Dock 的信号↔槽由 `DAAppController::initialize()` 直接 connect。

```
DAAgentBridge（10 个信号转发 + agentUsage 内部消费）
  ├─ agentToken / agentMessageComplete / agentToolCall / agentToolResult
  ├─ agentQuestion / agentError / agentReady / agentBusy / agentDone / agentSessionLoaded
  └──► DAAgentModule::connectSignals()（只连 Bridge→Module 的持久化/状态 lambda：
        agentMessageComplete 写会话、agentUsage→tokenUsageUpdated（补 context_window）、
        agentQuestion/agentReady/agentBusy/agentDone/agentError 状态处理；不连 Dock）
        └──► 转发/发射到 DAAgentInterface 的 14 个信号
              └──► DAAppController::initialize() 直接 connect 到
                    DAAgentDockWidget::onAgent* 槽（13 条）
                          └─► DAAgentWebChannel::append* → callJS() → chat.js 渲染函数
```

**接口↔Dock 的 connect 清单（`DAAppController::initialize()`，决策 D3b）**：

- **接口信号 → Dock 槽（13 条）**：`agentToken` / `agentMessageComplete` / `agentToolCall` / `agentToolResult` / `agentQuestion` / `agentError` / `agentReady` / `agentBusy` / `agentSessionLoaded` / `tokenUsageUpdated` / `sessionSwitched` / `sessionListChanged` / `sessionCreated` → `DAAgentDockWidget::onAgent*` / `onSession*` 槽。（接口的 `agentDone` 目前无 Dock 槽对接，作为生命周期事件保留在接口。）
- **Dock 信号 → 接口方法（7 条，信号→方法 PMF）**：`sendMessageRequested` → `sendMessage`、`stopRequested` → `stop`、`userAnswerSelected` → `sendUserAnswer`、`sessionSwitchRequested` → `switchSession`、`sessionDeleteRequested` → `deleteSession`、`sessionRenameRequested` → `renameSession`、`sessionCreateRequested` → **`newSession`**（不是 `createSession`——前者会 emit `sessionCreated` 触发 UI clearChat，后者不 emit）。Dock 的第 8 个信号 `agentStopRequested` 不连接口。

**用户消息反向路径**：
`onSendClicked()` → `m_channel->appendUserMessage(text)`（先渲染，勿重复调用）+ `emit sendMessageRequested` →（DAAppController connect）→ `DAAgentInterface::sendMessage` → `m_bridge->sendMessage` → stdin。

**用户回答路径**：
chat.js 选项按钮 → `chatBridge.onUserSelect(answer)` → `DAAgentWebChannel::userAnswerSelected` → `DAAgentDockWidget::onUserAnswer` → `userAnswerSelected`（Dock 信号）→（DAAppController connect）→ `DAAgentInterface::sendUserAnswer`（方法体吸收原 `connectSignals` 中 dock::userAnswerSelected 的持久化 lambda）→ `m_bridge->sendUserAnswer` → stdin `user_answer`。

> ⚠️ `setDockWidget` 已废弃删除；`showDockWidget`/`hideDockWidget` 为 no-op（仅留接口签名兼容）。Module 不持有 Dock、`connectSignals` 不再连 Dock，Dock 连接唯一入口是 `DAAppController::initialize()`。不要试图在别处 `new DAAgentDockWidget`（见铁律 T3）。

---

## 七、工具系统

### 7.1 抽象与基类

- `DAAbstractAgentTool`（纯虚）：`getToolSpec()` 返回 OpenAI function schema，`execute(params)` 返回结果 JSON，`getOwnerModule()` 返回归属模块。
- `DAAgentToolBase`（`src/DAAgent/DAAgentToolBase.h`，`class DAAgent_API DAAgentToolBase`）：**瘦**工具基类（模块根目录，`DAAgent_API` 导出供插件跨 DLL 继承），提供数据访问 + 响应方法：
  - `dataMgr()` / `findData(name)` / `allDatas()`
  - `errorResponse(msg)` / `successResponse(data|message)`
  - 图表方法已移入插件的 `DAAgentChartToolBase`（本模块不再 include 图表头文件，无 DAFigure 依赖）

### 7.2 平台内置工具（20 个，由插件 `plugins/DAAgentTools/` 注册）

| 类别 | 工具（name） | 文件 |
|------|-------------|------|
| 数据 (5) | `list_data` / `get_data_info` / `query_data` / `get_column_stats` / `export_data` | `DAAgentToolListData` / `DAAgentToolDataInfo` / `DAAgentToolQueryData` / `DAAgentToolColumnStats` / `DAAgentToolExportData` |
| 绘图 (10) | `create_chart` / `add_curve` / `set_chart_style` / `set_axis` / `update_curve_style` / `remove_chart_item` / `add_annotation` / `create_subplots` / `save_chart_image` / `list_figures` | `DAAgentToolCreateChart` / `DAAgentToolAddCurve` / `DAAgentToolSetChartStyle` / `DAAgentToolSetAxis` / `DAAgentToolUpdateCurveStyle` / `DAAgentToolRemoveChartItem` / `DAAgentToolAddAnnotation` / `DAAgentToolCreateSubplots` / `DAAgentToolSaveChartImage` / `DAAgentToolListFigures` |
| 文件/报告 (3) | `read_file` / `write_file` / `save_report` | `DAAgentToolReadFile` / `DAAgentToolWriteFile` / `DAAgentToolSaveReport` |
| 代码执行 (2) | `run_code` / `run_script` | `DAAgentToolRunCode` / `DAAgentToolRunScript`（`e6401be` 新增；权限分级 `code_exec`，auto 模式经 `tool_call.safety` 裁决，见铁律 T16） |

#### 绘图工具关键设计

- **`create_chart` / `create_subplots` 每次调用创建新 figure**：通过 `DAAgentChartToolBase::createFigure(name)` 创建新 figure（标签页），再在其内部创建 chart。不会复用已有 figure/chart，避免多张图叠加到同一绘图。
- **`figure_name` 参数**：所有绘图工具（`add_curve` / `set_chart_style` / `add_annotation` / `save_chart_image`）均支持可选 `figure_name` 参数，通过 `findChart(chartId, figureName)` 在指定 figure 中定位 chart。`create_chart` / `create_subplots` 的 `figure_name` 用于命名新 figure（标签页标题）。
- **`list_figures` 工具**：列出所有 figure 及其内部 chart 的名称/索引/标题，供 agent 检索已有绘图后通过 `figure_name` + `chart_id` 精确定位修改。
- **坐标轴自动缩放**：`DAAgentChartToolBase::enableAutoScale(chart)` 在添加数据后调用 `setAxisAutoScale(xBottom/yLeft, true)`，因为 `DAFigureWidget::createChart()` 会通过 `setAxisScale(0,800)/(0,500)` 锁定坐标轴范围（禁用 Qwt auto-scale），不恢复会导致数据落在可见范围外而显示空白。

> 新增平台内置工具：在 `plugins/DAAgentTools/tools/` 新建 `DAAgentToolXxx.h/.cpp`（数据/文件工具继承瘦 `DAAgentToolBase`，图表工具继承 `DAAgentChartToolBase`），并在 `DAAgentToolsPlugin::initialize()` 中 `core()->getAgentInterface()->registerTool(new DAAgentToolXxx(c, this))`。插件 CMake GLOB 自动收集。

### 7.3 注册与执行

- **内置工具插件**：20 个工具由独立插件 `plugins/DAAgentTools/` 提供。插件入口 `DAAgentToolsPlugin`（继承 `DAAbstractPlugin`，IID `org.da.abstract.plugin`）在 `initialize()` 中经 `core()->getAgentInterface()->registerTool(...)` 依次注册 20 个工具。继承关系：`DAAbstractAgentTool` → `DAAgentToolBase`（瘦，`DAAgent_API` 导出，数据/响应方法）→ 10 个非图表工具（5 数据 + 3 文件/报告 + 2 代码执行）；`DAAgentToolBase` → `DAAgentChartToolBase`（7 个图表方法）→ 10 个图表工具。
- **第三方扩展**：领域工具插件可继承瘦 `DAAgentToolBase`（数据工具）或 `DAAgentChartToolBase`（图表工具），经 `DAAgentInterface::registerTool` 注入，无需改 DAAgent。跨 DLL 派生需要 `DAAgent_API` 导出宏（`DAAGENT_BUILD` 只在编译 DAAgent 库时定义，`DAAgentToolBase` 已 `DAAgent_API` 导出）。
- 注册：`DAAgentModule::registerTool` → `m_tools[name]` → `m_bridge->setTools(m_tools)`。
- 执行：`DAAgentBridge::executeTool()` 查表 → **try/catch 兜底**（工具抛异常时返回 `{success:false, error:...}`，避免 Bridge 崩溃导致子进程永久挂起）→ 回传 `tool_result` → 同时 emit `agentToolResult` 供 UI 展示。工具未设置 `success` 字段时自动补 `true`。

---

## 八、DAGui 模块涉及的界面

> 聊天 UI（`DAAgentDockWidget` / `DAAgentWebChannel` / 前端资源）在 **DAGui 模块**（`src/DAGui/Agent/`）；LLM 设置页 `DAAgentSettingsWidget` 在 **APP 层** `src/APP/SettingPages/`（见 § 九）。DAAgent 库只持有信号与逻辑，不依赖 DAGui。

### 8.1 DAAgentDockWidget（聊天面板）

- 继承 **QWidget**，**不是 QDockWidget**——ADS 的 `DAAppDockingArea::createDockWidget(QWidget*, ...)` 会包装成 `ads::CDockWidget`；若继承 QDockWidget 会出现双标题栏/拖拽冲突。
- 布局：`QWebEngineView`（主区）+ 状态标签 + `QTextEdit`（输入）+ 发送按钮；`Ctrl+Enter` 发送快捷键。
- 加载 `qrc:///DAAgent/chat.html`；启用开发者工具（Qt6，`DeveloperToolsEnabled`；Qt5 用环境变量 `QTWEBENGINE_CHROMIUM_FLAGS=--remote-debugging-port=9222`）。
- 槽函数接收来自 `DAAgentInterface` 的信号（经 `DAAppController` connect），透传给 `DAAgentWebChannel` 的 append* 方法；信号/槽签名不变。
- **Dock 集成**：`DAAppDockingArea::buildDockingArea()` 创建，作为左侧管理区标签页（与工作流节点列表 `mWorkflowNodeListDock` 同组），Ribbon 大按钮 `actionShowAgentArea` 控制显隐（`DAAppRibbonArea.cpp`）。Dock 的信号↔槽由 `DAAppController::initialize()` connect 到 `DAAgentInterface`（§ 六），Module 不再持有 Dock。

### 8.2 DAAgentWebChannel（C++↔JS 桥）

- 是 **QObject**（不是 QWebChannel）；`QWebChannel` 实例单独 `new`，通过 `registerObject("chatBridge", m_channel)` 暴露，**名字必须与 chat.js 的 `channel.objects.chatBridge` 一致**。
- JS → C++：`onUserSelect(answer)`（选项点击）、`onUserMessage(text)`。
- C++ → JS：`callJS("funcName(...)")` 调 `page()->runJavaScript`；`toJsString()` 做字符串转义（引号/反斜杠/控制字符 → `\uXXXX`），工具参数/结果用 JSON 直接内嵌。
- 对应 chat.js 渲染函数：`appendUserMessage` / `appendToken`（防抖 50ms 渲染） / `finalizeAgentMessage` / `appendToolCall` / `appendToolResult` / `appendQuestion`（选项按钮 → `onUserSelect`） / `clearChat`。

### 8.3 前端资源

> LLM 设置页 `DAAgentSettingsWidget` 已迁至 APP 层 `src/APP/SettingPages/`，持久化走 `DAAgentInterface::get/setLLMConfig`，见 § 九。

`chat.html` 引入 `qrc:///qtwebchannel/qwebchannel.js` + markdown-it + highlight.js；`chat.js` 维护 `currentAgentMsg` 与防抖渲染。改 UI 样式在 `chat.css`。

---

## 九、配置持久化（agent-config.ini）

> DAAgent 库**无法链接 APP 的 DAAppConfig**，因此用 QSettings 持久化。**唯一入口是 `DAAgentInterface::getLLMConfig` / `setLLMConfig`**，读写显式路径 `DA::DADir::getConfigPath() + "/agent-config.ini"`。APP 设置页（`src/APP/SettingPages/DAAgentSettingsWidget`）经 `setAgentInterface` 注入接口，`loadConfig` 调 `getLLMConfig`（返回明文 QJsonObject）、`saveConfig` 调 `setLLMConfig`（传明文 QJsonObject）——页面不接触 QSettings / 加解密。

| Key | 含义 | 默认值 |
|-----|------|--------|
| `agent/llm_base_url` | LLM Base URL（激活供应商派生，见 `agent/providers`） | — |
| `agent/llm_api_key` | **DPAPI 加密**后 Base64（Windows 当前用户作用域；非 Windows 仅为 base64，开发用）；激活供应商派生 | — |
| `agent/llm_model` | 当前激活模型 id（激活供应商下的某个模型） | — |
| `agent/providers` | 供应商 JSON 数组（Compact 字符串）：每元素 `{name, base_url, api_key(加密 base64), models:[{id,context_window,max_output_tokens}]}`。由 `getProviders`/`setProviders` 读写，api_key 内部 DPAPI 加解密；模型为对象，含每模型上下文窗口与最大输出 token | — |
| `agent/active_provider` | 当前激活供应商名称（Dock 模型下拉选择 / 设置页 apply 写入） | — |
| `agent/max_output_tokens` | 当前激活模型最大输出 token（由激活模型派生，随 init 下发 Python `max_tokens`），默认 8192 | 8192 |
| `agent/ready_timeout_sec` | 子进程就绪超时（覆盖 langchain 冷启动 ~17s） | 60 |
| `agent/stop_timeout_sec` | stopAgent 等待退出超时 | 5 |
| `agent/context_window` | 模型上下文窗口大小（tokens），用于触发压缩判断 | 1048576 |
| `agent/compaction_threshold` | 自动压缩触发比例（0.85 = 窗口 85% 时触发） | 0.85 |
| `agent/max_recent_messages` | 压缩后保留最近消息条数 | 10 |
| `agent/tool_result_max_chars` | 工具结果截断阈值（字符数），超此长度截断为预览 | 50000 |
| `agent/tool_result_preview_chars` | 截断后工具结果预览长度（字符数） | 2000 |
| `agent/max_sessions` | 配置目录保留的自由会话最大数量（超出按 updatedAt 倒序删最旧），由 `DAAgentModule::cleanupSessions` 读取 | 20 |
| `agent/session_retention_days` | 自由会话保留天数（早于此天数的会话启动时清理），由 `DAAgentModule::cleanupSessions` 读取 | 30 |

> 接口层 JSON 键（12 个）：`base_url` / `model` / `api_key` / `context_window` / `compaction_threshold` / `max_recent_messages` / `tool_result_max_chars` / `tool_result_preview_chars` / `ready_timeout_sec` / `stop_timeout_sec` / `max_sessions` / `session_retention_days`。`getLLMConfig` 全量 round-trip（带默认值兜底）；`setLLMConfig` 对每个 key 逐一 `config.contains()` 守卫（key 存在即写，含空串；api_key 传空串可清空），api_key 经 `DAAgentModule` 内部 DPAPI 加密后写入。

> 注：`max_sessions` / `session_retention_days` 仅 C++ 侧用（`getLLMConfig` 不下发 Python）。三处默认值须一致：设置页 spin range/setValue、`src/APP/SettingPages/DAAgentSettingsWidget::loadConfig`/`saveConfig`、`DAAgentModule::cleanupSessions` 的 QSettings 读取。`cleanupOldSessions` 入口已加 `qMax(1, maxCount)` / `qMax(0, retentionDays)` 防护 ini 被手改为 0/负时误删全部。

加密：`api_key` 由 `DAAgentModule` 内部 DPAPI 加解密（Win，DAAgent link `Crypt32`；`encryptApiKey`/`decryptApiKey` 为本模块匿名命名空间静态函数）/ base64 fallback（非 Win，开发用）。设置页只经接口传明文，不做任何加解密。**日志/诊断禁止打印 api_key 明文**（只打加密 blob 大小）。

---

## 十、APP 层集成（生命周期）

1. `DAAppCore::initialize()` → `new DAAgentModule(this, this)` + `initialize()`（创建 Bridge、预连接 Bridge→Module 信号；**不注册工具、不创建 Dock**——20 个内置工具由插件 `DAAgentTools` 注册）。
2. `DAAppDockingArea::buildDockingArea()` → `new DAAgentDockWidget` + `createDockWidgetAsTab`（左侧标签页）。
3. `DAAppController::initialize()` → 用 `connect()` 把 Dock 的 8 个信号（其中 7 个连到接口方法）↔ 接口的 14 个信号（其中 13 个连到 Dock 槽）对接（决策 D3b，替代旧的 `setDockWidget` 注入）+ 绑定 `actionShowAgentArea` toggle action（详见 § 六）。
4. **懒启动**：首次 `sendMessage()` → `startAgentInternal()` → 读 `agent-config.ini` LLM 配置 + 探测 Python/脚本路径 → `m_bridge->startAgent(...)`。
5. **退出**：`DAAgentBridge` 析构自动 `stopAgent()`（写 `stop` 消息 → `waitForFinished(stopTimeout)` → 必要时 `kill()`）。

---

## 十一、常见陷阱（铁律）

> 改 Agent 代码前必读。多数陷阱是已发生过的线上事故（见 `da_log.log` 与提交历史）。

### T1. stdout = 协议通道，日志禁止进 stdout
Python 端任何 `print`/调试输出必须走 stderr，否则污染 JSON Lines 流，C++ 端解析失败。`agent_runner.py:41-47` 的 logging 已配置到 stderr。

### T2. booting 心跳必须在导入 langchain 之前发送
`agent_runner.py:38` 在 import langchain 前 `sys.stdout.write('{"type": "booting"}\n')`。若不发，C++ 的 ready 超时计时器（默认 60s）会在冷启动导入（~16s）期间 `kill()` 子进程；`TerminateProcess` 不 flush Python stderr 块缓冲 → 表现为**静默崩溃**（exitCode=62097，无任何 stderr 输出）。

### T3. Dock 只能由 DAAppDockingArea 创建，信号链由 DAAppController connect
Dock 是 DAGui 的 `DAAgentDockWidget`，只能由 `DAAppDockingArea::buildDockingArea()` 创建（`initialize()` 里 `new` 会导致**两个 Dock 实例**，信号链连到隐藏的那个）。**DAAgentModule 不再持有 Dock**，Dock 的信号↔槽由 `DAAppController::initialize()` connect 到 `DAAgentInterface`（§ 六）；`setDockWidget` 已废弃删除。DAGui 与 DAAgent 互不依赖。

### T4. 只在接口已声明的信号处 emit
`DAAgentInterface` 已声明 **14 个信号**（含 `agentBusy(bool)`，plan-01 加入），`DAAgentModule` 在这些信号上 emit 是合法的（10 个 Bridge 转发 + 4 个自身 emit）。但 busy 状态**不要**在 `sendMessage` 里手动 emit——它由 `DAAgentBridge` 统一发射（`sendMessage`/`tool_call` 时 true，`done`/进程退出时 false）并经 `connectSignals` 转发到接口。原则：**勿在接口未声明的信号处 emit**（会编译失败）；新增信号必须先在 `DAAgentInterface` 声明。

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

### T14. 上下文管理（compact_node / RemoveMessage / 溢出恢复）
- **compact_node 必须在 agent 之前**：图结构为 `START → compact → agent → {ask_user | tools | END}`，`tools → compact`、`ask_user → compact`。compact 不需要压缩时返回 `{"messages": []}`（零开销）。
- **RemoveMessage 需消息有 id**：`ContextCompactor.compact()` 用 `RemoveMessage(id=msg.id)` 删除中间消息。langchain 消息自动生成 id，但若消息无 id 则跳过删除（不报错）。
- **摘要失败要熔断不阻塞流程**：compact_node 的 `try/except` 捕获摘要 LLM 调用失败，返回 `{"messages": []}`（不压缩），`_consecutive_failures` 计数；连续 3 次后 `should_compact` 返回 False（熔断），直到成功一次重置。
- **溢出恢复是局部的（不写回 state）**：agent_node 捕获 `ContextWindowExceededError` 后调 `force_compact` 生成压缩后消息列表，**仅用于本次 LLM 重试**；下轮 compact_node 会做持久化压缩（RemoveMessage 写回 state）。
- **系统提示词不进 state**：`agent_node` 每轮 prepend `SystemMessage`（不返回到 state），compact 的 head/tail 选择中不含 system 消息。
- **工具结果截断在 tool_node**：`ToolResultTruncator.truncate()` 在 `json.dumps(result)` 后、构造 `ToolMessage` 前执行，截断后的内容直接进 state，后续轮次受益。
- **摘要用 self.llm（不绑 tools）**：`ContextCompactor._generate_summary()` 用 `ChatOpenAI.ainvoke()`（无 `bind_tools`），摘要无需工具调用。
- **token 估算只用于提前触发**：tiktoken/char-based 估算偏向早触发（宁可早压缩不要溢出），不用于"跳过"判断。反应式溢出恢复是安全网，覆盖估算不准的场景。

### T15. 会话持久化（plan-03/05/06）
- **JSONL 文件不含 api_key 明文**：会话记录只存对话消息（user/assistant/tool_result/usage），init 时下发的 LLM 配置（含 api_key）绝不下发到磁盘的 jsonl；工具结果截断沿用 `tool_result_max_chars`，避免敏感数据膨胀。
- **切换会话不重启子进程**：`DAAgentModule::switchSession` 直接调 `m_bridge->sendLoadSession(sessionId, messages)` 下发历史重建 langgraph state，**不** `stopAgent()`/`startAgent()`（冷启动 ~16s 不可接受）。切换延迟须 <500ms。
- **`load_session` 后须等 `session_loaded` 再对话**：C++ 发 `load_session` → Python `graph.aupdate_state` 注入历史 → 回 `session_loaded` 确认。在收到 `session_loaded` 之前**禁止**发 `user_msg`（state 未重建完毕会丢历史）。`DAAgentBridge` 收到 `session_loaded` 发 `agentSessionLoaded(sessionId)` 信号，调用方（UI/Module）据此恢复输入框可用态。
- **`cleanupOldSessions` 入口加 `qMax` 防护**：`maxCount = qMax(1, maxCount); retentionDays = qMax(0, retentionDays);`（plan-06 边界）——ini 被手改为 0/负时钳到合法下限，避免「保留 0 个」误删全部自由会话。
- **`last_active` 精确匹配 projectPath**：空 filter 只返回自由会话（指针 projectPath 必须空），非空 filter 精确匹配工程路径——避免启动恢复把工程绑定会话当自由会话恢复（plan-05 MAJOR-4 回归保护，见 `testLastActive`）。
- **崩溃安全**：JSONL append-only + 每条即写 flush，崩溃后最多丢最后一两行；`parseLineTolerant` 跳过损坏行不整体丢弃。

### T16. 权限层（permission-layer）：判定前置、C++ 唯一执法、不落盘
- **权限门在 `executeTool` 前置、C++ 是唯一执法点**：`DAAgentBridge::executeTool` 先调 `DAAgentPermissionManager::decide(tool, params, safety)` 产出 Allow/Deny/Ask，再执行工具；Python 只是咨询方（产出 `tool_call.safety` 裁决），不执法。工具内部**不要**再实现路径/内容安全检查（三处 `isPathSafe` 已删除）。
- **判定由 Python 在发起 `tool_call` 之前完成**（`tool_node` 调 `permission_judge`），裁决附在 `tool_call.safety` 随消息下发。**禁止实现成"运行时 C++→Python 判定 RPC"**：run 期间 `_wait_for_result`（`agent_runner.py`）只认 `tool_result`/`stop`，其余消息类型记日志后**丢弃**——C++ 发判定请求会被丢弃、等响应挂起至超时。
- **硬 deny 全模式生效**：系统目录（`c:/windows/**` 等 4 条 `tool:"*"` deny 种子）在 yolo/auto/manual 任何模式、任何分级之前先行求值，加载时强制回填，设置页锁定不可删。
- **审批与判定均不落盘**：审批是 `executeTool` 前置门，不进会话 JSONL（`tool_call`/`tool_result` 正常持久化，审批只延迟 result）；`safety` 裁决也不持久化。会话记忆仅内存态（仅 `file_write`，`code_exec` 永不记忆），会话切换/进程退出/崩溃即清空。
- **模式与超时**：模式是 C++ 状态，切换即时生效（门即时消费）并经 `reconfigure` 同步 Python（仅用于决定是否花费判定成本）；gated_tools（file_write+code_exec）无论模式一律用 `tool_approval_timeout_sec` 长超时（默认 600s），与模式解耦。

---

## 十二、调试指南

| 现象 | 排查路径 |
|------|---------|
| 子进程启动后 UI 干等/被杀 | 看 `da_log.log` 是否有 "Agent 子进程启动后 X 毫秒内未就绪"；检查 Python 依赖是否装齐（langgraph/langchain-openai） |
| 静默崩溃 exitCode=62097 无 stderr | 几乎都是 T2 问题（booting 心跳缺失/超时被杀，TerminateProcess 不 flush stderr） |
| 中文乱码/协议解析失败 | stdout 编码（`reconfigure(encoding="utf-8")`）与 `\r\n`（T10） |
| 工具结果不显示在对话流 | 检查 `DAAgentInterface::agentToolResult` → `DAAgentDockWidget::onAgentToolResult` 的连接（`DAAppController::initialize()` 是否执行；接口信号→Dock 槽共 13 条，见 § 六） |
| 配置不保存 | `da_log.log` 搜 `[DAAgentSettings]`；确认字段变更连接了 `settingChanged()`（`src/APP/SettingPages/DAAgentSettingsWidget`，§ 九）；检查 `%APPDATA%\DAWorkBench\config\agent-config.ini` |
| 提问重复/卡在提问 | T8：question 是否在 ask_user_node 内误发；thread_id 是否一致（`agent_session_1`） |
| API key 相关 | 日志禁止打印明文；排查 `agent/llm_api_key` 是否为空（`api_key_enc_size=0`） |

**日志位置**：`%APPDATA%\DAWorkBench\log\da_log.log`（含 Agent stderr 转发）、`da_log.1.log`（上一次运行）。详见根 AGENTS.md「运行日志与调试」。

---

## 十三、WHERE TO LOOK（速查）

| 任务 | 位置 |
|------|------|
| 新增工具 | `plugins/DAAgentTools/tools/`（数据/文件工具继承瘦 `DAAgentToolBase`，图表工具继承 `DAAgentChartToolBase`）+ `DAAgentToolsPlugin::initialize()` 注册 |
| 改系统提示词 | 编辑 `src/DAAgent/system_prompt.md`（运行时由 `DAAgentModule::assembleSystemPrompt()` 读取，缺失回退内置默认）；插件注入走 `registerSystemPrompt` |
| 改聊天渲染 | `src/DAGui/Agent/resources/chat.js` / `chat.html` / `chat.css` |
| 改聊天 UI 结构 | `src/DAGui/Agent/DAAgentDockWidget.cpp` |
| 改 C++↔JS 桥 | `src/DAGui/Agent/DAAgentWebChannel.cpp`（注意 `toJsString` 转义与 `chatBridge` 注册名） |
| 改协议 | `DAAgentBridge.cpp`（C++ 侧）+ `agent_runner.py` `StdioProtocol`/`main()`（Python 侧）——**两端必须同步** |
| 改 agent 推理逻辑 | `agent_runner.py` `AgentRunner`（图构建/节点/路由） |
| LLM 配置 | `src/APP/SettingPages/DAAgentSettingsWidget.cpp`（设置页，经 `setAgentInterface`）+ `src/DAAgent/DAAgentModule.cpp` `getLLMConfig`/`setLLMConfig`（agent-config.ini） |
| 权限模式/规则/判官配置 | `src/APP/SettingPages/DAAgentPermissionSettingsWidget.cpp`（设置页，经 `get/setPermissionConfig`）+ `src/DAAgent/DAAgentPermissionManager.h/.cpp`（引擎：模式/分级/路径策略/会话记忆，`agent-permissions.json` + `agent-config.ini`） |
| 代码内容判定 | `src/PyScripts/DAWorkbench/agent/permission_judge.py`（静态规则+判官）+ `agent_runner.py` `tool_node`（`tool_call.safety` 生产），见铁律 T16 |
| Dock/Ribbon 集成 | `src/APP/DAAppDockingArea.cpp`（创建）/ `DAAppController.cpp`（connect 信号链，§ 六）/ `DAAppRibbonArea.cpp`（toggle action） |

---

## 十四、开发 Checklist

新增/修改 Agent 功能时逐项确认：

- [ ] 新类归属模块正确（工具→`plugins/DAAgentTools/`，UI→DAGui，不违反依赖方向；DAAgent 不依赖 DAGui）
- [ ] Python 改动已同步到运行时 `bin/PyScripts/` 并重启程序验证
- [ ] 协议改动 C++/Python 两端同步，消息 type 大小写一致
- [ ] 新工具：`getToolSpec` schema 完整（含 required）、`execute` 内部 try/catch、错误用 `errorResponse`、已在 `DAAgentToolsPlugin::initialize()` 中 `registerTool`（`registerBuiltinTools` 已删除）
- [ ] UI 字符串英文源 + `//cn:` 注释；`daCritical`/`daWarning` 已翻译
- [ ] 不打印 api_key 明文日志
- [ ] 符合根 AGENTS.md 铁律（stdout 协议、booting 心跳、单 Dock 实例、ToolMessage str 等）

---

## 十五、破坏性接口变更（plan-02/03/05，agent 上下文管理一期）

> 以下接口在 `DAAgentInterface` 新增为**纯虚函数**，会破坏二进制兼容：现有派生类（含插件）必须重编译。新增派生时须实现全部。

### 15.1 `DAAgentInterface` 新增 9 个纯虚函数

| # | 签名 | 用途 | 实现处 |
|---|------|------|--------|
| 1 | `virtual QString createSession() = 0` | 创建新会话（写 index + 空 jsonl），返回 UUID4 | `DAAgentModule::createSession` |
| 2 | `virtual bool switchSession(const QString& sessionId) = 0` | 切换会话：内部调 `m_bridge->sendLoadSession` 下发历史重建 state（不重启子进程） | `DAAgentModule::switchSession` |
| 3 | `virtual void deleteSession(const QString& sessionId) = 0` | 删除会话（删 jsonl + 移 index） | `DAAgentModule::deleteSession` |
| 4 | `virtual void renameSession(const QString& sessionId, const QString& title) = 0` | 重命名（更新 index title + updatedAt） | `DAAgentModule::renameSession` |
| 5 | `virtual QVariantList listSessions() const = 0` | 列出会话（供 UI 下拉，按 updatedAt 倒序） | `DAAgentModule::listSessions` |
| 6 | `virtual QString currentSessionId() const = 0` | 当前活跃会话 ID | `DAAgentModule::currentSessionId` |
| 7 | `virtual QHash<QString, QByteArray> exportActiveSessions() const = 0` | 导出指定会话的 JSONL 字节（供 DAAppProject 存工程 zip） | `DAAgentModule::exportActiveSessions` |
| 8 | `virtual void loadSessionsFromProject(const QHash<QString, QByteArray>& files, const QString& projectPath) = 0` | 导入工程内会话文件（写 sessions/ + 更新 index + 标记 projectPath） | `DAAgentModule::loadSessionsFromProject` |
| 9 | `virtual void setCurrentProjectPath(const QString& path) = 0` | 设置当前工程路径（供 DAAppProject L5 经 `core()->getAgentInterface()` 多态调用，影响 listSessions 过滤与 last_active 指针） | `DAAgentModule::setCurrentProjectPath` |

> **注 1**：`loadSession` 不在纯虚列表中——经 round2-contract 契约1 删除。`switchSession` 直接调 `DAAgentBridge::sendLoadSession`（Bridge 私有方法），不经 `DAAgentInterface`，避免把 Bridge 实现细节泄漏到接口。
>
> **注 2**：`setCurrentProjectPath` 经 round-3 提升为第 9 纯虚——DAAppProject（L5）需要经 `core()->getAgentInterface()` 多态调用以同步工程路径，不能走 Module 内部直接调用。

### 15.2 `DAAgentBridge` 新增 2 个公共信号（非破坏性，新增连接点）

| 信号 | 用途 | 典型连接方 |
|------|------|-----------|
| `void agentUsage(int inputTokens, int outputTokens, int totalTokens, const QString& source)` | LLM `usage_metadata` 权威 token 统计回传（来自 `usage` 协议消息或 `message_end` 附带 usage）；`source` 为 `agent`/`summary` 时累加到会话累计 token（`streaming_estimate` 为流式估算不累加），UI 显示 `tokens: 累计值 / 1048576` 占比条 + 点击展开分类（input/output/total/window/source）。**会话累计 token 跨轮次单调增长，上下文压缩不重置**——压缩后显示值不会骤降 | `DAAgentModule` 内部 lambda（补 `context_window` + 累加会话累计成员）→ 接口 `tokenUsageUpdated` →（AppController）→ Dock → chat.js |
| `void agentSessionLoaded(const QString& sessionId)` | `load_session` 后 Python 重建 state 完成的确认（`session_loaded` 协议消息）；调用方据此恢复输入框可用态，方可发下一轮 `user_msg`（见铁律 T15） | 转发到接口 `agentSessionLoaded` →（AppController）→ Dock 槽 |

**连接示例**（这两个 Bridge 信号已被 Module 消费/转发；UI 侧在 `DAAppController::initialize()` 连接口信号，插件可监听接口信号）：

```cpp
// UI 侧连接在 DAAppController::initialize()（决策 D3b，见 § 六）
connect(agent, &DAAgentInterface::tokenUsageUpdated, dock, &DAAgentDockWidget::onAgentUsage);
connect(agent, &DAAgentInterface::agentSessionLoaded, dock, &DAAgentDockWidget::onAgentSessionLoaded);
```

> 这两个信号是**新增连接点**，不改变既有信号签名，不破坏二进制兼容。重构后 Bridge 是 DAAgent 内部对象，外部不直接连接——改为监听 `DAAgentInterface` 上对应的转发信号。

### 15.3 多供应商多模型管理（6 个新纯虚 + 2 个新信号）

> 支持配置多个供应商（每个供应商含 base_url/api_key/多个模型），Dock/web 两级选择器可选不同模型。
> 模型为对象 `{id, context_window, max_output_tokens}`（默认 256K / 8192）。激活供应商+模型派生
> `agent/llm_base_url`/`llm_api_key`/`llm_model`/`agent/context_window`/`agent/max_output_tokens`
> （经 `getLLMConfig` 下发子进程 init，Python 端 `ChatOpenAI(max_tokens=max_output_tokens)`），切换模型
> 时经 `reconfigure` 消息热替换（不重启子进程、不丢 MemorySaver 会话状态，详见 §5.1）。
> `_build_graph` 闭包内 `llm_with_tools`/`compactor`/`token_estimator`/`truncator`
> 经 `self.*` call-time 读取（非局部值捕获），故 `reconfigure` 更新 `self.*` 后
> 无需重建图即可生效，下一轮节点执行自动用新模型。

| # | 签名 | 用途 | 实现处 |
|---|------|------|--------|
| 1 | `virtual QJsonArray getProviders() const = 0` | 取所有供应商（api_key 已解密明文）；旧 flat-key/字符串模型配置自动迁移为单 "Default" 供应商（模型对象化，默认 256K/8192） | `DAAgentModule::getProviders` |
| 2 | `virtual void setProviders(const QJsonArray& providers) = 0` | 存所有供应商（api_key 明文传入，内部 DPAPI 加密）；保存后 syncActiveConnection + emit 可用模型/激活变化 | `DAAgentModule::setProviders` |
| 3 | `virtual QString getActiveProvider() const = 0` | 当前激活供应商名 | `DAAgentModule::getActiveProvider` |
| 4 | `virtual QVariantList getAvailableModels() const = 0` | 所有可选模型（Dock/web 选择器用，不含 api_key）：每元素 `{provider,model,context_window,max_output_tokens}` | `DAAgentModule::getAvailableModels` |
| 5 | `virtual QString getActiveModel() const = 0` | 当前激活模型 id（= `agent/llm_model`） | `DAAgentModule::getActiveModel` |
| 6 | `virtual void setActiveModel(const QString& provider, const QString& model) = 0` | 设置激活供应商+模型：同步 base_url/api_key/model/context_window/max_output_tokens + emit activeModelChanged；子进程运行中则 `reconfigureAgent` 热替换（不重启子进程、不丢会话状态），未运行时仅写 ini 下次懒启动用新 config | `DAAgentModule::setActiveModel` |

**新增 2 个信号**（`DAAgentInterface`，AppController 连到 Dock）：

| 信号 | 用途 |
|------|------|
| `void availableModelsChanged(QVariantList models)` | 可用模型列表变化（setProviders / 设置页 apply），Dock 据此填充下拉 |
| `void activeModelChanged(const QString& provider, const QString& model)` | 激活模型变化，Dock 据此选中下拉项 + 刷新 "Model: <name>" 标签 |

**连接清单**（`DAAppController::initialize()`）：

```cpp
connect(agent, &DAAgentInterface::availableModelsChanged, dock, &DAAgentDockWidget::onAvailableModelsChanged);
connect(agent, &DAAgentInterface::activeModelChanged, dock, &DAAgentDockWidget::onActiveModelChanged);
connect(dock, &DAAgentDockWidget::activeModelChangeRequested, agent, &DAAgentInterface::setActiveModel);
```

**初始推送**：`DAAppController` 在接口↔Dock 信号链 connect 完成后调 `agentMod->pushModelSelection()`（DAAgentModule 非 interface 辅助方法），emit 两个信号填充 Dock 下拉；首次运行/旧配置时 `syncActiveConnection` 兜底取首个供应商为激活。

---

## 十六、会话持久化（plan-03/05/06）

### 16.1 设计要点

- **持久化主导方=C++ 主进程**（总纲 T1）：C++ 负责 JSONL 读写 / 会话列表 / 清理 / token 锚点存储；Python 子进程只负责推理与 state 重建。历史消息本就需在 UI 侧渲染，C++ 完全掌控历史格式，与 langgraph 解耦。
- **双存储**（总纲 D3）：未保存工程的自由会话存配置目录 `sessions/<id>.jsonl`；保存工程时活跃会话复制进 zip 的 `agent_sessions/<id>.jsonl`，配置目录原会话保留（两边独立）。
- **自动恢复**（总纲 D5）：启动程序自动恢复上次活跃自由会话；打开工程自动加载工程内上次活跃会话。

### 16.2 JSONL 记录格式（总纲 T6）

每行一条 JSON，append-only 追加（崩溃安全：每条即写 flush）：

```json
{"uuid":"...","parent_uuid":null,"session_id":"...","timestamp":"ISO8601",
 "type":"user|assistant|tool_result|usage",
 "message":{"role":"human|ai|tool","content":"...","tool_calls":[...]?,"tool_call_id":"..."?},
 "usage_metadata":{...}?}
```

- 一期 `type` 实际用 `user`/`assistant`/`tool_result`/`usage`；`question`/`answer` 复用 `tool_call`/`tool_result` 语义（ask_user 作为 assistant 的 tool_call）。
- `parent_uuid` 一期固定 `null`（二期 rewind 树用）。
- `readMessagesForLoad` 过滤 `usage`，只返回 user/assistant/tool_result 的 `message` 字段供 `load_session` 重建 state。
- **JSONL 不写 api_key 明文**（见铁律 T15）。

### 16.3 路径（总纲 T7，由 `DADir::getAppDataPath("sessions")` 决定）

| 项 | 路径 |
|----|------|
| 自由会话文件 | `<appData>/sessions/<sessionId>.jsonl` |
| 全局索引 | `<appData>/sessions/sessions_index.json`（`[{id,title,createdAt,updatedAt,messageCount,projectPath?}]`，原子写 tmp+rename） |
| 工程内会话 | `<project.zip>/agent_sessions/<sessionId>.jsonl` |
| 上次活跃会话指针 | `<appData>/sessions/last_active.json`（`{sessionId, projectPath?}`） |

Windows 展开为 `%APPDATA%/DA/DAWorkBench/DAWorkBench/sessions/`。

### 16.4 `load_session` 重建机制（总纲 T3）

切换会话**不重启子进程**（避免 ~16s 冷启动）：

1. C++ `DAAgentModule::switchSession(id)` → `m_sessionStore->readMessagesForLoad(id)` 取历史 messages 数组。
2. → `m_bridge->sendLoadSession(id, messages)` 下发 `load_session` 协议消息（stdin）。
3. Python 端 `graph.aupdate_state` 注入历史重建 state，回 `session_loaded` 确认。
4. C++ 收到 `session_loaded` → 发 `agentSessionLoaded(id)` 信号 → UI 恢复输入框，方可发下一轮 `user_msg`。

> 在收到 `session_loaded` 之前**禁止**发 `user_msg`（state 未重建完毕会丢历史，见铁律 T15）。

### 16.5 恢复时序

- **启动程序**：`DAAppController::initialize` 末尾调 `agentMod->cleanupSessions()`（清理超限会话）+ `restoreLastActiveSession()`（填充会话下拉但不自动恢复——始终以全新对话开始）。须在接口↔Dock 信号链 connect 完成后（Dock 就绪才能渲染，见 § 六）。用户可通过下拉或会话管理器手动切换到历史会话。
- **打开工程**：`DAAppProject` 加载任务解压 `agent_sessions/*.jsonl` → 主线程回调 → `agentMod->loadSessionsFromProject(files, projectPath)` + `setCurrentProjectPath(path)` → `restoreLastActiveSession()` 填充工程会话下拉但不自动恢复——始终以全新对话开始。
- **保存工程**：`DAAppProject::executeSave` 主线程先 `agentMod->exportActiveSessions()` 收集活跃会话字节 → `appendByteSaveTask` 写入 zip 的 `agent_sessions/`（子线程，不碰 UI）。
- **saveAs**：`setCurrentProjectPath` 同步更新新路径，`setSessionProjectPath` + `setLastActive` 更新 index 与指针的 projectPath（使打开新工程能恢复）。

### 16.6 单元测试

`src/tst/DAAgentSessionStoreTest/`（Qt Test，5 个 C++ 用例，不链接 Python）：

| 用例 | 覆盖 |
|------|------|
| `testAppendAndRead` | appendRecord + readMessagesForLoad（usage 被过滤，messageCount 只计对话消息） |
| `testParseTolerant` | JSONL 含损坏行/空行时 readMessagesForLoad 跳过坏行返回有效记录 |
| `testIndexAtomicWrite` | listSessions 倒序，renameSession 更新 index + title，索引文件为合法 JSON 数组 |
| `testCleanup` | 数量上限删最旧、时间上限删超期、skipSessionId 保护当前活跃、qMax 边界防护 |
| `testLastActive` | last_active 往返 + projectPath 精确匹配（plan-05 MAJOR-4 回归保护：空 filter 不返回工程绑定会话） |

> **不含 `testMessageRoundTrip`**：`message_to_json`/`json_to_message` 是 plan-01 在 Python 侧（`agent_runner.py`）定义的函数，`HumanMessage`/`AIMessage` 等是 langchain Python 类，不在 C++ `DAWorkbench::DAAgent` 库导出符号中。该序列化往返归 plan-01 的 Python 测试，见下方手测清单兜底。

隔离：`main()` 起手 `QStandardPaths::setTestModeEnabled(true)` 重定向 AppData 到临时目录，绝不污染真实 `%APPDATA%/DAWorkBench/sessions/`；每个用例 `init()` 清空 sessions 目录。

构建：`cmake -S . -B build -D DA_ENABLE_TESTING=ON` 后 `cmake --build build --target DAAgentSessionStoreTest --config Release`，运行 `./build/bin/DAAgentSessionStoreTest.exe -o result.txt`（Windows 上 Qt Test stdout 不可见，须用 `-o` 输出文件，见根 AGENTS.md COMMANDS）。

### 16.7 端到端手测清单

> 一期手测兜底（Python 序列化往返等无法在 C++ Qt Test 覆盖的场景）。逐项验证，全部应通过。

- [ ] **对话 → 重启 → 恢复**：发几轮对话（含工具调用）→ 关闭程序 → 重新启动 → 上次活跃自由会话历史完整恢复，可继续对话。
- [ ] **切换会话 → 历史重放 → 续聊**：新建会话 A 对话几轮 → 新建会话 B 对话几轮 → 下拉切回 A → 历史重放显示 A 的对话 → 发新消息续聊正常（state 已重建）。
- [ ] **保存工程 → zip 含 agent_sessions → 打开 → 恢复**：有活跃会话时保存工程 → 用解压工具打开 `.dwproj`（zip）确认含 `agent_sessions/*.jsonl` → 关闭程序 → 打开工程 → 工程内上次活跃会话恢复。
- [ ] **底部 token 占比随对话更新**：发消息后底部状态栏显示 `tokens: N / context_window`，点击展开看分类（system/tools/history/current）随对话增长。
- [ ] **超 20 个会话 → 启动清理最旧**：在 `%APPDATA%/DAWorkBench/DAWorkBench/sessions/` 手造 >20 个 `<id>.jsonl` + 索引项 → 启动程序 → 确认 `cleanupSessions` 删除最旧的超限会话（保留最近 20 个），当前活跃会话不被删。
- [ ] **超 30 天会话清理**：手造一个 updatedAt 早于 30 天的会话 → 启动 → 确认被清理。
- [ ] **Python 序列化往返手测**（一期手测兜底，二期转 pytest）：在 `bin/PyScripts/` 下手动构造 `HumanMessage`/`AIMessage`(含 `tool_calls`)/`ToolMessage`/`SystemMessage`/带 `additional_kwargs` 的 summary，调 `message_to_json` → `json_to_message` 比对往返一致（类型/content/tool_calls/tool_call_id/additional_kwargs 不丢）。该用例归 plan-01 Python 测试范围，此处仅手测兜底。
- [ ] **设置页配置生效**：设置页改 Max Sessions=5 / Session Retention Days=7 → 应用 → 关闭重开确认设置回显 → 造 >5 个会话启动清理保留 5 个；造 >7 天会话启动清理。

---

## 十七、WHERE TO LOOK（速查，更新）

> 原 §十三 速查表已含基础项，此处补会话持久化相关位置。

| 任务 | 位置 |
|------|------|
| 新增会话持久化逻辑 | `src/DAAgent/DAAgentSessionStore.h/.cpp`（非 QObject，PIMPL） |
| 改会话清理策略 | `DAAgentSessionStore::cleanupOldSessions`（入口已加 qMax 防护） |
| 改 last_active 过滤语义 | `DAAgentSessionStore::PrivateData::readLastActive`（精确匹配 projectPath） |
| 改 load_session 重建 | `DAAgentBridge::sendLoadSession`（C++）+ `agent_runner.py` `aupdate_state`（Python） |
| 改会话保留配置 | `src/APP/SettingPages/DAAgentSettingsWidget.cpp`（设置页 spin）+ `DAAgentModule::cleanupSessions`（读取） |
| 会话持久化单元测试 | `src/tst/DAAgentSessionStoreTest/main.cpp`（5 个 C++ 用例） |

