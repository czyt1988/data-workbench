# -*- coding: utf-8 -*-

import os
import sys
import atexit
from typing import List, Dict, Optional
from .DAPyBase.da_logger import setup_logging, shutdown_logging
# 同时暴露 da_logger、utils 为顶层属性（不依赖 C++ 绑定）
from .DAPyBase import da_logger
from .DAPyBase import utils
# re-export: 保持 DAWorkbench.io、DAWorkbench.dataframe 等旧路径可用
# 以下模块依赖 C++ pybind11 绑定，在纯 Python 环境下可能不可用
try:
    from .DAPyBase import io, dataframe, data_processing
    from .DAPyBase import app_wrapper, thread_status_manager, property_config_builder
except ImportError:
    pass

# 注册 sys.modules 别名，使 "from DAWorkbench.da_logger import ..." 等
# 按子模块路径导入的旧写法仍然有效
_module_name = __name__  # "DAWorkbench"
sys.modules[f"{_module_name}.da_logger"] = da_logger
sys.modules[f"{_module_name}.utils"] = utils
for _submod_name in ("io", "dataframe", "data_processing",
                     "app_wrapper", "thread_status_manager", "property_config_builder"):
    _submod = globals().get(_submod_name)
    if _submod is not None:
        sys.modules[f"{_module_name}.{_submod_name}"] = _submod


def initialize():
    setup_logging()


# !!! 注册清理函数 !!!
# 使用 atexit 确保在 Python 解释器退出前执行
atexit.register(shutdown_logging)


def stop_all_background_tasks():
    """供 C++ 端调用的统一清理函数"""
    shutdown_logging()
    print("[Python] interpreter is shutdown", file=sys.stderr)
