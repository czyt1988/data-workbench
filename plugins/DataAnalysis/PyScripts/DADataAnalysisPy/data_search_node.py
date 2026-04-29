# -*- coding: utf-8 -*-
"""DataSearch — 搜索节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Search", category="数据操作", icon="search")
class DataSearchNode:
    """在指定列中搜索匹配模式的行"""

    column = Parameter(str, default="", description="要搜索的列名")
    pattern = Parameter(str, default="", description="搜索模式，支持正则")
    case_sensitive = Parameter(bool, default=False, description="是否区分大小写")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        result = Output("DataFrame", description="搜索结果")
        match_count = Output("int", description="匹配行数")

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
        mask = df[col].str.contains(pattern, na=False, case=case, regex=True)
        result = df[mask]
        self._output_data["result"] = result
        self._output_data["match_count"] = len(result)
        return True
