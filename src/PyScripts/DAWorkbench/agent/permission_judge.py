#!/usr/bin/env python3
"""代码内容判定管线（权限层计划二，母文档 §6.3 / 决策 A1/A6/A8/A11/A12）。

职责定位（咨询方，A1）：
- 判定一律在 ``tool_call`` 发起**之前**完成，裁决以 ``safety`` 字段
  ``{verdict, reason, source}`` 附在 tool_call 消息上，由 C++ 权限门（唯一执法点）
  消费为 allow / deny / ask；Python 不执法。
- **禁止**任何"运行时 C++→Python 判定 RPC"形态：run 期间 ``_wait_for_result``
  （agent_runner.py）只认 ``tool_result`` / ``stop``，其余消息类型记日志后丢弃，
  判定请求会被丢弃、C++ 等响应挂起至超时。

判定管线（A6，规则先行 + 判官兜底）：
1. ``code_patterns.deny`` 命中 → ``verdict=deny``（source=rules）
2. ``code_patterns.escalate`` 命中 → 判官模型（已配置时）→ ``allow`` / ``deny``
   （source=model）；判官未配置 / 失败 / 超时 / 非法 JSON → ``uncertain``
   （source=none，由 C++ 门按 D1 兜底降级 ask）
3. 均不命中 → ``verdict=allow``（快速路径，零模型成本，source=rules）

``run_script``（A8）：按 ``workspace_root`` 解析相对路径、读取**入口文件**内容后
走同一管线；越界 / 读取失败 → ``uncertain``。判定边界=入口文件，脚本 import 的
其他文件不递归判定（已知局限，母文档 §9.2 [v2.1] 绕过向量）。

脱敏分工（A11 [v2.1]）：``reason`` 可含详细命中信息（供日志与审批卡内部展示）；
``code_exec`` 拒绝后呈现给 LLM 的脱敏文案由 C++ 侧合成，Python 不必也不应自行脱敏。

模式分工（A12）：``permission_mode != "auto"`` 时 agent_runner 不调用本模块
（不跑规则不跑模型），本模块自身不做模式判断。

清单来源：deny / escalate 正则清单从 ``ctx["code_patterns"]`` 配置注入
（C++ 经 init/reconfigure 下发，设置页可编辑）；仅当配置缺失/非法时回退
``DEFAULT_*_PATTERNS``（母文档 §9.2 种子清单）。
"""

import asyncio
import hashlib
import json
import logging
import os
import re

logger = logging.getLogger("permission_judge")

# ===========================================================================
# 常量与种子清单
# ===========================================================================

#: 代码执行工具名（与 C++ 侧 DAAgentPermissionManager::gatedTools 的 code_exec 子集一致）
CODE_EXEC_TOOLS = ("run_code", "run_script")

#: verdict 合法取值（不得自创新取值，契约 2）
VERDICT_ALLOW = "allow"
VERDICT_DENY = "deny"
VERDICT_UNCERTAIN = "uncertain"

#: source 合法取值：规则 / 判官模型 / 无（判官缺位或读取失败等未产出裁决的情形）
SOURCE_RULES = "rules"
SOURCE_MODEL = "model"
SOURCE_NONE = "none"

#: 母文档 §9.2 种子清单——仅作为配置缺失时的默认输入，正常运行以配置注入为准
DEFAULT_DENY_PATTERNS = [
    r"subprocess",
    r"os\.system",
    r"os\.popen",
    r"shutil\.rmtree",
    r"os\.remove\b",
    r"os\.unlink",
    r"os\.rmdir",
    r"ctypes",
    r"pickle\.loads",
    r"sys\.exit",
]

DEFAULT_ESCALATE_PATTERNS = [
    r"socket\.",
    r"requests\.(get|post|put|delete)",
    r"urllib",
    r"__import__",
    r"importlib",
    r"\beval\s*\(",
    r"\bexec\s*\(",
    r"open\s*\([^)]*['\"][wa]\+?b?['\"]",
    r"\.write_",
    r"shutil\.(copy|move)",
]

#: 送入判官提示词的代码长度上限（超长截断，保持轻量提示词）
_JUDGE_CODE_LIMIT = 24000

#: 判官回复 reason 截断长度
_REASON_LIMIT = 500

#: 判官提示词（独立轻量；输出严格 JSON）
_JUDGE_SYSTEM_PROMPT = (
    "You are a code safety reviewer for a desktop data-analysis application. "
    "The user allows the AI assistant to run Python code (pandas/numpy etc.) "
    "on their datasets inside a script workspace. Decide whether the code is "
    "SAFE to execute.\n"
    "Deny only when the code clearly attempts harmful or out-of-scope operations, "
    "such as: network exfiltration, downloading or running remote payloads, "
    "deleting or overwriting files unrelated to the analysis, executing system "
    "commands, reading credentials, or hidden obfuscation of such operations.\n"
    "Ordinary data-analysis code (reading data, pandas/numpy processing, plotting, "
    "writing result files) is SAFE.\n"
    'Respond with ONLY a JSON object: {"verdict": "allow" or "deny", '
    '"reason": "<one short sentence>"}.'
)

# 模块级正则缓存：模式串 → 编译结果（None 表示非法模式，跳过）
_REGEX_CACHE = {}


# ===========================================================================
# 内部工具
# ===========================================================================

def _compile_pattern(pattern):
    """编译单个正则并缓存；非法模式记日志并缓存 None（跳过，不让管线崩溃）。"""
    if pattern in _REGEX_CACHE:
        return _REGEX_CACHE[pattern]
    try:
        compiled = re.compile(pattern)
    except re.error as e:
        logger.warning("permission_judge: invalid pattern %r skipped: %s", pattern, e)
        _REGEX_CACHE[pattern] = None
        return None
    _REGEX_CACHE[pattern] = compiled
    return compiled


def _pattern_list(code_patterns, key, defaults):
    """从配置取模式清单；配置缺失/类型非法时回退默认种子清单。

    配置中显式给空列表视为用户清空（不回退默认），仅结构缺失才用默认。
    """
    if not isinstance(code_patterns, dict):
        return list(defaults)
    value = code_patterns.get(key)
    if not isinstance(value, list):
        return list(defaults)
    return [p for p in value if isinstance(p, str) and p]


def _first_match(code, patterns):
    """返回首个命中的模式串；无命中返回 None。"""
    for pattern in patterns:
        compiled = _compile_pattern(pattern)
        if compiled is not None and compiled.search(code):
            return pattern
    return None


def _truncate(text, limit):
    if len(text) <= limit:
        return text
    return text[:limit] + "...(truncated)"


def _parse_judge_reply(text):
    """严格解析判官回复为 {verdict, reason}；非法返回 None。

    先整体 json.loads；失败时退化为截取首个 '{' 到末尾 '}' 的子串再解析
    （容忍模型偶发的 ```json 包裹或前后赘述）。verdict 必须是 allow/deny，
    其余取值视为非法（交由调用方降级 uncertain）。
    """
    if not isinstance(text, str):
        return None
    candidate = text.strip()
    obj = None
    try:
        obj = json.loads(candidate)
    except (json.JSONDecodeError, ValueError):
        start = candidate.find("{")
        end = candidate.rfind("}")
        if 0 <= start < end:
            try:
                obj = json.loads(candidate[start:end + 1])
            except (json.JSONDecodeError, ValueError):
                return None
        else:
            return None
    if not isinstance(obj, dict):
        return None
    verdict = str(obj.get("verdict", "")).strip().lower()
    if verdict not in (VERDICT_ALLOW, VERDICT_DENY):
        return None
    reason = _truncate(str(obj.get("reason", "")), _REASON_LIMIT)
    return {"verdict": verdict, "reason": reason}


async def _call_judge_model(code, ctx):
    """调用判官模型裁决代码；未配置/失败/超时/非法回复返回 None。

    复用当前供应商的 base_url / api_key（ctx 提供）+ judge.model（A7：
    判官不跨供应商，零新依赖）。延迟导入 langchain_openai，保证纯规则
    路径的单元测试无需安装重依赖。
    """
    judge = ctx.get("judge") if isinstance(ctx, dict) else None
    if not isinstance(judge, dict):
        judge = {}
    model = str(judge.get("model", "") or "").strip()
    if not model:
        logger.info("permission_judge: judge not configured, escalate falls back to uncertain")
        return None
    try:
        timeout_sec = float(judge.get("timeout_sec", 30))
    except (TypeError, ValueError):
        timeout_sec = 30.0
    timeout_sec = max(1.0, timeout_sec)

    try:
        from langchain_openai import ChatOpenAI
        from langchain_core.messages import HumanMessage, SystemMessage
    except ImportError as e:
        logger.warning("permission_judge: langchain unavailable, judge skipped: %s", e)
        return None

    base_url = str(ctx.get("base_url", "") or "")
    api_key = str(ctx.get("api_key", "") or "")
    if not base_url or not api_key:
        logger.warning("permission_judge: missing base_url/api_key for judge, skipped")
        return None

    code_block = _truncate(code, _JUDGE_CODE_LIMIT)
    try:
        llm = ChatOpenAI(
            base_url=base_url,
            api_key=api_key,
            model=model,
            streaming=False,
            max_retries=0,          # 判定路径不做重试，超时即降级
            timeout=timeout_sec,    # HTTP 请求超时
            max_tokens=300,         # 轻量回复
        )
        response = await asyncio.wait_for(
            llm.ainvoke([
                SystemMessage(content=_JUDGE_SYSTEM_PROMPT),
                HumanMessage(content=code_block),
            ]),
            timeout=timeout_sec,
        )
    except asyncio.TimeoutError:
        logger.warning("permission_judge: judge call timed out after %.0fs", timeout_sec)
        return None
    except Exception as e:
        logger.warning("permission_judge: judge call failed: %s", e)
        return None

    content = getattr(response, "content", "")
    parsed = _parse_judge_reply(content)
    if parsed is None:
        logger.warning("permission_judge: judge reply is not valid JSON verdict: %r",
                       content[:200])
    return parsed


# ===========================================================================
# 对外接口
# ===========================================================================

def resolve_script_path(rel_path, workspace_root):
    """把 run_script 的工作区相对路径解析为绝对路径；越界/不可解析返回 None。

    镜像 C++ 工具侧防护（DAAgentToolRunScript：cleanPath 词法归一 ../ 后前缀校验）：
    normpath 归一后必须仍位于工作区内。绝对路径输入同样校验包含关系。
    """
    if not isinstance(rel_path, str) or not rel_path.strip():
        return None
    if not isinstance(workspace_root, str) or not workspace_root.strip():
        return None
    root = os.path.normpath(os.path.abspath(workspace_root))
    candidate = rel_path.strip()
    if os.path.isabs(candidate):
        resolved = os.path.normpath(candidate)
    else:
        resolved = os.path.normpath(os.path.join(root, candidate))
    # 前缀校验：必须等于根或位于根之下（防 ../ 逃逸）
    if resolved != root and not resolved.startswith(root + os.sep):
        return None
    return resolved


async def judge_code(code, ctx):
    """判定一段代码内容，返回 ``{verdict, reason, source}``（母文档 §8 形状）。

    @param code 待判定代码文本（run_code 的 code 参数 / run_script 入口文件内容）
    @param ctx 判定上下文：``{"code_patterns": {"deny":[], "escalate":[]},
        "judge": {"model": str, "timeout_sec": float},
        "base_url": str, "api_key": str}``
    @return 裁决字典；verdict ∈ allow/deny/uncertain，source ∈ rules/model/none
    """
    if not isinstance(code, str):
        code = "" if code is None else str(code)
    ctx = ctx if isinstance(ctx, dict) else {}

    patterns = ctx.get("code_patterns")
    deny_patterns = _pattern_list(patterns, "deny", DEFAULT_DENY_PATTERNS)
    escalate_patterns = _pattern_list(patterns, "escalate", DEFAULT_ESCALATE_PATTERNS)

    # 1. deny 命中 → 直接裁决拒绝（A6；reason 含命中详情，脱敏是 C++ 职责，契约 2）
    hit = _first_match(code, deny_patterns)
    if hit is not None:
        logger.info("permission_judge: deny pattern matched: %r", hit)
        return {
            "verdict": VERDICT_DENY,
            "reason": "denied by static rule: pattern %r matched" % hit,
            "source": SOURCE_RULES,
        }

    # 2. escalate 命中 → 判官（启用时）；判官缺位/失败 → uncertain（C++ 门降级 ask）
    hit = _first_match(code, escalate_patterns)
    if hit is not None:
        logger.info("permission_judge: escalate pattern matched: %r, consulting judge", hit)
        judged = await _call_judge_model(code, ctx)
        if judged is not None:
            return {
                "verdict": judged["verdict"],
                "reason": "judge verdict on escalate pattern %r: %s"
                          % (hit, judged["reason"]),
                "source": SOURCE_MODEL,
            }
        return {
            "verdict": VERDICT_UNCERTAIN,
            "reason": "escalate pattern %r matched but judge unavailable "
                      "(not configured / failed / timed out / invalid reply)" % hit,
            "source": SOURCE_NONE,
        }

    # 3. 快速路径：均不命中 → allow，零模型成本（A6；判官是否配置不影响本路径，
    #    "判官未配置→ask" 的兜底由 C++ 门执行，D1 [v2.1]，契约 2）
    return {
        "verdict": VERDICT_ALLOW,
        "reason": "no deny/escalate pattern matched (fast path)",
        "source": SOURCE_RULES,
    }


async def judge_tool_call(tool, args, ctx):
    """对代码执行工具调用做判定，返回 ``{verdict, reason, source}``。

    - ``run_code``：直接判定 ``args["code"]``。
    - ``run_script``：按 ``ctx["workspace_root"]`` 解析 ``args["path"]``，读取
      入口文件内容后走同一管线（A8）；解析越界/读取失败 → uncertain。
      判定边界=入口文件，不递归其 import 的模块。

    @param tool 工具名（run_code / run_script）
    @param args 工具参数字典
    @param ctx 判定上下文（见 judge_code，另需 "workspace_root"）
    @return 裁决字典
    """
    args = args if isinstance(args, dict) else {}
    ctx = ctx if isinstance(ctx, dict) else {}

    if tool == "run_code":
        return await judge_code(args.get("code", ""), ctx)

    if tool == "run_script":
        rel_path = str(args.get("path", "") or "")
        if not rel_path.strip():
            return {
                "verdict": VERDICT_UNCERTAIN,
                "reason": "run_script without a script path cannot be judged",
                "source": SOURCE_NONE,
            }
        abs_path = resolve_script_path(rel_path, ctx.get("workspace_root", ""))
        if abs_path is None:
            logger.warning("permission_judge: script path escapes workspace: %r", rel_path)
            return {
                "verdict": VERDICT_UNCERTAIN,
                "reason": "script path %r escapes the script workspace or workspace "
                          "root is not configured" % rel_path,
                "source": SOURCE_NONE,
            }
        try:
            with open(abs_path, "rb") as f:
                raw = f.read()
        except OSError as e:
            logger.warning("permission_judge: failed to read script %s: %s", abs_path, e)
            return {
                "verdict": VERDICT_UNCERTAIN,
                "reason": "failed to read script %r: %s" % (rel_path, e),
                "source": SOURCE_NONE,
            }
        content = raw.decode("utf-8", errors="replace")
        # 入口文件内容走同一管线（判定边界=入口文件，不递归，A8）
        verdict = await judge_code(content, ctx)
        verdict["reason"] = "run_script entry file %r: %s" % (rel_path, verdict["reason"])
        # 审计问题 26（TOCTOU 闭环）：判定载荷携带判定时刻原始字节的 sha256——
        # C++ 执行侧（DAAgentToolRunScript）执行前重读同一文件校验哈希一致，
        # 不一致拒绝执行。判定与执行之间的窗口（现含全局队列排队段）内，共享
        # 工作区的脚本可能被 write_file/其它会话改写，使实际执行代码≠被判定
        # 代码，绕过 deny/escalate 规则。哈希对象为文件原始字节（与 C++ 侧
        # QFile::readAll + Sha256 一致），不经 decode 再编码
        verdict["content_hash"] = hashlib.sha256(raw).hexdigest()
        return verdict

    # 非代码执行工具不应进入本模块（调用方已过滤）；防御性回退
    return {
        "verdict": VERDICT_UNCERTAIN,
        "reason": "unsupported tool for code judging: %r" % tool,
        "source": SOURCE_NONE,
    }
