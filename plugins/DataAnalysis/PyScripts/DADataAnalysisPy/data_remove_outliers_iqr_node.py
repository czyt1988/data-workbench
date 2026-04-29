# -*- coding: utf-8 -*-
"""DataRemoveOutliersIQR — IQR 离群值移除节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Remove Outliers IQR", category="数据清洗", icon="outlier_iqr")
class DataRemoveOutliersIQRNode:
    """基于四分位距(IQR)移除离群值"""

    column = Parameter(str, default="", description="要检查的列名")
    multiplier = Parameter(float, default=1.5, description="IQR 倍数，默认 1.5")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        cleaned = Output("DataFrame", description="清理后的数据")
        removed_count = Output("int", description="移除的行数")

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
        q1 = df[col].quantile(0.25)
        q3 = df[col].quantile(0.75)
        iqr = q3 - q1
        lower = q1 - multiplier * iqr
        upper = q3 + multiplier * iqr
        cleaned = df[(df[col] >= lower) & (df[col] <= upper)]
        self._output_data["cleaned"] = cleaned
        self._output_data["removed_count"] = len(df) - len(cleaned)
        return True
