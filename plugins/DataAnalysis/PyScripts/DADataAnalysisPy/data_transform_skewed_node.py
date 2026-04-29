# -*- coding: utf-8 -*-
"""DataTransformSkewed — 偏态变换节点"""
import numpy as np
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Transform Skewed", category="数据清洗", icon="skew")
class DataTransformSkewedNode:
    """对偏态数据进行变换使其接近正态分布"""

    column = Parameter(str, default="", description="要变换的列名")
    method = Parameter(str, default="log", description="log/sqrt/boxcox")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        transformed = Output("DataFrame", description="变换后的数据")

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
        result = df.copy()
        values = result[col].astype(float)
        if method == "log":
            result[col] = np.log1p(values - values.min() + 1)
        elif method == "sqrt":
            result[col] = np.sqrt(values - values.min())
        elif method == "boxcox":
            from scipy.stats import boxcox
            shifted = values - values.min() + 1
            transformed, _ = boxcox(shifted)
            result[col] = transformed
        else:
            result[col] = np.log1p(values - values.min() + 1)
        self._output_data["transformed"] = result
        return True
