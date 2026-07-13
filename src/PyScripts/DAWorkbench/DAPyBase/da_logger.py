# -*- coding: utf-8 -*-
import os
import sys
import threading
import functools
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


def log_function_call(func):
    """
    装饰器：自动记录函数调用时的所有参数。
    """
    sig = inspect.signature(func)  # 装饰时计算一次

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        try:
            bound_args = sig.bind(*args, **kwargs)
            bound_args.apply_defaults()  # 应用默认值
            logger.debug(f"Calling {func.__name__} with arguments: {bound_args.arguments}")
        except Exception:
            # 绑定失败时仅记录函数名，不影响正常调用
            logger.debug(f"Calling {func.__name__}")
        return func(*args, **kwargs)

    return wrapper


@log_function_call
def fun_test(a: str, b: int, c: bool):
    pass


if __name__ == "__main__":
    # __tst_insert_column()
    fun_test("123", 12, True)
