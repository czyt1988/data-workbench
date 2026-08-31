# -*- coding: utf-8 -*-
"""DataFillInterpolate — interpolation filling node"""
import os
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import interpolate_impl


# 图标目录（包根/icon），按本文件位置计算绝对路径，兼容目录扫描与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon")

@NodeDef(
    name="Interpolate",
    category=_("Data Cleaning"),  # cn:数据清洗
    icon=os.path.join(_ICON_DIR, "interpolate.svg"),
    description=_("Fills missing values (NaN) by interpolation. Supports linear, polynomial, spline, and time-based interpolation methods."),  # cn:通过插值填充缺失值（NaN）。支持线性、多项式、样条和时间插值方法。
)
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
