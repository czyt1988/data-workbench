# -*- coding: utf-8 -*-
"""
Data source node - read data from files

Supports CSV, Excel, JSON, and Parquet formats.
The node uses the corresponding pandas function to read the file and passes
the DataFrame to downstream nodes via the output port.
"""

import os
import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.io import read_data


# 图标目录（包根/icon），按本文件位置计算绝对路径，兼容目录扫描与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon")

@NodeDef(
    name="Data Source",
    category=_("Data Analysis"),  # cn:数据分析
    icon=os.path.join(_ICON_DIR, "dataSource.svg"),
    description=_("Loads data from a file (CSV, Excel, JSON, or Parquet) into a DataFrame. Configure the file path and format; CSV supports encoding and separator options, Excel supports sheet selection."),  # cn:从文件（CSV、Excel、JSON 或 Parquet）加载数据为 DataFrame。配置文件路径和格式；CSV 支持编码和分隔符选项，Excel 支持工作表选择。
)
class DataSourceNode:
    """Multi-format data source node."""

    file_path = Parameter("file", default="", description=_("Data file path"), filter="All Supported (*.csv *.xlsx *.xls *.json *.parquet);;CSV Files (*.csv);;Excel Files (*.xlsx *.xls);;JSON Files (*.json);;Parquet Files (*.parquet);;All Files (*.*)")  # cn:数据文件路径
    file_type = Parameter("enum", default="csv", description=_("File format"), enum=["csv", "excel", "json", "parquet"])  # cn:文件格式
    encoding = Parameter(str, default="utf-8", description=_("File encoding (CSV only)"))  # cn:文件编码（仅 CSV 有效）
    separator = Parameter(str, default=",", description=_("Field separator (CSV only)"))  # cn:字段分隔符（仅 CSV 有效）
    sheet_name = Parameter(str, default="0", description=_("Excel sheet name or index (Excel only)"))  # cn:Excel 工作表名称或索引（仅 Excel 有效）

    class Outputs:
        data = Output("DataFrame", description=_("Loaded DataFrame data"))  # cn:读取的 DataFrame 数据
        row_count = Output("int", description=_("Row count"))  # cn:数据行数
        file_type_out = Output("str", description=_("Actual file format loaded"))  # cn:实际读取的文件格式

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
            self._output_data = {"data": None, "row_count": 0, "file_type_out": file_type, "error": _("file_path is empty")}  # cn:file_path 为空
            return False

        if not os.path.exists(file_path):
            self._output_data = {"data": None, "row_count": 0, "file_type_out": file_type, "error": _("File not found: {}").format(file_path)}  # cn:文件不存在: {}
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
