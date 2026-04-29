# -*- coding: utf-8 -*-
"""DataDescribe — 描述性统计节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Describe", category="数据操作", icon="describe")
class DataDescribeNode:
    """生成描述性统计摘要"""

    percentiles = Parameter(str, default="0.25,0.5,0.75", description="分位数，逗号分隔")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        stats = Output("dict", description="统计信息字典")
        stats_json = Output("str", description="统计信息 JSON 字符串")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        import json
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        if df is None:
            return False
        pct_str = params.get("percentiles", "0.25,0.5,0.75")
        percentiles = [float(p.strip()) for p in pct_str.split(",") if p.strip()]
        result = df.describe(percentiles=percentiles)
        stats_dict = result.to_dict()
        try:
            stats_json = json.dumps(stats_dict, default=str)
        except Exception:
            stats_json = "{}"
        self._output_data["stats"] = stats_dict
        self._output_data["stats_json"] = stats_json
        return True
