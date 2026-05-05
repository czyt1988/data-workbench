# -*- coding: utf-8 -*-
"""
数据源节点 — 从文件读取数据

支持 CSV、Excel、JSON、Parquet 四种格式。
节点使用 pandas 对应函数读取文件，将 DataFrame 通过输出端口传递给下游节点。
"""

import os
import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Source", category="Data Analysis", icon="data_source")
class DataSourceNode:
    """多格式数据源节点"""

    file_path = Parameter("file", default="", description="数据文件路径", file_filter="All Supported (*.csv *.xlsx *.xls *.json *.parquet);;CSV Files (*.csv);;Excel Files (*.xlsx *.xls);;JSON Files (*.json);;Parquet Files (*.parquet);;All Files (*.*)")
    file_type = Parameter("enum", default="csv", description="文件格式", enum_values=["csv", "excel", "json", "parquet"])
    encoding = Parameter(str, default="utf-8", description="文件编码（仅 CSV 有效）")
    separator = Parameter(str, default=",", description="字段分隔符（仅 CSV 有效）")
    sheet_name = Parameter(str, default="0", description="Excel 工作表名称或索引（仅 Excel 有效）")

    class Outputs:
        data = Output("DataFrame", description="读取的 DataFrame 数据")
        row_count = Output("int", description="数据行数")
        file_type_out = Output("str", description="实际读取的文件格式")

    def __init__(self):
        self._output_data = {}

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

        file_path = params.get("file_path", self.file_path.default)
        file_type = params.get("file_type", self.file_type.default).strip().lower()
        encoding = params.get("encoding", self.encoding.default)
        sep = params.get("separator", self.separator.default)
        sheet_name = params.get("sheet_name", self.sheet_name.default)

        if not file_path:
            self._output_data = {"data": None, "row_count": 0, "file_type_out": file_type, "error": "file_path 为空"}
            return False

        if not os.path.exists(file_path):
            self._output_data = {"data": None, "row_count": 0, "file_type_out": file_type, "error": f"文件不存在: {file_path}"}
            return False

        try:
            if file_type == "csv":
                df = pd.read_csv(file_path, encoding=encoding, sep=sep)
            elif file_type in ("excel", "xlsx"):
                df = pd.read_excel(file_path, sheet_name=sheet_name)
            elif file_type == "json":
                df = pd.read_json(file_path)
            elif file_type == "parquet":
                df = pd.read_parquet(file_path)
            else:
                self._output_data = {"data": None, "row_count": 0, "file_type_out": file_type, "error": f"不支持的文件格式: {file_type}"}
                return False

            self._output_data["data"] = df
            self._output_data["row_count"] = len(df)
            self._output_data["file_type_out"] = file_type
            return True
        except Exception as e:
            self._output_data = {"data": None, "row_count": 0, "file_type_out": file_type, "error": str(e)}
            return False
