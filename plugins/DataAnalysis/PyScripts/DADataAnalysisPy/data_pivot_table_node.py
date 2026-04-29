# -*- coding: utf-8 -*-
"""DataPivotTable — 透视表节点"""
import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Pivot Table", category="数据操作", icon="pivot")
class DataPivotTableNode:
    """创建数据透视表"""

    index = Parameter(str, default="", description="行索引列名，逗号分隔")
    columns = Parameter(str, default="", description="列索引列名，逗号分隔")
    values = Parameter(str, default="", description="值列名，逗号分隔")
    aggfunc = Parameter(str, default="mean", description="聚合函数: mean/sum/count/min/max")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        pivot = Output("DataFrame", description="透视表结果")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        if df is None:
            return False
        idx = [s.strip() for s in params.get("index", "").split(",") if s.strip()]
        cols = [s.strip() for s in params.get("columns", "").split(",") if s.strip()]
        vals = [s.strip() for s in params.get("values", "").split(",") if s.strip()]
        agg = params.get("aggfunc", "mean")
        if not idx or not vals:
            return False
        result = pd.pivot_table(df, index=idx, columns=cols if cols else None, values=vals, aggfunc=agg)
        self._output_data["pivot"] = result
        return True
