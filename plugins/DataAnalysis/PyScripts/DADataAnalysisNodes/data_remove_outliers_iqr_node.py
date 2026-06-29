# -*- coding: utf-8 -*-
"""DataRemoveOutliersIQR — IQR outlier removal node"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import remove_outliers_iqr_impl


@NodeDef(name="Remove Outliers IQR", category=_("Data Cleaning"), icon="outlier_iqr")  # cn:数据清洗
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
