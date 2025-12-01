# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
from loguru import logger
#获取当前python脚本文件的上级目录的log文件夹
da_log_path = os.getenv('APPDATA') + r'\DAWorkBench\log'

#日志初始化，添加一个可旋转的日志文件，旋转大小为10Mb，把日志文件存入log_path文件夹下，文件名为da_py_log.log
logger.add(da_log_path+"/da_pyscript.log",rotation="10 MB",level="DEBUG")

# 你还可以导入包中的其他模块，以便它们可以在包加载时立即可用。  
from . import io, dataframe, data_processing, logger, utils
