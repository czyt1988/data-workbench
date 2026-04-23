"""
测试节点定义样本

提供多种 @NodeDef 装饰的节点类，用于工作流和执行器测试。
"""

from DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Source", category="Data", icon="source", render_template="rect")
class DataSourceNode:
    """数据源节点 — 模拟产生数据"""

    class Outputs:
        data = Output("DataFrame", description="输出数据")
        count = Output("int", description="数据行数")

    def __init__(self):
        self._output_data = {"data": [1, 2, 3], "count": 3}

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Data Filter", category="Data Processing", icon="filter")
class DataFilterNode:
    """数据筛选节点 — 模拟接收并筛选数据"""

    threshold = Parameter(float, default=0.5, description="筛选阈值")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    class Outputs:
        filtered = Output("DataFrame", description="筛选后的数据")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"filtered": None}

    def set_input_data(self, channel, data):
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        data = self._input_data.get("data")
        self._output_data["filtered"] = data
        return True


@NodeDef(name="Data Plot", category="Visualization", icon="chart")
class DataPlotNode:
    """数据绘图节点 — 模拟绘图"""

    class Inputs:
        data = Input("DataFrame", required=True, description="绘图数据")

    def __init__(self):
        self._input_data = {}

    def set_input_data(self, channel, data):
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Global Config", category="Config", icon="settings")
class GlobalConfigNode:
    """全局配置节点 — 模拟 is_global"""

    is_global = True

    config_path = Parameter(str, default="/tmp/config.ini", description="配置路径")

    class Outputs:
        config = Output("dict", description="配置字典")

    def __init__(self):
        self._output_data = {"config": {"key": "value"}}

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Error Node", category="Test", icon="error")
class ErrorNode:
    """错误节点 — execute 总是抛异常"""

    class Inputs:
        data = Input("any", required=True)

    class Outputs:
        result = Output("any")

    def __init__(self):
        self._input_data = {}
        self._output_data = {}

    def set_input_data(self, channel, data):
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        raise RuntimeError("ErrorNode 模拟执行失败")


@NodeDef(name="No Output Node", category="Test", icon="noop")
class NoOutputNode:
    """无输出端口节点"""

    class Inputs:
        data = Input("any", required=True)

    def __init__(self):
        self._input_data = {}

    def set_input_data(self, channel, data):
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        return True


@NodeDef(name="Return None Node", category="Test", icon="noop")
class ReturnNoneNode:
    """execute 返回 None 的节点"""

    class Outputs:
        data = Output("any")

    def __init__(self):
        self._output_data = {"data": "some_data"}

    def execute(self, inputs=None, params=None):
        return None