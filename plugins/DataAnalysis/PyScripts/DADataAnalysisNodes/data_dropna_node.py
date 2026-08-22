# -*- coding: utf-8 -*-
"""DataDropNa - drop missing values node"""
import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import dropna_impl


@NodeDef(
    name="Drop NA",
    category=_("Data Cleaning"),  # cn:数据清洗
    icon="drop_na",
    description=_("Removes rows or columns containing missing values (NaN). Supports 'any' (drop if any value is missing) or 'all' (drop only if all values are missing) strategies, with optional column subset and minimum non-missing count threshold."),  # cn:删除包含缺失值（NaN）的行或列。支持 'any'（任一缺失即删除）或 'all'（全部缺失才删除）策略，可选列子集和非缺失值最小数量阈值。
)
class DataDropNaNode:
    """Drop rows or columns containing missing values."""

    how = Parameter(str, default="any", description=_("any: drop if any missing; all: drop only if all missing"))  # cn:any: 任一缺失即删除; all: 全部缺失才删除
    axis = Parameter(int, default=0, description=_("0: drop rows; 1: drop columns"))  # cn:0: 删除行; 1: 删除列
    subset = Parameter(str, default="", description=_("Column names to check for missing values, comma-separated, empty means all columns"))  # cn:指定检查缺失值的列名，逗号分隔，空表示全部列
    thresh = Parameter(int, default=0, description=_("Minimum number of non-missing values, 0 means not used"))  # cn:非缺失值最少数量，0 表示不使用

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        cleaned = Output("DataFrame", description=_("Cleaned data"))  # cn:清理后的数据
        removed_count = Output("int", description=_("Number of removed rows/columns"))  # cn:删除的行/列数

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
        if axis == 0:
            cleaned = dropna_impl(df, subset=subset, how=how, min_non_na=thresh, reindex=False)
        else:
            cleaned = df.dropna(how=how, axis=axis, subset=subset, thresh=thresh if thresh > 0 else None)
        removed = original_len - (len(cleaned) if axis == 0 else len(cleaned.columns))
        self._output_data["cleaned"] = cleaned
        self._output_data["removed_count"] = removed
        return True
