# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
# 引入DAWorkbench包
import DAWorkbench
# 对外暴露国际化配置函数（核心API）——必须在导入 GUI 层模块之前调用 setup_i18n()，
# 确保 _() 在子模块加载时已就绪（即便子模块当前仅在函数体内调用 _()，提前初始化也
# 符合 i18n 规范，避免后续在模块顶层误用 _() 时未定义）。
from .i18n.core import setup_i18n
setup_i18n()
# 导入 GUI 层模块
from . import dataframe_io, dataframe_cleaner, utils
# 导入核心算法包（供 GUI 层调用）
import DADataAnalysisCore
