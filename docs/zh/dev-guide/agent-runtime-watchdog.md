# Agent 运行期卡死问题诊断与修复

!!! success "STATUS: 三个结构性缺口均已修复"
    本文档原记录的三个导致 agent 静默卡死的结构性缺口（缺口 A / B / C）**已全部修复**，并补充了重复工具调用硬终止守卫。当前运行时看门狗的权威规范已迁至 [崩溃恢复与重连](agent/crash-recovery.md)，本页保留为历史诊断记录与修复证据索引。

    - **缺口 A — LLM 流式客户端无超时**：已修复（commit `6a7a31e`）
    - **缺口 B — 运行期无看门狗**：已修复（commit `6a7a31e`，暂停于用户回答见 `5c690cc`）
    - **缺口 C — 无 recursion_limit**：已修复（commit `644fe5c`）
    - **补充守卫 — 模型热替换不重启**（commit `bae7567`）、**token 统计会话累计**（commit `44e11eb`）

> **范围说明**：本文档**不**包含已修复的 `DAPyDataFrame::query` ValueError（该问题已另行修复，见 `src/DAPyBindQt/pandas/DAPyDataFrame.cpp` 的 `query` 方法——根因是从 C++ 绑定裸调 `df.query` 致使 `pandas` 的 `sys._getframe(level)` 取不到调用者栈帧）。本文档只记录曾导致 agent **静默卡死、无日志、无恢复**的三个独立结构性缺口及其修复现状。

## 1. 问题现象（历史）

agent 对话运行到一半停止响应：

- **UI 表现**：agent 正在"思考/输出"，但长时间无新 token、无结束、无错误提示，无限转圈。
- **日志表现**：`da_log.log` 最后一行是子进程 stderr 转发的 `receive_response_body.failed exception=GeneratorExit()`，之后**再无任何**进程退出 / 崩溃 / 超时 / 看门狗记录——agent 子进程既没正常退出也没被 kill，静默挂住。

## 2. 本次复现的日志时间线（2026-08-07）

| 时间 | 事件 | 日志证据 |
|------|------|---------|
| 16:38:59 | agent 启动，model=`deepseek-v4-flash`，base_url=`http://10.2.25.136:4000` | `da_log.log:3892` `[DAAgentBridge.cpp:81]` |
| 16:39:49–16:40:51 | 数据查询工具反复抛 `ValueError: call stack is not deep enough`（约 12 次） | `da_log.log:3913…4069` `[critical] [DAPyDataFrame.cpp:464]`（**已修复**） |
| 16:40:53 | 最后一次 LLM 请求返回 `200 OK`，开始流式读响应体 | `da_log.log` 尾部 `receive_response_body.started` |
| 16:40:53→16:45:43 | 流式响应**挂起约 5 分钟**，期间无任何日志 | — |
| 16:45:43.938 | 响应流以 `GeneratorExit()` 异常结束，`da_log.log` 最后一行 | `da_log.log:4087` `[DAAgentBridge.cpp:423] Agent stderr: "...receive_response_body.failed exception=GeneratorExit()"` |
| 之后 | agent 静默卡住，无退出 / 崩溃 / 超时 / 看门狗记录 | — |

> 关键链路：工具报错让 LLM 每轮重试同一查询 → 每轮一次流式调用 → 其中一次流式响应挂死 → 没有任何兜底，agent 永久卡住。

## 3. 根因与修复现状：三个独立的结构性缺口

### 缺口 A — LLM 流式客户端无超时（已修复）

**修复**：`agent_runner.py` 构造 `ChatOpenAI` 时显式传入 `timeout` 与 `max_retries=0`（禁用 openai-python 内置重试，由 wrapper 控制）：

```python
# agent_runner.py — AgentRunner.__init__ 构造 LLM
self.llm = ChatOpenAI(
    base_url=config["base_url"],
    api_key=config["api_key"],
    model=config["model"],
    streaming=True,
    max_tokens=config.get("max_output_tokens", 8192),
    max_retries=0,                                        # 禁用 openai-python 内置重试，由 wrapper 控制
    timeout=config.get("request_timeout_sec", 120),       # HTTP 请求超时（连接+首字节）
)
```

热替换路径（`AgentRunner.reconfigure()`）构造新 `ChatOpenAI` 时同样设置 `max_retries=0` + `timeout=config.get("request_timeout_sec", 120)`，确保切换模型后不回退到无超时状态。配置键为 `agent/llm_request_timeout_sec`（默认 120 秒）与 `agent/llm_max_retries`（默认 7，由 `RetryWrapper` 而非 SDK 消费）。

> 修复 commit：`6a7a31e`。

### 缺口 B — 运行期无看门狗（已修复，最关键）

**修复**：`DAAgentBridge` 引入运行期无活动看门狗 `mInactivityTimer`（`QTimer`，single-shot，默认 240000ms = 4 分钟），在 `DAAgentBridge` 构造时创建并连接到 `onInactivityTimeout`：

```cpp
// DAAgentBridge.cpp — 构造函数
d->mInactivityTimer = new QTimer(this);
d->mInactivityTimer->setSingleShot(true);
connect(d->mInactivityTimer, &QTimer::timeout, this, &DAAgentBridge::onInactivityTimeout);
```

**启动时机**：收到 `user_msg`（`sendMessage` 后）、`tool_call`（开始工具执行前的活动信号）、`sendToolResult` 后（工具执行完 Python 会继续工作）等"对话进行中"的活动节点启动。

**停止时机**：收到 `done` / `error` / `question`（等待用户回答）/ `stop`（用户主动停止）/ 进程退出时停止，避免误杀。

**`ToolExecGuard` 暂停**：工具执行期间用 RAII 守卫 `ToolExecGuard` 暂停看门狗（构造时 `mToolExecuting=true` + `stop()`，析构时恢复），覆盖所有 return 路径——因为工具执行耗时由 C++ 侧掌控，不应计入"子进程无活动"。

**超时处理**：`onInactivityTimeout` 先 `emit agentError(..., "timeout", "")` 让用户更快看到超时提示，再 `requestStop()` 让 Python 优雅退出，最终经 `onProcessFinished` 恢复 `agentBusy(false)`。

**重要细节（per-token 不重置）**：token 流处理分支（`type == "token"`）**不**重置该计时器——即 240 秒窗口覆盖的是一次完整流式响应（从 `user_msg`/`tool_call` 到 `message_end`/`done`），而非每个 token 重置。这意味着一次正常的长回复若超过 240 秒仍会触发看门狗。如需更细粒度的流式活动感知，可在 token 分支按时间窗口重置，但当前实现选择"整轮覆盖"以简化语义。

> 修复 commit：`6a7a31e`（看门狗主体）、`5c690cc`（暂停于等待用户回答期间）。

### 缺口 C — 无 recursion_limit（已修复）

**修复**：`AgentRunner.__init__` 从 config 读取 `recursion_limit`（默认 150），注入 `thread_config["recursion_limit"]`：

```python
# agent_runner.py
self._recursion_limit = config.get("recursion_limit", 150)
self.thread_config = {
    "configurable": {"thread_id": "agent_session_1"},
    "recursion_limit": self._recursion_limit,
}
```

`GraphRecursionError` 在 `main()` 的 `run()` / `resume()` 调用处被捕获，报为 `error_type="recursion_limit"` 并发 `done` 结束本轮：

```python
except GraphRecursionError:
    logger.warning("GraphRecursionError: agent reached recursion limit (%d)", runner._recursion_limit)
    await stdio.send_error(
        "Agent reached maximum reasoning iterations (possible infinite loop). "
        "Try shortening the conversation history or starting a new session.",
        error_type="recursion_limit",
    )
    await stdio.send_done()
```

配置键为 `agent/recursion_limit`（默认 150 步，约支持 50 轮工具调用），用户可在设置页调整。

> 修复 commit：`644fe5c`。

### 补充守卫 — 重复工具调用硬终止

为在撞到 `recursion_limit` 之前更优雅地终止 LLM 死循环，`AgentRunner` 额外实现了重复工具调用检测：

- **软引导**：`tool_node` 用 `_executed_call_sigs`（`deque(maxlen=8)`）记录本轮已执行的工具调用签名（`name`, `args_canonical`），重复时返回引导性 `ToolMessage` 而非重复执行（不同参数不受影响）。
- **硬终止**：`agent_node` 跟踪 `_last_full_sig` / `_full_sig_repeat_count`，连续 3 次相同完整 `tool_calls` 签名（阈值 `_repeat_terminate_threshold=3`）则强制剥离 `tool_calls` 并以最终回复结束（router → END）。

详见 [架构设计 - LangGraph 状态机](agent/architecture.md#langgraph-状态机) 与 [崩溃恢复 - 死循环防护](agent/crash-recovery.md#死循环防护)。

### 补充能力 — 模型热替换（不重启子进程）

切换 LLM 配置不再需要杀子进程重启。`DAAgentBridge::reconfigureAgent()` 下发 `reconfigure` 消息，Python 端 `AgentRunner.reconfigure()` 在两轮之间热替换 `ChatOpenAI` 实例，不丢 `MemorySaver` 会话状态，作为崩溃恢复路径之外的容错替代。详见 [通信协议 - reconfigure](agent/protocol.md#reconfigure--模型热替换)。

> 修复 commit：`bae7567`。

## 4. 当前实现摘录

!!! tip "权威规范已迁出"
    运行时看门狗、崩溃恢复、重试退避的**当前权威规范**位于 [崩溃恢复与重连](agent/crash-recovery.md)。以下仅作历史诊断与代码定位参考。注意 `agent_runner.py` 已增至 1236 行、`DAAgentBridge.cpp` 行号亦已变化，下述行号为诊断当时值，定位时请以函数名为准。

### agent_runner.py 关键路径（函数名定位）

- **流式调用** `_stream_llm`：先**完整消费流**累积出完整 `AIMessage`，之后 `agent_node` 才读 `tool_calls` 交 `tool_node` 执行。即工具失败发生在流式结束之后，**无法用 `GeneratorExit` 打断在途流**——早期诊断中"工具失败打断流式 → SDK 重试"的因果链在本架构下不成立。`GeneratorExit()` 是那次流式响应体读取本身中途失败（服务端断连 / 客户端生成器被关闭）的记录。
- **工具 RPC** `_rpc_call`：有 `asyncio.wait_for(..., timeout=60)` 兜底，工具 60s 无结果返回 `{"error": ...}`。
- **工具失败处理** `tool_node`：结果 `json.dumps` 成 `ToolMessage` 喂回 LLM；配合软引导 / 硬终止避免无限重试同一坏工具。
- **溢出恢复** `agent_node`：仅处理 `is_context_overflow_error`，与流式中断无关。
- **热替换** `AgentRunner.reconfigure()`：构造新 `ChatOpenAI`（同样设 `max_retries=0` + `timeout`），成功后才更新 `self.*`，失败发 `error` 不破坏旧 LLM；不发 `done`，仅发 `ready` 确认。

### DAAgentBridge.cpp 关键路径（函数名定位）

- `startAgent`：QProcess 启动 + 连接信号 + `m_readyTimer` 启动期看门狗 + `emit agentStarting()`（UI"启动中"过渡态）。
- `reconfigureAgent`：下发 `reconfigure` 消息热替换 LLM（不重启子进程）。
- `handleJsonLine`：`booting` 重置 readyTimer；`ready` 销毁 readyTimer；`user_msg`/`tool_call`/`sendToolResult` 启动 `mInactivityTimer`；`done`/`error`/`question`/`stop` 停止之。
- `executeTool` + `ToolExecGuard`：RAII 暂停 / 恢复 `mInactivityTimer`，覆盖工具执行全程。
- `onInactivityTimeout`：先 `emit agentError("timeout")` 再 `requestStop()`，经 `onProcessFinished` 恢复 `agentBusy(false)`。
- `stopAgent` / `requestStop`：主动停止，`m_stopTimer` 5s 后 kill。
- `onProcessFinished`：记录 exitCode / exitStatus；异常退出 `emit agentError`；`emit agentBusy(false)` 恢复 UI。
- `onReadyReadStandardError`：转发全部 stderr 到日志；仅当 chunk 含 `Traceback` / `Error` 时才 `emit agentError` 转发到 UI。

## 5. 建议修复方向（历史，已落地）

以下为诊断当时提出的修复方向，现已全部落地（见第 3 节修复现状），保留以记录设计取舍。

### 缺口 A

给 `ChatOpenAI` 加显式超时与重试上限。**已实现**：`timeout=config.get("request_timeout_sec", 120)` + `max_retries=0`（重试交由 `RetryWrapper`）。

> 注：`request_timeout` 对流式响应"已开始但中途挂死"是否生效，取决于 langchain-openai / httpx 版本如何处理流式读超时。因此缺口 B 的看门狗作为最终兜底仍然必要——即便 SDK 流式读超时不完善，240s 看门狗也能 kill 子进程恢复。

### 缺口 B（最关键）

在 `DAAgentBridge` 加运行期心跳看门狗。**已实现**为 `mInactivityTimer`（240s single-shot），启动 / 停止 / `ToolExecGuard` 暂停语义见第 3 节。

### 缺口 C

给图传显式 `recursion_limit`，并在 `tool_node` 加重复调用检测。**已实现**：`recursion_limit=150`（可配置）+ 软引导 + 硬终止（阈值 3）。

## 6. 验证方法

修复后人为制造挂死场景：

1. **mock 网关不返回 body**：把 `base_url` 指向一个建立 TCP 连接后不发 HTTP body 的 mock 服务，触发缺口 A / B。
2. **确认**：`da_log.log` 出现看门狗触发的 kill + exitCode + `Agent ...` 错误日志；UI 给出可读错误提示而非无限转圈；`agentBusy(false)` 恢复输入。
3. **回归**：正常 agent 对话不受影响（长回复不被误杀、正常工具调用不被中断）。

## 7. 附：本次诊断日志证据索引

| 内容 | 位置 |
|------|------|
| agent 启动 | `da_log.log:3892` `[DAAgentBridge.cpp:81]` |
| 工具 ValueError（12 次，首 / 末） | `da_log.log:3913` / `4069` `[critical] [DAPyDataFrame.cpp:464]` |
| 流式中断最后一行 | `da_log.log:4087` `[DAAgentBridge.cpp:423] GeneratorExit()` |
| 数据加载收尾 | `da_pyscript.log:21598` `add_data: total 237.9ms` |

> 日志路径：`%APPDATA%\DAWorkBench\log\da_log.log`、`da_pyscript.log`（Windows）。详见项目 AGENTS.md「运行日志与调试」章节。
