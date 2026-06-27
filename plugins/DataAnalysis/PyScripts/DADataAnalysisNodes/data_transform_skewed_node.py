# -*- coding: utf-8 -*-
"""DataTransformSkewed — skewed data transformation node."""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import transform_skewed_impl


@NodeDef(name="Transform Skewed", category=_("Data Cleaning"), icon="skew")  # cn:数据清洗
class DataTransformSkewedNode:
    """Transform skewed data to approximate a normal distribution."""

    column = Parameter(str, default="", description=_("Column to transform"))  # cn:要变换的列名
    method = Parameter(str, default="log", description=_("log/sqrt/boxcox"))  # cn:log/sqrt/boxcox

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        transformed = Output("DataFrame", description=_("Transformed data"))  # cn:变换后的数据

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        col = params.get("column", "")
        if df is None or not col or col not in df.columns:
            return False
        method = params.get("method", "log")
        result = transform_skewed_impl(df, columns=[col], method=method, add_one=True)
        self._output_data["transformed"] = result
        return True
