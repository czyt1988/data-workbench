# -*- coding: utf-8 -*-
"""
数据源节点 — 从 CSV 文件读取数据

本节点使用 pandas.read_csv 读取 CSV 文件，
将结果 DataFrame 通过输出端口传递给下游节点。
"""

import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Source", category="Data Analysis", icon="data_source")
class DataSourceNode:
    """CSV 数据读取节点"""

    file_path = Parameter(str, default="", description="CSV 文件路径")
    encoding = Parameter(str, default="utf-8", description="文件编码")
    separator = Parameter(str, default=",", description="字段分隔符")

    class Outputs:
        data = Output("DataFrame", description="读取的 DataFrame 数据")
        row_count = Output("int", description="数据行数")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        """
        执行 CSV 数据读取

        根据 params 中指定的 file_path、encoding、separator 参数，
        使用 pandas.read_csv 读取 CSV 文件，并将结果存入 _output_data。

        :param inputs: 输入数据（本节点无输入端口，忽略此参数）
        :param params: 参数字典，包含 file_path、encoding、separator
        :return: 执行成功返回 True，失败返回 False
        """
        if params is None:
            params = {}
        file_path = params.get("file_path", self.file_path.default)
        encoding = params.get("encoding", self.encoding.default)
        sep = params.get("separator", self.separator.default)

        if not file_path:
            return False

        try:
            df = pd.read_csv(file_path, encoding=encoding, sep=sep)
            self._output_data["data"] = df
            self._output_data["row_count"] = len(df)
            return True
        except Exception:
            self._output_data["data"] = None
            self._output_data["row_count"] = 0
            return False
