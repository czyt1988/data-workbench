"""
pytest 配置文件

设置 sys.path 使 DAWorkbench 包及其子包可从 src/PyScripts/ 导入。
"""

import os
import sys

# 将 PyScripts 目录加入 sys.path，使 DAWorkbench 及其子包可被导入
_PYSRC_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "src", "PyScripts",
)
if _PYSRC_DIR not in sys.path:
    sys.path.insert(0, _PYSRC_DIR)
