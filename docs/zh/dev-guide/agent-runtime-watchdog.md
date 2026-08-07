# Agent 运行期卡死问题诊断与修复待办

> 本文档记录 DAWorkBench agent 功能"运行到一半静默卡死"问题的诊断结论与待修复的结构性缺口，供后续实现参考。
>
> **范围说明**：本文档**不**包含已修复的 `DAPyDataFrame::query` ValueError（该问题已另行修复，见 `src/DAPyBindQt/pandas/DAPyDataFrame.cpp` 的 `query` 方法——根因是从 C++ 绑定裸调 `df.query` 致使 `pandas` 的 `sys._getframe(level)` 取不到调用者栈帧）。本文档只记录导致 agent **静默卡死、无日志、无恢复**的三个独立结构性缺口。

## 1. 问题现象

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

## 3. 根因：三个独立的结构性缺口

### 缺口 A — LLM 流式客户端无超时

**位置**：`src/PyScripts/DAWorkbench/agent/agent_runner.py:255-260`

```python
self.llm = ChatOpenAI(
    base_url=config["base_url"],
    api_key=config["api_key"],
    model=config["model"],
    streaming=True,
)
```

**问题**：构造 `ChatOpenAI` 时未传 `request_timeout` / `timeout` / `max_retries`，单次流式调用（`llm_with_tools.astream(...)`，见 `_stream_llm` 第 364 行）挂死时无客户端侧读超时兜底，挂多久都没人管。本次 5 分钟挂死即由此而来。

### 缺口 B — 运行期无看门狗（最关键）

**位置**：`src/DAAgent/DAAgentBridge.cpp`

当前 `m_readyTimer`（100-112 行）是**仅在启动期生效**的心跳看门狗：收到第一条 `ready` 消息后即被 `stop` + `deleteLater` + 置空：

```cpp
// DAAgentBridge.cpp:268-274
} else if (type == "ready") {
    // 收到 ready 消息——停止 ready 超时计时器
    if (m_readyTimer) {
        m_readyTimer->stop();
        m_readyTimer->deleteLater();
        m_readyTimer = nullptr;   // ← 此后运行期再无任何计时器
    }
    emit agentReady(msg["model"].toString());
}
```

之后 `handleJsonLine` 对 `token` / `message_end` / `tool_call` / `tool_result` / `done` 等所有消息类型都**不再碰任何计时器**（276 行起）。`m_stopTimer`（154-163 行，默认 5000ms）**只在用户主动 `requestStop()` 时用**，不是自动看门狗。

**后果**：agent 就绪后，子进程长时间无输出（流式挂死、死循环、死锁）时**不会被检测、不会被 kill**，UI 干等无响应。`onProcessFinished`（366-378 行）虽有 exitCode 记录、406 行有 `emit agentError`，但**只在进程真正退出时才触发**——卡死场景进程根本不退出，所以这条诊断路径不会生效，这正是"静默卡死、无任何退出日志"的结构性原因。

### 缺口 C — 无 recursion_limit

**位置**：`src/PyScripts/DAWorkbench/agent/agent_runner.py:594-597` 与 `616-619`

```python
# run()
async for _event in self.graph.astream(
    {"messages": [HumanMessage(user_message)]},
    config=self.thread_config
):
    pass
```

**问题**：`graph.astream` 未传 `recursion_limit`，靠 LangGraph 默认 25 步。当某个工具持续返回空 / 错误、LLM 反复重试同一工具时，无显式上限，最长 25 个 super-step 后才抛 `GraphRecursionError`——但每步含一次完整流式调用，一次流式挂死就够卡住，recursion_limit 兜不住流式挂死本身，但能限制死循环规模、避免无限重试。

## 4. 当前实现摘录（供实现者参照）

### agent_runner.py 关键路径

- **流式调用** `_stream_llm`（349-373 行）：先**完整消费流**累积出完整 AIMessage，之后 `agent_node`（448 行）才读 `tool_calls` 交 `tool_node` 执行。即工具失败发生在流式结束之后，**无法用 `GeneratorExit` 打断在途流**——早期诊断中"工具失败打断流式 → SDK 重试"的因果链在本架构下不成立。`GeneratorExit()` 是那次流式响应体读取本身中途失败（服务端断连 / 客户端生成器被关闭）的记录。
- **工具 RPC** `_rpc_call`（301-312 行）：有 `asyncio.wait_for(..., timeout=60)` 兜底，工具 60s 无结果返回 `{"error": ...}`。
- **工具失败处理** `tool_node`（463-479 行）：结果 `json.dumps` 成 `ToolMessage` 喂回 LLM，**不中断 agent**——LLM 通常会再次调用同一坏工具，形成循环。
- **溢出恢复** `agent_node`（423-446 行）：仅处理 `is_context_overflow_error`，与流式中断无关。

### DAAgentBridge.cpp 关键路径

- `startAgent`（55-113 行）：QProcess 启动 + 连接信号 + `m_readyTimer` 启动期看门狗。
- `handleJsonLine`（258 行起）：`booting` 重置 readyTimer；`ready` 销毁 readyTimer；之后各消息类型均无计时器。
- `stopAgent` / `requestStop`（115-166 行）：主动停止，`m_stopTimer` 5s 后 kill。
- `onProcessFinished`（366-378 行）：`daDebug` 记 exitCode / exitStatus；406 行异常退出 `emit agentError`；415 行 `emit agentBusy(false)` 恢复 UI。
- `onReadyReadStandardError`（418-428 行）：423 行 `daDebug << "Agent stderr:"` 转发全部 stderr 到日志；425-427 行仅当 chunk 含 `Traceback` / `Error` 时才 `emit agentError` 转发到 UI。

## 5. 建议修复方向（供参考，非死方案）

### 缺口 A

给 `ChatOpenAI` 加显式超时与重试上限：

```python
self.llm = ChatOpenAI(
    base_url=config["base_url"],
    api_key=config["api_key"],
    model=config["model"],
    streaming=True,
    request_timeout=120,   # 或从 config 读取，如 config.get("request_timeout", 120)
    max_retries=2,
)
```

> 注：`request_timeout` 对流式响应"已开始但中途挂死"是否生效，取决于 langchain-openai / httpx 版本如何处理流式读超时。建议实现时验证：mock 一个建立连接后不继续推 body 的网关，确认 `request_timeout` 能在 120s 后抛出而非无限挂；若 SDK 流式读超时不完善，需依赖缺口 B 的看门狗兜底。

### 缺口 B（最关键）

在 `DAAgentBridge` 加运行期心跳看门狗，命名如 `m_runtimeTimer`：

- 在 `handleJsonLine` 收到 `token` / `message_end` / `tool_call` / `tool_result` 等活动消息时重置该单次 `QTimer`；超时则 kill 子进程 + `emit agentError` + 落日志。
- 可复用 `m_readyTimer` 的 config 读取模式（`DAAgentModule.cpp` 读 `agent/ready_timeout_sec`），新增 `agent/runtime_timeout_sec`（默认如 180s）。
- 确保看门狗触发 kill 后，`onProcessFinished` 被调用 → 记 exitCode → `emit agentError` → `emit agentBusy(false)` 恢复 UI 可输入。
- 用户主动 `requestStop()` / 收到 `done` 时停止该计时器，避免误杀。
- 流式输出 token 期间持续重置计时器（每个 token 重置，或按时间窗口重置），避免正常长回复被误杀。

### 缺口 C

给 `astream` 传显式 `recursion_limit`（可从 config 读取），或在 `tool_node` 加"同一工具连续失败 N 次则中断 / 上报"的计数。

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
