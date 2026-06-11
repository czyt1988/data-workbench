"""
工作流序列化模块

本模块定义了 DAWorkflowSerializer 类，提供 DAWorkflow 的 JSON 序列化和反序列化功能。
序列化格式包含工作流名称、版本号、节点列表和连接列表。

DAWorkflowSerializer 是纯 Python 实现，不依赖 C++。

序列化格式::

    {
      "name": "My Workflow",
      "version": "1.0",
      "nodes": [
        {
          "node_id": "pkg.DataFilter_1",
          "qualified_name": "pkg.DataFilter",
          "parameters": { "column": "value" }
        }
      ],
      "connections": [
        {
          "source_node_id": "pkg.DataFilter_1",
          "source_output_channel": "filtered",
          "target_node_id": "pkg.DataSort_1",
          "target_input_channel": "data"
        }
      ]
    }

使用示例::

    serializer = DAWorkflowSerializer()
    # 序列化
    data = serializer.to_dict(workflow)

    # 反序列化
    workflow = serializer.from_dict(data, node_factory)

    # JSON 文件读写
    serializer.save_to_file(workflow, "/path/to/workflow.json")
    workflow = serializer.load_from_file("/path/to/workflow.json", node_factory)
"""

import json
from pathlib import Path

from .workflow import DAWorkflow
from .connection import DAConnection
from .node_factory import DANodeFactory

# 序列化格式版本号
SERIALIZER_VERSION = "1.0"


class DAWorkflowSerializer:
    """
    工作流序列化器

    提供 DAWorkflow 的 JSON 序列化和反序列化功能。
    序列化将工作流的节点拓扑、参数和连接关系保存为 JSON dict，
    反序列化从 JSON dict 重建 DAWorkflow 实例。

    反序列化需要 DANodeFactory 来根据 qualified_name 创建节点实例。

    使用示例::

        serializer = DAWorkflowSerializer()
        data = serializer.to_dict(workflow)
        workflow = serializer.from_dict(data, node_factory)

    :param node_factory: 可选的节点工厂，设置后可作为 from_dict 的默认工厂
    """

    def __init__(self, node_factory: DANodeFactory = None):
        self._node_factory = node_factory

    def to_dict(self, workflow: DAWorkflow) -> dict:
        """
        将 DAWorkflow 序列化为 JSON dict

        序列化内容结构如下::

            {
              "name": str,            # 工作流名称
              "version": str,         # 序列化格式版本号（当前为 "1.0"）
              "nodes": [              # 节点列表
                {
                  "node_id": str,           # 节点唯一运行时 ID（如 "pkg.DataFilter_1"）
                  "qualified_name": str,    # 节点类型标识（如 "pkg.DataFilter"）
                  "parameters": dict        # 参数当前值，键为参数名，值为参数值
                },
                ...
              ],
              "connections": [        # 连接列表
                {
                  "source_node_id": str,           # 源节点 ID
                  "source_output_channel": str,    # 源节点输出端口名称
                  "target_node_id": str,           # 目标节点 ID
                  "target_input_channel": str,     # 目标节点输入端口名称
                  "connection_id": str             # 连接唯一标识（UUID4）
                },
                ...
              ]
            }

        :param workflow: DAWorkflow 实例
        :return: JSON 可序列化的字典，包含 name、version、nodes、connections 四个顶层键
        :rtype: dict[str, str | list[dict]]
        """
        nodes_data = []
        for node_id, node_instance in workflow._nodes.items():
            node_dict = {
                "node_id": node_id,
                "qualified_name": getattr(node_instance, "qualified_name", ""),
            }
            # 收集参数当前值
            params = {}
            for param_name in getattr(node_instance, "parameters", {}):
                if param_name:
                    # 从实例属性读取当前值（__init__ 已用 default 初始化）
                    value = getattr(node_instance, param_name, None)
                    if value is not None:
                        params[param_name] = value
            node_dict["parameters"] = params
            nodes_data.append(node_dict)

        connections_data = []
        for conn in workflow.get_connections():
            conn_dict = {
                "source_node_id": conn.source_node_id,
                "source_output_channel": conn.source_output_channel,
                "target_node_id": conn.target_node_id,
                "target_input_channel": conn.target_input_channel,
            }
            # 保留 connection_id 用于反序列化时保持 ID 一致性
            conn_dict["connection_id"] = conn.connection_id
            connections_data.append(conn_dict)

        return {
            "name": workflow.name,
            "version": SERIALIZER_VERSION,
            "nodes": nodes_data,
            "connections": connections_data,
        }

    def from_dict(self, data: dict, node_factory: DANodeFactory = None) -> DAWorkflow:
        """
        从 JSON dict 反序列化重建 DAWorkflow

        根据 dict 中的节点列表，通过 node_factory 创建节点实例，
        添加到新的 DAWorkflow 中，然后建立连接关系。

        :param data: 序列化字典，需包含 "nodes" 键（list[dict]），
            每个节点 dict 需包含 "qualified_name"（str）和可选的 "node_id"（str）、
            "parameters"（dict[str, Any]）。可选包含 "connections" 键（list[dict]）
        :param node_factory: DANodeFactory 实例，用于根据 qualified_name 创建节点实例。
            若未指定则使用构造时设置的默认工厂
        :return: 重建的 DAWorkflow 实例，包含所有原始节点和连接
        :raises ValueError: 如果数据格式无效或缺少 "nodes" 字段
        :raises KeyError: 如果节点工厂无法创建指定 qualified_name 的节点
        """
        factory = node_factory or self._node_factory
        if factory is None:
            raise ValueError("反序列化需要提供 node_factory")

        # 验证数据格式
        if "nodes" not in data:
            raise ValueError("序列化数据缺少 'nodes' 字段")

        # 创建工作流
        workflow = DAWorkflow(name=data.get("name", ""))

        # 创建并添加节点
        for node_data in data["nodes"]:
            qualified_name = node_data.get("qualified_name", "")
            node_id = node_data.get("node_id", "")
            parameters = node_data.get("parameters", {})

            # 通过工厂创建节点实例
            node_instance = factory.create_node(qualified_name)

            # 设置参数值
            for param_name, param_value in parameters.items():
                setattr(node_instance, param_name, param_value)

            # 设置 node_id（保持原始 ID 一致性）
            if node_id:
                node_instance.node_id = node_id

            # 添加到工作流（如果 node_id 已设置，add_node 会使用它）
            workflow.add_node(node_instance)

        # 建立连接
        connections_data = data.get("connections", [])
        for conn_data in connections_data:
            conn = DAConnection(
                source_node_id=conn_data["source_node_id"],
                source_output_channel=conn_data["source_output_channel"],
                target_node_id=conn_data["target_node_id"],
                target_input_channel=conn_data["target_input_channel"],
                connection_id=conn_data.get("connection_id", None),
            )
            workflow.add_connection(conn)

        return workflow

    def to_json(self, workflow: DAWorkflow) -> str:
        """
        将 DAWorkflow 序列化为 JSON 字符串

        :param workflow: DAWorkflow 实例
        :return: JSON 字符串
        """
        return json.dumps(self.to_dict(workflow), ensure_ascii=False, indent=2)

    def from_json(self, json_str: str, node_factory: DANodeFactory = None) -> DAWorkflow:
        """
        从 JSON 字符串反序列化重建 DAWorkflow

        :param json_str: JSON 字符串
        :param node_factory: 节点工厂
        :return: 重建的 DAWorkflow 实例
        """
        data = json.loads(json_str)
        return self.from_dict(data, node_factory)

    def save_to_file(self, workflow: DAWorkflow, file_path: str):
        """
        将 DAWorkflow 序列化保存到 JSON 文件

        :param workflow: DAWorkflow 实例
        :param file_path: 目标文件路径
        """
        data = self.to_dict(workflow)
        path = Path(file_path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)

    def load_from_file(self, file_path: str, node_factory: DANodeFactory = None) -> DAWorkflow:
        """
        从 JSON 文件加载并重建 DAWorkflow

        :param file_path: JSON 文件路径
        :param node_factory: 节点工厂
        :return: 重建的 DAWorkflow 实例
        """
        with open(file_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return self.from_dict(data, node_factory)

    def __repr__(self) -> str:
        return f"DAWorkflowSerializer(factory={self._node_factory})"