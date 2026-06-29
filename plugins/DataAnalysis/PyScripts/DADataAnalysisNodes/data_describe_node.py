# -*- coding: utf-8 -*-
"""DataDescribe - descriptive statistics node"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(
    name="Describe",
    category=_("Data Operations"),  # cn:数据操作
    icon="describe",
)
class DataDescribeNode:
    """Generate descriptive statistics summary."""

    percentiles = Parameter(
        str, default="0.25,0.5,0.75", description=_("Percentiles, comma-separated")  # cn:分位数，逗号分隔
    )

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        stats_data = Output("DataFrame", description=_("Descriptive statistics result"))  # cn:描述性统计结果

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        if df is None:
            return False
        pct_str = params.get("percentiles", "0.25,0.5,0.75")
        percentiles = [float(p.strip()) for p in pct_str.split(",") if p.strip()]
        result = df.describe(percentiles=percentiles)
        self._output_data["stats_data"] = result
        return True
