# -*- coding: utf-8 -*-
"""
数据变换节点 — 对 DataFrame 列进行变换操作

本节点接收上游 DataFrame，支持列重命名、列删除、缺失值填充等操作，
将变换结果通过输出端口传递给下游节点。
"""

import pandas as pd
from DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Transform", category="Data Analysis", icon="data_transform")
class DataTransformNode:
    """列变换节点"""

    column = Parameter(str, default="", description="操作目标列名")
    operation = Parameter(str, default="rename", description="操作类型：rename/drop/fillna")
    new_name = Parameter(str, default="", description="rename 操作的新列名")
    fill_value = Parameter(str, default="0", description="fillna 操作的填充值")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入 DataFrame")

    class Outputs:
        transformed = Output("DataFrame", description="变换后的 DataFrame")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"transformed": None}

    def set_input_data(self, channel, data):
        """
        设置输入端口数据

        :param channel: 输入端口名称
        :param data: 输入数据
        """
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """
        执行列变换操作

        根据 operation 参数执行不同变换：
        - rename: 将 column 列重命名为 new_name
        - drop: 删除 column 列
        - fillna: 用 fill_value 填充 column 列的缺失值

        :param inputs: 输入数据字典
        :param params: 参数字典，包含 column、operation、new_name、fill_value
        :return: 执行成功返回 True，失败返回 False
        """
        if params is None:
            params = {}
        column = params.get("column", self.column.default)
        operation = params.get("operation", self.operation.default)
        new_name = params.get("new_name", self.new_name.default)
        fill_value = params.get("fill_value", self.fill_value.default)

        df = self._input_data.get("data")
        if df is None:
            return False

        if not column:
            self._output_data["transformed"] = df
            return True

        try:
            result = df.copy()

            if operation == "rename":
                if new_name:
                    result = result.rename(columns={column: new_name})
            elif operation == "drop":
                result = result.drop(columns=[column])
            elif operation == "fillna":
                # 尝试将 fill_value 转换为数值类型
                try:
                    numeric_fill = float(fill_value)
                    if numeric_fill == int(numeric_fill):
                        numeric_fill = int(numeric_fill)
                    result[column] = result[column].fillna(numeric_fill)
                except ValueError:
                    result[column] = result[column].fillna(fill_value)
            else:
                # 未知操作，原样返回
                pass

            self._output_data["transformed"] = result
            return True
        except Exception:
            self._output_data["transformed"] = df
            return False