# -*- coding: utf-8 -*-
"""DataFillNa — 填充缺失值节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


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
        if method == "constant":
            try:
                val = float(fill_value)
            except (ValueError, TypeError):
                val = fill_value
            filled = df.fillna(val)
        elif method == "ffill":
            filled = df.ffill()
        elif method == "bfill":
            filled = df.bfill()
        elif method == "mean":
            filled = df.fillna(df.mean(numeric_only=True))
        elif method == "median":
            filled = df.fillna(df.median(numeric_only=True))
        elif method == "mode":
            mode_vals = df.mode().iloc[0] if len(df) > 0 else None
            filled = df.fillna(mode_vals) if mode_vals is not None else df
        else:
            filled = df.fillna(0)
        self._output_data["filled"] = filled
        return True
