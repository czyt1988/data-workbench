# -*- coding: utf-8 -*-
"""DataQuery — 查询表达式节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Query", category="数据操作", icon="query")
class DataQueryNode:
    """使用 pandas query 表达式筛选数据"""

    query_string = Parameter(str, default="", description="query 表达式，如 'age > 25 and name == \"John\"'")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        result = Output("DataFrame", description="查询结果")
        row_count = Output("int", description="结果行数")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        query_str = params.get("query_string", "")
        if df is None or not query_str:
            return False
        result = df.query(query_str)
        self._output_data["result"] = result
        self._output_data["row_count"] = len(result)
        return True
