"""上下文管理模块：token 估算、工具结果截断、自动压缩、溢出恢复。

借鉴 qwen-code 和 kimi-code 的上下文管理策略：
- head/tail 保留 + 中间摘要（kimi-code 式）
- 工具结果截断 + 预览（kimi-code v2 式）
- 反应式溢出恢复（qwen-code 式安全网）
- 3 次熔断 + 自动恢复（qwen-code 式）

防线层次：
1. Token 估算（tiktoken + char-based 降级）— 触发判断用
2. 工具结果截断（tool_node 中截断超长结果）
3. 自动压缩（compact_node 中 head/tail + 中间摘要）
4. 反应式溢出恢复（agent_node 捕获 ContextWindowExceededError）

所有日志写入 stderr（stdout 是协议通道，禁止污染）。
"""

import json
import logging

from langchain_core.messages import (
    AIMessage, HumanMessage, RemoveMessage, SystemMessage, ToolMessage
)

logger = logging.getLogger("agent_runner")

# 尝试导入 tiktoken（langchain-openai 通常已间接依赖）
# tiktoken 提供 OpenAI 模型的精确 token 计数；对非 OpenAI 模型（如 deepseek）
# 用 cl100k_base 作为近似——只用于"提前触发"判断，偏向早触发更安全
try:
    import tiktoken
    _HAS_TIKTOKEN = True
except ImportError:
    _HAS_TIKTOKEN = False
    logger.warning("tiktoken not available, using char-based token estimation")

# 摘要提示词：第一人称交接笔记（参考 kimi-code compaction-instruction.md）
COMPACTION_INSTRUCTION = """你是一个上下文压缩助手。请将以下对话历史总结为一份简洁但完整的"交接笔记"。

要求：
1. 用第一人称（"我"）撰写
2. 保留以下关键信息：
   - 用户的原始请求和意图
   - 已完成的工作（精确到文件名、命令、关键结果）
   - 仍未知的信息（未读文件、未验证的假设、用户未回答的问题）
   - 下一步计划
3. 不要逐字转录对话，要提炼要点
4. 对未验证的"已完成"声明要诚实标注
5. 用中文撰写，不超过 2000 字

对话历史：
{history}"""


class TokenEstimator:
    """Token 估算器。

    优先用 tiktoken 精确计数，降级为 char-based 估算。
    参考 qwen-code/kimi-code：两者都不用 tiktoken，用 char/4 估算。
    但本项目使用 langchain-openai，tiktoken 通常已安装，可提供更精确的计数。
    估算只用于"提前触发"判断，偏向早触发更安全（宁可早压缩不要溢出）。
    """

    CHARS_PER_TOKEN_ASCII = 4   # ASCII: ~4 chars/token
    CJK_CHARS_PER_TOKEN = 1.0   # CJK: ~1 char/token（中文场景更贴合）
    ROLE_OVERHEAD = 4            # 每条消息的角色标记开销

    def __init__(self, model_name: str = None):
        self._model_name = model_name
        self._encoder = None
        if _HAS_TIKTOKEN and model_name:
            try:
                self._encoder = tiktoken.encoding_for_model(model_name)
            except Exception:
                try:
                    # cl100k_base 是 GPT-4/多数 OpenAI 模型的编码器
                    # 对非 OpenAI 模型（deepseek 等）作为近似估算
                    self._encoder = tiktoken.get_encoding("cl100k_base")
                except Exception:
                    self._encoder = None
                    logger.warning("Failed to initialize tiktoken encoder, using char-based estimation")

    def count_text_tokens(self, text: str) -> int:
        """估算文本的 token 数。"""
        if not text:
            return 0
        if self._encoder:
            try:
                return len(self._encoder.encode(text))
            except Exception:
                pass  # 编码失败时降级
        # char-based 估算：ASCII ~4 chars/token，CJK ~1 char/token
        ascii_count = sum(1 for c in text if ord(c) < 128)
        non_ascii_count = len(text) - ascii_count
        return max(1, int(ascii_count / self.CHARS_PER_TOKEN_ASCII + non_ascii_count * self.CJK_CHARS_PER_TOKEN))

    def count_messages_tokens(self, messages: list) -> int:
        """估算消息列表的总 token 数。

        包含每条消息的角色开销（~4 token/消息）和 tool_calls args 的开销。
        """
        total = 0
        for msg in messages:
            content = msg.content if hasattr(msg, 'content') else str(msg)
            if isinstance(content, str):
                total += self.count_text_tokens(content)
            elif isinstance(content, list):
                # 多模态内容（list of parts）
                for part in content:
                    if isinstance(part, dict):
                        total += self.count_text_tokens(json.dumps(part, ensure_ascii=False))
                    else:
                        total += self.count_text_tokens(str(part))
            # 角色标记开销
            total += self.ROLE_OVERHEAD
            # tool_calls args 的开销
            if hasattr(msg, 'tool_calls') and msg.tool_calls:
                for tc in msg.tool_calls:
                    args = tc.get('args', {}) if isinstance(tc, dict) else getattr(tc, 'args', {})
                    total += self.count_text_tokens(json.dumps(args, ensure_ascii=False))
                    total += self.ROLE_OVERHEAD  # tool_call 标记开销
        return total


def _tool_call_to_dict(tc) -> dict:
    """把 langchain ToolCall（dict 或 namedtuple）统一拍平为纯 dict。"""
    if isinstance(tc, dict):
        return {
            "name": tc.get("name", ""),
            "args": tc.get("args", {}),
            "id": tc.get("id", ""),
        }
    # 兼容旧版 langchain 返回 namedtuple 的情况
    return {
        "name": getattr(tc, "name", ""),
        "args": getattr(tc, "args", {}),
        "id": getattr(tc, "id", ""),
    }


def message_to_json(msg) -> dict:
    """langchain BaseMessage -> JSON dict（用于持久化与 load_session 下发）。

    返回 {"role": "human"|"ai"|"tool"|"system", "content": str,
          "tool_calls": [...]?, "tool_call_id": "..."?, "id": "..."?,
          "additional_kwargs": {...}?}

    字段对齐总纲 T6 的 message 子对象。usage_metadata 不进本字段
    （单独存 record 的 usage_metadata，由 C++ 处理），但
    additional_kwargs.da_type="summary" 必须保留以便 C++ 识别 summary 类型。
    """
    # msg.type 映射 role: human/ai/tool/system
    obj = {"role": msg.type, "content": msg.content}

    # tool_calls: ai only（list of {"name","args","id"}）
    tool_calls = getattr(msg, "tool_calls", None)
    if tool_calls:
        obj["tool_calls"] = [_tool_call_to_dict(tc) for tc in tool_calls]

    # tool_call_id: tool only
    tool_call_id = getattr(msg, "tool_call_id", None)
    if tool_call_id is not None:
        obj["tool_call_id"] = tool_call_id

    # id
    msg_id = getattr(msg, "id", None)
    if msg_id is not None:
        obj["id"] = msg_id

    # additional_kwargs（含 da_type=summary 标记）
    additional_kwargs = getattr(msg, "additional_kwargs", None)
    if additional_kwargs:
        obj["additional_kwargs"] = additional_kwargs

    return obj


def json_to_message(d: dict):
    """JSON dict -> langchain BaseMessage。role->class 映射：
    human->HumanMessage, ai->AIMessage, tool->ToolMessage, system->SystemMessage。
    还原 content/tool_calls/tool_call_id/id/additional_kwargs。

    注意 ToolMessage 构造需 tool_call_id 参数；ToolMessage.content 必须 str（T6）。
    """
    role = d.get("role", "human")
    content = d.get("content", "")
    # ToolMessage.content 必须 str（T6 铁律）；防御性归一化所有 role 的 content
    if not isinstance(content, str):
        content = json.dumps(content, ensure_ascii=False)

    additional_kwargs = d.get("additional_kwargs") or {}
    msg_id = d.get("id")

    if role == "ai":
        tool_calls = d.get("tool_calls") or []
        return AIMessage(
            content=content,
            tool_calls=tool_calls,
            id=msg_id,
            additional_kwargs=additional_kwargs,
        )
    if role == "tool":
        tool_call_id = d.get("tool_call_id", "")
        return ToolMessage(
            content=content,
            tool_call_id=tool_call_id,
            id=msg_id,
            additional_kwargs=additional_kwargs,
        )
    if role == "system":
        return SystemMessage(
            content=content,
            id=msg_id,
            additional_kwargs=additional_kwargs,
        )
    if role != "human":
        logger.warning(
            "Unknown role '%s' in json_to_message, falling back to HumanMessage",
            role,
        )
    return HumanMessage(
        content=content,
        id=msg_id,
        additional_kwargs=additional_kwargs,
    )


class ToolResultTruncator:
    """工具结果截断器。

    超过阈值的工具结果只保留前 N 字符预览 + 占位提示，
    告知模型可重新请求完整数据。
    参考 kimi-code v2 toolResultTruncationService。
    """

    def __init__(self, max_chars: int = 50000, preview_chars: int = 2000):
        self._max_chars = max_chars
        self._preview_chars = preview_chars

    def truncate(self, content: str) -> str:
        """截断超长工具结果。

        不超过阈值时原样返回；超过时返回预览 + 提示。
        """
        if not content or len(content) <= self._max_chars:
            return content
        preview = content[:self._preview_chars]
        return (
            preview
            + f"\n\n[Tool output exceeded {self._max_chars} characters. "
            f"Showing first {self._preview_chars} characters as preview. "
            f"Re-run the tool with different parameters (e.g. limit rows/columns) "
            f"if you need the full data.]"
        )


class ContextCompactor:
    """上下文压缩器：head/tail 保留 + 中间摘要。

    借鉴 kimi-code 的 compaction 策略：
    - head：保留最早的 1-2 轮对话（原始任务声明）
    - tail：保留最近 N 条消息（近期上下文）
    - middle：head 和 tail 之间的历史，用 LLM 生成"交接笔记"摘要
    - 压缩后历史 = [head] + [摘要] + [tail]

    借鉴 qwen-code 的安全机制：
    - 3 次熔断：连续失败 3 次后不再尝试压缩
    - force 模式：溢出恢复时跳过熔断
    """

    HEAD_TOKEN_BUDGET = 2000     # head 的 token 预算
    TAIL_TOKEN_BUDGET = 18000    # tail 的 token 预算
    SUMMARY_MAX_TOKENS = 2000    # 摘要输出上限
    MAX_HISTORY_CHARS_FOR_SUMMARY = 200000  # 摘要输入的字符上限（防摘要请求自身溢出）
    MAX_SINGLE_MSG_CHARS = 5000  # 摘要输入中单条消息的字符上限
    MAX_FAILURES = 3             # 熔断阈值

    def __init__(self, llm, token_estimator: TokenEstimator,
                 context_window: int, threshold: float, max_recent_messages: int):
        """
        @param llm ChatOpenAI 实例（不绑 tools，用于摘要生成）
        @param token_estimator token 估算器
        @param context_window 模型上下文窗口大小（tokens）
        @param threshold 压缩触发比例（0-1）
        @param max_recent_messages 压缩后保留最近消息条数
        """
        self._llm = llm
        self._token_estimator = token_estimator
        self._context_window = context_window
        self._threshold = threshold
        self._max_recent_messages = max_recent_messages
        self._consecutive_failures = 0

    def should_compact(self, messages: list) -> bool:
        """判断是否需要压缩。"""
        if self._consecutive_failures >= self.MAX_FAILURES:
            logger.warning("Compaction circuit breaker tripped (%d failures), skipping",
                           self._consecutive_failures)
            return False
        if len(messages) < 4:
            return False  # 太少不压缩
        total_tokens = self._token_estimator.count_messages_tokens(messages)
        threshold_tokens = int(self._context_window * self._threshold)
        should = total_tokens >= threshold_tokens
        if should:
            logger.info("Compaction triggered: %d tokens >= %d threshold (%.0f%% of %d window)",
                        total_tokens, threshold_tokens,
                        self._threshold * 100, self._context_window)
        return should

    async def compact(self, messages: list) -> tuple:
        """执行压缩，返回 (state 更新消息列表, summary_usage)。

        用于 compact_node 的 return：删除中间消息 + 添加摘要。
        压缩后 state messages 变成 [head] + [摘要] + [tail]。

        @return (updates, usage)：
            updates = [RemoveMessage(id=...) for middle, HumanMessage(summary)]
                （summary 带 additional_kwargs={"da_type":"summary"} 标记，
                仅为前向兼容——一期 C++ 不读该字段、不写 summary 记录，
                二期持久化 summary 时 C++ 用其识别 type=summary 记录并跳过
                已压缩消息，对齐总纲 T9）
            usage = summary 的 usage_metadata（dict 或 None）
        """
        head_end = self._find_head_end(messages)
        tail_start = self._find_tail_start(messages, head_end)

        if tail_start <= head_end:
            logger.info("Head and tail overlap, nothing to compact")
            return [], None

        middle = messages[head_end:tail_start]
        if not middle:
            logger.info("No middle messages to compact")
            return [], None

        # 生成摘要（CRITICAL2：解构 _generate_summary 的 (summary, usage) 返回）
        summary, usage = await self._generate_summary(middle)

        # 构建 RemoveMessage 列表（只删除有 id 的消息）
        removals = []
        for msg in middle:
            msg_id = getattr(msg, 'id', None)
            if msg_id:
                removals.append(RemoveMessage(id=msg_id))

        # 摘要消息（HumanMessage 角色，带前缀标记 + da_type 标记）。
        # 仅 compact() 的 summary（会写回 state）加 da_type 标记；
        # force_compact() 的 summary 不加（产物仅本地用，见步骤6）。
        summary_msg = HumanMessage(
            content=f"[Context Summary]\n{summary}",
            additional_kwargs={"da_type": "summary"},
        )

        logger.info("Compaction plan: removing %d middle messages, adding summary "
                    "(head=%d, tail=%d, total=%d)",
                    len(removals), head_end, len(messages) - tail_start, len(messages))

        return removals + [summary_msg], usage

    async def force_compact(self, messages: list) -> tuple:
        """强制压缩，返回 (压缩后消息列表, summary_usage, removed_ids)。

        溢出恢复用：跳过 should_compact 和熔断检查。
        如果 LLM 摘要失败，降级为简单截断（只保留 tail）。

        @return (compacted, usage, removed_ids)：
            compacted = [head] + [摘要] + [tail]（用于本地 LLM 重试）
            usage = summary 的 usage_metadata（dict 或 None；
            降级路径无 _generate_summary，返回 None）
            removed_ids = 被移除的中间消息 ID 列表（供 agent_node 构造
            RemoveMessage 写回 state，打断"400 → force_compact → 400"循环）

        force_compact 的 summary 加 da_type 标记——产物现在会写回 state
        （agent_node 返回 RemoveMessage + summary + final_message），故需标记。

        注意：head 取自 messages[:head_end]，若 messages 开头含 SystemMessage
        （agent_node 正常路径已 prepend），则 head 会保留它。调用方
        （agent_node 溢出恢复路径）在重新 prepend system prompt 时已有
        not-any(m.type=="system") 防护，避免重复。
        """
        try:
            head_end = self._find_head_end(messages)
            tail_start = self._find_tail_start(messages, head_end)

            if tail_start <= head_end:
                # 无法分割，只保留 tail（降级路径无 usage）
                tail_start = max(0, len(messages) - self._max_recent_messages)
                while tail_start < len(messages) and messages[tail_start].type != "human":
                    tail_start += 1
                removed_ids = [
                    getattr(m, 'id', None)
                    for m in messages[:tail_start]
                    if getattr(m, 'id', None)
                ]
                return list(messages[tail_start:]), None, removed_ids

            head = list(messages[:head_end])
            middle = messages[head_end:tail_start]
            tail = list(messages[tail_start:])

            # CRITICAL2：解构 _generate_summary 的 (summary, usage) 返回
            summary, usage = await self._generate_summary(middle)
            # summary 加 da_type 标记（现在会写回 state，与 compact() 一致）
            summary_msg = HumanMessage(
                content=f"[Context Summary]\n{summary}",
                additional_kwargs={"da_type": "summary"},
            )
            # 收集被移除的中间消息 ID（供 agent_node 构造 RemoveMessage）
            removed_ids = [
                getattr(m, 'id', None)
                for m in middle
                if getattr(m, 'id', None)
            ]

            return head + [summary_msg] + tail, usage, removed_ids
        except Exception as e:
            logger.exception("Force compact failed, falling back to simple truncation: %s", e)
            # 降级：只保留 tail（简单截断）；降级路径无 usage
            tail_start = self._find_tail_start(messages, 0)
            removed_ids = [
                getattr(m, 'id', None)
                for m in messages[:tail_start]
                if getattr(m, 'id', None)
            ]
            return list(messages[tail_start:]), None, removed_ids

    def _find_head_end(self, messages: list) -> int:
        """找到 head 的结束索引（不包含）。

        head 包含前 1-2 轮对话（到第 3 条 HumanMessage 前，或到结尾）。
        受 HEAD_TOKEN_BUDGET 约束。
        """
        # 找到所有 HumanMessage 的索引
        human_indices = [i for i, m in enumerate(messages) if m.type == "human"]
        if not human_indices:
            return 0

        # head_end = 第 3 条 HumanMessage 的索引（不包含），或到结尾
        if len(human_indices) >= 3:
            head_end = human_indices[2]
        else:
            head_end = len(messages)

        # token 预算检查：如果 head 太大，逐步缩减
        while head_end > 1 and self._token_estimator.count_messages_tokens(messages[:head_end]) > self.HEAD_TOKEN_BUDGET:
            head_end -= 1

        # 确保 head 以"完整轮次"结尾：回退尾部 ToolMessage **以及**带 tool_calls
        # 的 AIMessage。只剥 ToolMessage 会露出其配对 tool_calls 落在 middle 的
        # AIMessage——head+summary 序列违反 API 配对约束（assistant 的 tool_calls
        # 必须后随对应 tool 消息），下次请求直接 400。
        while head_end > 0:
            last = messages[head_end - 1]
            if last.type == "tool":
                head_end -= 1
                continue
            if last.type == "ai" and getattr(last, "tool_calls", None):
                head_end -= 1
                continue
            break

        return max(head_end, 0)

    def _find_tail_start(self, messages: list, head_end: int) -> int:
        """找到 tail 的起始索引。

        tail 包含最近 max_recent_messages 条消息，从 HumanMessage 开始。
        受 TAIL_TOKEN_BUDGET 约束。
        """
        if not messages:
            return 0

        # 从末尾向前选 max_recent_messages 条
        tail_start = max(head_end, len(messages) - self._max_recent_messages)

        # 确保从 HumanMessage 开始（避免半截 AI/Tool 对话开头）
        while tail_start < len(messages) and messages[tail_start].type != "human":
            tail_start += 1

        # token 预算检查：如果 tail 太大，从前面删整轮对话
        while tail_start < len(messages) - 1 and \
                self._token_estimator.count_messages_tokens(messages[tail_start:]) > self.TAIL_TOKEN_BUDGET:
            # 找下一个 HumanMessage，从那里截断
            next_human = None
            for i in range(tail_start + 1, len(messages)):
                if messages[i].type == "human":
                    next_human = i
                    break
            if next_human is not None and next_human < len(messages):
                tail_start = next_human
            else:
                break

        return tail_start

    async def _generate_summary(self, messages: list) -> tuple:
        """用 LLM 对中间消息生成摘要。返回 (summary_text, usage_metadata)。

        如果中间消息太多导致摘要请求可能溢出，先截断到安全长度。
        usage_metadata 可能为 None（provider 不返回时）。
        ContextCompactor 不持 stdio，故 usage 经返回值向上传递
        （由 compact()/force_compact() 透传至调用方发送）。

        摘要调用包裹 retry_with_backoff（max_retries=3），不发 retrying 消息
        （摘要对用户不可见）。失败后异常由调用方（compact_node / force_compact）
        的 try/except 兜底。
        """
        history = self._format_history_for_summary(messages)

        # 防止摘要请求自身溢出：如果 history 太长，截断保留最近部分
        if len(history) > self.MAX_HISTORY_CHARS_FOR_SUMMARY:
            history = history[-self.MAX_HISTORY_CHARS_FOR_SUMMARY:]
            logger.warning("History too long for summary, truncated to %d chars",
                           len(history))

        prompt = COMPACTION_INSTRUCTION.format(history=history)

        from error_classifier import classify_error
        from retry_wrapper import retry_with_backoff

        async def _call_summary():
            response = await self._llm.ainvoke(
                [HumanMessage(content=prompt)],
                config={"max_tokens": self.SUMMARY_MAX_TOKENS}
            )
            return response

        # 直接调用，不加 try/except——异常由调用方（compact_node / force_compact）的
        # try/except 兜底。此处 try/except: raise 是死代码，已移除。
        response = await retry_with_backoff(
            _call_summary,
            max_retries=3,  # 摘要调用用较少重试次数
            on_retry=None,  # 不发 retrying 消息
            stop_event=None,
        )

        summary = response.content if hasattr(response, 'content') else str(response)
        usage = getattr(response, 'usage_metadata', None)
        return summary, usage

    def _format_history_for_summary(self, messages: list) -> str:
        """格式化消息列表为摘要输入文本。"""
        parts = []
        for msg in messages:
            role = msg.type  # "human" / "ai" / "tool" / "system"
            content = msg.content if hasattr(msg, 'content') else str(msg)
            if not isinstance(content, str):
                content = json.dumps(content, ensure_ascii=False) if isinstance(content, (dict, list)) else str(content)
            # 截断过长的单条消息
            if len(content) > self.MAX_SINGLE_MSG_CHARS:
                content = content[:self.MAX_SINGLE_MSG_CHARS] + "...[truncated]"
            parts.append(f"[{role}] {content}")
        return "\n\n".join(parts)


def is_context_overflow_error(exc: Exception) -> bool:
    """检测异常是否为上下文窗口超限。

    覆盖 OpenAI BadRequestError 和 litellm ContextWindowExceededError。
    关键词表必须与 error_classifier._CONTEXT_OVERFLOW_KEYWORDS 保持同步。
    注意：禁止加入 litellm 的通用包装词（如 "request is invalid"）——它出现在
    所有上游 400 的外层文案中，会把悬空 tool_calls 等无关的 BadRequestError
    误判为溢出；真实溢出消息均含下列特异性短语。
    """
    exc_str = str(exc).lower()
    keywords = [
        "context length",
        "maximum context",
        "contextwindowexceedederror",
        "context window",
        "too many tokens",
    ]
    return any(kw in exc_str for kw in keywords)
