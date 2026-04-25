# -*- coding: utf-8 -*-
"""
数据筛选节点 — 按条件筛选 DataFrame 行

本节点接收上游 DataFrame，使用 pandas df.query() 按条件筛选数据行，
将筛选结果通过输出端口传递给下游节点。
"""

import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Filter", category="Data Analysis", icon="data_filter")
class DataFilterNode:
    """条件筛选节点"""

    column = Parameter(str, default="", description="筛选目标列名")
    condition = Parameter(str, default="", description="筛选条件表达式（df.query 语法）")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入 DataFrame")

    class Outputs:
        filtered = Output("DataFrame", description="筛选后的 DataFrame")
        removed_count = Output("int", description="移除的行数")

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
        condition = params.get("condition", self.condition.default)
        column = params.get("column", self.column.default)

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
