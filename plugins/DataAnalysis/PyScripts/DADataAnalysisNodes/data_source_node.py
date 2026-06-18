# -*- coding: utf-8 -*-
"""
数据源节点 — 从文件读取数据

支持 CSV、Excel、JSON、Parquet 四种格式。
节点使用 pandas 对应函数读取文件，将 DataFrame 通过输出端口传递给下游节点。
"""

import os
import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.io import read_data


@NodeDef(name="Data Source", category="Data Analysis", icon="data_source")
class DataSourceNode:
    """多格式数据源节点"""

    file_path = Parameter("file", default="", description="数据文件路径", filter="All Supported (*.csv *.xlsx *.xls *.json *.parquet);;CSV Files (*.csv);;Excel Files (*.xlsx *.xls);;JSON Files (*.json);;Parquet Files (*.parquet);;All Files (*.*)")
    file_type = Parameter("enum", default="csv", description="文件格式", enum=["csv", "excel", "json", "parquet"])
    encoding = Parameter(str, default="utf-8", description="文件编码（仅 CSV 有效）")
    separator = Parameter(str, default=",", description="字段分隔符（仅 CSV 有效）")
    sheet_name = Parameter(str, default="0", description="Excel 工作表名称或索引（仅 Excel 有效）")

    class Outputs:
        data = Output("DataFrame", description="读取的 DataFrame 数据")
        row_count = Output("int", description="数据行数")
        file_type_out = Output("str", description="实际读取的文件格式")

    def __init__(self):
        super().__init__()

    def execute(self, inputs=None, params=None):
        """
        执行数据读取

        根据 file_type 参数选择对应的 pandas 读取函数。
        成功返回 True，失败返回 False。

        :param inputs: 输入数据（本节点无输入端口，忽略）
        :param params: 参数字典
        :return: 成功返回 True，失败返回 False
        """
        if params is None:
            params = {}

        file_path = params.get("file_path", "")
        file_type = (params.get("file_type", "csv") or "csv").strip().lower()
        encoding = params.get("encoding", "utf-8")
        sep = params.get("separator", ",")
        sheet_name = params.get("sheet_name", "0")

        if not file_path:
            self._output_data = {"data": None, "row_count": 0, "file_type_out": file_type, "error": "file_path 为空"}
            return False

        if not os.path.exists(file_path):
            self._output_data = {"data": None, "row_count": 0, "file_type_out": file_type, "error": f"文件不存在: {file_path}"}
            return False

        try:
            df = read_data(file_path, file_type=file_type, encoding=encoding,
                           separator=sep, sheet_name=sheet_name)
            self._output_data["data"] = df
            self._output_data["row_count"] = len(df)
            self._output_data["file_type_out"] = file_type
            return True
        except Exception as e:
            self._output_data = {"data": None, "row_count": 0, "file_type_out": file_type, "error": str(e)}
            return False
