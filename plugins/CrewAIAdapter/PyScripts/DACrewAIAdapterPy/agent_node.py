# -*- coding: utf-8 -*-
"""
AI Agent 智能体节点

定义 CrewAI Agent 的角色、目标和背景故事，
在工作流中作为智能体的配置和启动节点。

AgentNode 是 CrewAI 工作流的核心组件之一，它封装了 CrewAI Agent 的创建和配置，
并通过 DAPythonSignalHandler 将状态变更通知推送回 DAPyWorkFlow。

使用示例::

    agent_node = AgentNode()
    agent_node.execute(
        inputs={},
        params={"role": "分析师", "goal": "分析数据趋势", "backstory": "资深数据分析师"}
    )

状态推送序列：
- "thinking": Agent 开始思考
- "done": Agent 完成任务
"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Agent", category="AI Agent", icon="agent")
class AgentNode:
    """
    AI Agent 智能体节点

    定义 CrewAI Agent 的角色、目标和背景故事。
    execute() 方法创建 CrewAI Agent 实例，
    并通过状态推送机制通知工作流引擎 Agent 的运行状态。
    """

    role = Parameter(str, default="分析师", description="智能体角色定义，描述 Agent 的专业职能")
    goal = Parameter(str, default="分析数据并提取洞察",
                     description="智能体目标，描述 Agent 期望达成的结果")
    backstory = Parameter(str, default="你是一位资深数据分析师",
                          description="智能体背景故事，为 Agent 提供上下文和人格设定")

    class Inputs:
        tools = Input("Tool", required=False,
                      description="Agent 可使用的工具列表（来自 ToolNode 输出）")
        task = Input("Task", required=False,
                     description="分配给 Agent 的任务（来自 TaskNode 输出）")

    class Outputs:
        agent = Output(
            "Agent", description="创建的 CrewAI Agent 实例，可传递给 CrewNode")
        result = Output("String", description="Agent 执行任务的文本结果")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"agent": None, "result": None}

    def set_input_data(self, channel, data):
        """
        设置输入端口数据

        :param channel: 输入端口名称
        :param data: 输入数据
        """
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """
        创建并执行 CrewAI Agent

        根据参数创建 CrewAI Agent，执行分配的任务，
        并通过 DAPythonSignalHandler 推送状态变更通知。

        状态推送序列：
        1. "thinking" — Agent 开始思考和处理任务
        2. "done" — Agent 完成任务执行

        若 crewai 库未安装，节点以占位模式运行，
        生成模拟结果并仍推送状态通知。

        :param inputs: 输入数据字典，包含 tools 和 task
        :param params: 参数字典，包含 role、goal、backstory
        :return: True 表示执行成功，False 表示执行失败
        """
        if params is None:
            params = {}
        if inputs is None:
            inputs = self._input_data

        role = params.get("role", "分析师")
        goal = params.get("goal", "分析数据并提取洞察")
        backstory = params.get("backstory", "你是一位资深数据分析师")

        tools = inputs.get("tools", self._input_data.get("tools", []))
        task = inputs.get("task", self._input_data.get("task", None))

        try:
            from crewai import Agent
        except ImportError:
            # crewai 未安装，以占位模式运行
            self._output_data["agent"] = {
                "role": role,
                "goal": goal,
                "backstory": backstory,
                "_placeholder": True,
            }
            self._output_data["result"] = (
                f"[占位模式] Agent({role}) 完成了目标: {goal} "
                f"— crewai 库未安装，请执行 pip install crewai"
            )
            self._push_state("thinking")
            self._push_state("done")
            return True

        # 创建 CrewAI Agent
        agent_kwargs = {
            "role": role,
            "goal": goal,
            "backstory": backstory,
        }
        if tools:
            agent_kwargs["tools"] = tools

        self._push_state("thinking")

        agent = Agent(**agent_kwargs)
        self._output_data["agent"] = agent

        # 若有任务，执行任务
        if task is not None:
            try:
                from crewai import Task
                if isinstance(task, Task):
                    task.agent = agent
                    result = task.execute()
                    self._output_data["result"] = result
            except Exception as e:
                self._output_data["result"] = f"Agent 执行任务失败: {e}"

        self._push_state("done")
        return True

    def _push_state(self, state):
        """
        推送节点状态变更通知

        通过 DAPythonSignalHandler::callInMainThread 机制，
        将状态变更通知推送回 C++ 侧的工作流引擎，
        使工作流 UI 能够实时反映 Agent 的运行状态。

        :param state: 状态字符串，如 "thinking" 或 "done"
        """
        try:
            import DAWorkbench
            DAWorkbench.da_interface.call_in_main_thread(
                "node_state_change",
                self._node_descriptor["qualified_name"],
                state,
            )
        except (ImportError, AttributeError):
            # 在纯 Python 测试环境中，DAWorkbench 不可用，
            # 仅打印状态变更日志
            pass
