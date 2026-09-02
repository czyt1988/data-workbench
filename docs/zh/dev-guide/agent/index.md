# Agent 模块概述

DAWorkbench 的 AI Agent 助手模块为平台提供内嵌 LLM 聊天与数据分析工具调用能力。Agent 能够理解用户的自然语言指令，自动调用平台的数据查询、绘图、文件操作等工具完成任务，并通过流式输出实时展示推理过程。

---

## 主要功能特性

- ✅ **双进程架构**：C++ 主进程管理工具执行与会话持久化，Python 子进程运行 LLM 推理，两者通过 stdin/stdout 管道以 JSON Lines 协议通信，互不影响
- ✅ **流式输出**：LLM 生成的 token 实时流式推送到聊天界面，用户无需等待完整回复即可看到推理过程
- ✅ **工具调用**：内置 23 个 C++ 工具（`DAAgentTools` 插件 21 个：数据 5 + 绘图 11 + 文件/报告 3 + 代码执行 2；`DAPaperAgent` 插件 2 个文献工具）+ 平台注入的 `ask_user`（人机交互提问）与 `dispatch_subagents`（子 Agent 委派），覆盖数据分析全流程；支持插件扩展自定义工具。完整清单见[工具与内置 Agent 总览](./available-tools.md)
- ✅ **多供应商多模型**：支持多个 LLM 供应商（每供应商含多模型）的 CRUD 管理，Dock 下拉切换激活供应商+模型；运行期可热替换 LLM 配置（`reconfigure` 消息）而不重启子进程、不丢 MemorySaver 会话状态
- ✅ **提示词库管理**：内置 agent 提示词的注入 / 按标题执行 / CRUD，由 `DAAgentManager` + `DAAgentPromptOps` + `DAAgentPrompt` 承载，UI 经 `DAAgentManagerDialog` / `DAAgentEditorDialog` 维护
- ✅ **上下文管理**：自动检测上下文窗口占用，超过阈值时智能压缩历史对话（保留头部任务描述 + 尾部近期消息 + 中间摘要），避免上下文溢出
- ✅ **人机交互提问**：Agent 在需要额外信息时主动向用户提问，支持选项选择与自由输入，实现 human-in-the-loop 协作
- ✅ **会话持久化**：对话历史以 JSONL 格式持久化到磁盘，支持多会话切换、工程内会话导出/导入、自动清理过期会话；token 用量按会话累计持久化，切换会话时从持久化记录重放累计统计
- ✅ **预启动与三态 UI**：受 `auto_prestart`（默认 `true`）控制，程序启动时即预热子进程；`agentStarting` 信号驱动"启动中"过渡态，`agentReady` 进入"就绪"态
- ✅ **崩溃恢复**：子进程异常退出时自动重启（最多 3 次），恢复会话状态并重发最后一条用户消息
- ✅ **重试与退避**：网络波动、服务端 5xx 等可重试错误自动指数退避重试，支持用户随时中断
- ✅ **看门狗与防死循环**：240s 无活动看门狗 + `recursion_limit`（默认 150）+ 重复工具调用硬终止（连续 3 次相同完整签名），多重兜底防止 agent 卡死或陷入死循环
- ✅ **API Key 加密存储**：LLM API Key 通过 Windows DPAPI 加密后存储，设置页面只处理明文

---

## 核心概念

### 双进程模型

Agent 采用 C++ + Python 双进程架构。C++ 主进程持有工具注册表、会话持久化层和 QProcess 管理器；Python 子进程运行 LangGraph 状态机驱动 LLM 推理。工具的 schema（OpenAI function schema 格式）在启动时一次性下发给 Python 侧，但工具的实际执行始终在 C++ 主进程中完成——LLM 只看到工具的接口描述，不直接执行工具代码。

### 预启动 / 懒启动

子进程默认在程序启动时即预热创建（`prestartAgent()`，受 `auto_prestart` 配置开关控制，默认 `true`），并在启动前发射 `agentStarting` 信号令 UI 进入"启动中"过渡态，`agentReady` 后转为"就绪"。若关闭预启动，则回退到懒启动：在用户首次发送消息时才通过 `startAgentInternal()` 创建子进程。启动时 C++ 侧一次性下发 `init` 消息，包含 LLM 配置、工具规格和系统提示词。由于 LangChain 冷启动导入约需 16 秒，Python 侧在导入前先发送 `booting` 心跳以重置 C++ 侧的就绪超时计时器。

### JSON Lines 协议

两个进程通过 QProcess 的匿名管道以 JSON Lines 协议通信——每行一条 JSON 消息，stdout 专用于协议通信，所有日志输出走 stderr。协议定义了 7 种 C++→Python 消息（`init`、`stop`、`user_msg`、`tool_result`、`user_answer`、`load_session`、`reconfigure`）和 11 种 Python→C++ 消息，覆盖用户消息、工具调用、流式 token、会话切换、模型热替换等全部交互场景。

---

## 架构总览

```mermaid
graph TB
    subgraph "C++ 主进程"
        UI["DAAgentDockWidget<br/>(聊天 UI, DAGui 模块)"]
        IF["DAAgentInterface<br/>(公共接口, 28 信号)"]
        MOD["DAAgentModule<br/>(接口实现: 工具注册/会话/配置)"]
        BRIDGE["DAAgentBridge<br/>(QProcess 管理/协议解析)"]
        STORE["DAAgentSessionStore<br/>(JSONL 持久化)"]
        TOOLS["DAAgentToolsPlugin<br/>(21 个内置工具)"]
    end

    subgraph "Python 子进程"
        PROTO["StdioProtocol<br/>(stdin/stdout 收发)"]
        RUNNER["AgentRunner<br/>(LangGraph 状态机)"]
        CTX["ContextCompactor<br/>(上下文压缩)"]
        RETRY["RetryWrapper<br/>(重试/退避)"]
    end

    UI -->|"8 信号→方法"| IF
    IF -->|"18 信号→槽"| UI
    IF --> MOD
    MOD --> BRIDGE
    MOD --> STORE
    TOOLS -->|"registerTool"| IF

    BRIDGE -->|"stdin: JSON Lines"| PROTO
    PROTO -->|"stdout: JSON Lines"| BRIDGE
    PROTO --> RUNNER
    RUNNER --> CTX
    RUNNER --> RETRY
    BRIDGE -->|"tool_call RPC"| TOOLS
```

> 图中 DAGui 与 DAAgent 互不依赖，两者的信号↔槽由 APP 层 `DAAppController` 直接 connect。

---

## 模块组成

### 源码目录

| 目录 | 模块 | 职责 |
|------|------|------|
| `src/DAAgent/` | DAAgent (L4 接口层) | Agent 框架库：接口定义、子进程管理、会话持久化、工具基类 |
| `src/DAGui/Agent/` | DAGui (L3 界面层) | 聊天 UI：DockWidget、WebChannel、前端资源 |
| `src/APP/SettingPages/` | APP (L5 应用层) | LLM 设置页 |
| `src/PyScripts/DAWorkbench/agent/` | Python 脚本 | 子进程入口：LLM 推理、上下文管理、重试逻辑 |
| `plugins/DAAgentTools/` | 插件 | 21 个内置工具实现（数据/绘图/文件/代码执行） |
| `plugins/DAPaperAgent/` | 插件 | 论文撰写 agent：内置「论文撰写助手」提示词 + 2 个文献工具 |

### 关键类

| 类名 | 模块 | 职责 |
|------|------|------|
| `DAAgentInterface` | DAAgent | 公共接口：28 个信号 + 工具/提示词注册 + 多供应商多模型管理（CRUD、激活切换、热替换）+ 提示词库 API（`runAgent`/`registerBuiltinAgent`/`agentPromptOps`）+ 会话管理 |
| `DAAgentModule` | DAAgent | 接口实现：工具注册表、系统提示词组装、预启动/懒启动、配置读写、DPAPI 加密、token 会话累计统计 |
| `DAAgentBridge` | DAAgent | QProcess 生命周期管理、JSON Lines 协议解析、工具执行、`reconfigureAgent` 热替换、崩溃恢复、无活动看门狗 |
| `DAAgentSessionStore` | DAAgent | JSONL 会话持久化：读写、索引、清理、标题、导出导入 |
| `DAAgentManager` | DAAgent | 提示词库管理：内置 agent 注入、按标题执行、CRUD（实现 `DAAgentPromptOps`） |
| `DAAgentPromptOps` / `DAAgentPrompt` | DAAgent | 提示词库操作回调接口 / 单条提示词模型 |
| `DAAbstractAgentTool` | DAAgent | 工具抽象基类（纯虚）：`getToolSpec` / `execute` / `getOwnerModule` |
| `DAAgentToolBase` | DAAgent | 瘦工具基类：数据访问 + 响应方法（`DAAgent_API` 导出供插件继承） |
| `DAAgentDockWidget` | DAGui | 聊天面板（QWebEngine + 输入框 + 会话下拉 + 模型下拉） |
| `DAAgentWebChannel` | DAGui | C++↔JS 桥（QWebChannel） |
| `DAAgentSettingsWidget` | APP | LLM 设置页（经 `setAgentInterface` 注入接口） |
| `DAAgentManagerDialog` / `DAAgentEditorDialog` | APP | 提示词库管理对话框 / 提示词编辑对话框 |
| `DAMarkdownView` | DAGui | 通用 Markdown 渲染器（`src/DAGui/MarkdownView/`），用于报告查看等 |
| `DAAgentToolsPlugin` | 插件 | 注册 21 个内置工具 + `figure_reference` 系统提示词 |
| `DAAgentChartToolBase` | 插件 | 图表工具基类（继承瘦 `DAAgentToolBase`，提供图表访问方法） |
| `StdioProtocol` | Python | JSON Lines 协议收发（后台线程读 stdin → asyncio.Queue） |
| `AgentRunner` | Python | LangGraph 状态机：compact → agent → {ask_user \| tools \| END}；含 `reconfigure()` 热替换 |
| `ContextCompactor` | Python | 上下文压缩：head/tail 保留 + 中间 LLM 摘要 + 熔断 |
| `TokenEstimator` | Python | Token 估算：tiktoken 优先，char-based 兜底 |

---

## 文档索引

| 文档 | 内容 |
|------|------|
| [架构设计](./architecture.md) | 双进程模型、模块职责、信号链、核心设计决策 |
| [通信协议](./protocol.md) | JSON Lines 协议规范、消息类型、启动握手、工具 RPC |
| [上下文管理](./context-management.md) | Token 估算、上下文压缩、工具结果截断、溢出恢复 |
| [崩溃恢复与重连](./crash-recovery.md) | 进程生命周期、自动重启、会话恢复、重试退避 |
| [工具与内置 Agent 总览](./available-tools.md) | 全部 25 个工具的参数与作用、内置 Agent / 子 Agent、权限分级 |
| [工具开发指南](./tool-development.md) | 工具基类、注册机制、内置工具、自定义工具开发 |
| [通过插件注册内置 Agent](./register-builtin-agent-via-plugin.md) | 插件注入 agent 提示词/领域工具/系统提示词的完整模式（DAPaperAgent 参考实现） |
| [会话持久化](./session-management.md) | JSONL 格式、会话索引、导出导入、自动清理 |

---

## 参考资料

- `src/DAAgent/AGENTS.md` — DAAgent 模块 AI 开发必读指南（700+ 行，含铁律 T1~T17）
- `DAAgentDecouplePlan.md` — DAAgent 解耦重构方案（从 DAGui 依赖到纯框架库的演进）
- `src/DAAgent/system_prompt.md` — 系统提示词（外部可编辑 markdown）
- `src/PyScripts/DAWorkbench/agent/agent_runner.py` — Python 子进程入口脚本
- `docs/zh/dev-guide/agent/agent-runtime-watchdog.md` — Agent 运行时看门狗诊断文档
