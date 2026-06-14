# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
# 引入DAWorkbench包
import DAWorkbench
# 导入 GUI 层模块
from . import dataframe_io, dataframe_cleaner, utils
# 导入核心算法包（供 GUI 层调用）
import DADataAnalysisCore

# 对外暴露国际化配置函数（核心API）
from .i18n.core import setup_i18n

# 提前初始化默认语言
setup_i18n()