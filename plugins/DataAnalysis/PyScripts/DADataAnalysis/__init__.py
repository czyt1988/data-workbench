# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
# 引入DAWorkbench包
import DAWorkbench
# 你还可以导入包中的其他模块，以便它们可以在包加载时立即可用。  
from . import dataframe_io, dataframe_cleaner

# 对外暴露国际化配置函数（核心API）
from .i18n.core import setup_i18n

# 提前初始化默认语言
setup_i18n()