# -*- coding: utf-8 -*-
import os
import sys
import threading
import functools
import time
from loguru import logger
import inspect

"""
本文件da_打头的变量和函数属于da系统的默认函数，如果改动会导致da系统异常

此文件封装日志相关的操作
"""
_initialized = False
# 保存原始的 stdout/stderr，以便在关闭时恢复
_original_stdout = None
_original_stderr = None

# ── 慢日志（slow log）运行期配置，模块导入时读取一次 ──
# DA_SLOW_LOG_BUILD 由 C++ 在 Python 初始化前写入（"1"/"0"，见 main.cpp），
# 与 C++ 侧编译期开关 DA_ENABLE_SLOW_LOG 保持一致；用户手动设置会被启动覆盖。
# 为 "1" 之外的值时不生成计时分支，等价于 C++ 宏关闭（零残留）。
_SLOW_BUILD = os.getenv("DA_SLOW_LOG_BUILD") == "1"
_SLOW_ENABLED = _SLOW_BUILD and os.getenv("DA_SLOW_LOG", "on").lower() not in ("0", "off", "false", "no")


def _parse_positive_float_env(name, default):
    """解析浮点环境变量；非法值（非数字、负数）回退默认值"""
    try:
        v = float(os.getenv(name, "") or default)
        return v if v >= 0 else default
    except (TypeError, ValueError):
        return default


_SLOW_DEFAULT_MS = _parse_positive_float_env("DA_SLOW_LOG_MS", 50.0)

# 慢日志嵌套栈（线程局部）：帧为 [开始时刻, 子调用累计秒数]
_slow_local = threading.local()


def _slow_stack():
    stack = getattr(_slow_local, "stack", None)
    if stack is None:
        stack = []
        _slow_local.stack = stack
    return stack


class LoguruWriter:
    """将 stdout/stderr 的输出重定向到 loguru，使 print() 也写入日志文件"""

    # 跨实例共享的重入守卫：loguru handler emit 失败时会通过
    # _error_interceptor.print 回调本 writer，若不加守卫会形成无限递归。
    # 用 threading.local 保证线程安全。
    _reentry_guard = threading.local()

    def __init__(self, level):
        self._level = level

    def write(self, buf):
        # 已经在 loguru 调用链内时直接丢弃，避免 print→log→print 递归
        if getattr(self._reentry_guard, "active", False):
            return
        self._reentry_guard.active = True
        try:
            for line in buf.rstrip().splitlines():
                stripped = line.strip()
                if stripped:
                    logger.opt(depth=1).log(self._level, stripped)
        finally:
            self._reentry_guard.active = False

    def flush(self):
        pass


def setup_logging():
    global _initialized, _original_stdout, _original_stderr
    if _initialized:
        return logger

    appdata = os.getenv("APPDATA") or os.path.expanduser("~")
    da_log_path = os.path.join(appdata, "DAWorkBench", "log")
    os.makedirs(da_log_path, exist_ok=True)
    log_file = os.path.join(da_log_path, "da_pyscript.log")

    # 移除 loguru 默认的 stderr handler（id=0），否则它会输出到已被替换的
    # sys.stderr（LoguruWriter），形成 print→log→print 无限递归。
    # 仅保留下面的文件 handler，日志统一写入文件。
    logger.remove()
    logger.add(log_file, rotation="10 MB", level="DEBUG", enqueue=True)

    # 保存原始 stdout/stderr，然后重定向到 loguru
    _original_stdout = sys.stdout
    _original_stderr = sys.stderr
    sys.stdout = LoguruWriter("INFO")
    sys.stderr = LoguruWriter("WARNING")

    _initialized = True
    return logger


def shutdown_logging():
    global _initialized, _original_stdout, _original_stderr
    if not _initialized:
        return

    # 恢复原始 stdout/stderr
    if _original_stdout is not None:
        sys.stdout = _original_stdout
    if _original_stderr is not None:
        sys.stderr = _original_stderr

    logger.remove()
    _initialized = False
    _original_stdout = None
    _original_stderr = None


def _log_call_args(func, sig, args, kwargs):
    """记录函数调用参数（@log_function_call 的历史行为）"""
    try:
        bound_args = sig.bind(*args, **kwargs)
        bound_args.apply_defaults()  # 应用默认值
        logger.debug(f"Calling {func.__name__} with arguments: {bound_args.arguments}")
    except Exception:
        # 绑定失败时仅记录函数名，不影响正常调用
        logger.debug(f"Calling {func.__name__}")


def log_function_call(func=None, *, slow=False, slow_tag=None, slow_ms=None, slow_detail=None):
    """
    装饰器：自动记录函数调用时的所有参数；可选启用慢调用计时（slow log）。

    兼容两种用法：
    - 直接用作 @log_function_call：仅记录参数，行为与历史版本完全一致，不引入计时；
    - 带参数用作 @log_function_call(slow=True, ...)：记录参数之外增加慢调用计时，
      超过阈值时经 loguru 输出一行 [slow] 记录（写 da_pyscript.log，格式与 C++ 侧一致）。

    慢日志开关（模块导入时读取一次）：
    - DA_SLOW_LOG_BUILD != "1"（C++ 编译期未开启慢日志）：不生成计时分支；
    - DA_SLOW_LOG 为 off/0/false/no：运行期关闭，同样不计时；
    - 阈值缺省读 DA_SLOW_LOG_MS（默认 50ms），可用 slow_ms 按函数覆盖。

    :param func: 直接用作 @log_function_call 时传入的被装饰函数
    :param slow: True 时启用慢调用计时
    :param slow_tag: 自定义慢日志标签（优先于自动生成的 python.<模块>.<限定名>）
    :param slow_ms: 自定义阈值（毫秒），缺省读 DA_SLOW_LOG_MS
    :param slow_detail: 可调用对象，入参 (args, kwargs, result)，仅在超阈值时调用，返回详情字符串
    """

    def decorator(f):
        sig = inspect.signature(f)  # 装饰时计算一次

        # 未启用慢计时（未请求 / 编译期等价关闭 / 运行期关闭）：仅保留历史的参数记录行为
        if not slow or not _SLOW_ENABLED:

            @functools.wraps(f)
            def plain_wrapper(*args, **kwargs):
                _log_call_args(f, sig, args, kwargs)
                return f(*args, **kwargs)

            return plain_wrapper

        # 慢计时路径：标签与阈值在装饰时确定，每次调用只做 perf_counter 与栈操作
        tag = slow_tag if slow_tag else "python.%s.%s" % (f.__module__.split(".")[-1], f.__qualname__)
        threshold_s = (float(slow_ms) if slow_ms is not None else _SLOW_DEFAULT_MS) / 1000.0
        try:
            src_file = os.path.basename(inspect.getsourcefile(f) or f.__code__.co_filename)
            src_line = f.__code__.co_firstlineno
        except Exception:  # pragma: no cover - 拿不到源码信息时用占位
            src_file, src_line = "-", 0

        @functools.wraps(f)
        def slow_wrapper(*args, **kwargs):
            _log_call_args(f, sig, args, kwargs)
            stack = _slow_stack()
            start = time.perf_counter()
            frame = [start, 0.0]  # [开始时刻, 子调用累计秒数]
            stack.append(frame)
            result = None
            try:
                result = f(*args, **kwargs)
                return result
            finally:
                elapsed = time.perf_counter() - start
                if stack and stack[-1] is frame:
                    stack.pop()
                child_s = frame[1]
                if stack:
                    # 自身耗时累加进父帧的子调用累计
                    stack[-1][1] += elapsed
                if elapsed >= threshold_s:
                    try:
                        if slow_detail is not None:
                            detail = str(slow_detail(args, kwargs, result))
                        else:
                            detail = "args_len=%d" % len(args)
                    except Exception:
                        detail = "args_len=%d" % len(args)
                    # 与 C++ 侧字段一致：ms/self_ms/depth/thread/file/line/func/detail
                    logger.warning(
                        '[slow] tag={} ms={:.1f} self_ms={:.1f} depth={} thread={} file={} line={} func={} detail="{}"',
                        tag,
                        elapsed * 1000.0,
                        max(elapsed - child_s, 0.0) * 1000.0,
                        len(stack),
                        threading.current_thread().name,
                        src_file,
                        src_line,
                        f.__qualname__,
                        detail,
                    )

        return slow_wrapper

    if func is not None:
        # 直接用作 @log_function_call（不带参数）
        return decorator(func)
    return decorator


@log_function_call
def fun_test(a: str, b: int, c: bool):
    pass


if __name__ == "__main__":
    # __tst_insert_column()
    fun_test("123", 12, True)
