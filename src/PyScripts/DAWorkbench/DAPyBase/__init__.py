# -*- coding: utf-8 -*-
from .da_logger import setup_logging, shutdown_logging, log_function_call
from . import utils


def _safe_import_submodule(name: str):
    """独立导入子模块，失败时通过 loguru 记录异常而非静默吞掉。

    `from . import a, b, c` 是原子操作，任一子模块导入失败会导致三个名称
    都不绑定到当前命名空间，从而 DAWorkbench.io 等属性缺失，C++ 端 attr("io")
    返回 None。这里逐个独立导入，单个失败不影响其他可用模块。
    """
    import importlib
    try:
        return importlib.import_module(f"{__name__}.{name}")
    except Exception as _e:
        try:
            from loguru import logger
            logger.exception("Failed to import {}.{}: {}", __name__, name, _e)
        except Exception:
            pass
        return None


# 以下模块依赖 C++ pybind11 绑定（da_app, da_interface, da_data），
# 在纯 Python 环境（如 pytest）下可能不可用
io = _safe_import_submodule("io")
dataframe = _safe_import_submodule("dataframe")
data_processing = _safe_import_submodule("data_processing")
app_wrapper = _safe_import_submodule("app_wrapper")
# 纯 Python 模块，无 C++ 依赖
thread_status_manager = _safe_import_submodule("thread_status_manager")
form_spec = _safe_import_submodule("form_spec")
form_builder = _safe_import_submodule("form_builder")
