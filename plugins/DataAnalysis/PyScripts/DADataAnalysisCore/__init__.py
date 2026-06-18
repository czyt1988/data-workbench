# -*- coding: utf-8 -*-
"""
DADataAnalysisCore - 数据分析核心算法包

提供纯 pandas/numpy 实现的数据操作、I/O 和清洗函数。
不依赖 Qt 或 DA 应用框架，可被 GUI 层和工作流节点层共同引用。
"""

from . import operations, io, cleaning
