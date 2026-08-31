# -*- coding: utf-8 -*-
"""DataSort — sort node."""
import os
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.operations import sort_dataframe


# 图标目录（包根/icon），按本文件位置计算绝对路径，兼容目录扫描与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon")

@NodeDef(
    name="Sort",
    category=_("Data Operations"),  # cn:数据操作
    icon=os.path.join(_ICON_DIR, "sort.svg"),
    description=_("Sorts a DataFrame by one or more columns. Specify column names as a comma-separated list and choose ascending or descending order."),  # cn:按一个或多个列对 DataFrame 排序。列名以逗号分隔，可选择升序或降序。
)
class DataSortNode:
    """Sort a DataFrame."""

    columns = Parameter(str, default="", description=_("Sort column names, comma separated"))  # cn:排序列名，逗号分隔
    ascending = Parameter(bool, default=True, description=_("Ascending order"))  # cn:是否升序

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        sorted = Output("DataFrame", description=_("Sorted data"))  # cn:排序后的数据

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        cols_str = params.get("columns", "")
        if df is None or not cols_str:
            return False
        cols = [c.strip() for c in cols_str.split(",")]
        asc = params.get("ascending", True)
        result = sort_dataframe(df, cols, asc)
        self._output_data["sorted"] = result
        return True
