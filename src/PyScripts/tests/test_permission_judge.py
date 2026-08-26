"""Tests for permission_judge (pure Python, judge calls mocked, no C++ bindings).

镜像 test_workflow.py 约定：importlib.util.spec_from_file_location 按相对路径加载
被测模块，纯 Python 运行，不依赖 langchain/Qt（判官模型调用经 fake 模块注入或
直接 patch _call_judge_model 隔离）。
"""

import asyncio
import importlib.util
import os
import sys
import types

import pytest

_TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
_SRC_DIR = os.path.join(_TESTS_DIR, "..", "..")


def _load_module(name, rel_path):
    spec = importlib.util.spec_from_file_location(
        name, os.path.join(_SRC_DIR, rel_path)
    )
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod


pj = _load_module("DAWorkbench.agent.permission_judge",
                  "PyScripts/DAWorkbench/agent/permission_judge.py")


# ===========================================================================
# helpers
# ===========================================================================

def _ctx(patterns=None, judge=None, workspace_root=""):
    """构造判定上下文；patterns 缺省时注入 §9.2 种子清单（契约 6）。"""
    if patterns is None:
        patterns = {"deny": list(pj.DEFAULT_DENY_PATTERNS),
                    "escalate": list(pj.DEFAULT_ESCALATE_PATTERNS)}
    if judge is None:
        judge = {"model": "", "timeout_sec": 30}
    return {
        "code_patterns": patterns,
        "judge": judge,
        "workspace_root": workspace_root,
        "base_url": "http://fake.local/v1",
        "api_key": "fake-key",
    }


def _run(coro):
    return asyncio.run(coro)


def _assert_shape(result):
    """协议形状契约（母文档 §8）：verdict/reason/source 取值受限。"""
    assert set(result.keys()) >= {"verdict", "reason", "source"}
    assert result["verdict"] in (pj.VERDICT_ALLOW, pj.VERDICT_DENY, pj.VERDICT_UNCERTAIN)
    assert result["source"] in (pj.SOURCE_RULES, pj.SOURCE_MODEL, pj.SOURCE_NONE)
    assert isinstance(result["reason"], str)


def _install_fake_langchain(monkeypatch, reply_content=None, raise_exc=None,
                            hang=False):
    """向 sys.modules 注入 fake langchain，使 _call_judge_model 可离线执行。

    reply_content: ainvoke 返回消息的 content（字符串）
    raise_exc: ainvoke 抛出该异常
    hang: ainvoke 永久挂起（供 wait_for 超时路径）
    """
    calls = []

    class FakeChatOpenAI:
        def __init__(self, **kwargs):
            self.kwargs = kwargs

        async def ainvoke(self, messages):
            calls.append({"messages": messages, "kwargs": self.kwargs})
            if hang:
                await asyncio.Event().wait()  # 永不返回，触发 wait_for 超时
            if raise_exc is not None:
                raise raise_exc
            return types.SimpleNamespace(content=reply_content)

    fake_openai = types.ModuleType("langchain_openai")
    fake_openai.ChatOpenAI = FakeChatOpenAI
    fake_msgs = types.ModuleType("langchain_core.messages")

    class _Msg:
        def __init__(self, content=""):
            self.content = content

    fake_msgs.SystemMessage = _Msg
    fake_msgs.HumanMessage = _Msg
    monkeypatch.setitem(sys.modules, "langchain_openai", fake_openai)
    monkeypatch.setitem(sys.modules, "langchain_core.messages", fake_msgs)
    return calls


# ===========================================================================
# 1. deny 清单命中 → verdict=deny, source=rules（契约：reason 可含命中详情）
# ===========================================================================

class TestDenyPatterns:
    def test_subprocess_hit(self):
        result = _run(pj.judge_code("import subprocess\nsubprocess.run(['x'])", _ctx()))
        _assert_shape(result)
        assert result["verdict"] == pj.VERDICT_DENY
        assert result["source"] == pj.SOURCE_RULES
        assert "subprocess" in result["reason"]  # reason 含命中详情（脱敏是 C++ 职责）

    def test_os_system_hit(self):
        result = _run(pj.judge_code("import os\nos.system('del /s c:\\')", _ctx()))
        assert result["verdict"] == pj.VERDICT_DENY
        assert result["source"] == pj.SOURCE_RULES

    def test_deny_beats_escalate(self):
        """同时命中 deny 与 escalate 时，deny 优先（A6 判定顺序）。"""
        code = "import subprocess, requests\nrequests.get('http://x')"
        result = _run(pj.judge_code(code, _ctx()))
        assert result["verdict"] == pj.VERDICT_DENY
        assert result["source"] == pj.SOURCE_RULES


# ===========================================================================
# 2. 均不命中 → verdict=allow（快速路径；判官未配置时 Python 同样裁决 allow，
#    ask 兜底在 C++ 侧，契约 2）
# ===========================================================================

class TestFastPath:
    def test_plain_analysis_code_allowed(self):
        result = _run(pj.judge_code("df.describe()\ndf.mean()", _ctx()))
        _assert_shape(result)
        assert result["verdict"] == pj.VERDICT_ALLOW
        assert result["source"] == pj.SOURCE_RULES

    def test_fast_path_without_judge(self):
        """判官未配置不影响快速路径（D1 兜底由 C++ 门执行）。"""
        ctx = _ctx(judge={"model": "", "timeout_sec": 30})
        result = _run(pj.judge_code("df.describe()", ctx))
        assert result["verdict"] == pj.VERDICT_ALLOW

    def test_fast_path_with_judge_configured_no_model_call(self, monkeypatch):
        """快速路径零判官调用（A6 成本控制）。"""
        calls = _install_fake_langchain(monkeypatch, reply_content='{"verdict": "deny"}')
        result = _run(pj.judge_code("df.describe()",
                                    _ctx(judge={"model": "m", "timeout_sec": 5})))
        assert result["verdict"] == pj.VERDICT_ALLOW
        assert calls == []  # 判官从未被调用


# ===========================================================================
# 3. escalate 命中 + 判官裁决 → source=model
# ===========================================================================

class TestEscalateWithJudge:
    def test_judge_allow(self, monkeypatch):
        _install_fake_langchain(monkeypatch,
                                reply_content='{"verdict": "allow", "reason": "benign"}')
        ctx = _ctx(judge={"model": "judge-model", "timeout_sec": 5})
        result = _run(pj.judge_code("import requests\nrequests.get('http://x')", ctx))
        _assert_shape(result)
        assert result["verdict"] == pj.VERDICT_ALLOW
        assert result["source"] == pj.SOURCE_MODEL

    def test_judge_deny(self, monkeypatch):
        _install_fake_langchain(monkeypatch,
                                reply_content='{"verdict": "deny", "reason": "exfiltration"}')
        ctx = _ctx(judge={"model": "judge-model", "timeout_sec": 5})
        result = _run(pj.judge_code("import urllib.request", ctx))
        assert result["verdict"] == pj.VERDICT_DENY
        assert result["source"] == pj.SOURCE_MODEL

    def test_judge_reply_with_code_fence(self, monkeypatch):
        """判官偶发以 ```json 包裹回复 → 仍能解析（容忍降级路径）。"""
        _install_fake_langchain(monkeypatch,
                                reply_content='```json\n{"verdict": "allow", "reason": "ok"}\n```')
        ctx = _ctx(judge={"model": "judge-model", "timeout_sec": 5})
        result = _run(pj.judge_code("import socket\nsocket.create_connection(('x', 80))", ctx))
        assert result["verdict"] == pj.VERDICT_ALLOW
        assert result["source"] == pj.SOURCE_MODEL

    def test_judge_receives_credentials(self, monkeypatch):
        """判官复用当前供应商 base_url/api_key + judge.model（A7）。"""
        calls = _install_fake_langchain(monkeypatch,
                                        reply_content='{"verdict": "allow", "reason": "ok"}')
        ctx = _ctx(judge={"model": "judge-model", "timeout_sec": 5})
        _run(pj.judge_code("import socket\nsocket.create_connection(('x', 80))", ctx))
        assert len(calls) == 1
        kwargs = calls[0]["kwargs"]
        assert kwargs["model"] == "judge-model"
        assert kwargs["base_url"] == "http://fake.local/v1"
        assert kwargs["api_key"] == "fake-key"


# ===========================================================================
# 4. escalate 命中 + 判官缺位 → verdict=uncertain
# ===========================================================================

class TestEscalateJudgeUnavailable:
    def test_judge_not_configured(self):
        ctx = _ctx(judge={"model": "", "timeout_sec": 5})
        result = _run(pj.judge_code("import requests\nrequests.get('http://x')", ctx))
        _assert_shape(result)
        assert result["verdict"] == pj.VERDICT_UNCERTAIN
        assert result["source"] == pj.SOURCE_NONE

    def test_judge_timeout(self, monkeypatch):
        _install_fake_langchain(monkeypatch, hang=True)
        ctx = _ctx(judge={"model": "judge-model", "timeout_sec": 1})
        result = _run(pj.judge_code("import requests\nrequests.get('http://x')", ctx))
        assert result["verdict"] == pj.VERDICT_UNCERTAIN
        assert result["source"] == pj.SOURCE_NONE

    def test_judge_exception(self, monkeypatch):
        _install_fake_langchain(monkeypatch, raise_exc=RuntimeError("connection refused"))
        ctx = _ctx(judge={"model": "judge-model", "timeout_sec": 5})
        result = _run(pj.judge_code("import requests\nrequests.get('http://x')", ctx))
        assert result["verdict"] == pj.VERDICT_UNCERTAIN

    def test_judge_invalid_json(self, monkeypatch):
        _install_fake_langchain(monkeypatch, reply_content="I cannot decide, sorry")
        ctx = _ctx(judge={"model": "judge-model", "timeout_sec": 5})
        result = _run(pj.judge_code("import requests\nrequests.get('http://x')", ctx))
        assert result["verdict"] == pj.VERDICT_UNCERTAIN
        assert result["source"] == pj.SOURCE_NONE

    def test_judge_unknown_verdict_value(self, monkeypatch):
        """verdict 非 allow/deny → 视为非法 → uncertain。"""
        _install_fake_langchain(monkeypatch,
                                reply_content='{"verdict": "maybe", "reason": "?"}')
        ctx = _ctx(judge={"model": "judge-model", "timeout_sec": 5})
        result = _run(pj.judge_code("import requests\nrequests.get('http://x')", ctx))
        assert result["verdict"] == pj.VERDICT_UNCERTAIN

    def test_missing_credentials(self, monkeypatch):
        _install_fake_langchain(monkeypatch,
                                reply_content='{"verdict": "allow", "reason": "ok"}')
        ctx = _ctx(judge={"model": "judge-model", "timeout_sec": 5})
        ctx["api_key"] = ""  # 凭据缺失 → 不调判官 → uncertain
        result = _run(pj.judge_code("import requests\nrequests.get('http://x')", ctx))
        assert result["verdict"] == pj.VERDICT_UNCERTAIN


# ===========================================================================
# 5. run_script：工作区内路径解析 / 越界 / 读取失败
# ===========================================================================

class TestRunScript:
    def test_script_in_workspace_denied_by_content(self, tmp_path):
        """工作区内脚本读取成功 → 内容走同一管线（含 subprocess → deny）。"""
        script = tmp_path / "scripts" / "bad.py"
        script.parent.mkdir(parents=True)
        script.write_text("import subprocess\n", encoding="utf-8")
        ctx = _ctx(workspace_root=str(tmp_path))
        result = _run(pj.judge_tool_call(
            "run_script", {"path": "scripts/bad.py"}, ctx))
        _assert_shape(result)
        assert result["verdict"] == pj.VERDICT_DENY
        assert result["source"] == pj.SOURCE_RULES
        assert "scripts/bad.py" in result["reason"]

    def test_script_in_workspace_fast_path(self, tmp_path):
        script = tmp_path / "ok.py"
        script.write_text("print(df.describe())\n", encoding="utf-8")
        ctx = _ctx(workspace_root=str(tmp_path))
        result = _run(pj.judge_tool_call("run_script", {"path": "ok.py"}, ctx))
        assert result["verdict"] == pj.VERDICT_ALLOW

    def test_path_escape_uncertain(self, tmp_path):
        ctx = _ctx(workspace_root=str(tmp_path))
        result = _run(pj.judge_tool_call(
            "run_script", {"path": "../outside.py"}, ctx))
        _assert_shape(result)
        assert result["verdict"] == pj.VERDICT_UNCERTAIN
        assert result["source"] == pj.SOURCE_NONE

    def test_absolute_path_outside_uncertain(self, tmp_path):
        ctx = _ctx(workspace_root=str(tmp_path))
        result = _run(pj.judge_tool_call(
            "run_script", {"path": "c:/windows/system32/evil.py"}, ctx))
        assert result["verdict"] == pj.VERDICT_UNCERTAIN

    def test_missing_file_uncertain(self, tmp_path):
        ctx = _ctx(workspace_root=str(tmp_path))
        result = _run(pj.judge_tool_call(
            "run_script", {"path": "not_exist.py"}, ctx))
        assert result["verdict"] == pj.VERDICT_UNCERTAIN

    def test_workspace_root_not_configured(self):
        ctx = _ctx(workspace_root="")
        result = _run(pj.judge_tool_call("run_script", {"path": "a.py"}, ctx))
        assert result["verdict"] == pj.VERDICT_UNCERTAIN

    def test_missing_path_arg(self, tmp_path):
        ctx = _ctx(workspace_root=str(tmp_path))
        result = _run(pj.judge_tool_call("run_script", {}, ctx))
        assert result["verdict"] == pj.VERDICT_UNCERTAIN

    def test_run_code_dispatch(self):
        """run_code 直接判定 code 参数。"""
        result = _run(pj.judge_tool_call(
            "run_code", {"code": "import shutil\nshutil.rmtree('/')"}, _ctx()))
        assert result["verdict"] == pj.VERDICT_DENY

    def test_resolve_script_path_helper(self, tmp_path):
        assert pj.resolve_script_path("a/b.py", str(tmp_path)) == os.path.normpath(
            os.path.join(str(tmp_path), "a", "b.py"))
        assert pj.resolve_script_path("../escape.py", str(tmp_path)) is None
        assert pj.resolve_script_path("", str(tmp_path)) is None
        assert pj.resolve_script_path("a.py", "") is None


# ===========================================================================
# 6. 清单来自配置注入（§9.2 种子清单作为默认输入）
# ===========================================================================

class TestConfigInjection:
    def test_custom_deny_pattern(self):
        ctx = _ctx(patterns={"deny": [r"FORBIDDEN_TOKEN"], "escalate": []})
        result = _run(pj.judge_code("x = 'FORBIDDEN_TOKEN'", ctx))
        assert result["verdict"] == pj.VERDICT_DENY
        # 种子清单被配置替换：subprocess 不再命中（配置显式注入优先）
        result2 = _run(pj.judge_code("import subprocess", ctx))
        assert result2["verdict"] == pj.VERDICT_ALLOW

    def test_custom_escalate_pattern(self):
        ctx = _ctx(patterns={"deny": [], "escalate": [r"MY_ESCALATE"]},
                   judge={"model": "", "timeout_sec": 5})
        result = _run(pj.judge_code("do MY_ESCALATE thing", ctx))
        assert result["verdict"] == pj.VERDICT_UNCERTAIN  # escalate 命中且判官未配置

    def test_empty_list_disables_category(self):
        """配置显式清空 escalate → 原本升级的代码走快速路径。"""
        ctx = _ctx(patterns={"deny": [], "escalate": []})
        result = _run(pj.judge_code("import socket\nsocket.socket()", ctx))
        assert result["verdict"] == pj.VERDICT_ALLOW

    def test_malformed_config_falls_back_to_defaults(self):
        """code_patterns 非法（非 dict）→ 回退默认种子清单。"""
        ctx = _ctx()
        ctx["code_patterns"] = "not-a-dict"
        result = _run(pj.judge_code("import subprocess", ctx))
        assert result["verdict"] == pj.VERDICT_DENY

    def test_invalid_regex_skipped_not_crash(self):
        """非法正则跳过，不让管线崩溃。"""
        ctx = _ctx(patterns={"deny": [r"(unclosed"], "escalate": []})
        result = _run(pj.judge_code("anything", ctx))
        assert result["verdict"] == pj.VERDICT_ALLOW

    def test_seed_lists_match_plan_section_9_2(self):
        """默认种子清单与母文档 §9.2 一致（计划二验收基线）。"""
        assert pj.DEFAULT_DENY_PATTERNS == [
            r"subprocess", r"os\.system", r"os\.popen", r"shutil\.rmtree",
            r"os\.remove\b", r"os\.unlink", r"os\.rmdir", r"ctypes",
            r"pickle\.loads", r"sys\.exit",
        ]
        assert pj.DEFAULT_ESCALATE_PATTERNS == [
            r"socket\.", r"requests\.(get|post|put|delete)", r"urllib",
            r"__import__", r"importlib", r"\beval\s*\(", r"\bexec\s*\(",
            r"open\s*\([^)]*['\"][wa]\+?b?['\"]", r"\.write_",
            r"shutil\.(copy|move)",
        ]


# ===========================================================================
# _parse_judge_reply 单元（严格 JSON 解析）
# ===========================================================================

class TestParseJudgeReply:
    def test_valid(self):
        parsed = pj._parse_judge_reply('{"verdict": "allow", "reason": "ok"}')
        assert parsed == {"verdict": "allow", "reason": "ok"}

    def test_verdict_case_insensitive(self):
        parsed = pj._parse_judge_reply('{"verdict": "DENY", "reason": "bad"}')
        assert parsed["verdict"] == "deny"

    def test_embedded_json(self):
        parsed = pj._parse_judge_reply('Sure! {"verdict": "allow", "reason": "fine"} hope it helps')
        assert parsed is not None and parsed["verdict"] == "allow"

    def test_invalid_json(self):
        assert pj._parse_judge_reply("not json at all") is None

    def test_unknown_verdict(self):
        assert pj._parse_judge_reply('{"verdict": "maybe", "reason": "?"}') is None

    def test_non_object_json(self):
        assert pj._parse_judge_reply("[1, 2, 3]") is None

    def test_non_string(self):
        assert pj._parse_judge_reply(None) is None

    def test_reason_truncated(self):
        parsed = pj._parse_judge_reply(
            '{"verdict": "deny", "reason": "%s"}' % ("x" * 5000))
        assert len(parsed["reason"]) <= 500 + 20  # 截断 + 后缀余量
