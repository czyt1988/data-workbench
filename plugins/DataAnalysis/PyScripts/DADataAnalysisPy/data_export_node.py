# -*- coding: utf-8 -*-
"""
数据导出节点 — 将 DataFrame 导出到文件

本节点接收上游 DataFrame，支持导出为 CSV、JSON、Excel、Parquet、Feather 格式文件。
"""

import os
import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Export", category="Data Analysis", icon="data_export")
class DataExportNode:
    """数据导出节点"""

    file_path = Parameter(str, default="", description="导出文件路径")
    export_format = Parameter(str, default="csv", description="导出格式：csv/json/excel/parquet/feather")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入 DataFrame")

    class Outputs:
        success = Output("bool", description="导出是否成功")
        file_path_out = Output("str", description="实际导出文件路径")

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
        file_path = params.get("file_path", self.file_path.default)
        fmt = params.get("export_format", self.export_format.default).strip().lower()

        df = self._input_data.get("data")
        if df is None or not file_path:
            return False

        try:
            # 确保输出目录存在
            output_dir = os.path.dirname(file_path)
            if output_dir and not os.path.exists(output_dir):
                os.makedirs(output_dir, exist_ok=True)

            if fmt == "csv":
                df.to_csv(file_path, index=False)
            elif fmt == "json":
                df.to_json(file_path, force_ascii=False)
            elif fmt in ("excel", "xlsx"):
                df.to_excel(file_path, index=False)
            elif fmt == "parquet":
                df.to_parquet(file_path, index=False)
            elif fmt == "feather":
                df.reset_index().to_feather(file_path)
            else:
                df.to_csv(file_path, index=False)

            self._output_data["success"] = True
            self._output_data["file_path_out"] = file_path
            return True
        except Exception:
            self._output_data["success"] = False
            self._output_data["file_path_out"] = ""
            return False
