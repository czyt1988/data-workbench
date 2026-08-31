# -*- coding: utf-8 -*-
"""DataPivotTable — pivot table node"""
import os
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.operations import create_pivot_table


# 图标目录（包根/icon），按本文件位置计算绝对路径，兼容目录扫描与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon")

@NodeDef(
    name="Pivot Table",
    category=_("Data Operations"),  # cn:数据操作
    icon=os.path.join(_ICON_DIR, "pivotTable.svg"),
    description=_("Creates a pivot table from a DataFrame. Specify row index columns, optional column index columns, value columns, and an aggregation function (mean, sum, count, min, or max)."),  # cn:从 DataFrame 创建透视表。指定行索引列、可选的列索引列、值列和聚合函数（均值、求和、计数、最小值或最大值）。
)
class DataPivotTableNode:
    """Create a pivot table"""

    index = Parameter(str, default="", description=_("Row index column name(s), comma-separated"))  # cn:行索引列名，逗号分隔
    columns = Parameter(str, default="", description=_("Column index column name(s), comma-separated"))  # cn:列索引列名，逗号分隔
    values = Parameter(str, default="", description=_("Value column name(s), comma-separated"))  # cn:值列名，逗号分隔
    aggfunc = Parameter(str, default="mean", description=_("Aggregation function: mean/sum/count/min/max"))  # cn:聚合函数: mean/sum/count/min/max

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        pivot = Output("DataFrame", description=_("Pivot table result"))  # cn:透视表结果

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
        result = create_pivot_table(df, index=idx, columns=cols if cols else None, values=vals, aggfunc=agg)
        self._output_data["pivot"] = result
        return True
