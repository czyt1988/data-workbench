"""Tests for the slow log extension of @log_function_call (DAPyBase/da_logger.py).

慢日志 Python 侧验收（对应实施方案 §11.4）：
1. DA_SLOW_LOG_BUILD=0 时装饰器不测时、不输出；
2. DA_SLOW_LOG_BUILD=1、低阈值时，超阈值调用输出 [slow] tag=... 记录；
3. 未超阈值时不输出；
4. 嵌套两个装饰函数，depth 与 self_ms 字段存在且数值关系合理。

运行方式：
    python -m pytest src/PyScripts/tests/test_slow_log.py -q
"""

import importlib.util
import os
import re
import time

import pytest

_TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
_SRC_DIR = os.path.join(_TESTS_DIR, "..", "..")
_DA_LOGGER_PATH = os.path.join(_SRC_DIR, "PyScripts", "DAWorkbench", "DAPyBase", "da_logger.py")


def _load_da_logger(env=None):
    """在指定环境变量下把 da_logger.py 加载为全新模块。

    模块级开关（DA_SLOW_LOG_BUILD / DA_SLOW_LOG / DA_SLOW_LOG_MS）在导入时
    读取一次，因此每个用例都要以目标环境重新加载模块；加载完成后恢复原环境。
    """
    env = env or {}
    saved = {k: os.environ.get(k) for k in env}
    try:
        for k, v in env.items():
            if v is None:
                os.environ.pop(k, None)
            else:
                os.environ[k] = v
        spec = importlib.util.spec_from_file_location("_da_logger_under_test", _DA_LOGGER_PATH)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        return mod
    finally:
        for k, v in saved.items():
            if v is None:
                os.environ.pop(k, None)
            else:
                os.environ[k] = v


@pytest.fixture
def slow_records():
    """捕获 loguru WARNING 及以上消息文本（[slow] 记录以 warning 级别输出）"""
    from loguru import logger

    records = []
    handler_id = logger.add(lambda m: records.append(str(m)), level="WARNING", format="{message}")
    yield records
    logger.remove(handler_id)


def _slow_lines(records):
    return [r for r in records if "[slow]" in r]


def _parse_fields(line):
    """解析 [slow] 行中的 key=value 数值字段"""
    out = {}
    for key in ("ms", "self_ms", "depth"):
        m = re.search(r"(?:^|\s)%s=([\d.]+)" % key, line)
        assert m, "field %s missing in: %s" % (key, line)
        out[key] = float(m.group(1))
    return out


# 统一的环境：编译期等价开启 + 默认阈值 50ms
_BUILD_ON = {"DA_SLOW_LOG_BUILD": "1", "DA_SLOW_LOG": None, "DA_SLOW_LOG_MS": "50"}


class TestSlowLogBuildSwitch:
    def test_build_off_no_timing(self, slow_records):
        """DA_SLOW_LOG_BUILD=0：装饰器不测时、不输出（等价宏关闭）"""
        mod = _load_da_logger({"DA_SLOW_LOG_BUILD": "0", "DA_SLOW_LOG": "on", "DA_SLOW_LOG_MS": "1"})
        assert mod._SLOW_BUILD is False
        assert mod._SLOW_ENABLED is False

        @mod.log_function_call(slow=True, slow_tag="test.slow", slow_ms=1)
        def work():
            time.sleep(0.02)
            return 7

        assert work() == 7
        assert _slow_lines(slow_records) == []

    def test_runtime_disabled(self, slow_records):
        """DA_SLOW_LOG=off：运行期关闭，不输出"""
        mod = _load_da_logger({"DA_SLOW_LOG_BUILD": "1", "DA_SLOW_LOG": "off", "DA_SLOW_LOG_MS": "1"})
        assert mod._SLOW_BUILD is True
        assert mod._SLOW_ENABLED is False

        @mod.log_function_call(slow=True, slow_tag="test.slow", slow_ms=1)
        def work():
            time.sleep(0.02)

        work()
        assert _slow_lines(slow_records) == []


class TestSlowLogOutput:
    def test_threshold_exceeded_emits_record(self, slow_records):
        """BUILD=1 且超阈值：输出一条含 [slow] tag=test.slow 的记录"""
        mod = _load_da_logger(dict(_BUILD_ON))

        @mod.log_function_call(slow=True, slow_tag="test.slow", slow_ms=5)
        def work(x):
            time.sleep(0.05)
            return x * 2

        assert work(21) == 42
        lines = _slow_lines(slow_records)
        assert len(lines) == 1
        line = lines[0]
        assert "tag=test.slow" in line
        # 与 C++ 侧一致的自包含字段
        for field in ("ms=", "self_ms=", "depth=", "thread=", "file=", "line=", "func="):
            assert field in line, "missing %s in %s" % (field, line)
        assert 'detail="args_len=1"' in line
        f = _parse_fields(line)
        assert f["ms"] >= 45
        assert f["depth"] == 0

    def test_below_threshold_silent(self, slow_records):
        """未超阈值：不输出"""
        mod = _load_da_logger(dict(_BUILD_ON))

        @mod.log_function_call(slow=True, slow_tag="test.slow.silent", slow_ms=5000)
        def work():
            time.sleep(0.01)

        work()
        assert _slow_lines(slow_records) == []

    def test_detail_lazy_and_payload(self, slow_records):
        """slow_detail 惰性：未超阈值不调用；超阈值调用一次且带 (args, kwargs, result)"""
        mod = _load_da_logger(dict(_BUILD_ON))
        calls = []

        def detail(args, kwargs, result):
            calls.append((args, kwargs, result))
            return "n=%s,r=%s" % (args[0], result)

        @mod.log_function_call(slow=True, slow_tag="test.slow.detail", slow_ms=5000, slow_detail=detail)
        def quiet(x):
            return x

        quiet(1)
        assert calls == []
        assert _slow_lines(slow_records) == []

        @mod.log_function_call(slow=True, slow_tag="test.slow.detail", slow_ms=5, slow_detail=detail)
        def loud(x, k=0):
            time.sleep(0.05)
            return x + k

        assert loud(2, k=3) == 5
        assert len(calls) == 1
        assert calls[0] == ((2,), {"k": 3}, 5)
        lines = _slow_lines(slow_records)
        assert len(lines) == 1
        assert 'detail="n=2,r=5"' in lines[0]

    def test_default_tag_from_module_and_qualname(self, slow_records):
        """未指定 slow_tag：标签自动生成为 python.<模块>.<限定名>"""
        mod = _load_da_logger(dict(_BUILD_ON))

        @mod.log_function_call(slow=True, slow_ms=5)
        def auto_tagged():
            time.sleep(0.05)

        auto_tagged()
        lines = _slow_lines(slow_records)
        assert len(lines) == 1
        assert "tag=python." in lines[0]
        assert "auto_tagged" in lines[0]

    def test_nested_depth_and_self_ms(self, slow_records):
        """嵌套：depth 递增，外层 self_ms ≈ 外层 ms - 内层 ms"""
        mod = _load_da_logger(dict(_BUILD_ON))

        @mod.log_function_call(slow=True, slow_tag="test.slow.inner", slow_ms=5)
        def inner():
            time.sleep(0.05)

        @mod.log_function_call(slow=True, slow_tag="test.slow.outer", slow_ms=5)
        def outer():
            time.sleep(0.02)
            inner()

        outer()
        lines = _slow_lines(slow_records)
        # 内层先析构先输出
        assert len(lines) == 2
        inner_line = next(l for l in lines if "tag=test.slow.inner" in l)
        outer_line = next(l for l in lines if "tag=test.slow.outer" in l)

        fi = _parse_fields(inner_line)
        fo = _parse_fields(outer_line)
        assert fi["depth"] == 1
        assert fo["depth"] == 0
        assert fi["ms"] >= 45
        assert fo["ms"] >= 65
        # 外层自身耗时 = 总耗时 - 子作用域耗时（≈20ms 的 sleep，容忍定时器粒度抖动）
        assert fo["self_ms"] >= 10
        assert abs(fo["self_ms"] - (fo["ms"] - fi["ms"])) <= 10
        assert fo["self_ms"] < fo["ms"] - 30


class TestBackwardCompatibility:
    def test_plain_decorator_unchanged(self, slow_records):
        """不带参数的 @log_function_call：行为与历史一致，不引入计时输出"""
        mod = _load_da_logger({"DA_SLOW_LOG_BUILD": "1", "DA_SLOW_LOG": "on", "DA_SLOW_LOG_MS": "1"})

        @mod.log_function_call
        def legacy(a, b=2):
            time.sleep(0.02)
            return a + b

        assert legacy(1) == 3
        assert legacy(1, b=5) == 6
        assert _slow_lines(slow_records) == []

    def test_call_macro_semantics_preserved(self, slow_records):
        """slow=True 时返回值与异常语义不变"""
        mod = _load_da_logger(dict(_BUILD_ON))

        @mod.log_function_call(slow=True, slow_tag="test.slow.exc", slow_ms=1)
        def boom():
            time.sleep(0.02)
            raise ValueError("boom")

        with pytest.raises(ValueError):
            boom()
        # 异常路径也计时（finally 输出）
        lines = _slow_lines(slow_records)
        assert len(lines) == 1
        assert 'detail="args_len=0"' in lines[0]
