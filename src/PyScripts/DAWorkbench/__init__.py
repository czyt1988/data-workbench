# -*- coding: utf-8 -*-

import os
import sys  # 添加这一行
import atexit
from typing import List, Dict, Optional
from .DALogger import setup_logging, shutdown_logging
# 你还可以导入包中的其他模块，但要避免命名冲突
from . import io, dataframe, data_processing, utils


def initialize():
    setup_logging()

# !!! 注册清理函数 !!!
# 使用 atexit 确保在 Python 解释器退出前执行
atexit.register(shutdown_logging)

def stop_all_background_tasks():
    """供 C++ 端调用的统一清理函数"""
    shutdown_logging()
    print("[Python] interpreter is shutdown", file=sys.stderr)

