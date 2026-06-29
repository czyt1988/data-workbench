# -*- coding: utf-8 -*-
"""DataSort — sort node."""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.operations import sort_dataframe


@NodeDef(name="Sort", category=_("Data Operations"), icon="sort")  # cn:数据操作
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
