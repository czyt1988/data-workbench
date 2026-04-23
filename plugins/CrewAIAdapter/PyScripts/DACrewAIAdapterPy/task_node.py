# -*- coding: utf-8 -*-
"""
AI Agent 任务节点

定义 CrewAI Task 的描述和期望输出，
在工作流中作为任务配置节点。

TaskNode 是 CrewAI 工作流中描述任务需求的节点，
它封装了 CrewAI Task 的创建和配置，
任务的执行由 CrewNode 统一编排。

使用示例::

    task_node = TaskNode()
    task_node.execute(
        inputs={},
        params={
            "description": "分析销售数据并生成趋势报告",
            "expected_output": "一份包含数据趋势分析的报告"
        }
    )
"""

from DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Task", category="AI Agent", icon="task")
class TaskNode:
    """
    AI Agent 任务节点

    定义 CrewAI Task 的描述和期望输出。
    execute() 方法创建 CrewAI Task 实例，
    该 Task 可传递给 CrewNode 进行统一编排执行。
    """

    description = Parameter(str, default="分析数据并生成报告", description="任务描述，详细说明 Agent 需要完成的工作")
    expected_output = Parameter(str, default="一份详细的分析报告", description="期望输出，描述任务完成后应产生的结果格式和内容")

    class Inputs:
        agent = Input("Agent", required=False, description="分配给此任务的 Agent（来自 AgentNode 输出）")
        context = Input("Task", required=False, description="前置任务，此任务的输出将作为上下文输入")

    class Outputs:
        task = Output("Task", description="创建的 CrewAI Task 实例，可传递给 CrewNode")
        result = Output("String", description="任务执行的文本结果")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"task": None, "result": None}

    def set_input_data(self, channel, data):
        """
        设置输入端口数据

        :param channel: 输入端口名称
        :param data: 输入数据
        """
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """
        创建 CrewAI Task

        根据参数创建 CrewAI Task 实例，可配置分配的 Agent 和前置任务上下文。
        Task 实例将通过输出端口传递给 CrewNode 进行编排执行。

        若 crewai 库未安装，节点以占位模式运行，
        生成模拟的 Task 数据结构。

        :param inputs: 输入数据字典，包含 agent 和 context
        :param params: 参数字典，包含 description 和 expected_output
        :return: True 表示执行成功，False 表示执行失败
        """
        if params is None:
            params = {}
        if inputs is None:
            inputs = self._input_data

        description = params.get("description", "分析数据并生成报告")
        expected_output = params.get("expected_output", "一份详细的分析报告")

        agent = inputs.get("agent", self._input_data.get("agent", None))
        context = inputs.get("context", self._input_data.get("context", None))

        try:
            from crewai import Task
        except ImportError:
            # crewai 未安装，以占位模式运行
            self._output_data["task"] = {
                "description": description,
                "expected_output": expected_output,
                "_placeholder": True,
            }
            self._output_data["result"] = (
                f"[占位模式] Task: {description} "
                f"— crewai 库未安装，请执行 pip install crewai"
            )
            return True

        # 创建 CrewAI Task
        task_kwargs = {
            "description": description,
            "expected_output": expected_output,
        }

        if agent is not None:
            task_kwargs["agent"] = agent

        if context is not None:
            # CrewAI 支持多个前置任务作为上下文
            if isinstance(context, list):
                task_kwargs["context"] = context
            else:
                task_kwargs["context"] = [context]

        task = Task(**task_kwargs)
        self._output_data["task"] = task
        self._output_data["result"] = (
            f"Task 已创建: {description[:50]}... "
            f"期望输出: {expected_output[:50]}..."
        )
        return True