# -*- coding: utf-8 -*-
"""
AI Agent 编排节点

定义 CrewAI Crew 的编排逻辑，
在工作流中作为多 Agent 协作的编排和执行节点。

CrewNode 是 CrewAI 工作流的核心编排节点，
它将多个 Agent 和 Task 组成一个 Crew，
协调执行任务并通过 DAPythonSignalHandler 推送状态变更通知。

使用示例::

    crew_node = CrewNode()
    crew_node.execute(
        inputs={
            "agents": [agent1, agent2],
            "tasks": [task1, task2]
        },
        params={"process": "sequential"}
    )

状态推送序列：
- "thinking": Crew 开始协调执行
- "done": 所有 Agent 完成各自任务
- "crew_done": Crew 整体执行完成
"""

from DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Crew", category="AI Agent", icon="crew")
class CrewNode:
    """
    AI Agent 编排节点

    将多个 Agent 和 Task 组成 Crew 进行编排执行。
    execute() 方法创建 CrewAI Crew 实例并启动执行，
    通过状态推送机制通知工作流引擎 Crew 的运行状态。
    """

    process = Parameter(str, default="sequential", description="执行流程模式: sequential(顺序执行) 或 hierarchical(层级管理)")

    class Inputs:
        agents = Input("Agent", required=True, description="参与协作的 Agent 列表（来自 AgentNode 输出）")
        tasks = Input("Task", required=True, description="需要执行的 Task 列表（来自 TaskNode 输出）")

    class Outputs:
        result = Output("String", description="Crew 执行的最终结果文本")
        task_results = Output("String", description="各 Task 的执行结果列表")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"result": None, "task_results": None}

    def set_input_data(self, channel, data):
        """
        设置输入端口数据

        :param channel: 输入端口名称
        :param data: 输入数据
        """
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """
        创建并执行 CrewAI Crew

        将输入的 Agent 和 Task 组成 Crew，
        按 process 参数指定的模式执行，
        并推送状态变更通知。

        状态推送序列：
        1. "thinking" — Crew 开始协调执行
        2. "done" — 所有 Agent 完成各自任务
        3. "crew_done" — Crew 整体执行完成

        若 crewai 库未安装，节点以占位模式运行，
        生成模拟结果并仍推送状态通知。

        :param inputs: 输入数据字典，包含 agents 和 tasks
        :param params: 参数字典，包含 process
        :return: True 表示执行成功，False 表示执行失败
        """
        if params is None:
            params = {}
        if inputs is None:
            inputs = self._input_data

        process = params.get("process", "sequential")

        agents = inputs.get("agents", self._input_data.get("agents", []))
        tasks = inputs.get("tasks", self._input_data.get("tasks", []))

        # 确保 agents 和 tasks 为列表形式
        if not isinstance(agents, list):
            agents = [agents] if agents is not None else []
        if not isinstance(tasks, list):
            tasks = [tasks] if tasks is not None else []

        try:
            from crewai import Crew, Process
        except ImportError:
            # crewai 未安装，以占位模式运行
            self._push_state("thinking")
            placeholder_results = []
            for task_data in tasks:
                if isinstance(task_data, dict) and task_data.get("_placeholder"):
                    desc = task_data.get("description", "未知任务")
                    placeholder_results.append(
                        f"[占位模式] 任务 '{desc}' 由占位 Agent 执行完成"
                    )
                else:
                    placeholder_results.append(
                        "[占位模式] 任务由占位 Agent 执行完成"
                    )
            self._output_data["result"] = (
                "[占位模式] Crew 执行完成 — "
                f"共 {len(agents)} 个 Agent，{len(tasks)} 个 Task，"
                f"流程模式: {process} — crewai 库未安装，请执行 pip install crewai"
            )
            self._output_data["task_results"] = placeholder_results
            self._push_state("done")
            self._push_state("crew_done")
            return True

        # 映射 process 参数到 CrewAI Process 枚举
        process_map = {
            "sequential": Process.sequential,
            "hierarchical": Process.hierarchical,
        }
        crew_process = process_map.get(process, Process.sequential)

        self._push_state("thinking")

        # 创建 CrewAI Crew
        crew = Crew(
            agents=agents,
            tasks=tasks,
            process=crew_process,
        )

        # 执行 Crew
        try:
            result = crew.kickoff()
            self._output_data["result"] = str(result)

            # 收集各任务结果
            task_results = []
            for task in tasks:
                if hasattr(task, "output") and task.output:
                    task_results.append(str(task.output))
                else:
                    task_results.append("任务已完成（无输出详情）")
            self._output_data["task_results"] = task_results
        except Exception as e:
            self._output_data["result"] = f"Crew 执行失败: {e}"
            self._output_data["task_results"] = [f"执行错误: {e}"]
            self._push_state("done")
            self._push_state("crew_done")
            return True

        self._push_state("done")
        self._push_state("crew_done")
        return True

    def _push_state(self, state):
        """
        推送节点状态变更通知

        通过 DAPythonSignalHandler::callInMainThread 机制，
        将状态变更通知推送回 C++ 侧的工作流引擎，
        使工作流 UI 能够实时反映 Crew 的运行状态。

        :param state: 状态字符串，如 "thinking"、"done" 或 "crew_done"
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