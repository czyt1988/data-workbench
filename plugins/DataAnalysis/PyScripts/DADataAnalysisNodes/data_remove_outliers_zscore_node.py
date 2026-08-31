# -*- coding: utf-8 -*-
"""DataRemoveOutliersZScore — Z-score outlier removal node."""
import os
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import remove_outliers_zscore_impl


# 图标目录（包根/icon），按本文件位置计算绝对路径，兼容目录扫描与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon")

@NodeDef(
    name="Remove Outliers Z-Score",
    category=_("Data Cleaning"),  # cn:数据清洗
    icon=os.path.join(_ICON_DIR, "outlierZscore.svg"),
    description=_("Removes outlier rows from a DataFrame using Z-score method. A row is removed if its Z-score (standard deviations from the mean) in the specified column exceeds the threshold (default 3.0)."),  # cn:使用 Z-score 方法移除 DataFrame 中的异常值行。若指定列的 Z-score（偏离均值的标准差倍数）超过阈值（默认 3.0）则移除该行。
)
class DataRemoveOutliersZScoreNode:
    """Remove outliers based on Z-score."""

    column = Parameter(str, default="", description=_("Column to check"))  # cn:要检查的列名
    threshold = Parameter(float, default=3.0, description=_("Z-score threshold, default 3.0"))  # cn:Z-Score 阈值，默认 3.0

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        cleaned = Output("DataFrame", description=_("Cleaned data"))  # cn:清理后的数据
        removed_count = Output("int", description=_("Number of removed rows"))  # cn:移除的行数

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        col = params.get("column", "")
        if df is None or not col or col not in df.columns:
            return False
        threshold = params.get("threshold", 3.0)
        cleaned = remove_outliers_zscore_impl(df, columns=[col], threshold=threshold,
                                              robust=False, action="remove", reindex=False)
        self._output_data["cleaned"] = cleaned
        self._output_data["removed_count"] = len(df) - len(cleaned)
        return True
