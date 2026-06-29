# -*- coding: utf-8 -*-
"""DataFillInterpolate — interpolation filling node"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import interpolate_impl


@NodeDef(name="Interpolate", category=_("Data Cleaning"), icon="interpolate")  # cn:数据清洗
class DataFillInterpolateNode:
    """Fill missing values using interpolation"""

    method = Parameter(str, default="linear", description=_("linear/polynomial/spline/time"))  # cn:linear/polynomial/spline/time
    order = Parameter(int, default=2, description=_("Order of polynomial or spline"))  # cn:polynomial 或 spline 的阶数

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        interpolated = Output("DataFrame", description=_("Interpolated data"))  # cn:插值后的数据

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
        result = interpolate_impl(df, subset=None, method=method, limit=None, order=order)
        self._output_data["interpolated"] = result
        return True
