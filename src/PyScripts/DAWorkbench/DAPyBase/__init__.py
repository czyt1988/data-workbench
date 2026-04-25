# -*- coding: utf-8 -*-
from .da_logger import setup_logging, shutdown_logging, log_function_call
from . import utils
# 以下模块依赖 C++ pybind11 绑定（da_app, da_interface, da_data），
# 在纯 Python 环境（如 pytest）下可能不可用
try:
    from . import io, dataframe, data_processing
    from . import app_wrapper, thread_status_manager, property_config_builder
except ImportError:
    pass
