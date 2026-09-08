#!/usr/bin/env python3
"""子 agent 编排器（子 agent 一期 P3，母文档 §4 / 决策 Q1/Q5-Q11/Q17/Q19/Q20）。

职责：
- 解析并热更新 C++ 经 init / update_subagents 下发的子 agent 定义（协议四字段
  name/description/tools/system_prompt）；tools 白名单与已注册工具求交——
  未知工具名告警剔除，空白名单合法（=纯推理子图）；
- 按定义集动态注入 / 移除 ``dispatch_subagents`` 工具 schema 并重绑
  ``llm_with_tools``（定义集为空则不注入，Q6 深度 1：子图无派发工具）；
- 批派发子任务：批大小校验（``subagent_batch_limit``）→ 信号量并发限流
  （``subagent_max_concurrency``）→ 每任务墙钟（``subagent_timeout_sec``）+
  子图递归上限（``subagent_recursion_limit``）→ 汇总
  ``{tasks: [{task_id, subagent_type, status, summary?, error?}]}``；
- 进度上报 ``subagent_progress``：派发开始（spawned）、每任务状态变化
  （running/done/error/timeout/stopped）、≤30s 心跳（保活 C++ 无活动看门狗）、
  派发结束聚合（results 携带全部任务，C++ 据此按 Q18 撤销挂起审批卡）；
- usage 聚合：各子图最终 AIMessage.usage_metadata 求和，派发结束后发一条
  ``usage``（source="subagents"）。

隔离与共享（Q1）：子图与主图同进程，共享 runner 的 LLM（继承当前 active
模型，Q7）、stop_event（停止级联）、stdio 协议通道、工具结果截断器与权限层
配置；工具执行最终经 call_id 多路复用 RPC 汇合于 Qt 主线程（Q20）。子图
独立 MemorySaver + 独立消息历史 + 独立循环检测状态；子图静默——不输出
token/message_end/usage/retrying 协议消息（子转录不进主聊天流，Q8）。
stdout 是协议通道（铁律 T1），本模块所有日志走 stderr（logging 已配置）。
"""

import asyncio
import collections
import json
import logging
import sys

from langchain_core.messages import AIMessage, HumanMessage

# GraphRecursionError：子图达到 recursion_limit 时抛出，映射为 error 状态并
# 给出友好文案（同主图 main() 的处理先例）。低版本 langgraph 可能无此类，
# 降级为不捕获（占位类 isinstance 永远不匹配）。
try:
    from langgraph.errors import GraphRecursionError
except ImportError:
    GraphRecursionError = type(None)  # 占位，isinstance 永远不匹配

logger = logging.getLogger("subagent_orchestrator")

# ===========================================================================
# 常量
# ===========================================================================

#: 图表类工具集（与 plugins/DAAgentTools 的 10 个图表工具一致）：
#: 白名单与之相交时子图系统提示词附加 figure_reference 约定（Q10）
CHART_TOOLS = frozenset({
    "create_chart", "add_curve", "set_chart_style", "set_axis",
    "update_curve_style", "remove_chart_item", "add_annotation",
    "create_subplots", "save_chart_image", "list_figures",
})

#: 单任务 summary 截断上限（字符）
_SUMMARY_MAX_CHARS = 4000

#: 单任务 error 文案截断上限（字符）
_ERROR_MAX_CHARS = 500

#: 心跳间隔（秒，保活 C++ 无活动看门狗；协议要求 ≤30s 一次）
_HEARTBEAT_INTERVAL_SEC = 30.0

#: spawned 进度消息中任务提示词摘要长度
_SPAWN_PROMPT_PREVIEW_CHARS = 200

#: 子图系统提示词——固定前导（英文内容，LLM 面向，不涉及 tr()）
_SUBAGENT_PREAMBLE = (
    "You are a focused subagent working inside data-workbench, delegated by "
    "the main AI assistant to complete one specific task autonomously.\n"
    "Rules:\n"
    "- Complete the task yourself. You CANNOT ask the user questions (there is "
    "no ask_user tool); make reasonable assumptions and note them in your "
    "summary if needed.\n"
    "- Stay within the scope of the task you were given; do not pursue "
    "unrelated work.\n"
    "- Use your tools efficiently; do not repeat identical calls.\n"
    "- When finished, reply with a single structured summary in markdown "
    "covering: what you did, what you found, data quality observations, and "
    "suggested next steps (if applicable)."
)

#: 白名单含图表工具时附加（镜像主提示词"在回复中引用图表"节）
_FIGURE_REFERENCE_SECTION = (
    "## Referring to figures\n"
    "- `create_chart` / `create_subplots` return `figure_name` and `figure_id`.\n"
    "- When mentioning a figure in your summary, insert a "
    "`[name](da-figure:<figure_name>)` hyperlink so the user can jump to it."
)

#: 白名单含 gated 工具（文件写入/代码执行）时附加（镜像主提示词"权限与安全"节，
#: Q10：否则子 agent 会对审批延迟/拒绝结果困惑重试）
_PERMISSION_SECTION = (
    "## Permissions and safety\n"
    "- Your tool calls are subject to the permission policy: some calls may "
    "require the user's approval in the UI before they execute. Waiting for "
    "approval is normal — do not retry the same call while waiting.\n"
    "- If a tool returns an \"Access denied\" style error, the operation was "
    "refused by the permission policy or the user. Do not try to bypass it "
    "with other paths or methods; report the refusal in your summary.\n"
    "- Keep file writes inside the script workspace; system directories "
    "(Windows, Program Files, etc.) are always forbidden.\n"
    "- Avoid dangerous operations in generated code (deleting files, running "
    "system commands, external network access) unless explicitly required by "
    "the task."
)


# ===========================================================================
# 定义解析
# ===========================================================================

class SubagentDefinition:
    """单个子 agent 定义（解析自 C++ 下发的协议 dict，四字段）。

    tools 白名单在解析时与已注册工具求交（双保险之一，C++ 保存时已校验，
    Q9）：未知工具名告警剔除；空白名单合法（=纯推理，Q5）。ask_user /
    dispatch_subagents 不在已注册工具中，天然被求交剔除（Q6 深度 1 /
    Q7 子图无提问）。
    """

    def __init__(self, raw: dict, registered_tools):
        self.name = str(raw.get("name", "") or "").strip()
        self.description = str(raw.get("description", "") or "")
        self.system_prompt = str(raw.get("system_prompt", "") or "")
        raw_tools = raw.get("tools")
        if not isinstance(raw_tools, list):
            raw_tools = []
        registered = set(registered_tools)
        tools = []
        dropped = []
        for t in raw_tools:
            t = str(t or "").strip()
            if not t:
                continue
            if t in registered:
                tools.append(t)
            else:
                dropped.append(t)
        if dropped:
            logger.warning(
                "subagent %r references unregistered tools %s, dropped",
                self.name, dropped
            )
        self.tools = tools


# ===========================================================================
# 子图运行期上下文
# ===========================================================================

class SubagentRunContext:
    """子图节点的运行期属性袋（build_agent_graph 的 ctx 参数）。

    与主图共用构建器的关键：节点闭包经 ctx.* call-time 读取。本对象提供——
    - 固定的 ``llm_with_tools``（派发时以工具子集绑定，继承当前 active
      模型，Q7）；
    - 独立的循环检测状态（不污染主图的 _executed_call_sigs / 硬终止计数）；
    - 权限层 / 配置 / 截断器 / stop_event 等穿透读 runner——子图天然继承
      主图上下文（Q19：白名单含 code_exec 工具的子 agent 在 auto 模式同样
      产出 safety，判定块不绕过）；
    - ``_rpc_call`` 转发时附加 ``subagent_id``（任务 id），C++ 侧据此过滤
      持久化与渲染、审批卡附加来源上下文。
    不持有 ``subagent_orchestrator`` 属性——构建器 tool_node 以
    getattr(ctx, "subagent_orchestrator", None) 判定派发路由，子图恒为
    None（深度 1 结构性禁止，Q6）。
    """

    def __init__(self, runner, llm_with_tools, task_id: str):
        self._runner = runner
        self._task_id = task_id
        self.llm_with_tools = llm_with_tools
        # 独立循环检测状态（结构同 AgentRunner.__init__ 的初始化）
        self._executed_call_sigs = collections.deque(maxlen=8)
        self._last_full_sig = None
        self._full_sig_repeat_count = 0

    # —— 穿透读（属性名与 AgentRunner 一致，构建器节点无差别访问）——

    @property
    def _repeat_terminate_threshold(self):
        return self._runner._repeat_terminate_threshold

    @property
    def _stop_event(self):
        return self._runner._stop_event

    @property
    def _max_retries(self):
        return self._runner._max_retries

    @property
    def _max_output_tokens(self):
        # P1 截断检测的 cap 穿透读 runner（与 ChatOpenAI max_tokens 同源）
        return self._runner._max_output_tokens

    @property
    def token_estimator(self):
        return self._runner.token_estimator

    @property
    def tool_result_truncator(self):
        return self._runner.tool_result_truncator

    @property
    def compactor(self):
        # 子图不做上下文压缩：墙钟/递归预算小，独立短历史无压缩必要；
        # 构建器 compact_node / 溢出恢复对 None 天然短路
        return None

    @property
    def config(self):
        return self._runner.config

    @property
    def _permission_mode(self):
        return self._runner._permission_mode

    @property
    def _code_patterns(self):
        return self._runner._code_patterns

    @property
    def _judge_config(self):
        return self._runner._judge_config

    @property
    def _workspace_root(self):
        return self._runner._workspace_root

    async def _rpc_call(self, tool_call: dict, safety: dict | None = None):
        """子图工具调用：经 runner RPC 汇合主线程，附加 subagent_id 标记。"""
        return await self._runner._rpc_call(
            tool_call, safety=safety, subagent_id=self._task_id
        )


# ===========================================================================
# 编排器
# ===========================================================================

class SubagentOrchestrator:
    """子 agent 编排器：定义热更新 + dispatch_subagents 注入 + 批派发。

    由 AgentRunner.__init__ 构造（持有 runner 引用）；init 的 subagents
    数组与运行期 update_subagents 消息均经 update_definitions() 生效。
    """

    def __init__(self, runner):
        self._runner = runner
        #: 当前定义集（name → SubagentDefinition）；派发时快照，热更新不影响
        #: 运行中任务（Q17）
        self.definitions: dict = {}

    # —— 定义管理 ——

    def update_definitions(self, defs: list):
        """热更新定义集（init / update_subagents），并重建工具绑定。

        重建内容：dispatch_subagents schema（定义集为空则移除）与
        ``llm_with_tools`` 绑定（同 reconfigure 的 bind_tools 路径）。不重建
        图、不动会话状态——图节点 call-time 读取 llm_with_tools 与
        编排器定义（派发时快照），下一轮推理即生效（Q17）。
        """
        new_defs = {}
        for raw in defs or []:
            if not isinstance(raw, dict):
                logger.warning("skipping malformed subagent definition (not a dict): %r", raw)
                continue
            d = SubagentDefinition(raw, self._registered_tool_names())
            if not d.name:
                logger.warning("skipping subagent definition with empty name")
                continue
            if d.name in new_defs:
                logger.warning("duplicate subagent name %r, keeping first", d.name)
                continue
            new_defs[d.name] = d
        self.definitions = new_defs
        logger.info(
            "subagent definitions updated: %d (%s)",
            len(new_defs), ", ".join(new_defs) if new_defs else "-"
        )
        self._runner._rebuild_tool_bindings()

    def _registered_tool_names(self):
        """已注册工具名（C++ 下发的工具规格集，不含 Python 注入的 ask_user）。"""
        return [
            spec.get("name", "")
            for spec in getattr(self._runner, "tool_specs", []) or []
            if isinstance(spec, dict)
        ]

    def build_dispatch_schema(self):
        """生成 dispatch_subagents 工具 schema；定义集为空返回 None（不注入）。

        description 自动拼入当前可用子类型 name + description 清单，
        使父 agent 的 LLM 按清单选择子类型。
        """
        if not self.definitions:
            return None
        lines = [
            "Delegate one or more self-contained research tasks to specialized "
            "subagents that run independently and report back structured "
            "summaries. Subagents cannot ask the user questions. Use this for "
            "parallel exploration/investigation work; keep each task "
            "self-contained (the subagent sees no conversation context).",
            "Available subagent types:",
        ]
        for d in self.definitions.values():
            lines.append(f"- {d.name}: {d.description}".rstrip())
        return {
            "name": "dispatch_subagents",
            "description": "\n".join(lines),
            "parameters": {
                "type": "object",
                "properties": {
                    "tasks": {
                        "type": "array",
                        "description": (
                            "Tasks to delegate in this batch. Keep the batch "
                            "small (a few tasks at most); each task runs "
                            "independently."
                        ),
                        "items": {
                            "type": "object",
                            "properties": {
                                "subagent_type": {
                                    "type": "string",
                                    "description": (
                                        "Subagent type name; must be one of the "
                                        "available types listed above."
                                    ),
                                },
                                "prompt": {
                                    "type": "string",
                                    "description": (
                                        "Complete, self-contained instructions "
                                        "for the subagent (it sees no other "
                                        "context)."
                                    ),
                                },
                            },
                            "required": ["subagent_type", "prompt"],
                        },
                    },
                },
                "required": ["tasks"],
            },
        }

    # —— 配置读取（经 init/reconfigure 下发，Python 只消费）——

    def _cfg_int(self, key: str, default: int, minimum: int = 1) -> int:
        try:
            value = int(self._runner.config.get(key, default))
        except (TypeError, ValueError):
            return default
        return max(minimum, value)

    def _cfg_opt_int(self, key: str, default: int) -> int | None:
        """同 _cfg_int 但允许 ≤0 → None（无限制），用于 recursion_limit。

        与 agent_runner._sanitize_recursion_limit 语义一致：C++ ini 层面用
        -1 表达"用户要求不限制"，langgraph 只接受 ≥1 或 None。
        """
        try:
            value = int(self._runner.config.get(key, default))
        except (TypeError, ValueError):
            return default
        return value if value > 0 else None

    # —— 派发 ——

    async def dispatch(self, args: dict, call_id: str) -> dict:
        """派发一批子任务并等待全部结束，返回聚合结果。

        返回 ``{"tasks": [{task_id, subagent_type,
        status: ok|error|timeout|stopped, summary?, error?}]}``（经
        tool_node json.dumps 后入父图 ToolMessage）。本方法不抛业务异常——
        批级错误（参数非法/超批上限）以 ``{"error": ...}`` 返回让父
        agent 自行纠错；单任务失败映射为对应状态（失败不传染，Q11）。
        """
        runner = self._runner
        # 缺省保守 2/1（审计问题 27 短期动作，与 C++ DAAgentLLMConfig 默认对齐）：
        # 并发会话下 N 进程 × M 子 agent 对同一 LLM 供应商无跨进程配额协调，
        # 缺省收敛降低放大效应；配置显式下发时以配置为准（上限 4/2）
        batch_limit = self._cfg_int("subagent_batch_limit", 2)
        concurrency = self._cfg_int("subagent_max_concurrency", 1)
        timeout_sec = float(self._cfg_int("subagent_timeout_sec", 600))
        # 子图步数上限：≤0（ini 配置 -1）视为无限制（None 传入 langgraph；
        # 显式 -1/0 会被 langgraph ValueError 拒绝，见 agent_runner 的守卫）
        recursion_limit = self._cfg_opt_int("subagent_recursion_limit", 60)

        tasks_arg = args.get("tasks") if isinstance(args, dict) else None
        if not isinstance(tasks_arg, list) or not tasks_arg:
            return {"error": "dispatch_subagents requires a non-empty 'tasks' array "
                             "of {subagent_type, prompt} objects"}
        if len(tasks_arg) > batch_limit:
            return {"error": f"Too many subagent tasks: got {len(tasks_arg)}, "
                             f"but the batch limit is {batch_limit}. Split the work "
                             f"into at most {batch_limit} tasks per call."}

        # 派发时快照——热更新不影响运行中任务（Q17）
        defs_snapshot = dict(self.definitions)
        if not defs_snapshot:
            return {"error": "No subagent definitions are available"}

        planned = []
        for i, item in enumerate(tasks_arg):
            item = item if isinstance(item, dict) else {}
            stype = str(item.get("subagent_type", "") or "").strip()
            prompt = str(item.get("prompt", "") or "")
            task_id = f"{stype or 'unknown'}#{i + 1}"
            planned.append((task_id, stype, prompt, defs_snapshot.get(stype)))

        # 派发开始：全部任务 spawned（进度卡片创建锚点，UI 见 C 计划）
        for task_id, stype, prompt, _def in planned:
            preview = prompt.strip().replace("\n", " ")
            if len(preview) > _SPAWN_PROMPT_PREVIEW_CHARS:
                preview = preview[:_SPAWN_PROMPT_PREVIEW_CHARS] + "..."
            await self._progress(
                call_id, task_id=task_id, subagent=stype,
                state="spawned", message=preview
            )

        usage_totals = {"input_tokens": 0, "output_tokens": 0, "total_tokens": 0}
        sem = asyncio.Semaphore(concurrency)
        heartbeat_stop = asyncio.Event()
        heartbeat_task = asyncio.create_task(
            self._heartbeat_loop(call_id, heartbeat_stop)
        )
        stop_event = runner._stop_event

        async def _guarded(task_id, stype, prompt, definition):
            # 信号量限流（并发 2 默认，Q6）；检查停止放在 _run_one 内部，
            # 使"已 set 则未开始的任务直接标记 stopped"（Q7）
            async with sem:
                return await self._run_one(
                    call_id, task_id, stype, prompt, definition,
                    timeout_sec, recursion_limit, stop_event, usage_totals
                )

        try:
            results = list(await asyncio.gather(
                *[_guarded(*p) for p in planned]
            ))
        finally:
            heartbeat_stop.set()
            heartbeat_task.cancel()

        aggregate = {"tasks": results}
        # 汇总前对总结果体积做兜底（避免父图 ToolMessage 二次截断破坏 JSON）
        total_max = self._cfg_int(
            "tool_result_max_chars", 20000, minimum=2000
        )
        self._cap_total_size(aggregate, total_max)
        # 派发结束：聚合进度（results 携带全部 task_id，C++ 据此按 Q18 撤卡）
        await self._progress(call_id, state="done", results=aggregate)
        # usage 聚合：各子图最终 AIMessage.usage_metadata 求和，单条下发
        await runner.stdio.send_usage(
            usage_totals["input_tokens"],
            usage_totals["output_tokens"],
            usage_totals["total_tokens"],
            source="subagents",
        )
        logger.info(
            "subagent dispatch %s finished: %d task(s), statuses=%s",
            call_id, len(results),
            [f"{t.get('task_id')}={t.get('status')}" for t in results]
        )
        return aggregate

    async def _run_one(self, call_id, task_id, stype, prompt, definition,
                       timeout_sec, recursion_limit, stop_event, usage_totals):
        """运行单个子任务并上报状态；永不抛业务异常（失败不传染，Q11）。

        状态映射：正常结束=ok（附 summary）；异常=error；墙钟超时=timeout；
        用户停止（stop_event 已 set，含未开始即停止的任务）=stopped。
        审批等待计入墙钟（gated 工具 _rpc_call 用长超时等待用户裁决，
        任务级 wait_for 不豁免审批段）。
        """
        if definition is None:
            available = ", ".join(self.definitions) if self.definitions else "(none)"
            err = (f"Unknown subagent type '{stype}'. Available types: {available}")
            await self._progress(
                call_id, task_id=task_id, subagent=stype,
                state="error", message=err
            )
            return {"task_id": task_id, "subagent_type": stype,
                    "status": "error", "error": err}

        # gather 前/信号量等待期间已停止 → 未开始的任务直接 stopped（Q7）
        if stop_event.is_set():
            await self._progress(
                call_id, task_id=task_id, subagent=stype, state="stopped"
            )
            return {"task_id": task_id, "subagent_type": stype, "status": "stopped"}

        await self._progress(
            call_id, task_id=task_id, subagent=stype, state="running"
        )
        try:
            summary, usage = await asyncio.wait_for(
                self._run_subgraph(task_id, definition, prompt, recursion_limit),
                timeout=timeout_sec
            )
        except asyncio.TimeoutError:
            err = f"Task exceeded the wall-clock limit of {timeout_sec:.0f}s"
            logger.warning("subagent task %s timed out after %.0fs", task_id, timeout_sec)
            await self._progress(
                call_id, task_id=task_id, subagent=stype,
                state="timeout", message=err
            )
            return {"task_id": task_id, "subagent_type": stype,
                    "status": "timeout", "error": err}
        except GraphRecursionError:
            err = (f"Task reached the subagent recursion limit "
                   f"({recursion_limit if recursion_limit else 'unlimited'})")
            logger.warning("subagent task %s hit recursion limit %s", task_id, recursion_limit)
            await self._progress(
                call_id, task_id=task_id, subagent=stype,
                state="error", message=err
            )
            return {"task_id": task_id, "subagent_type": stype,
                    "status": "error", "error": err}
        except Exception as e:
            # 用户停止语义：stop_event 已 set（AgentStoppedError /
            # RetryAbortedError / EOF 等停止路径均先 set stop_event）→
            # stopped；其余为真实错误。不依赖异常类身份（编排器与
            # agent_runner 在入口脚本场景分属不同模块实例）。
            if stop_event.is_set():
                await self._progress(
                    call_id, task_id=task_id, subagent=stype, state="stopped"
                )
                return {"task_id": task_id, "subagent_type": stype,
                        "status": "stopped"}
            logger.exception("subagent task %s failed: %s", task_id, e)
            err = str(e)[:_ERROR_MAX_CHARS] or e.__class__.__name__
            await self._progress(
                call_id, task_id=task_id, subagent=stype,
                state="error", message=err
            )
            return {"task_id": task_id, "subagent_type": stype,
                    "status": "error", "error": err}

        summary = (summary or "").strip() or "(subagent returned no summary)"
        if len(summary) > _SUMMARY_MAX_CHARS:
            summary = summary[:_SUMMARY_MAX_CHARS] + "... [truncated]"
        if usage:
            usage_totals["input_tokens"] += int(usage.get("input_tokens", 0) or 0)
            usage_totals["output_tokens"] += int(usage.get("output_tokens", 0) or 0)
            usage_totals["total_tokens"] += int(usage.get("total_tokens", 0) or 0)
        await self._progress(
            call_id, task_id=task_id, subagent=stype, state="done"
        )
        return {"task_id": task_id, "subagent_type": stype,
                "status": "ok", "summary": summary}

    async def _run_subgraph(self, task_id: str, definition: SubagentDefinition,
                            prompt: str, recursion_limit: int):
        """构建并运行一个子图实例，返回 (summary, usage_metadata)。

        独立 MemorySaver + 独立消息历史 + 工具子集绑定（派发时继承当前
        active 模型，Q7）；enable_ask_user=False（Q7）、无 compact、静默
        （stdio=None，Q8）；权限判定块随 tool_node 共享（Q19）。
        """
        runner = self._runner
        # build_agent_graph 必须取"构造 runner 的那个模块实例"中的版本——
        # 入口脚本场景下 agent_runner 以 __main__ 运行，常规 `import
        # agent_runner` 会得到第二个模块实例（节点闭包引用的全局量不一致）；
        # 经 runner 类的 __module__ 索引可避免双实例。
        build_agent_graph = sys.modules[type(runner).__module__].build_agent_graph

        sub_schemas = self._build_sub_tool_schemas(definition)
        if sub_schemas:
            sub_llm = runner.llm.bind_tools(sub_schemas)
        else:
            sub_llm = runner.llm  # 空白名单 = 纯推理（Q5）
        ctx = SubagentRunContext(runner, sub_llm, task_id)
        graph = build_agent_graph(
            ctx,
            system_prompt=self._compose_system_prompt(definition),
            stdio=None,               # 静默图：子转录不进主聊天流（Q8）
            enable_ask_user=False,    # 子图禁止提问（Q7）
            enable_compact=False,     # 子图预算小，无压缩必要
        )
        thread_config = {
            # 每任务独立 thread（MemorySaver 本就按图实例独立，双保险）
            "configurable": {"thread_id": f"subagent_{task_id}"},
            "recursion_limit": recursion_limit,
        }
        logger.info(
            "subagent task %s starting (type=%s, tools=%d, recursion_limit=%s)",
            task_id, definition.name, len(sub_schemas), recursion_limit
        )
        async for _event in graph.astream(
            {"messages": [HumanMessage(content=prompt)]},
            config=thread_config
        ):
            # 子图静默，事件仅用于推进图
            pass
        # 从图终态读取最终回复作为 summary：最后一条无 tool_calls 的 AIMessage
        state = await graph.aget_state(thread_config)
        messages = (state.values or {}).get("messages", []) if state else []
        summary = ""
        usage = None
        for m in reversed(messages):
            if isinstance(m, AIMessage) and not getattr(m, "tool_calls", None):
                summary = m.content if isinstance(m.content, str) else ""
                usage = getattr(m, "usage_metadata", None)
                break
        return summary, usage

    def _build_sub_tool_schemas(self, definition: SubagentDefinition) -> list:
        """白名单子集的工具 schema（子图绑定用）。

        求交已在定义解析时完成；此处按白名单从 C++ 原始规格取子集。
        ask_user（Python 注入）与 dispatch_subagents（编排器注入）不在
        C++ 规格集中，天然缺席（Q6/Q7）。
        """
        allowed = set(definition.tools)
        subset = []
        for spec in getattr(self._runner, "tool_specs", []) or []:
            if isinstance(spec, dict) and spec.get("name") in allowed:
                subset.append({
                    "name": spec["name"],
                    "description": spec.get("description", ""),
                    "parameters": spec.get(
                        "parameters", {"type": "object", "properties": {}}
                    ),
                })
        return subset

    def _compose_system_prompt(self, definition: SubagentDefinition) -> str:
        """组装子图系统提示词（Q10）：固定前导 + md 正文 ± 图表引用 ± 权限约定。"""
        parts = [_SUBAGENT_PREAMBLE]
        body = definition.system_prompt.strip()
        if body:
            parts.append(body)
        toolset = set(definition.tools)
        if toolset & CHART_TOOLS:
            parts.append(_FIGURE_REFERENCE_SECTION)
        # gated 工具（file_write + code_exec 全集，经 init 下发）→ 权限约定
        gated = getattr(self._runner, "_gated_tools", set()) or set()
        if toolset & gated:
            parts.append(_PERMISSION_SECTION)
        return "\n\n".join(parts)

    # —— 进度与心跳 ——

    async def _progress(self, call_id: str, **fields):
        """发送一条 subagent_progress 协议消息（母文档 §7）。"""
        msg = {"type": "subagent_progress", "call_id": call_id}
        msg.update(fields)
        try:
            await self._runner.stdio.send(msg)
        except Exception as e:
            # 进度消息失败不阻塞派发本体（仅影响 UI 卡片/看门狗保活）
            logger.warning("failed to send subagent_progress: %s", e)

    async def _heartbeat_loop(self, call_id: str, stop_evt: asyncio.Event):
        """派发期间每 30s 发一条无 task_id 的 running 态心跳。

        C++ 端 handleJsonLine 对任意协议消息重置无活动计时器，心跳保活
        看门狗（长派发期间无 token 输出，否则会被误判卡死）。审批挂起
        期间 C++ 有 mPendingApprovals 守卫，心跳不会误启计时器。
        """
        try:
            while True:
                try:
                    await asyncio.wait_for(
                        stop_evt.wait(), timeout=_HEARTBEAT_INTERVAL_SEC
                    )
                    return  # stop_evt 已 set，派发结束
                except asyncio.TimeoutError:
                    pass  # 间隔到达，发心跳
                await self._progress(call_id, state="running")
        except asyncio.CancelledError:
            pass

    # —— 体积兜底 ——

    @staticmethod
    def _cap_total_size(aggregate: dict, max_chars: int):
        """兜底控制聚合结果体积，保证 json.dumps 后不超过 max_chars。

        策略（绝不破坏 JSON 结构）：先压缩 error 文案，再阶梯收缩各任务
        summary；极端情况下丢弃全部 summary。单任务 summary 已在 _run_one
        截断到 4000，此处仅防御"任务数 × 4000"越限。
        """
        tasks = aggregate.get("tasks") or []
        if not tasks:
            return
        if len(json.dumps(aggregate, ensure_ascii=False)) <= max_chars:
            return
        for t in tasks:
            if isinstance(t, dict) and isinstance(t.get("error"), str):
                t["error"] = t["error"][:_ERROR_MAX_CHARS]
        for cap in (2000, 1000, 500, 200, 50):
            for t in tasks:
                if isinstance(t, dict) and isinstance(t.get("summary"), str):
                    t["summary"] = t["summary"][:cap]
            if len(json.dumps(aggregate, ensure_ascii=False)) <= max_chars:
                return
        for t in tasks:
            if isinstance(t, dict):
                t.pop("summary", None)
