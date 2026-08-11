#!/usr/bin/env python3
"""Agent 子进程入口脚本。

通过 stdin/stdout JSON Lines 协议与 data-workbench 主进程通信。
接收 init 消息配置 LLM 和工具，接收 user_msg 执行 agent 循环，
通过 stdout 流式输出 token、工具调用请求、用户提问。

重要：stdout 专用于 JSON Lines 协议，所有调试/日志输出必须写入 stderr。
"""

import asyncio
import codecs
import collections
import json
import logging
import os
import sys
import threading

from langchain_core.messages import (
    AIMessage, HumanMessage, SystemMessage, ToolMessage, RemoveMessage
)
from langchain_openai import ChatOpenAI
from langgraph.checkpoint.memory import MemorySaver
from langgraph.graph import StateGraph, MessagesState, START, END
from langgraph.types import interrupt, Command

# GraphRecursionError 在 agent 图达到 recursion_limit 时抛出，需在 main() 中
# 捕获并给出友好提示，而非当作普通异常报错。低版本 langgraph 可能无此类，
# 降级为不捕获（图会直接抛出普通 Exception）。
try:
    from langgraph.errors import GraphRecursionError
    _HAS_RECURSION_ERROR = True
except ImportError:
    _HAS_RECURSION_ERROR = False
    GraphRecursionError = type(None)  # 占位，isinstance 永远不匹配

# 上下文管理模块（token 估算、工具结果截断、自动压缩、溢出恢复）
# 与本文件同目录，直接导入。导入失败时降级为无上下文管理（agent 仍可工作，
# 但长会话可能因 token 超限而报错）
try:
    from context_manager import (
        TokenEstimator, ToolResultTruncator, ContextCompactor,
        is_context_overflow_error, json_to_message
    )
    _HAS_CONTEXT_MANAGER = True
except ImportError:
    _HAS_CONTEXT_MANAGER = False
    logger.warning("context_manager module not available, running without context management")

# 错误分类 + 指数退避重试 wrapper（plan-01 创建的基础设施）
from error_classifier import classify_error, ErrorType
from retry_wrapper import retry_with_backoff, RetryAbortedError

# Pin stdout/stderr to UTF-8 — required because:
# 1. stdout 是 JSON Lines 协议通道（C++ 端 QJsonDocument::fromJson 按 UTF-8 解析）
# 2. Windows 默认为 cp936/cp1252，会破坏中文内容
# 3. newline="\n" 保证行尾一致（不在 Windows 上产生 \r\n）
sys.stdout.reconfigure(encoding="utf-8", newline="\n")
sys.stderr.reconfigure(encoding="utf-8")

# 在导入重模块（langchain_openai 冷启动 ~16s）之前立即发送 booting 心跳。
# C++ 端 DAAgentBridge 收到 booting 后会重置 ready 超时计时器，避免进程
# 在导入期间被 m_process->kill() 误杀——TerminateProcess 不 flush Python
# stderr 块缓冲，会导致日志无任何 stderr 输出、表现为静默崩溃（exitCode=62097）。
# 此处只能用已导入的标准库（json/sys），不能用 langchain（尚未导入）。
sys.stdout.write('{"type": "booting"}\n')
sys.stdout.flush()

# 所有调试输出写入 stderr，绝对禁止写入 stdout（stdout 是协议通道）
logging.basicConfig(
    stream=sys.stderr,
    level=logging.DEBUG,
    format='%(asctime)s [%(levelname)s] %(message)s'
)
logger = logging.getLogger("agent_runner")

# 抑制 openai/httpx 库的 DEBUG/INFO 日志，避免请求体等巨大文本刷屏 stderr。
# C++ 端 onReadyReadStandardError 捕获 stderr 后转发含 "[ERROR]"/"Traceback" 的内容到 UI，
# 若不抑制，httpx 的 DEBUG 日志（含完整 Request options/traceback）会被误当错误弹给用户。
logging.getLogger("openai").setLevel(logging.WARNING)
logging.getLogger("httpx").setLevel(logging.WARNING)


class AgentStoppedError(Exception):
    """用户主动停止 agent 时抛出，不视为错误，不应在对话中显示报错。"""
    pass


def _to_exhausted_error_type(exc: Exception, classification) -> str:
    """将可重试错误映射到 *_exhausted error_type，供 main() 的 send_error 使用。

    优先用 isinstance 精确判断异常类型，退化为 classification.error_type 字符匹配。
    """
    # 优先：isinstance 精确判断
    try:
        import openai
        if isinstance(exc, openai.RateLimitError):
            return ErrorType.RATE_LIMIT_EXHAUSTED
        if isinstance(exc, (openai.APIConnectionError, openai.APITimeoutError)):
            return ErrorType.NETWORK_EXHAUSTED
        if isinstance(exc, openai.InternalServerError):
            return ErrorType.SERVER_ERROR_EXHAUSTED
        if hasattr(exc, 'status_code') and isinstance(exc.status_code, int):
            if 500 <= exc.status_code <= 529:
                return ErrorType.SERVER_ERROR_EXHAUSTED
            if exc.status_code == 429:
                return ErrorType.RATE_LIMIT_EXHAUSTED
    except ImportError:
        pass
    # 兜底：classification.error_type 已是 *_exhausted（plan-01 修复后）
    if classification.error_type in (
        ErrorType.RATE_LIMIT_EXHAUSTED,
        ErrorType.NETWORK_EXHAUSTED,
        ErrorType.SERVER_ERROR_EXHAUSTED,
    ):
        return classification.error_type
    # 最终兜底：字符串匹配
    et = classification.error_type.lower()
    msg = str(exc).lower()
    if "rate" in et or "429" in msg:
        return ErrorType.RATE_LIMIT_EXHAUSTED
    if "network" in et or "connection" in et or "timeout" in msg or "timed out" in msg:
        return ErrorType.NETWORK_EXHAUSTED
    return ErrorType.SERVER_ERROR_EXHAUSTED


class StdioProtocol:
    """stdin/stdout JSON Lines 协议封装。

    stdout 专用于协议消息；所有日志走 stderr。
    stdin 通过后台线程阻塞读取 sys.stdin.buffer，再经 asyncio.Queue
    把字节跨线程投递到事件循环，规避 Windows 上 asyncio pipe transport
    的兼容性问题。增量 UTF-8 解码器处理跨读取边界的多字节字符(strict)。
    """

    def __init__(self):
        self._data_queue: asyncio.Queue = None
        self._stdin_thread: threading.Thread = None
        self._buffer = b""
        self._decoder = codecs.getincrementaldecoder("utf-8")(errors="strict")
        self._loop = None
        self._stop_event = None  # 由 set_stop_event() 注册，用于 stdin 线程即时检测 stop

    async def init_reader(self):
        """初始化 stdin 读取器(后台线程 + asyncio.Queue)。

        Windows 上 asyncio 的 pipe transport 存在不可调和的兼容性问题:
          * ProactorEventLoop (IOCP) 对 QProcess 创建的匿名管道句柄执行
            CreateIoCompletionPort 会失败(WinError 6，句柄无效)
          * SelectorEventLoop 干脆不实现 pipe transport
            (connect_read_pipe 直接抛 NotImplementedError)
        两条 asyncio 路都走不通，改用后台线程阻塞读取 sys.stdin.buffer，
        通过 loop.call_soon_threadsafe + asyncio.Queue 把字节跨线程投递到
        事件循环。此方案不依赖 event loop 类型，Selector/Proactor 均可工作；
        httpx/langchain 的 async socket I/O 不受影响。
        """
        self._loop = asyncio.get_running_loop()
        self._data_queue = asyncio.Queue()

        def _stdin_reader():
            # 后台线程: 阻塞读取 stdin 字节，跨线程投递到事件循环的 Queue。
            # 必须用 read1 而非 read:read(n) 会阻塞等满 n 字节才返回，
            # 对 QProcess 管道(stdin 没有.EOF，数据量小)会永久卡死；
            # read1 只发起一次底层 read()，有数据就立即返回(不论多少字节)。
            #
            # ★ stop 消息即时检测：线程内维护独立行缓冲 scan_buf，
            # 扫描完整行检测 type=="stop"，命中时通过 call_soon_threadsafe
            # 即时设置 stop_event，不等 main() 从 Queue 消费——退避期间
            # main() 阻塞在 runner.run()，无法消费 Queue 中的 stop 消息。
            scan_buf = bytearray()
            try:
                while True:
                    data = sys.stdin.buffer.read1(4096)
                    if not data:
                        # EOF — stdin 关闭，投递 None 作为结束标志
                        self._loop.call_soon_threadsafe(self._data_queue.put_nowait, None)
                        return
                    # ★ 扫描完整行检测 stop 消息，即时设置 stop_event
                    if self._stop_event is not None:
                        scan_buf.extend(data)
                        while b'\n' in scan_buf:
                            line_bytes, scan_buf = scan_buf.split(b'\n', 1)
                            line_bytes = line_bytes.strip()
                            if not line_bytes:
                                continue
                            try:
                                scan_msg = json.loads(line_bytes.decode('utf-8'))
                                if scan_msg.get("type") == "stop":
                                    self._loop.call_soon_threadsafe(self._stop_event.set)
                            except (json.JSONDecodeError, UnicodeDecodeError):
                                pass
                    # 原始字节照常入队，main() 仍通过 receive() 正常处理所有消息
                    self._loop.call_soon_threadsafe(self._data_queue.put_nowait, data)
            except Exception as e:
                logger.exception("stdin reader thread crashed: %s", e)
                # 出错也投递 EOF，避免事件循环永久挂起等待
                self._loop.call_soon_threadsafe(self._data_queue.put_nowait, None)

        # daemon=True: 主进程退出时线程立即终止，不阻塞解释器关闭
        self._stdin_thread = threading.Thread(target=_stdin_reader, daemon=True, name="agent-stdin")
        self._stdin_thread.start()

    # —— 发送（stdout）——
    async def send(self, msg: dict):
        """发送一条 JSON 消息到 stdout。"""
        line = json.dumps(msg, ensure_ascii=False)
        sys.stdout.write(line + "\n")
        sys.stdout.flush()

    async def send_ready(self, model: str):
        await self.send({"type": "ready", "model": model})

    async def send_token(self, content: str):
        await self.send({"type": "token", "content": content})

    async def send_message_end(self, content: str = "", usage=None):
        obj = {"type": "message_end", "content": content}
        if usage:
            obj["usage"] = {
                "input_tokens": usage.get("input_tokens", 0),
                "output_tokens": usage.get("output_tokens", 0),
                "total_tokens": usage.get("total_tokens", 0),
            }
        await self.send(obj)

    async def send_usage(self, input_tokens, output_tokens, total_tokens, source="agent"):
        await self.send({
            "type": "usage",
            "input_tokens": int(input_tokens or 0),
            "output_tokens": int(output_tokens or 0),
            "total_tokens": int(total_tokens or 0),
            "source": source,
        })

    async def send_session_loaded(self, session_id):
        await self.send({"type": "session_loaded", "session_id": session_id})

    async def send_tool_call(self, call_id: str, tool: str, arguments: dict):
        await self.send({
            "type": "tool_call", "call_id": call_id,
            "tool": tool, "arguments": arguments
        })

    async def send_question(self, text: str, options: list[str], multi_select: bool = False):
        await self.send({"type": "question", "text": text, "options": options, "multi_select": multi_select})

    async def send_error(self, message: str, error_type: str = "unknown", detail: str = ""):
        obj = {"type": "error", "message": message, "error_type": error_type}
        if detail:
            obj["detail"] = detail
        await self.send(obj)

    async def send_retrying(self, attempt: int, max_attempts: int, delay_ms: int,
                            error_type: str, error_message: str):
        await self.send({
            "type": "retrying",
            "attempt": attempt,
            "max_attempts": max_attempts,
            "delay_ms": delay_ms,
            "error_type": error_type,
            "error_message": error_message,
        })

    async def send_done(self):
        await self.send({"type": "done"})

    def set_stop_event(self, event: asyncio.Event, loop: asyncio.AbstractEventLoop):
        """注册 stop_event，使 stdin 后台线程在收到 stop 消息时即时设置。

        stdin 线程在读取到 type=="stop" 的消息行时，通过 loop.call_soon_threadsafe
        直接设置 event，不等 main() 从 Queue 消费——退避期间 main() 阻塞在
        runner.run()，无法消费 Queue 中的 stop 消息。
        """
        self._stop_event = event
        self._loop = loop

    # —— 接收（stdin）——
    async def receive(self) -> dict:
        """异步读取一行 JSON(从 asyncio.Queue 取数据，不卡住事件循环)。"""
        while True:
            # 先在缓冲区里找完整的一行
            idx = self._buffer.find(b"\n")
            if idx >= 0:
                line = self._buffer[:idx]
                self._buffer = self._buffer[idx + 1:]
                # 用增量解码器处理跨边界的多字节字符
                text = self._decoder.decode(line)
                return json.loads(text)
            # 从 Queue 等待后台线程投递的数据(可被事件循环中断/取消)
            data = await self._data_queue.get()
            if data is None:
                # EOF — stdin 关闭
                raise EOFError("stdin closed")
            self._buffer += data


class ToolFactory:
    """为 ChatOpenAI.bind_tools() 生成工具 schema。

    直接传递 C++ 端 getToolSpec() 返回的 OpenAI schema 字典给 bind_tools()，
    不做 Pydantic 转换，保留完整的参数描述、enum、嵌套对象等信息。
    无需 StructuredTool / Pydantic 模型——工具在此仅作为 LLM 的 schema，
    实际执行由 tool_node 通过 RPC 回调 C++ 完成（不存在死代码 _execute）。
    """

    @staticmethod
    def build_tool_schemas(tool_specs: list) -> list:
        """将 C++ tool specs 转为 OpenAI function schema 字典列表。

        ChatOpenAI.bind_tools 接受原始 schema 字典，无需 Pydantic 模型。
        额外注入一个 ask_user 工具供 LLM 显式调用以触发 HITL 提问
        （LLM 无法通过 prompt 约定在 additional_kwargs 里产生标记，
        只能通过注册为真实工具让模型以 tool_call 形式调用）。
        """
        schemas = []
        for spec in tool_specs:
            schemas.append({
                "name": spec["name"],
                "description": spec.get("description", ""),
                "parameters": spec.get("parameters", {"type": "object", "properties": {}})
            })
        # 注入 ask_user 工具用于 HITL 提问
        schemas.append({
            "name": "ask_user",
            "description": "Ask the user a question to get clarification or confirmation. Use when additional information from the user is needed to proceed.",
            "parameters": {
                "type": "object",
                "properties": {
                    "question": {
                        "type": "string",
                        "description": "The question to ask the user"
                    },
                    "options": {
                        "type": "array",
                        "items": {"type": "string"},
                        "description": "List of selectable options"
                    },
                    "multi_select": {
                        "type": "boolean",
                        "description": "Whether multi-select is allowed (default false, single-select)",
                        "default": False
                    }
                },
                "required": ["question"]
            }
        })
        return schemas


class AgentRunner:
    """LangGraph agent 运行器。"""

    def __init__(self, config: dict, tool_specs: list[dict],
                 system_prompt: str, stdio: StdioProtocol):
        self.stdio = stdio
        self.system_prompt = system_prompt
        self.config = config

        # 配置 LLM
        self.llm = ChatOpenAI(
            base_url=config["base_url"],
            api_key=config["api_key"],
            model=config["model"],
            streaming=True,
            max_retries=0,                                        # 禁用 openai-python 内置重试，由 wrapper 控制
            timeout=config.get("request_timeout_sec", 120),       # HTTP 请求超时（连接+首字节）
        )

        # 生成工具 schema 并绑定（直接传原始 schema 字典，无 Pydantic 转换）
        self.tool_schemas = ToolFactory.build_tool_schemas(tool_specs)
        self.llm_with_tools = self.llm.bind_tools(self.tool_schemas)

        # 重试配置 + stop_event（plan-02 步骤 2）
        self._max_retries = config.get("max_retries", 7)
        self._stop_event = asyncio.Event()  # 用户 stop 时 set，中断退避
        # StdioProtocol 的 init_reader 后台线程在解析到 stop 消息时直接 set 此 event
        self.stdio.set_stop_event(self._stop_event, asyncio.get_running_loop())

        # —— 重复工具调用检测（防止 LLM 陷入死循环撞 recursion_limit）——
        # 软引导：记录本轮已执行的工具调用签名 (name, args_canonical)，重复时
        # tool_node 返回引导性 ToolMessage 而非重复执行（不同参数不受影响）。
        self._executed_call_sigs = collections.deque(maxlen=8)
        # 硬终止：跟踪连续相同的完整 tool_calls 签名，超阈值则 agent_node
        # 强制剥离 tool_calls 并以最终回复结束（router → END），兜底防止死循环。
        self._last_full_sig: str | None = None
        self._full_sig_repeat_count: int = 0
        # 阈值：连续 3 次相同完整签名触发硬终止
        # （第 1 次正常执行，第 2 次软引导拦截，第 3 次硬终止）
        self._repeat_terminate_threshold: int = 3

        # 上下文管理组件初始化
        # 从 config 读取参数（C++ 端 getLLMConfig 下发，带默认值兜底）
        self.context_window = config.get("context_window", 262144)
        self.compaction_threshold = config.get("compaction_threshold", 0.85)
        self.max_recent_messages = config.get("max_recent_messages", 10)
        tool_result_max_chars = config.get("tool_result_max_chars", 20000)
        tool_result_preview_chars = config.get("tool_result_preview_chars", 2000)

        if _HAS_CONTEXT_MANAGER:
            self.token_estimator = TokenEstimator(config.get("model"))
            self.tool_result_truncator = ToolResultTruncator(
                tool_result_max_chars, tool_result_preview_chars
            )
            self.compactor = ContextCompactor(
                self.llm, self.token_estimator,
                self.context_window, self.compaction_threshold,
                self.max_recent_messages
            )
            logger.info("Context management enabled: window=%d threshold=%.2f "
                        "max_recent=%d tool_max=%d tool_preview=%d",
                        self.context_window, self.compaction_threshold,
                        self.max_recent_messages, tool_result_max_chars,
                        tool_result_preview_chars)
        else:
            self.token_estimator = None
            self.tool_result_truncator = None
            self.compactor = None

        # 构建图（带 checkpointer 以支持 interrupt/resume）
        self.graph = self._build_graph()

        # LangGraph 线程配置（固定 thread_id，配合 MemorySaver 支持
        # interrupt/resume；run 与 resume 共用同一 thread 以保持状态）
        # recursion_limit 限制图的最大迭代步数（compact→agent→tools→compact→...），
        # 防止 agent 陷入工具调用死循环时跑数千步不终止。默认 150 步约支持 50 轮
        # 工具调用，满足数据分析频繁查数据的场景；C++ 端 getLLMConfig 下发此值，
        # 用户可在设置页调整。此外 tool_node 软引导 + agent_node 硬终止提供
        # 智能循环检测兜底，避免仅靠此粗暴上限。
        self._recursion_limit = config.get("recursion_limit", 150)
        self.thread_config = {
            "configurable": {"thread_id": "agent_session_1"},
            "recursion_limit": self._recursion_limit,
        }

    def stop(self):
        """用户请求停止——设置 stop_event 中断退避等待。"""
        self._stop_event.set()

    async def _rpc_call(self, tool_call: dict, timeout: float = 60.0) -> dict:
        """通过 stdin/stdout RPC 调用 C++ 侧工具执行器，带超时。"""
        call_id = tool_call["id"]
        await self.stdio.send_tool_call(call_id, tool_call["name"], tool_call["args"])
        try:
            result = await asyncio.wait_for(
                self._wait_for_result(call_id),
                timeout=timeout
            )
            return result
        except asyncio.TimeoutError:
            return {"error": f"Tool '{tool_call['name']}' timed out after {timeout}s"}

    async def _wait_for_result(self, expected_call_id: str) -> dict:
        """阻塞等待对应 expected_call_id 的 tool_result 消息。

        严格匹配 call_id——避免在乱序或迟到的 tool_result 之间错配
        （当前虽为顺序执行，但显式匹配更健壮）。
        """
        while True:
            msg = await self.stdio.receive()
            msg_type = msg.get("type")
            if msg_type == "tool_result":
                if msg.get("call_id") == expected_call_id:
                    return msg.get("result", {})
                # 属于其它 call_id 的结果——记日志后继续等待本 call_id
                logger.warning(
                    "收到 tool_result call_id=%s，期望 %s，已忽略",
                    msg.get("call_id"), expected_call_id
                )
            elif msg_type == "stop":
                raise AgentStoppedError("Received stop while waiting for tool_result, agent has stopped")
            else:
                logger.warning("期望 tool_result，但收到 type=%s", msg_type)

    def _build_graph(self):
        """构建 LangGraph 图（带 MemorySaver checkpointer 支持 interrupt/resume）。

        图结构：START → compact → agent → {ask_user | tools | END}
                 tools → compact, ask_user → compact
        compact 节点在每次 agent 之前检查并压缩历史（不需要时返回空，零开销）。
        """
        stdio = self.stdio
        system_prompt = self.system_prompt
        llm_with_tools = self.llm_with_tools
        compactor = self.compactor
        truncator = self.tool_result_truncator
        token_estimator = self.token_estimator

        async def _stream_llm(messages):
            """流式调用 LLM 并输出 token 到 UI，返回 (AIMessage, usage_metadata)。

            重试逻辑：仅在首个 token 之前重试（stream_yielded 追踪）。
            首 token 后的错误不重试，直接抛出（D3 决策）。
            error_type 的 exhausted 映射由 main() 的 catch block 负责（步骤 4）。
            """

            stream_yielded = False  # 闭包变量，追踪是否已推送 token

            async def _call_llm():
                """每次调用都是一次完整的 LLM 流式请求。"""
                nonlocal stream_yielded
                collected_chunks = None
                # langchain-openai >= 0.2 经 _should_stream_usage 支持该 kwarg，
                # 更旧版本静默忽略，不影响流式，仅 usage 为 None。
                astream_kwargs = {"stream_options": {"include_usage": True}}

                # 流式开始前：用 tiktoken 估算 input tokens，发初始 usage 让 UI
                # 进度条即时反映上下文占用。真实 usage_metadata 在流结束后由
                # send_message_end 回传覆盖此估算值。
                input_estimate = 0
                if token_estimator:
                    try:
                        input_estimate = token_estimator.count_messages_tokens(messages)
                    except Exception:
                        pass
                if input_estimate > 0:
                    await stdio.send_usage(
                        input_estimate, 0, input_estimate,
                        source="streaming_estimate",
                    )

                chunk_count = 0
                async for chunk in llm_with_tools.astream(messages, **astream_kwargs):
                    if collected_chunks is None:
                        collected_chunks = chunk
                    else:
                        collected_chunks = collected_chunks + chunk  # AIMessageChunk 支持累加
                    if chunk.content:
                        stream_yielded = True
                        await stdio.send_token(chunk.content)
                    # 每 20 个 chunk 发一次估算 usage，让进度条/标签实时增长。
                    # 每个 astream chunk ≈ 1 token（OpenAI 流式逐 token 输出）。
                    chunk_count += 1
                    if chunk_count % 20 == 0:
                        output_estimate = chunk_count
                        total_estimate = input_estimate + output_estimate
                        await stdio.send_usage(
                            input_estimate, output_estimate, total_estimate,
                            source="streaming_estimate",
                        )
                usage = getattr(collected_chunks, 'usage_metadata', None) if collected_chunks else None
                return collected_chunks, usage

            def _should_retry(exc):
                """retryable_check 回调：只有首个 token 之前才允许重试（D3 铁律）。

                retry_with_backoff 在每次捕获异常时调用此函数。
                返回 False 时 retry_with_backoff 直接 raise，不进入退避。
                """
                classification = classify_error(exc)
                if not classification.retryable:
                    return False
                if stream_yielded:
                    return False  # 已推送 token，首 token 后不重试
                return True

            async def _on_retry(attempt, max_retries, delay_ms, classification):
                """退避期间发送 retrying 协议消息。"""
                await stdio.send_retrying(
                    attempt, max_retries, int(delay_ms),
                    classification.error_type, classification.user_message
                )

            # retry_with_backoff 在 _should_retry 返回 False 或重试耗尽时 raise 原始异常。
            # _stream_llm 不做任何 except 处理——异常直接传播到 main() 的 catch block，
            # 由 main() 负责分类和 *_exhausted 映射（步骤 4）。
            return await retry_with_backoff(
                _call_llm,
                max_retries=self._max_retries,
                on_retry=_on_retry,
                stop_event=self._stop_event,
                retryable_check=_should_retry,
            )

        async def compact_node(state: MessagesState):
            """上下文压缩节点：在 agent 之前检查并压缩历史。

            不需要压缩时返回空（{"messages": []}），不影响流程。
            需要压缩时返回 [RemoveMessage(id=...) for middle, HumanMessage(summary)]，
            MessagesState 的 add_messages reducer 会删除中间消息并追加摘要，
            使下轮 agent_node 读到的是压缩后历史。

            3 次熔断：连续失败 3 次后不再尝试（qwen-code 式），成功时重置计数。
            """
            if not compactor:
                return {"messages": []}
            messages = state["messages"]
            if not compactor.should_compact(messages):
                return {"messages": []}
            logger.info("Starting context compaction, current tokens=%d",
                        self.token_estimator.count_messages_tokens(messages))
            try:
                # MAJOR3：compact() 现返回 (updates, summary_usage)
                updates, summary_usage = await compactor.compact(messages)
                compactor._consecutive_failures = 0  # 成功重置
                logger.info("Compaction done, returning %d updates", len(updates))
                # summary 的 usage 经独立 send_usage(source="summary") 回传
                # （summary 无 message_end，只能走独立 usage 消息——MAJOR6）
                if summary_usage:
                    await stdio.send_usage(
                        summary_usage.get("input_tokens", 0),
                        summary_usage.get("output_tokens", 0),
                        summary_usage.get("total_tokens", 0),
                        source="summary",
                    )
                return {"messages": updates}
            except Exception as e:
                compactor._consecutive_failures += 1
                logger.exception("Compaction failed (%d/%d): %s",
                                 compactor._consecutive_failures,
                                 compactor.MAX_FAILURES, e)
                return {"messages": []}  # 失败不压缩，下轮再试

        async def agent_node(state: MessagesState):
            messages = state["messages"]  # 已被 compact_node 处理
            # 在开头插入 system prompt（避免重复插入）
            if system_prompt and not any(m.type == "system" for m in messages):
                messages = [SystemMessage(content=system_prompt)] + messages

            # 溢出恢复时产生的 state 更新（RemoveMessage + summary），
            # 与 final_message 一起返回，让 MessagesState reducer 删除中间消息、
            # 追加 summary，从而打断"400 → force_compact（局部）→ 400"循环。
            compaction_updates = []

            try:
                # 正常路径：流式调用 LLM
                final_message, usage = await _stream_llm(messages)  # 解构
            except Exception as e:
                # 记录异常完整信息（str(e) 对 BadRequestError 含 400 响应体 JSON），
                # 便于在 da_log.log 中诊断 400 的确切原因（LiteLLM 返回的具体错误描述）。
                logger.warning("LLM call failed: %s", e)
                # 反应式溢出恢复（qwen-code 式安全网）：
                # 当 token 估算不准导致实际请求超出上下文窗口时，
                # API 返回 ContextWindowExceededError/BadRequestError。
                # 此时强制压缩并重试一次。与原设计的区别：
                # force_compact 的结果现在写回 state（RemoveMessage + summary），
                # 使下一轮 agent_node 读到的是压缩后历史，不再 400 循环。
                if compactor and is_context_overflow_error(e):
                    logger.warning("Context overflow detected, force-compacting and retrying")
                    # force_compact 返回 (compacted, summary_usage, removed_ids)
                    compacted, summary_usage, removed_ids = await compactor.force_compact(messages)
                    # force_compact 路径的 summary usage 也经独立
                    # send_usage(source="summary") 回传（与 compact_node 正常路径一致）
                    if summary_usage:
                        await stdio.send_usage(
                            summary_usage.get("input_tokens", 0),
                            summary_usage.get("output_tokens", 0),
                            summary_usage.get("total_tokens", 0),
                            source="summary",
                        )
                    # 重新 prepend system prompt（force_compact 的 head 可能保留原有
                    # SystemMessage，需检查避免重复 prepend——与上方正常路径同一防护）
                    if system_prompt and not any(m.type == "system" for m in compacted):
                        compacted = [SystemMessage(content=system_prompt)] + compacted
                    final_message, usage = await _stream_llm(compacted)  # 解构

                    # 构造 state 更新：删除被压缩的中间消息 + 追加 summary。
                    # MessagesState 的 add_messages reducer 会：
                    #   1. 按 RemoveMessage(id=...) 删除中间消息
                    #   2. 追加 summary HumanMessage（da_type=summary）
                    #   3. 追加 final_message（由下方 return 添加）
                    # 结果：state 从 [head][middle...][tail] 变为
                    #       [head][summary][tail][final_message]，大幅缩小。
                    compaction_updates = [RemoveMessage(id=mid) for mid in removed_ids]
                    # 从 compacted 中找到 summary 消息（HumanMessage 含 [Context Summary]）
                    for m in compacted:
                        if (isinstance(m, HumanMessage)
                                and "[Context Summary]" in str(m.content)):
                            compaction_updates.append(m)
                            break
                    # 熔断恢复——force_compact 成功说明压缩仍然有效
                    compactor._consecutive_failures = 0
                else:
                    raise

            # 从累积后的完整消息上读取 tool_calls（不是逐 chunk 读）
            if final_message.tool_calls:
                # —— 硬终止检测：连续相同的完整 tool_calls 签名 ——
                # 当 LLM 连续多次发出完全相同的工具调用组合（name + args 全相同），
                # 说明它陷入了重复循环（软引导已被忽略或未覆盖此情形）。
                # 超阈值时剥离 tool_calls，强制以最终回复结束（router → END），
                # 避免 agent 撞到 recursion_limit 才粗暴报错。
                full_sig = json.dumps(
                    [(tc.get("name", ""), tc.get("args", {}))
                     for tc in final_message.tool_calls],
                    sort_keys=True, ensure_ascii=False,
                )
                if full_sig == self._last_full_sig:
                    self._full_sig_repeat_count += 1
                else:
                    self._full_sig_repeat_count = 1
                    self._last_full_sig = full_sig
                if self._full_sig_repeat_count >= self._repeat_terminate_threshold:
                    logger.warning(
                        "Repeated tool-call signature detected (%d consecutive), "
                        "forcing termination to avoid loop",
                        self._full_sig_repeat_count,
                    )
                    loop_msg = AIMessage(
                        content=(
                            "Detected repeated tool-calling pattern (possible loop). "
                            "This turn has been automatically terminated to avoid "
                            "wasting tokens. The information gathered so far is "
                            "available in the conversation history. To continue, try "
                            "shortening the conversation history, starting a new "
                            "session, or switching to a stronger model."
                        ),
                        id=getattr(final_message, 'id', None),
                    )
                    await stdio.send_message_end(loop_msg.content, None)
                    # 无 tool_calls → _should_ask_user 返回 "end" → END
                    return {"messages": [loop_msg]}
                # 有完整 tool_calls，交给 tool_node 执行
                # （tool_call 路径不发 usage——非最终回复，下一轮 agent 还会再发）
                return {"messages": compaction_updates + [final_message]}
            else:
                # 无 tool_calls——最终回复。agent usage 挂在 message_end 上回传
                # （MAJOR6：不单独发 send_usage(source="agent")，避免 C++ 双发
                # 导致 token 统计翻倍）
                await stdio.send_message_end(
                    final_message.content if isinstance(final_message.content, str) else "",
                    usage,
                )
                return {"messages": compaction_updates + [final_message]}

        async def tool_node(state: MessagesState):
            last_msg = state["messages"][-1]  # AIMessage with tool_calls
            results = []
            for tool_call in last_msg.tool_calls:
                name = tool_call["name"]
                args = tool_call.get("args", {})
                # 软引导：本轮已执行过相同 (name, args) → 返回引导而非重复执行。
                # 不同参数不受影响（签名含 args）；仅拦截本轮内重复，跨轮不累积
                # （_executed_call_sigs 在 run()/resume() 开头已清空）。
                sig = (name, json.dumps(args, sort_keys=True, ensure_ascii=False))
                if sig in self._executed_call_sigs:
                    guidance = (
                        f"Tool '{name}' was already called with the same arguments "
                        f"earlier in this turn. The result is already in the conversation "
                        f"above — re-calling with identical arguments will not produce "
                        f"new information. Do NOT repeat this call. Use the existing result, "
                        f"change your arguments if you need different data, or produce your "
                        f"final answer. If uncertain how to proceed, use the ask_user tool."
                    )
                    results.append(ToolMessage(
                        content=guidance,
                        tool_call_id=tool_call["id"]
                    ))
                    continue
                # RPC 调用 C++ host
                result = await self._rpc_call(tool_call)
                # ToolMessage.content 必须是 str/list，不能是 dict
                content = json.dumps(result, ensure_ascii=False)  # str，而非 dict
                # 工具结果截断：超长结果只保留预览（kimi-code v2 式）
                # 截断后的内容直接进 state，后续轮次受益
                if truncator:
                    content = truncator.truncate(content)
                results.append(ToolMessage(
                    content=content,
                    tool_call_id=tool_call["id"]
                ))
                # 记录已执行签名，供后续重复检测
                self._executed_call_sigs.append(sig)
            return {"messages": results}

        async def ask_user_node(state: MessagesState):
            """通过 interrupt() 中断图执行以向用户提问。

            关键：本节点只负责构造 interrupt 值并暂停图，**不**在此处发送
            question 给 C++——否则 resume 后节点会重新执行导致重复发送。
            question 的实际发送由 _send_question_if_paused() 共享助手在
            检测到 interrupt 状态后做一次（run() 与 resume() 均调用之）。
            """
            messages = state["messages"]
            last_msg = messages[-1]

            # 找到 ask_user 工具调用（同一 AIMessage 可能还含其它 tool_calls）
            ask_call = None
            if hasattr(last_msg, "tool_calls") and last_msg.tool_calls:
                for tc in last_msg.tool_calls:
                    if tc.get("name") == "ask_user":
                        ask_call = tc
                        break

            if not ask_call:
                return {"messages": []}

            args = ask_call.get("args", {})
            question_text = args.get("question", "")
            options = args.get("options", [])
            multi_select = bool(args.get("multi_select", False))

            # 中断图执行——执行暂停于此
            # question 与 options 作为 interrupt 值，run() 通过 aget_state 读取
            user_answer = interrupt({"question": question_text, "options": options, "multi_select": multi_select})

            # 恢复后，user_answer 包含用户的回答
            # 返回 ToolMessage（而非 HumanMessage），keyed 到 tool_call_id，
            # 这样 LLM 能正确把回答与 ask_user 工具调用配对
            return {
                "messages": [
                    ToolMessage(
                        content=str(user_answer),
                        tool_call_id=ask_call["id"]
                    )
                ]
            }

        def _should_ask_user(state: MessagesState) -> str:
            """判断 agent 输出应走提问 / 工具 / 结束。

            路由依据是 AIMessage.tool_calls 中是否含名为 ask_user 的调用
            （而非 additional_kwargs['ask_user']——ChatOpenAI 永远不会
            填充该字段，prompt 也无法让 LLM 产出该标记）。
            """
            messages = state["messages"]
            if not messages:
                return "tools"
            last_msg = messages[-1]
            if hasattr(last_msg, "tool_calls") and last_msg.tool_calls:
                for tc in last_msg.tool_calls:
                    if tc.get("name") == "ask_user":
                        return "ask_user"
                return "tools"  # 其它工具调用
            return "end"  # 无 tool_calls，对话结束

        graph = StateGraph(MessagesState)
        graph.add_node("compact", compact_node)  # 上下文压缩（在 agent 之前）
        graph.add_node("agent", agent_node)
        graph.add_node("tools", tool_node)
        graph.add_node("ask_user", ask_user_node)
        graph.add_edge(START, "compact")           # START → compact → agent
        graph.add_edge("compact", "agent")
        # agent 条件分支：提问 / 工具 / 结束
        graph.add_conditional_edges(
            "agent", _should_ask_user,
            {"ask_user": "ask_user", "tools": "tools", "end": END}
        )
        graph.add_edge("tools", "compact")        # tools → compact → agent
        # ask_user 之后回到 compact 再到 agent 继续处理用户的回答
        graph.add_edge("ask_user", "compact")      # ask_user → compact → agent

        # 必须带 checkpointer，否则 interrupt() 无法工作
        memory = MemorySaver()
        return graph.compile(checkpointer=memory)

    async def _send_question_if_paused(self) -> bool:
        """检查图是否在 interrupt 处暂停。若是，发送一次 question 并返回 True。

        run() 与 resume() 共用本方法：若图暂停于 ask_user 的 interrupt，
        则通过 aget_state 读取 interrupt 值（question + options），把
        question 发送给 C++ **一次**（绝不在 ask_user_node 内发送，否则
        resume 后节点重新执行会重复发送），并返回 True（调用方据此跳过
        send_done）；若图未暂停（已正常结束），返回 False（调用方应发 done）。
        """
        state = await self.graph.aget_state(self.thread_config)
        if state.next:  # 图已暂停（被 interrupt 阻断）
            for task in state.tasks:
                if hasattr(task, "interrupts") and task.interrupts:
                    interrupt_value = task.interrupts[0].value
                    question = interrupt_value.get("question", "")
                    options = interrupt_value.get("options", [])
                    multi_select = bool(interrupt_value.get("multi_select", False))
                    # 只发送一次 question（节点 resume 后不会重发，避免重复）
                    await self.stdio.send_question(question, options, multi_select)
                    return True  # 暂停中——调用方不应发送 done
        return False  # 图未暂停，调用方应发送 done

    async def run(self, user_message: str):
        """执行一轮 agent 对话（带 thread_id 以支持 interrupt/resume）。

        agent_node 内部已通过 llm.astream 流式输出 token，本方法只需
        驱动图执行。若图在 ask_user_node 处因 interrupt() 暂停，则经
        _send_question_if_paused() 读取 interrupt 值（question + options）
        并发送一次 question，且**不**发送 done（等待 user_answer 触发
        resume）；若图正常结束，发送 done。
        """
        self._stop_event.clear()  # 每轮开始时重置，确保上一轮的 stop 不影响本轮
        # 每轮开始重置循环检测状态（跨轮不累积——上轮的签名不应影响本轮判断）
        self._executed_call_sigs.clear()
        self._last_full_sig = None
        self._full_sig_repeat_count = 0
        # 仅传入新增的 HumanMessage——MemorySaver checkpointer 会维护完整历史
        async for _event in self.graph.astream(
            {"messages": [HumanMessage(user_message)]},
            config=self.thread_config
        ):
            # agent_node 内部已流式输出 token，这里仅消费事件以推进图
            pass

        # 检查图是否在 interrupt 处暂停；若暂停则发送一次 question 且不发 done
        if not await self._send_question_if_paused():
            # 图正常完成（agent_node 已在无 tool_calls 时发送 message_end）
            await self.stdio.send_done()

    async def resume(self, answer: str):
        """从 interrupt 恢复图执行（收到 user_answer 后调用）。

        使用 Command(resume=answer) 与原 thread_config 恢复同一图实例，
        ask_user_node 返回 ToolMessage 后图继续执行至 agent 产出最终回复。

        恢复后图可能在后续 ask_user 处再次 interrupt（多轮 HITL 提问），
        故同样经 _send_question_if_paused() 判断：暂停则发送一次 question
        且**不**发 done（等待下一次 user_answer），正常结束才发送 done。
        """
        # 恢复执行视为新一轮，重置循环检测状态
        self._executed_call_sigs.clear()
        self._last_full_sig = None
        self._full_sig_repeat_count = 0
        async for _event in self.graph.astream(
            Command(resume=answer),
            config=self.thread_config
        ):
            # agent_node 内部已流式输出 token，这里仅消费事件
            pass
        if not await self._send_question_if_paused():
            await self.stdio.send_done()

    async def load_session(self, session_id: str, messages_json: list):
        """切换会话：重建图 + 注入历史 state。

        每次 load_session 重建 graph（_build_graph 内 compile 开销可忽略），
        避免旧 MemorySaver thread state 污染。self.llm / self.llm_with_tools
        客户端复用不重建（LLM 配置不变）。aupdate_state 用 add_messages
        reducer，对空 thread 即"设置"，对已有 thread 即"追加"——重建图保证
        空 thread，所以等价于设置。

        messages_json 是 C++ 下发的对话消息数组（user/assistant/tool_result），
        按 JSONL 时序排列。一期不含 summary（对齐总纲 T9：一期不持久化
        compact 产生的 summary，JSONL 完整 append-only 保留对话消息；
        load_session 下发全量历史后由 compact_node 在下一轮 agent 前自动
        重新压缩，零额外改动）。C++ 负责过滤掉 usage 记录（不参与 state）。

        空 messages_json 合法（新建会话或空会话），重建空图，发 session_loaded。
        逐条 try/except 跳过坏记录并 logger.warning（到 stderr，T1），不崩溃；
        最终正常发 session_loaded(session_id)（一期签名无 error 字段，坏记录
        的影响由 C++ 侧 plan-03 在读取时已容错）。

        load_session 不发 done（重建 state 不是一轮对话）；session_loaded
        由本方法内部发送。
        """
        self.thread_config = {
            "configurable": {"thread_id": session_id},
            "recursion_limit": self._recursion_limit,
        }
        self.graph = self._build_graph()  # 新 MemorySaver，丢弃旧 thread state
        msgs = []
        if _HAS_CONTEXT_MANAGER:
            for m in messages_json or []:
                try:
                    msgs.append(json_to_message(m))
                except Exception as e:
                    logger.warning(
                        "Skipping bad message record during load_session: %s", e
                    )
        elif messages_json:
            logger.warning(
                "load_session received %d messages but context_manager "
                "unavailable, loading empty state",
                len(messages_json),
            )
        # 修复悬空 tool_call：若 AIMessage 含 tool_calls 但后续无配对的
        # ToolMessage（如进程在写 tool_call 后、写 tool_result 前崩溃），
        # OpenAI API 会返回 400 Bad Request。为每个未满足的 tool_call_id
        # 插入占位 ToolMessage，使消息序列满足 API 要求。
        if msgs:
            satisfied_ids = set()
            for m in msgs:
                tcid = getattr(m, 'tool_call_id', None)
                if tcid:
                    satisfied_ids.add(tcid)
            fixed = []
            for m in msgs:
                fixed.append(m)
                if hasattr(m, 'tool_calls') and m.tool_calls:
                    for tc in m.tool_calls:
                        tc_id = tc.get('id', '') if isinstance(tc, dict) else getattr(tc, 'id', '')
                        if tc_id and tc_id not in satisfied_ids:
                            fixed.append(ToolMessage(
                                content="Tool execution was interrupted, result unavailable.",
                                tool_call_id=tc_id,
                            ))
                            logger.warning(
                                "Inserted placeholder ToolMessage for "
                                "dangling tool_call_id=%s", tc_id,
                            )
            msgs = fixed
        if msgs:
            await self.graph.aupdate_state(self.thread_config, {"messages": msgs})
        await self.stdio.send_session_loaded(session_id)


async def main():
    stdio = StdioProtocol()
    await stdio.init_reader()  # 初始化异步 stdin 读取器

    # 1. 等待 init 消息
    init_msg = await stdio.receive()
    if not isinstance(init_msg, dict) or init_msg.get("type") != "init":
        # 不回显原始 init_msg——若 C++ 误发含 api_key 的 init，repr 会把密钥泄漏到 UI
        await stdio.send_error("Expected init message (type='init'), got invalid message")
        return

    # 显式校验 init 字段（dict.get(key, default) 不会抛 KeyError，
    # 旧版 try/except 毫无意义；真正的异常来自 AgentRunner.__init__ 的下标访问）
    config = init_msg.get("config")
    if not isinstance(config, dict):
        #cn:init 消息缺少 config 字段或格式错误
        await stdio.send_error("init message missing 'config' field or invalid format")
        return

    tools = init_msg.get("tools", [])
    system_prompt = init_msg.get("system_prompt", "")

    if not config.get("base_url") or not config.get("api_key") or not config.get("model"):
        #cn:config 缺少 base_url/api_key/model
        await stdio.send_error("config missing base_url/api_key/model")
        return

    # 2. 创建 agent（真正可能抛异常的地方——下标访问 / 网络初始化）
    try:
        runner = AgentRunner(config, tools, system_prompt, stdio)
    except Exception as e:
        logger.exception("Agent init failed")
        await stdio.send_error(f"Agent init failed: {e}")
        return

    await stdio.send_ready(config.get("model", ""))

    # 3. 主循环：等待用户消息 / user_answer 恢复 / stop
    while True:
        msg = await stdio.receive()
        msg_type = msg.get("type")
        if msg_type == "user_msg":
            try:
                await runner.run(msg["content"])
            except (AgentStoppedError, RetryAbortedError):
                logger.info("Agent stopped by user during run")
                await stdio.send_done()
            except GraphRecursionError:
                #cn:Agent 达到最大推理轮次限制，可能陷入循环。请尝试缩短对话历史或新建会话。
                logger.warning("GraphRecursionError: agent reached recursion limit (%d)", runner._recursion_limit)
                await stdio.send_error(
                    "Agent reached maximum reasoning iterations (possible infinite loop). "
                    "Try shortening the conversation history or starting a new session.",
                    error_type="recursion_limit",
                )
                await stdio.send_done()
            except Exception as e:
                logger.exception("Agent error")
                classification = classify_error(e)
                if classification.retryable:
                    error_type = _to_exhausted_error_type(e, classification)
                else:
                    error_type = classification.error_type
                await stdio.send_error(
                    classification.user_message or str(e),
                    error_type=error_type,
                    detail=classification.detail,
                )
                await stdio.send_done()
        elif msg_type == "user_answer":
            # 从 interrupt 恢复图执行
            answer = msg.get("answer", "")
            try:
                await runner.resume(answer)
            except (AgentStoppedError, RetryAbortedError):
                logger.info("Agent stopped by user during resume")
                await stdio.send_done()
            except GraphRecursionError:
                logger.warning("GraphRecursionError during resume: reached recursion limit (%d)", runner._recursion_limit)
                await stdio.send_error(
                    "Agent reached maximum reasoning iterations (possible infinite loop). "
                    "Try shortening the conversation history or starting a new session.",
                    error_type="recursion_limit",
                )
                await stdio.send_done()
            except Exception as e:
                logger.exception("Agent resume error")
                classification = classify_error(e)
                if classification.retryable:
                    error_type = _to_exhausted_error_type(e, classification)
                else:
                    error_type = classification.error_type
                await stdio.send_error(
                    classification.user_message or str(e),
                    error_type=error_type,
                    detail=classification.detail,
                )
                await stdio.send_done()
        elif msg_type == "tool_result":
            # 超时后迟到的 tool_result，或已被 _wait_for_result 消费——记日志后忽略
            logger.warning(
                "收到迟到的 tool_result call_id=%s，已超时或被处理，忽略",
                msg.get("call_id")
            )
        elif msg_type == "load_session":
            # C++ -> Python 下发历史 messages 重建 langgraph state（多会话切换）。
            # load_session 不发 done（重建 state 不是一轮对话）；session_loaded
            # 由 load_session 内部发送。空 messages 合法；坏记录逐条跳过。
            #
            # try/except 兜底：若历史消息格式异常（如悬空 tool_call 导致
            # aupdate_state 抛异常），异常逃逸 while 循环会导致 Python 进程崩溃，
            # 触发 C++ 崩溃恢复 → load_session（同一份坏历史）→ 再崩 → 循环 3 次。
            # 捕获后发 error + done，让 C++ 侧知道加载失败，进程保持存活。
            sid = msg.get("session_id", "")
            msgs = msg.get("messages", [])
            try:
                await runner.load_session(sid, msgs)
            except Exception as e:
                logger.exception("load_session failed")
                await stdio.send_error(
                    f"Failed to load session: {e}",
                    error_type="session_load_failed",
                )
                await stdio.send_done()
        elif msg_type == "stop":
            break


if __name__ == "__main__":
    # 保留 WindowsSelectorEventLoopPolicy: stdin 读取已改用后台线程(见
    # StdioProtocol.init_reader)，不再依赖 asyncio pipe transport，因此
    # event loop 类型对 stdin 读取无影响。但 SelectorEventLoop 仍优于
    # ProactorEventLoop:
    #   1. 避免 ProactorEventLoop 的 _empty_waiter bug(cpython#103631，
    #      3.12 已修复，但 3.11 仍存在，会导致二次崩溃掩盖原始错误)
    #   2. httpx/langchain 的 async I/O 走 socket，SelectorEventLoop 完整支持
    #   3. 避免 ProactorEventLoop 对 QProcess 管道句柄的 IOCP WinError 6 问题
    if sys.platform == "win32":
        asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
    try:
        asyncio.run(main())
    finally:
        # Safety net: close stdin fd 0 to unblock the reader daemon thread.
        # C++ side calls QProcess::closeWriteChannel() which causes EOF on
        # read1(), but this covers edge cases (e.g. process killed without
        # graceful stop). Without this, the daemon thread holds the
        # BufferedReader lock during interpreter finalization, causing
        # _enter_buffered_busy fatal error.
        try:
            os.close(0)
        except Exception:
            pass
        for t in threading.enumerate():
            if t.name == "agent-stdin" and t.is_alive():
                t.join(timeout=2.0)
                break
