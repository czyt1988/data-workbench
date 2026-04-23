"""
pytest 配置文件

设置 sys.path 使 DAWorkFlowPy 包可从 src/DAPyWorkFlow/PyScripts/ 导入。
"""

import os
import sys

# 将 DAWorkFlowPy 的父目录加入 sys.path，使包可被导入
_PYSRC_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "src", "DAPyWorkFlow", "PyScripts",
)
if _PYSRC_DIR not in sys.path:
    sys.path.insert(0, _PYSRC_DIR)