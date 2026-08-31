# -*- coding: utf-8 -*-
"""DataFillNa — missing value filling node"""
import os
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter
from DADataAnalysisCore.cleaning import fillna_impl


# 图标目录（包根/icon），按本文件位置计算绝对路径，兼容目录扫描与 entry_points 两种节点发现模式
_ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon")

@NodeDef(
    name="Fill NA",
    category=_("Data Cleaning"),  # cn:数据清洗
    icon=os.path.join(_ICON_DIR, "fillNa.svg"),
    description=_("Fills missing values (NaN) in a DataFrame using a specified method: constant value, forward fill, backward fill, mean, median, or mode."),  # cn:使用指定方法填充 DataFrame 中的缺失值（NaN）：常量值、前向填充、后向填充、均值、中位数或众数。
)
class DataFillNaNode:
    """Fill missing values in a DataFrame"""

    method = Parameter(str, default="value", description=_("constant: fixed value; ffill/bfill: forward/backward fill; mean/median/mode: statistical value"))  # cn:constant: 固定值; ffill/bfill: 前向/后向填充; mean/median/mode: 统计值
    value = Parameter(str, default="0", description=_("Fill value when method=constant"))  # cn:当 method=constant 时的填充值

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Input data"))  # cn:输入数据

    class Outputs:
        filled = Output("DataFrame", description=_("Filled data"))  # cn:填充后的数据

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        if inputs is None or params is None:
            return False
        df = inputs.get("data")
        if df is None:
            return False
        method = params.get("method", "value")
        fill_value = params.get("value", "0")
        # 节点参数名映射到 Core 函数参数名
        method_map = {
            "constant": "value",
            "ffill": "forward",
            "bfill": "backward",
            "mean": "mean",
            "median": "median",
            "mode": "mode",
        }
        core_method = method_map.get(method, "value")
        # 尝试将填充值转为数值
        try:
            value = float(fill_value)
        except (ValueError, TypeError):
            value = fill_value
        filled = fillna_impl(df, subset=None, method=core_method, value=value, limit=None)
        self._output_data["filled"] = filled
        return True
