# -*- coding: utf-8 -*-
"""DataFillInterpolate — 插值填充节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Interpolate", category="数据清洗", icon="interpolate")
class DataFillInterpolateNode:
    """使用插值法填充缺失值"""

    method = Parameter(str, default="linear", description="linear/polynomial/spline/time")
    order = Parameter(int, default=2, description="polynomial 或 spline 的阶数")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        interpolated = Output("DataFrame", description="插值后的数据")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        if df is None:
            return False
        method = params.get("method", "linear")
        order = params.get("order", 2)
        result = df.interpolate(method=method, order=order)
        self._output_data["interpolated"] = result
        return True
