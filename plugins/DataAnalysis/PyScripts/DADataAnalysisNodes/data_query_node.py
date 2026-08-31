# -*- coding: utf-8 -*-
"""DataQuery — query expression node"""
import os
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.operations import query_dataframe


# 图标目录（包根/icon），按本文件位置计算绝对路径，兼容目录扫描与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon")

@NodeDef(
    name="Query",
    category=_("Data Operations"),  # cn:数据操作
    icon=os.path.join(_ICON_DIR, "query.svg"),
    description=_("Filters DataFrame rows using a pandas query expression (e.g. 'age > 25 and name == \"John\"'). Outputs the filtered data and the result row count."),  # cn:使用 pandas query 表达式（如 'age > 25 and name == "John"'）筛选 DataFrame 行。输出筛选后的数据和结果行数。
)
class DataQueryNode:
    """Filter data using a pandas query expression"""

    query_string = Parameter(str, default="", description=_("query expression, e.g. 'age > 25 and name == \"John\"'"))  # cn:query 表达式，如 'age > 25 and name == "John"'

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        result = Output("DataFrame", description=_("Query result"))  # cn:查询结果
        row_count = Output("int", description=_("Number of result rows"))  # cn:结果行数

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        query_str = params.get("query_string", "")
        if df is None or not query_str:
            return False
        result = query_dataframe(df, query_str)
        self._output_data["result"] = result
        self._output_data["row_count"] = len(result)
        return True
