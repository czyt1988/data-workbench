# -*- coding: utf-8 -*-
"""DataDropDuplicates - drop duplicate rows node"""
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import drop_duplicates_impl


@NodeDef(
    name="Drop Duplicates",
    category=_("Data Cleaning"),  # cn:数据清洗
    icon="drop_dup",
    description=_("Removes duplicate rows from a DataFrame. Specify which columns to check for duplicates (comma-separated, empty = all columns) and whether to keep the first, last, or no duplicate occurrence."),  # cn:删除 DataFrame 中的重复行。指定用于识别重复的列（逗号分隔，空表示全部列），以及保留第一个、最后一个还是不保留重复项。
)
class DataDropDuplicatesNode:
    """Drop duplicate rows from a DataFrame."""

    subset = Parameter(str, default="", description=_("Column names for identifying duplicates, comma-separated, empty means all columns"))  # cn:用于识别重复的列名，逗号分隔，空表示全部列
    keep = Parameter(str, default="first", description=_("first/last/False: keep first/last/none"))  # cn:first/last/False: 保留第一个/最后一个/不保留

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        cleaned = Output("DataFrame", description=_("Deduplicated data"))  # cn:去重后的数据
        removed_count = Output("int", description=_("Number of removed rows"))  # cn:删除的行数

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
        cleaned = drop_duplicates_impl(df, subset=subset, keep=keep, ignore_index=True)
        self._output_data["cleaned"] = cleaned
        self._output_data["removed_count"] = original - len(cleaned)
        return True
