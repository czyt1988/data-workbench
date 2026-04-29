# -*- coding: utf-8 -*-
"""DataDropDuplicates — 删除重复行节点"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Drop Duplicates", category="数据清洗", icon="drop_dup")
class DataDropDuplicatesNode:
    """删除 DataFrame 中的重复行"""

    subset = Parameter(str, default="", description="用于识别重复的列名，逗号分隔，空表示全部列")
    keep = Parameter(str, default="first", description="first/last/False: 保留第一个/最后一个/不保留")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        cleaned = Output("DataFrame", description="去重后的数据")
        removed_count = Output("int", description="删除的行数")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        if df is None:
            return False
        subset_str = params.get("subset", "")
        subset = [s.strip() for s in subset_str.split(",") if s.strip()] if subset_str else None
        if not subset:
            subset = None
        keep = params.get("keep", "first")
        original = len(df)
        cleaned = df.drop_duplicates(subset=subset, keep=keep)
        self._output_data["cleaned"] = cleaned
        self._output_data["removed_count"] = original - len(cleaned)
        return True
