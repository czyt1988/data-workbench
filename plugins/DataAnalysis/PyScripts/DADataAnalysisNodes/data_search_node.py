# -*- coding: utf-8 -*-
"""DataSearch — search node."""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.operations import search_dataframe


@NodeDef(name="Search", category=_("Data Operations"), icon="search")  # cn:数据操作
class DataSearchNode:
    """Search rows matching a pattern in a given column."""

    column = Parameter(str, default="", description=_("Column to search"))  # cn:要搜索的列名
    pattern = Parameter(str, default="", description=_("Search pattern, supports regex"))  # cn:搜索模式，支持正则
    case_sensitive = Parameter(bool, default=False, description=_("Case sensitive"))  # cn:是否区分大小写

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        result = Output("DataFrame", description=_("Search results"))  # cn:搜索结果
        match_count = Output("int", description=_("Number of matched rows"))  # cn:匹配行数

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        col = params.get("column", "")
        pattern = params.get("pattern", "")
        if df is None or not col or not pattern or col not in df.columns:
            return False
        case = params.get("case_sensitive", False)
        result = search_dataframe(df, col, pattern, case)
        self._output_data["result"] = result
        self._output_data["match_count"] = len(result)
        return True
