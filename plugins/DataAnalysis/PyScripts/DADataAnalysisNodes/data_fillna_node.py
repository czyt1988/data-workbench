# -*- coding: utf-8 -*-
"""DataFillNa — 填充缺失值节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import fillna_impl


@NodeDef(name="Fill NA", category="数据清洗", icon="fill_na")
class DataFillNaNode:
    """填充 DataFrame 中的缺失值"""

    method = Parameter(str, default="value", description="constant: 固定值; ffill/bfill: 前向/后向填充; mean/median/mode: 统计值")
    value = Parameter(str, default="0", description="当 method=constant 时的填充值")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        filled = Output("DataFrame", description="填充后的数据")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        if df is None:
            return False
        method = params.get("method", "value")
        fill_value = params.get("value", "0")
        # 节点参数名映射到 Core 函数参数名
        method_map = {
            "constant": "value",
            "ffill": "forward",
            "bfill": "backward",
            "mean": "mean",
            "median": "median",
            "mode": "mode",
        }
        core_method = method_map.get(method, "value")
        # 尝试将填充值转为数值
        try:
            value = float(fill_value)
        except (ValueError, TypeError):
            value = fill_value
        filled = fillna_impl(df, subset=None, method=core_method, value=value, limit=None)
        self._output_data["filled"] = filled
        return True
