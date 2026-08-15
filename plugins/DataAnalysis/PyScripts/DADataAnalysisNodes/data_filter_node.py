# -*- coding: utf-8 -*-
"""
Data filter node - filter DataFrame rows by condition

This node receives an upstream DataFrame, uses pandas df.query() to filter
rows by condition, and passes the filtered result to downstream nodes via
the output port.
"""

import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(
    name="Data Filter",
    category=_("Data Analysis"),  # cn:数据分析
    icon="data_filter",
    description=_("Filters DataFrame rows using a pandas query expression (e.g. 'age > 25') or by non-null values in a specified column. Outputs the filtered data and the count of removed rows."),  # cn:使用 pandas query 表达式（如 'age > 25'）或指定列的非空值来筛选 DataFrame 行。输出筛选后的数据和移除的行数。
)
class DataFilterNode:
    """Conditional filtering node."""

    column = Parameter(str, default="", description=_("Filter target column name"))  # cn:筛选目标列名
    condition = Parameter(str, default="", description=_("Filter condition expression (df.query syntax)"))  # cn:筛选条件表达式（df.query 语法）

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input DataFrame"))  # cn:输入 DataFrame

    class Outputs:
        filtered = Output("DataFrame", description=_("Filtered DataFrame"))  # cn:筛选后的 DataFrame
        removed_count = Output("int", description=_("Number of removed rows"))  # cn:移除的行数

    def __init__(self):
        self._input_data = {}
        self._output_data = {"filtered": None, "removed_count": 0}

    def set_input_data(self, channel, data):
        """
        设置输入端口数据

        :param channel: 输入端口名称
        :param data: 输入数据
        """
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """
        执行条件筛选

        使用 df.query() 对输入 DataFrame 进行条件筛选。
        如果 condition 为空，则尝试按 column 列筛选非空值。

        :param inputs: 输入数据字典
        :param params: 参数字典，包含 column、condition
        :return: 执行成功返回 True，失败返回 False
        """
        if params is None:
            params = {}
        condition = params.get("condition", "")
        column = params.get("column", "")

        df = self._input_data.get("data")
        if df is None:
            return False

        try:
            original_len = len(df)
            if condition:
                filtered_df = df.query(condition)
            elif column:
                # 无条件表达式时，筛选指定列的非空值
                filtered_df = df[df[column].notna()]
            else:
                filtered_df = df

            self._output_data["filtered"] = filtered_df
            self._output_data["removed_count"] = original_len - \
                len(filtered_df)
            return True
        except Exception:
            self._output_data["filtered"] = df
            self._output_data["removed_count"] = 0
            return False
