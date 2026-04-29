# -*- coding: utf-8 -*-
"""DataDropNa — 删除缺失值节点"""
import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Drop NA", category="数据清洗", icon="drop_na")
class DataDropNaNode:
    """删除包含缺失值的行或列"""

    how = Parameter(str, default="any", description="any: 任一缺失即删除; all: 全部缺失才删除")
    axis = Parameter(int, default=0, description="0: 删除行; 1: 删除列")
    subset = Parameter(str, default="", description="指定检查缺失值的列名，逗号分隔，空表示全部列")
    thresh = Parameter(int, default=0, description="非缺失值最少数量，0 表示不使用")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        cleaned = Output("DataFrame", description="清理后的数据")
        removed_count = Output("int", description="删除的行/列数")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        if df is None:
            return False
        how = params.get("how", "any")
        axis = params.get("axis", 0)
        subset_str = params.get("subset", "")
        thresh = params.get("thresh", 0)
        subset = [s.strip() for s in subset_str.split(",") if s.strip()] if subset_str else None
        if not subset:
            subset = None
        original_len = len(df) if axis == 0 else len(df.columns)
        result = df.dropna(how=how, axis=axis, subset=subset)
        if thresh > 0:
            if axis == 0:
                result = result.dropna(thresh=thresh, axis=axis)
            else:
                result = df.dropna(thresh=thresh, axis=axis)
        cleaned = result
        removed = original_len - (len(cleaned) if axis == 0 else len(cleaned.columns))
        self._output_data["cleaned"] = cleaned
        self._output_data["removed_count"] = removed
        return True
