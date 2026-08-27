"""Tests for subagent_orchestrator + RPC 多路复用分发器 + 主图回归（计划 B §7.1）。

约定镜像 test_permission_judge.py：被测目录加入 sys.path 后按**运行时同款
flat 模块名**导入（agent 子进程内 `import agent_runner` / `import
subagent_orchestrator` 即 flat 导入），保证测试与生产解析到同一模块实例。
真实 langchain/langgraph 栈驱动真实图执行；LLM 以脚本化 FakeLLM mock、
stdio 以 FakeStdio / ScriptedStdio mock，全程离线无 C++。

覆盖：
- 编排器：定义解析、白名单求交（未知工具剔除、空白名单）、schema 注入/移除、
  批上限拒绝、并发限流（Semaphore）、ok/error/timeout/stopped 状态映射、
  停止级联（stop 已 set 时未开始任务直接 stopped）、summary/总结果截断、
  usage 聚合、系统提示词组装（figure_reference / 权限约定）
- 分发器：乱序到达的 tool_result 正确路由、迟到/错配结果记日志忽略、
  等待中收到 stop → AgentStoppedError、并发多 Future 互不干扰、
  approval_pending 挂起计时 / tool_exec_start 重计时、EOF 哨兵
- 权限：auto 模式下白名单含 run_code 的子图调用产出 safety；
  审批等待计入任务墙钟 → 超时映射 status=timeout
- 主图回归（§7.0 五场景自动化兜底）：普通对话、单工具调用、ask_user 提问与
  恢复、等待中停止、reconfigure 热替换；以及 dispatch_subagents 本地路由、
  子图无 ask_user 分支
"""

import asyncio
import json
import os
import sys

import pytest

_TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
_AGENT_DIR = os.path.abspath(os.path.join(_TESTS_DIR, "..", "DAWorkbench", "agent"))
if _AGENT_DIR not in sys.path:
    sys.path.insert(0, _AGENT_DIR)

# tiktoken 离线模式：模型编码缺失时立即抛错降级为字符估算，
# 避免每次构造 AgentRunner 都等待网络下载编码
os.environ.setdefault("TIKTOKEN_OFFLINE_MODE", "1")

import agent_runner                                    # noqa: E402
import permission_judge                                # noqa: E402
import subagent_orchestrator as so                     # noqa: E402
from langchain_core.messages import AIMessage          # noqa: E402


def _run(coro):
    return asyncio.run(coro)


# ===========================================================================
# mock 基础设施
# ===========================================================================

class FakeStdio:
    """协议通道 mock：记录全部发送消息；实现节点/编排器用到的发送方法。"""

    def __init__(self):
        self.sent = []
        self.stop_event = None

    async def send(self, msg):
        self.sent.append(msg)

    async def send_ready(self, model):
        await self.send({"type": "ready", "model": model})

    async def send_usage(self, input_tokens, output_tokens, total_tokens, source="agent"):
        await self.send({
            "type": "usage",
            "input_tokens": int(input_tokens or 0),
            "output_tokens": int(output_tokens or 0),
            "total_tokens": int(total_tokens or 0),
            "source": source,
        })

    async def send_token(self, content):
        await self.send({"type": "token", "content": content})

    async def send_message_end(self, content="", usage=None):
        obj = {"type": "message_end", "content": content}
        if usage:
            obj["usage"] = dict(usage)
        await self.send(obj)

    async def send_tool_call(self, call_id, tool, arguments,
                             safety=None, subagent_id=None):
        msg = {"type": "tool_call", "call_id": call_id,
               "tool": tool, "arguments": arguments}
        if safety:
            msg["safety"] = safety
        if subagent_id:
            msg["subagent_id"] = subagent_id
        await self.send(msg)

    async def send_question(self, text, options, multi_select=False):
        await self.send({"type": "question", "text": text,
                         "options": options, "multi_select": multi_select})

    async def send_error(self, message, error_type="unknown", detail=""):
        await self.send({"type": "error", "message": message,
                         "error_type": error_type})

    async def send_retrying(self, attempt, max_attempts, delay_ms,
                            error_type, error_message):
        await self.send({"type": "retrying", "attempt": attempt})

    async def send_done(self):
        await self.send({"type": "done"})

    def set_stop_event(self, event, loop):
        self.stop_event = event

    def by_type(self, msg_type):
        return [m for m in self.sent if m.get("type") == msg_type]


class ScriptedStdio(FakeStdio):
    """在 FakeStdio 基础上提供可控 receive()：测试侧 inject() 注入消息，
    分发器按到达顺序消费（注入 Exception 实例则抛出，模拟 EOF 等）。"""

    def __init__(self):
        super().__init__()
        self._rx = None

    def _queue(self):
        if self._rx is None:
            self._rx = asyncio.Queue()
        return self._rx

    def inject(self, msg):
        self._queue().put_nowait(msg)

    async def receive(self):
        item = await self._queue().get()
        if isinstance(item, Exception):
            raise item
        return item


class FakeLLM:
    """脚本化 mock LLM。

    bind_tools 返回共享同一脚本队列/调用记录的 clone（模拟
    ChatOpenAI.bind_tools 的绑定语义）；astream 按脚本顺序产出回复。
    脚本元素：AIMessage（正常回复）/ Exception（抛出）/ FakeLLM.HANG（挂起）。
    """

    HANG = object()

    def __init__(self, script=None, delay=0.0, _shared=None):
        if _shared is None:
            _shared = {
                "script": list(script or []),
                "calls": [],
                "bound": [],
                "delay": delay,
                "active": 0,
                "max_active": 0,
            }
        self._shared = _shared
        self.bound_tools = None

    def bind_tools(self, schemas):
        clone = FakeLLM(_shared=self._shared)
        clone.bound_tools = schemas
        self._shared["bound"].append(schemas)
        return clone

    @property
    def calls(self):
        return self._shared["calls"]

    async def astream(self, messages, **kwargs):
        s = self._shared
        s["calls"].append(list(messages))
        s["active"] += 1
        s["max_active"] = max(s["max_active"], s["active"])
        try:
            script = s["script"]
            reply = script.pop(0) if script else AIMessage(content="(empty script)")
            if reply is FakeLLM.HANG:
                await asyncio.Event().wait()  # 永久挂起（墙钟超时测试用）
            if isinstance(reply, Exception):
                raise reply
            if s["delay"]:
                await asyncio.sleep(s["delay"])
            yield reply
        finally:
            s["active"] -= 1


_DEFAULT_TOOLS = [
    {"name": "list_data", "description": "List datasets",
     "parameters": {"type": "object", "properties": {}}},
    {"name": "query_data", "description": "Query rows",
     "parameters": {"type": "object", "properties": {}}},
    {"name": "run_code", "description": "Run python code",
     "parameters": {"type": "object",
                    "properties": {"code": {"type": "string"}}}},
    {"name": "write_file", "description": "Write file",
     "parameters": {"type": "object",
                    "properties": {"path": {"type": "string"}}}},
    {"name": "create_chart", "description": "Create chart",
     "parameters": {"type": "object", "properties": {}}},
]

_EXPLORE_DEF = {
    "name": "explore",
    "description": "Read-only data exploration",
    "tools": ["list_data", "query_data"],
    "system_prompt": "Explore the workspace data.",
}


def _make_runner(script=None, config_extra=None, tools=None, subagents=None,
                 stdio=None, delay=0.0):
    """构造 AgentRunner（FakeLLM + FakeStdio，离线，无 C++）。"""
    config = {"base_url": "http://fake.local/v1", "api_key": "fake-key",
              "model": "fake-model"}
    if config_extra:
        config.update(config_extra)
    stdio = stdio or FakeStdio()
    runner = agent_runner.AgentRunner(
        config, tools if tools is not None else _DEFAULT_TOOLS,
        "You are the main agent.", stdio, subagents=subagents
    )
    runner.llm = FakeLLM(script, delay=delay)
    runner._rebuild_tool_bindings()  # 以 FakeLLM 重建绑定（替换真实客户端）
    return runner


async def _stop_dispatcher(runner):
    if runner._dispatcher_task is not None:
        runner._dispatcher_task.cancel()
        try:
            await runner._dispatcher_task
        except BaseException:
            pass


async def _feed_tool_result(stdio, expect_tool=None, result=None, delay=0.0):
    """模拟 C++ 因果：等到 tool_call 发出后按其 call_id 回 tool_result。"""
    if delay:
        await asyncio.sleep(delay)
    while True:
        calls = stdio.by_type("tool_call")
        if calls and (expect_tool is None or calls[-1]["tool"] == expect_tool):
            break
        await asyncio.sleep(0.01)
    tc = calls[-1]
    stdio.inject({"type": "tool_result", "call_id": tc["call_id"],
                  "result": result if result is not None else {"ok": True}})
    return tc


def _dispatch(runner, tasks, call_id="call_dispatch"):
    return runner.subagent_orchestrator.dispatch({"tasks": tasks}, call_id)


def _progress_states(stdio):
    return [(p.get("task_id"), p.get("state"))
            for p in stdio.by_type("subagent_progress")]


# ===========================================================================
# 1. 定义解析与白名单求交
# ===========================================================================

class TestDefinitions:
    def test_parse_intersect_and_skip_malformed(self):
        async def scenario():
            runner = _make_runner()
            orch = runner.subagent_orchestrator
            orch.update_definitions([
                {"name": "explore", "description": "d",
                 "tools": ["list_data", "no_such_tool", "query_data"],
                 "system_prompt": "body"},
                {"name": "", "description": "empty name skipped"},
                {"name": "explore", "description": "duplicate skipped"},
                "not-a-dict",
            ])
            assert set(orch.definitions) == {"explore"}
            # 未知工具告警剔除，其余保持顺序
            assert orch.definitions["explore"].tools == ["list_data", "query_data"]
        _run(scenario())

    def test_empty_whitelist_allowed_pure_reasoning(self):
        async def scenario():
            runner = _make_runner()
            runner.subagent_orchestrator.update_definitions(
                [{"name": "thinker", "description": "", "tools": [],
                  "system_prompt": "just think"}])
            assert runner.subagent_orchestrator.definitions["thinker"].tools == []
        _run(scenario())

    def test_ask_user_and_dispatch_not_in_whitelist(self):
        """ask_user（Python 注入）与 dispatch_subagents（编排器注入）不在
        C++ 注册工具集 → 求交天然剔除（Q6 深度 1 / Q7 无提问）。"""
        async def scenario():
            runner = _make_runner()
            runner.subagent_orchestrator.update_definitions(
                [{"name": "x", "description": "",
                  "tools": ["list_data", "ask_user", "dispatch_subagents"]}])
            assert runner.subagent_orchestrator.definitions["x"].tools == ["list_data"]
        _run(scenario())

    def test_dispatch_schema_injection_and_removal(self):
        async def scenario():
            runner = _make_runner(subagents=[_EXPLORE_DEF])
            names = [s["name"] for s in runner.tool_schemas]
            assert "dispatch_subagents" in names
            schema = next(s for s in runner.tool_schemas
                          if s["name"] == "dispatch_subagents")
            # description 自动拼入可用子类型清单
            assert "- explore: Read-only data exploration" in schema["description"]
            assert schema["parameters"]["required"] == ["tasks"]
            # 热更新为空 → 移除
            runner.subagent_orchestrator.update_definitions([])
            names = [s["name"] for s in runner.tool_schemas]
            assert "dispatch_subagents" not in names
            assert "ask_user" in names  # 主图注入不受影响
        _run(scenario())

    def test_update_definitions_rebinds_llm(self):
        async def scenario():
            runner = _make_runner()
            bound_before = len(runner.llm._shared["bound"])
            runner.subagent_orchestrator.update_definitions([_EXPLORE_DEF])
            assert len(runner.llm._shared["bound"]) == bound_before + 1
        _run(scenario())


# ===========================================================================
# 2. 批派发：校验 / 状态映射 / 停止级联
# ===========================================================================

class TestDispatchValidation:
    def test_batch_limit_rejected(self):
        async def scenario():
            runner = _make_runner(subagents=[_EXPLORE_DEF])
            tasks = [{"subagent_type": "explore", "prompt": f"t{i}"}
                     for i in range(5)]
            result = await _dispatch(runner, tasks)
            assert "error" in result
            assert "4" in result["error"] and "5" in result["error"]
            # 拒绝在任何任务启动之前——无进度消息
            assert runner.stdio.by_type("subagent_progress") == []
        _run(scenario())

    def test_empty_or_invalid_tasks_rejected(self):
        async def scenario():
            runner = _make_runner(subagents=[_EXPLORE_DEF])
            r1 = await runner.subagent_orchestrator.dispatch({"tasks": []}, "c1")
            r2 = await runner.subagent_orchestrator.dispatch({"tasks": "nope"}, "c2")
            r3 = await runner.subagent_orchestrator.dispatch({}, "c3")
            for r in (r1, r2, r3):
                assert "error" in r
        _run(scenario())

    def test_no_definitions_error(self):
        async def scenario():
            runner = _make_runner()  # 无定义
            result = await _dispatch(
                runner, [{"subagent_type": "explore", "prompt": "x"}])
            assert "error" in result and "No subagent definitions" in result["error"]
        _run(scenario())

    def test_unknown_subagent_type_per_task_error(self):
        async def scenario():
            runner = _make_runner(subagents=[_EXPLORE_DEF])
            runner.start_dispatcher()
            try:
                result = await _dispatch(runner, [
                    {"subagent_type": "ghost", "prompt": "x"},
                ])
            finally:
                await _stop_dispatcher(runner)
            task = result["tasks"][0]
            assert task["status"] == "error"
            assert "Unknown subagent type 'ghost'" in task["error"]
            assert ("ghost#1", "error") in _progress_states(runner.stdio)
        _run(scenario())


class TestDispatchExecution:
    def test_ok_path_with_tool_call_and_aggregate(self):
        """完整集成：子图发起工具调用（带 subagent_id）→ 结果回流 →
        summary/进度序列/聚合/usage 全部按契约。"""
        async def scenario():
            script = [
                AIMessage(content="", tool_calls=[
                    {"name": "list_data", "args": {}, "id": "sub_call_1"}]),
                AIMessage(content="Summary: 2 datasets found.",
                          usage_metadata={"input_tokens": 10, "output_tokens": 5,
                                          "total_tokens": 15}),
            ]
            stdio = ScriptedStdio()
            runner = _make_runner(script=script, stdio=stdio,
                                  subagents=[_EXPLORE_DEF])
            runner.start_dispatcher()

            async def feed():
                tc = await _feed_tool_result(
                    stdio, result={"data": ["d1", "d2"]})
                # 子图调用必须带 subagent_id（= 任务 id）
                assert tc["subagent_id"] == "explore#1"
                assert "safety" not in tc  # yolo 模式无裁决

            feeder = asyncio.create_task(feed())
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "explore", "prompt": "Explore data"}]),
                    10)
            finally:
                await _stop_dispatcher(runner)
            await feeder

            task = result["tasks"][0]
            assert task["task_id"] == "explore#1"
            assert task["subagent_type"] == "explore"
            assert task["status"] == "ok"
            assert task["summary"] == "Summary: 2 datasets found."

            states = _progress_states(stdio)
            assert ("explore#1", "spawned") in states
            assert ("explore#1", "running") in states
            assert ("explore#1", "done") in states
            # 聚合消息：无 task_id、results 携带全部任务（C++ Q18 撤卡依据）
            agg = [p for p in stdio.by_type("subagent_progress")
                   if "results" in p]
            assert len(agg) == 1
            assert agg[0].get("task_id") is None
            assert agg[0]["results"]["tasks"][0]["status"] == "ok"

            # usage 聚合为单条（source="subagents"）
            usages = [u for u in stdio.by_type("usage")
                      if u.get("source") == "subagents"]
            assert len(usages) == 1
            assert usages[0]["input_tokens"] == 10
            assert usages[0]["output_tokens"] == 5
            assert usages[0]["total_tokens"] == 15

            # 子图工具绑定 = 白名单子集（无 ask_user / dispatch_subagents）
            sub_bound = runner.llm._shared["bound"][-1]
            assert [s["name"] for s in sub_bound] == ["list_data", "query_data"]
            # 子图静默：无 token / message_end 输出（子转录不进主聊天流）
            assert stdio.by_type("token") == []
            assert stdio.by_type("message_end") == []
        _run(scenario())

    def test_error_mapping_llm_exception(self):
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(script=[RuntimeError("boom")], stdio=stdio,
                                  subagents=[_EXPLORE_DEF])
            runner.start_dispatcher()
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "explore", "prompt": "x"}]),
                    10)
            finally:
                await _stop_dispatcher(runner)
            task = result["tasks"][0]
            assert task["status"] == "error"
            assert "boom" in task["error"]
            assert ("explore#1", "error") in _progress_states(stdio)
        _run(scenario())

    def test_timeout_mapping_hanging_llm(self):
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(script=[FakeLLM.HANG], stdio=stdio,
                                  subagents=[_EXPLORE_DEF],
                                  config_extra={"subagent_timeout_sec": 1})
            runner.start_dispatcher()
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "explore", "prompt": "x"}]),
                    10)
            finally:
                await _stop_dispatcher(runner)
            task = result["tasks"][0]
            assert task["status"] == "timeout"
            assert ("explore#1", "timeout") in _progress_states(stdio)
        _run(scenario())

    def test_stop_cascade_before_start(self):
        """stop 已 set：未开始的任务直接标记 stopped，LLM 零调用（Q7）。"""
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(
                script=[AIMessage(content="must not be consumed")],
                stdio=stdio, subagents=[_EXPLORE_DEF])
            runner._stop_event.set()
            result = await asyncio.wait_for(
                _dispatch(runner, [
                    {"subagent_type": "explore", "prompt": "a"},
                    {"subagent_type": "explore", "prompt": "b"}]),
                10)
            assert [t["status"] for t in result["tasks"]] == ["stopped", "stopped"]
            assert runner.llm.calls == []  # 从未调用 LLM
            states = _progress_states(stdio)
            assert ("explore#1", "stopped") in states
            assert ("explore#2", "stopped") in states
        _run(scenario())

    def test_stop_during_execution(self):
        """执行中 stop（经分发器置 stop_event + 唤醒挂起 RPC）→ 运行中任务
        映射 stopped。停止取真实中断路径：子图挂起在工具 RPC 等待上，
        stop 使该 RPC 以 AgentStoppedError 失败、子图解体。"""
        async def scenario():
            script = [
                AIMessage(content="", tool_calls=[
                    {"name": "list_data", "args": {}, "id": "sc_stop"}]),
                # 此后不再有脚本回复——tool_result 永不注入，等待 stop 打断
            ]
            stdio = ScriptedStdio()
            runner = _make_runner(script=script, stdio=stdio,
                                  subagents=[_EXPLORE_DEF])
            runner.start_dispatcher()

            async def stopper():
                # 等子任务进入 running 且工具调用已发出后再停止
                while ("explore#1", "running") not in _progress_states(stdio):
                    await asyncio.sleep(0.01)
                while not stdio.by_type("tool_call"):
                    await asyncio.sleep(0.01)
                stdio.inject({"type": "stop"})

            stop_task = asyncio.create_task(stopper())
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "explore", "prompt": "x"}]),
                    10)
            finally:
                await _stop_dispatcher(runner)
            await stop_task
            assert result["tasks"][0]["status"] == "stopped"
            assert runner._stop_event.is_set()
            assert ("explore#1", "stopped") in _progress_states(stdio)
        _run(scenario())

    def test_concurrency_limited_by_semaphore(self):
        """3 任务并发 2（默认）：峰值同时运行 ≤2（Q6）。"""
        async def scenario():
            script = [
                AIMessage(content=f"summary {i}",
                          usage_metadata={"input_tokens": 1, "output_tokens": 1,
                                          "total_tokens": 2})
                for i in range(3)
            ]
            # 空白名单（纯推理）避免工具注入干扰
            defs = [{"name": "thinker", "description": "", "tools": [],
                     "system_prompt": ""}]
            runner = _make_runner(script=script, subagents=defs, delay=0.1)
            runner.start_dispatcher()
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "thinker", "prompt": f"t{i}"}
                        for i in range(3)]),
                    15)
            finally:
                await _stop_dispatcher(runner)
            assert [t["status"] for t in result["tasks"]] == ["ok"] * 3
            assert runner.llm._shared["max_active"] <= 2
            assert runner.llm._shared["max_active"] >= 1
        _run(scenario())

    def test_failure_isolation_between_tasks(self):
        """失败不传染（Q11）：一个任务抛错不影响兄弟任务与聚合。"""
        async def scenario():
            script = [
                RuntimeError("task one fails"),
                AIMessage(content="task two ok"),
            ]
            runner = _make_runner(script=script, subagents=[
                {"name": "explore", "description": "", "tools": [],
                 "system_prompt": ""}])
            runner.start_dispatcher()
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "explore", "prompt": "a"},
                        {"subagent_type": "explore", "prompt": "b"}]),
                    10)
            finally:
                await _stop_dispatcher(runner)
            statuses = {t["task_id"]: t["status"] for t in result["tasks"]}
            assert statuses == {"explore#1": "error", "explore#2": "ok"}
        _run(scenario())


class TestTruncationAndUsage:
    def test_summary_truncated_to_4000(self):
        async def scenario():
            runner = _make_runner(
                script=[AIMessage(content="x" * 6000)],
                subagents=[{"name": "explore", "description": "",
                            "tools": [], "system_prompt": ""}])
            runner.start_dispatcher()
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "explore", "prompt": "x"}]),
                    10)
            finally:
                await _stop_dispatcher(runner)
            summary = result["tasks"][0]["summary"]
            assert len(summary) <= 4000 + len("... [truncated]")
            assert summary.endswith("... [truncated]")
        _run(scenario())

    def test_total_result_capped(self):
        """总结果 ≤ tool_result_max_chars：阶梯收缩 summary，JSON 不破坏。"""
        async def scenario():
            script = [AIMessage(content="y" * 4000) for _ in range(4)]
            runner = _make_runner(
                script=script,
                subagents=[{"name": "explore", "description": "",
                            "tools": [], "system_prompt": ""}],
                config_extra={"tool_result_max_chars": 8000})
            runner.start_dispatcher()
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "explore", "prompt": f"t{i}"}
                        for i in range(4)]),
                    15)
            finally:
                await _stop_dispatcher(runner)
            text = json.dumps(result, ensure_ascii=False)
            assert len(text) <= 8000
            # 结构完好（可再次解析且任务齐全）
            assert len(json.loads(text)["tasks"]) == 4
        _run(scenario())

    def test_usage_aggregated_across_tasks(self):
        async def scenario():
            script = [
                AIMessage(content="s1", usage_metadata={
                    "input_tokens": 3, "output_tokens": 2, "total_tokens": 5}),
                AIMessage(content="s2", usage_metadata={
                    "input_tokens": 7, "output_tokens": 1, "total_tokens": 8}),
            ]
            stdio = ScriptedStdio()
            runner = _make_runner(
                script=script, stdio=stdio,
                subagents=[{"name": "explore", "description": "",
                            "tools": [], "system_prompt": ""}])
            runner.start_dispatcher()
            try:
                await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "explore", "prompt": "a"},
                        {"subagent_type": "explore", "prompt": "b"}]),
                    10)
            finally:
                await _stop_dispatcher(runner)
            usages = [u for u in stdio.by_type("usage")
                      if u.get("source") == "subagents"]
            assert len(usages) == 1
            assert usages[0]["input_tokens"] == 10
            assert usages[0]["output_tokens"] == 3
            assert usages[0]["total_tokens"] == 13
        _run(scenario())


# ===========================================================================
# 3. 系统提示词组装（Q10）
# ===========================================================================

class TestSystemPromptComposition:
    def _compose(self, tools, gated=("write_file", "run_code", "run_script")):
        async def build():
            runner = _make_runner(
                config_extra={"gated_tools": list(gated)},
                subagents=[{"name": "x", "description": "",
                            "tools": tools, "system_prompt": "BODY"}])
            d = runner.subagent_orchestrator.definitions["x"]
            return runner.subagent_orchestrator._compose_system_prompt(d)
        return _run(build())

    def test_preamble_and_body_always_present(self):
        prompt = self._compose(["list_data"])
        assert "focused subagent" in prompt
        assert "BODY" in prompt
        assert "CANNOT ask the user questions" in prompt

    def test_figure_reference_when_chart_tools(self):
        prompt = self._compose(["create_chart"])
        assert "da-figure:" in prompt
        prompt2 = self._compose(["list_data"])
        assert "da-figure:" not in prompt2

    def test_permission_section_when_gated_tools(self):
        prompt = self._compose(["run_code"])
        assert "Permissions and safety" in prompt
        prompt2 = self._compose(["list_data"])
        assert "Permissions and safety" not in prompt2


# ===========================================================================
# 4. RPC 多路复用分发器（Q20）
# ===========================================================================

class TestDispatcher:
    def test_out_of_order_results_routed(self):
        """并发两个挂起 Future，结果乱序到达 → 各自正确路由。"""
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(stdio=stdio)
            runner.start_dispatcher()
            try:
                t1 = asyncio.create_task(runner._rpc_call(
                    {"name": "list_data", "id": "c1", "args": {}}))
                t2 = asyncio.create_task(runner._rpc_call(
                    {"name": "query_data", "id": "c2", "args": {}}))
                while len(stdio.by_type("tool_call")) < 2:
                    await asyncio.sleep(0.01)
                # 乱序：c2 先到
                stdio.inject({"type": "tool_result", "call_id": "c2",
                              "result": {"v": 2}})
                stdio.inject({"type": "tool_result", "call_id": "c1",
                              "result": {"v": 1}})
                r1, r2 = await asyncio.wait_for(asyncio.gather(t1, t2), 5)
                assert r1 == {"v": 1}
                assert r2 == {"v": 2}
                # 主图调用不带 subagent_id
                for tc in stdio.by_type("tool_call"):
                    assert "subagent_id" not in tc
            finally:
                await _stop_dispatcher(runner)
        _run(scenario())

    def test_late_and_mismatched_results_ignored(self):
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(stdio=stdio)
            runner.start_dispatcher()
            try:
                # 无任何挂起槽位的 call_id → 记日志忽略，不崩溃
                stdio.inject({"type": "tool_result", "call_id": "ghost",
                              "result": {"x": 1}})
                await asyncio.sleep(0.05)
                # 超时后迟到的结果：先让调用超时
                t = asyncio.create_task(runner._rpc_call(
                    {"name": "list_data", "id": "c1", "args": {}},
                    timeout=0.1))
                r = await asyncio.wait_for(t, 5)
                assert "error" in r and "timed out" in r["error"]
                stdio.inject({"type": "tool_result", "call_id": "c1",
                              "result": {"late": True}})
                await asyncio.sleep(0.05)
                # 后续新调用不受污染
                t2 = asyncio.create_task(runner._rpc_call(
                    {"name": "list_data", "id": "c2", "args": {}}))
                while len(stdio.by_type("tool_call")) < 2:
                    await asyncio.sleep(0.01)
                stdio.inject({"type": "tool_result", "call_id": "c2",
                              "result": {"fresh": True}})
                assert await asyncio.wait_for(t2, 5) == {"fresh": True}
            finally:
                await _stop_dispatcher(runner)
        _run(scenario())

    def test_stop_while_waiting_raises_agent_stopped(self):
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(stdio=stdio)
            runner.start_dispatcher()
            try:
                t = asyncio.create_task(runner._rpc_call(
                    {"name": "list_data", "id": "c1", "args": {}}))
                while not stdio.by_type("tool_call"):
                    await asyncio.sleep(0.01)
                stdio.inject({"type": "stop"})
                with pytest.raises(agent_runner.AgentStoppedError):
                    await asyncio.wait_for(t, 5)
                assert runner._stop_event.is_set()
                # 挂起槽位已清理
                assert runner._pending_rpcs == {}
            finally:
                await _stop_dispatcher(runner)
        _run(scenario())

    def test_approval_pending_suspends_exec_start_restarts(self):
        """三段计时语义保持：审批等待不计时；批准后重新计完整超时。"""
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(stdio=stdio)
            runner.start_dispatcher()
            try:
                t = asyncio.create_task(runner._rpc_call(
                    {"name": "write_file", "id": "c1", "args": {}},
                    timeout=0.3))
                while not stdio.by_type("tool_call"):
                    await asyncio.sleep(0.01)
                stdio.inject({"type": "approval_pending", "call_id": "c1"})
                # 远超 timeout 的审批等待——不应超时
                await asyncio.sleep(0.6)
                assert not t.done()
                # 批准并开始执行 → 重新计完整 0.3s，期间结果到达
                stdio.inject({"type": "tool_exec_start", "call_id": "c1"})
                stdio.inject({"type": "tool_result", "call_id": "c1",
                              "result": {"ok": True}})
                assert await asyncio.wait_for(t, 5) == {"ok": True}
            finally:
                await _stop_dispatcher(runner)
        _run(scenario())

    def test_eof_fails_pending_and_signals_main_loop(self):
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(stdio=stdio)
            runner.start_dispatcher()
            try:
                t = asyncio.create_task(runner._rpc_call(
                    {"name": "list_data", "id": "c1", "args": {}}))
                while not stdio.by_type("tool_call"):
                    await asyncio.sleep(0.01)
                stdio.inject(EOFError("stdin closed"))
                with pytest.raises(agent_runner.AgentStoppedError):
                    await asyncio.wait_for(t, 5)
                # None 哨兵送达控制队列（主循环据此退出）
                sentinel = await asyncio.wait_for(
                    runner._control_queue.get(), 5)
                assert sentinel is None
            finally:
                await _stop_dispatcher(runner)
        _run(scenario())

    def test_runtime_messages_routed_to_control_queue(self):
        """user_msg / update_subagents 等经分发器入控制队列。"""
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(stdio=stdio)
            runner.start_dispatcher()
            try:
                stdio.inject({"type": "user_msg", "content": "hi"})
                stdio.inject({"type": "update_subagents", "subagents": []})
                m1 = await asyncio.wait_for(runner._control_queue.get(), 5)
                m2 = await asyncio.wait_for(runner._control_queue.get(), 5)
                assert m1["type"] == "user_msg"
                assert m2["type"] == "update_subagents"
            finally:
                await _stop_dispatcher(runner)
        _run(scenario())


# ===========================================================================
# 5. 权限（Q19 / 铁律 T16 延伸）
# ===========================================================================

class TestPermissionIntegration:
    def test_subgraph_code_exec_produces_safety_in_auto_mode(self):
        """auto 模式 + 白名单含 run_code：子图调用在 tool_call 前产出 safety
        （mock permission_judge），与 subagent_id 并存。"""
        async def scenario():
            script = [
                AIMessage(content="", tool_calls=[
                    {"name": "run_code", "args": {"code": "print(1)"},
                     "id": "pc1"}]),
                AIMessage(content="ran code"),
            ]
            stdio = ScriptedStdio()
            runner = _make_runner(
                script=script, stdio=stdio,
                config_extra={"permission_mode": "auto",
                              "gated_tools": ["run_code", "write_file"]},
                subagents=[{"name": "coder", "description": "",
                            "tools": ["run_code"], "system_prompt": ""}])
            runner.start_dispatcher()
            judge_calls = []

            async def fake_judge(name, args, ctx):
                judge_calls.append((name, dict(args)))
                return {"verdict": "allow", "reason": "benign",
                        "source": "rules"}

            orig = permission_judge.judge_tool_call
            permission_judge.judge_tool_call = fake_judge

            async def feed():
                tc = await _feed_tool_result(stdio, result={"result": "1"})
                # safety 与 subagent_id 并存（协议契约）
                assert tc["safety"]["verdict"] == "allow"
                assert tc["subagent_id"] == "coder#1"

            feeder = asyncio.create_task(feed())
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "coder", "prompt": "run it"}]),
                    10)
            finally:
                permission_judge.judge_tool_call = orig
                await _stop_dispatcher(runner)
            await feeder

            assert judge_calls == [("run_code", {"code": "print(1)"})]
            assert result["tasks"][0]["status"] == "ok"
            # 白名单含 gated 工具 → 系统提示词含权限约定（经 LLM 首条消息验证）
            first_call_msgs = runner.llm.calls[0]
            assert any("Permissions and safety" in str(m.content)
                       for m in first_call_msgs)
        _run(scenario())

    def test_approval_wait_counts_toward_wall_clock(self):
        """gated 工具审批等待计入任务墙钟：永不批准 → 任务超时（§3 消费语义）。"""
        async def scenario():
            script = [
                AIMessage(content="", tool_calls=[
                    {"name": "run_code", "args": {"code": "print(1)"},
                     "id": "pc1"}]),
            ]
            stdio = ScriptedStdio()
            runner = _make_runner(
                script=script, stdio=stdio,
                config_extra={"permission_mode": "auto",
                              "gated_tools": ["run_code"],
                              "subagent_timeout_sec": 1,
                              "tool_approval_timeout_sec": 600},
                subagents=[{"name": "coder", "description": "",
                            "tools": ["run_code"], "system_prompt": ""}])
            runner.start_dispatcher()

            async def feed_approval():
                while not stdio.by_type("tool_call"):
                    await asyncio.sleep(0.01)
                tc = stdio.by_type("tool_call")[0]
                # 权限门进入 Ask：下发 approval_pending，此后不批准
                stdio.inject({"type": "approval_pending",
                              "call_id": tc["call_id"]})

            feeder = asyncio.create_task(feed_approval())
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "coder", "prompt": "run it"}]),
                    15)
            finally:
                await _stop_dispatcher(runner)
            await feeder
            # 审批等待不豁免任务墙钟 → timeout（而非永远挂起）
            assert result["tasks"][0]["status"] == "timeout"
            assert ("coder#1", "timeout") in _progress_states(stdio)
        _run(scenario())


# ===========================================================================
# 6. 主图回归（§7.0 五场景自动化兜底 + 派发路由）
# ===========================================================================

class TestMainGraphRegression:
    def test_plain_conversation(self):
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(
                script=[AIMessage(content="Hello! How can I help?")],
                stdio=stdio)
            runner.start_dispatcher()
            try:
                await asyncio.wait_for(runner.run("hi"), 10)
            finally:
                await _stop_dispatcher(runner)
            ends = stdio.by_type("message_end")
            assert len(ends) == 1
            assert ends[0]["content"] == "Hello! How can I help?"
            assert len(stdio.by_type("done")) == 1
            assert stdio.by_type("question") == []
        _run(scenario())

    def test_single_tool_call_flow(self):
        """普通对话 → 工具调用（不带 subagent_id）→ 结果回流 → 最终回复。"""
        async def scenario():
            script = [
                AIMessage(content="", tool_calls=[
                    {"name": "list_data", "args": {}, "id": "call_1"}]),
                AIMessage(content="Here are your datasets."),
            ]
            stdio = ScriptedStdio()
            runner = _make_runner(script=script, stdio=stdio)
            runner.start_dispatcher()
            feeder = asyncio.create_task(_feed_tool_result(
                stdio, result={"data": ["d1"]}))
            try:
                await asyncio.wait_for(runner.run("list data"), 10)
            finally:
                await _stop_dispatcher(runner)
            await feeder
            calls = stdio.by_type("tool_call")
            assert len(calls) == 1
            assert calls[0]["tool"] == "list_data"
            assert "subagent_id" not in calls[0]  # 主图调用不带标记
            ends = stdio.by_type("message_end")
            assert [e["content"] for e in ends] == ["Here are your datasets."]
            assert stdio.by_type("done")
        _run(scenario())

    def test_ask_user_question_and_resume(self):
        script = [
            AIMessage(content="", tool_calls=[
                {"name": "ask_user",
                 "args": {"question": "Which dataset?", "options": ["A", "B"]},
                 "id": "ask_1"}]),
            AIMessage(content="Thanks, using A."),
        ]

        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(script=script, stdio=stdio)
            runner.start_dispatcher()
            try:
                await asyncio.wait_for(runner.run("analyze"), 10)
                # 暂停于 interrupt：question 恰发一次，不发 done（铁律 T8）
                qs = stdio.by_type("question")
                assert len(qs) == 1
                assert qs[0]["text"] == "Which dataset?"
                assert qs[0]["options"] == ["A", "B"]
                assert stdio.by_type("done") == []
                # 用户回答 → resume → 最终回复 + done
                await asyncio.wait_for(runner.resume("A"), 10)
                ends = stdio.by_type("message_end")
                assert ends[-1]["content"] == "Thanks, using A."
                assert len(stdio.by_type("done")) == 1
            finally:
                await _stop_dispatcher(runner)
        _run(scenario())

    def test_stop_during_tool_wait(self):
        async def scenario():
            script = [
                AIMessage(content="", tool_calls=[
                    {"name": "list_data", "args": {}, "id": "call_s"}]),
            ]
            stdio = ScriptedStdio()
            runner = _make_runner(script=script, stdio=stdio)
            runner.start_dispatcher()
            try:
                run_task = asyncio.create_task(runner.run("go"))
                while not stdio.by_type("tool_call"):
                    await asyncio.sleep(0.01)
                stdio.inject({"type": "stop"})
                # AgentStoppedError 穿透 run()（main() 负责吞掉并发 done）
                with pytest.raises(agent_runner.AgentStoppedError):
                    await asyncio.wait_for(run_task, 10)
                assert runner._stop_event.is_set()
            finally:
                await _stop_dispatcher(runner)
        _run(scenario())

    def test_reconfigure_hot_swap(self):
        async def scenario():
            stdio = ScriptedStdio()
            runner = _make_runner(
                script=[AIMessage(content="old")], stdio=stdio)
            new_config = {"base_url": "http://new.local/v1", "api_key": "k2",
                          "model": "new-model"}
            await runner.reconfigure(new_config)
            assert runner.config["model"] == "new-model"
            assert stdio.by_type("ready")[-1]["model"] == "new-model"
            # 工具 schema 不受影响（含 ask_user）
            names = [s["name"] for s in runner.tool_schemas]
            assert "ask_user" in names
            # 图未重建：thread_config 与 graph 保持
            assert runner.thread_config["configurable"]["thread_id"] == "agent_session_1"
        _run(scenario())

    def test_dispatch_routed_locally_not_rpc(self):
        """主图 dispatch_subagents 调用本地编排（跳过 _rpc_call）；
        子图（纯推理）回复经 ToolMessage 回流，主图继续推理。"""
        async def scenario():
            script = [
                AIMessage(content="", tool_calls=[
                    {"name": "dispatch_subagents",
                     "args": {"tasks": [
                         {"subagent_type": "explore", "prompt": "go"}]},
                     "id": "disp_1"}]),
                AIMessage(content="sub summary"),      # 子图最终回复
                AIMessage(content="Final answer."),    # 主图继续
            ]
            stdio = ScriptedStdio()
            runner = _make_runner(
                script=script, stdio=stdio,
                subagents=[{"name": "explore", "description": "",
                            "tools": [], "system_prompt": ""}])
            runner.start_dispatcher()
            try:
                await asyncio.wait_for(runner.run("delegate"), 10)
            finally:
                await _stop_dispatcher(runner)
            # dispatch 本体与纯推理子图都不产生 C++ RPC
            assert stdio.by_type("tool_call") == []
            # 进度消息序列存在（spawned/running/done + 聚合）
            assert stdio.by_type("subagent_progress")
            ends = stdio.by_type("message_end")
            assert ends[-1]["content"] == "Final answer."
            assert stdio.by_type("done")
        _run(scenario())

    def test_subgraph_has_no_ask_user_branch(self):
        """子图 LLM 幻觉 ask_user 调用 → 按普通工具走 RPC（带 subagent_id），
        绝不发 question（子图无提问分支，Q7）。"""
        async def scenario():
            script = [
                AIMessage(content="", tool_calls=[
                    {"name": "ask_user",
                     "args": {"question": "Q?"}, "id": "au_1"}]),
                AIMessage(content="sub done"),
            ]
            stdio = ScriptedStdio()
            runner = _make_runner(script=script, stdio=stdio,
                                  subagents=[_EXPLORE_DEF])
            runner.start_dispatcher()

            async def feed():
                tc = await _feed_tool_result(
                    stdio, result={"error": "Unknown tool: ask_user"})
                assert tc["tool"] == "ask_user"
                assert tc["subagent_id"] == "explore#1"

            feeder = asyncio.create_task(feed())
            try:
                result = await asyncio.wait_for(
                    _dispatch(runner, [
                        {"subagent_type": "explore", "prompt": "x"}]),
                    10)
            finally:
                await _stop_dispatcher(runner)
            await feeder
            assert result["tasks"][0]["status"] == "ok"
            # 静默图：绝无 question 消息
            assert stdio.by_type("question") == []
        _run(scenario())
