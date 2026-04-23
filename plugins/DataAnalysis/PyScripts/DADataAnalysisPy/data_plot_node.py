# -*- coding: utf-8 -*-
"""
数据绘图节点 — 使用 matplotlib 创建图表

本节点接收上游 DataFrame，使用 matplotlib 创建指定类型的图表。
matplotlib 不可用时，此模块导入会失败，__init__.py 会跳过此节点。
"""

try:
    import matplotlib
    matplotlib.use("Agg")  # 使用非交互式后端，避免 GUI 依赖
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

import pandas as pd
from DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Plot", category="Data Analysis", icon="data_plot")
class DataPlotNode:
    """数据绘图节点"""

    column = Parameter(str, default="", description="绘图目标列名")
    chart_type = Parameter(str, default="bar", description="图表类型：bar/line/scatter")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入 DataFrame")

    class Outputs:
        figure_path = Output("str", description="图表文件保存路径")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"figure_path": ""}

    def set_input_data(self, channel, data):
        """
        设置输入端口数据

        :param channel: 输入端口名称
        :param data: 输入数据
        """
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """
        执行数据绘图

        根据 chart_type 参数创建对应类型的图表：
        - bar: 柱状图
        - line: 折线图
        - scatter: 散点图（需要两个数值列，以 column 为 X 轴）

        图表保存为 PNG 文件，路径通过输出端口传递。

        :param inputs: 输入数据字典
        :param params: 参数字典，包含 column、chart_type
        :return: 执行成功返回 True，失败返回 False
        :note: matplotlib 不可用时，execute() 直接返回 False
        """
        if not HAS_MATPLOTLIB:
            return False

        if params is None:
            params = {}
        column = params.get("column", self.column.default)
        chart_type = params.get("chart_type", self.chart_type.default)

        df = self._input_data.get("data")
        if df is None:
            return False

        try:
            fig, ax = plt.subplots(figsize=(8, 6))

            if chart_type == "bar":
                if column and column in df.columns:
                    df[column].value_counts().plot(kind="bar", ax=ax)
                    ax.set_title(f"Bar Chart - {column}")
                else:
                    df.plot(kind="bar", ax=ax)
                    ax.set_title("Bar Chart")
            elif chart_type == "line":
                if column and column in df.columns:
                    df[column].plot(kind="line", ax=ax)
                    ax.set_title(f"Line Chart - {column}")
                else:
                    df.plot(kind="line", ax=ax)
                    ax.set_title("Line Chart")
            elif chart_type == "scatter":
                # 散点图需要两个数值列
                numeric_cols = df.select_dtypes(include=["number"]).columns.tolist()
                if len(numeric_cols) >= 2:
                    x_col = column if column and column in numeric_cols else numeric_cols[0]
                    y_col = numeric_cols[1] if x_col == numeric_cols[0] else numeric_cols[0]
                    df.plot(kind="scatter", x=x_col, y=y_col, ax=ax)
                    ax.set_title(f"Scatter - {x_col} vs {y_col}")
                else:
                    ax.set_title("Scatter: insufficient numeric columns")
                    ax.text(0.5, 0.5, "Need at least 2 numeric columns",
                            ha="center", va="center", transform=ax.transAxes)
            else:
                # 默认柱状图
                df.plot(kind="bar", ax=ax)
                ax.set_title("Bar Chart (default)")

            ax.set_xlabel(column if column else "Index")
            fig.tight_layout()

            # 保存为临时文件
            import tempfile
            tmp_dir = tempfile.gettempdir()
            figure_path = os.path.join(tmp_dir, f"data_plot_{chart_type}.png")
            fig.savefig(figure_path, dpi=150)
            plt.close(fig)

            self._output_data["figure_path"] = figure_path
            return True
        except Exception:
            self._output_data["figure_path"] = ""
            return False