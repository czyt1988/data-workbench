# -*- coding: utf-8 -*-
"""
Data export node - export a DataFrame to a file

This node receives an upstream DataFrame and supports exporting it to CSV,
JSON, Excel, Parquet, and Feather format files.
"""

import os
import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.io import export_data


# 图标目录（包根/icon），按本文件位置计算绝对路径，兼容目录扫描与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon")

@NodeDef(
    name="Data Export",
    category=_("Data Analysis"),  # cn:数据分析
    icon=os.path.join(_ICON_DIR, "dataExport.svg"),
    description=_("Exports a DataFrame to a file in CSV, JSON, Excel, Parquet, or Feather format. Specify the output path and format; the output directory is created automatically if it does not exist."),  # cn:将 DataFrame 导出为 CSV、JSON、Excel、Parquet 或 Feather 格式文件。指定输出路径和格式；输出目录不存在时自动创建。
)
class DataExportNode:
    """Data export node."""

    file_path = Parameter("file", default="", description=_("Export file path"), filter="All Files (*.*);;CSV Files (*.csv);;JSON Files (*.json);;Excel Files (*.xlsx);;Parquet Files (*.parquet);;Feather Files (*.feather)")  # cn:导出文件路径
    export_format = Parameter("enum", default="csv", description=_("Export format"), enum=["csv", "json", "excel", "parquet", "feather"])  # cn:导出格式

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input DataFrame"))  # cn:输入 DataFrame

    class Outputs:
        success = Output("bool", description=_("Whether export succeeded"))  # cn:导出是否成功
        file_path_out = Output("str", description=_("Actual export file path"))  # cn:实际导出文件路径

    def __init__(self):
        self._input_data = {}
        self._output_data = {"success": False, "file_path_out": ""}

    def set_input_data(self, channel, data):
        """
        设置输入端口数据

        :param channel: 输入端口名称
        :param data: 输入数据
        """
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """
        执行数据导出

        根据 export_format 参数将 DataFrame 导出为指定格式文件。

        :param inputs: 输入数据字典
        :param params: 参数字典，包含 file_path、export_format
        :return: 执行成功返回 True，失败返回 False
        """
        if params is None:
            params = {}
        file_path = params.get("file_path", "")
        fmt = (params.get("export_format", "csv") or "csv").strip().lower()

        df = self._input_data.get("data")
        if df is None or not file_path:
            return False

        try:
            # 确保输出目录存在
            output_dir = os.path.dirname(file_path)
            if output_dir and not os.path.exists(output_dir):
                os.makedirs(output_dir, exist_ok=True)

            export_data(df, file_path, fmt)

            self._output_data["success"] = True
            self._output_data["file_path_out"] = file_path
            return True
        except Exception:
            self._output_data["success"] = False
            self._output_data["file_path_out"] = ""
            return False
