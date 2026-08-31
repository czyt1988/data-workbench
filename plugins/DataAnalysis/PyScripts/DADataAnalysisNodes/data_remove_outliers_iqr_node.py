# -*- coding: utf-8 -*-
"""DataRemoveOutliersIQR — IQR outlier removal node"""
import os
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import remove_outliers_iqr_impl


# 图标目录（包根/icon），按本文件位置计算绝对路径，兼容目录扫描与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon")

@NodeDef(
    name="Remove Outliers IQR",
    category=_("Data Cleaning"),  # cn:数据清洗
    icon=os.path.join(_ICON_DIR, "outlierIqr.svg"),
    description=_("Removes outlier rows from a DataFrame using the IQR (interquartile range) method. Values outside 1.5×IQR (configurable) from the quartiles are treated as outliers and removed."),  # cn:使用 IQR（四分位距）方法移除 DataFrame 中的异常值行。偏离四分位数 1.5×IQR（可配置）以外的值被视为异常值并移除。
)
class DataRemoveOutliersIQRNode:
    """Remove outliers based on the interquartile range (IQR)"""

    column = Parameter(str, default="", description=_("Column name to check"))  # cn:要检查的列名
    multiplier = Parameter(float, default=1.5, description=_("IQR multiplier, default 1.5"))  # cn:IQR 倍数，默认 1.5

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
        multiplier = params.get("multiplier", 1.5)
        cleaned = remove_outliers_iqr_impl(df, columns=[col], multiplier=multiplier,
                                           action="remove", reindex=False)
        self._output_data["cleaned"] = cleaned
        self._output_data["removed_count"] = len(df) - len(cleaned)
        return True
