# -*- coding: utf-8 -*-
"""
AI Agent 工具节点

定义 Agent 可使用的工具能力，
在工作流中为 Agent 提供工具配置。

ToolNode 是 CrewAI 工作流中为 Agent 提供工具能力的节点，
它封装了 CrewAI Tool 的创建和配置，
创建的工具实例可传递给 AgentNode 供 Agent 在执行任务时使用。

使用示例::

    tool_node = ToolNode()
    tool_node.execute(
        inputs={},
        params={
            "tool_name": "数据查询",
            "tool_description": "查询数据库中的数据记录"
        }
    )
"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Tool", category="AI Agent", icon="tool")
class ToolNode:
    """
    AI Agent 工具节点

    定义 Agent 可使用的工具能力。
    execute() 方法创建 CrewAI Tool 实例，
    该 Tool 可传递给 AgentNode 供 Agent 在执行任务时调用。
    """

    tool_name = Parameter(str, default="数据查询", description="工具名称，标识工具的功能类型")
    tool_description = Parameter(
        str, default="查询数据库中的数据记录", description="工具描述，详细说明工具的功能和使用方式")

    class Inputs:
        config = Input("String", required=False,
                       description="工具配置参数（JSON 格式字符串）")

    class Outputs:
        tool = Output("Tool", description="创建的 CrewAI Tool 实例，可传递给 AgentNode")
        tool_info = Output("String", description="工具信息摘要")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"tool": None, "tool_info": None}

    def set_input_data(self, channel, data):
        """
        设置输入端口数据

        :param channel: 输入端口名称
        :param data: 输入数据
        """
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """
        创建 CrewAI Tool

        根据参数创建 CrewAI Tool 实例，
        该 Tool 可传递给 AgentNode 供 Agent 在执行任务时使用。

        CrewAI Tool 的创建方式取决于工具类型：
        - 对于 crewai 内置工具（如 SerperDevTool、ScrapeWebsiteTool 等），
          可直接从 crewai_tools 导入并实例化
        - 对于自定义工具，可使用 @tool 装饰器定义

        若 crewai 库未安装，节点以占位模式运行，
        生成模拟的 Tool 数据结构。

        :param inputs: 输入数据字典，包含 config
        :param params: 参数字典，包含 tool_name 和 tool_description
        :return: True 表示执行成功，False 表示执行失败
        """
        if params is None:
            params = {}
        if inputs is None:
            inputs = self._input_data

        tool_name = params.get("tool_name", "数据查询")
        tool_description = params.get("tool_description", "查询数据库中的数据记录")
        config = inputs.get("config", self._input_data.get("config", None))

        try:
            from crewai.tools import tool as crewai_tool_decorator
        except ImportError:
            try:
                # crewai 旧版本可能使用不同路径
                from crewai import tool as crewai_tool_decorator
            except ImportError:
                # crewai 未安装，以占位模式运行
                self._output_data["tool"] = {
                    "name": tool_name,
                    "description": tool_description,
                    "_placeholder": True,
                }
                self._output_data["tool_info"] = (
                    f"[占位模式] Tool: {tool_name} — {tool_description} "
                    "— crewai 库未安装，请执行 pip install crewai"
                )
                return True

        # 使用 crewai @tool 装饰器创建自定义工具
        @crewai_tool_decorator(name=tool_name, description=tool_description)
        def custom_tool(query: str) -> str:
            """
            自定义工具的执行函数

            :param query: 工具调用时的查询参数
            :return: 工具执行结果
            """
            # 占位执行逻辑，实际使用时应替换为真实的功能实现
            return f"工具 '{tool_name}' 执行查询: {query}"

        self._output_data["tool"] = custom_tool
        self._output_data["tool_info"] = f"Tool 已创建: {tool_name} — {tool_description}"
        return True
