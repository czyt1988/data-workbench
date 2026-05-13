"""
工作流 DAG 模型模块

本模块定义了 DAWorkflow 类，用于管理工作流的有向无环图（DAG）模型。
DAWorkflow 维护节点实例和连接关系，提供拓扑排序验证、序列化等功能。

DAWorkflow 是纯 Python 实现，不依赖 Qt。节点实例为 NodeDef 装饰器修饰的类实例，
每个节点拥有唯一的字符串 ID（qualified_name + 实例后缀）。

使用示例::

    workflow = DAWorkflow()
    # 添加节点实例
    node1 = DataFilter()  # DataFilter 是 @NodeDef 装饰的类
    workflow.add_node(node1)
    # 添加连接
    conn = DAConnection(
        source_node_id=node1.node_id,
        source_output_channel="filtered",
        target_node_id=node2.node_id,
        target_input_channel="data",
    )
    workflow.add_connection(conn)
    # 检查是否为有效的 DAG
    if workflow.is_valid_dag():
        ...
    # 序列化
    d = workflow.to_dict()
"""

from collections import defaultdict, deque
from .connection import DAConnection
from .syntax import NodeProxy


def _get_desc_attr(descriptor, key, default=None):
    """安全获取节点描述符属性，兼容 dict 和 DANodeDescriptor C++ 结构体。"""
    if isinstance(descriptor, dict):
        return descriptor.get(key, default)
    # DANodeDescriptor C++ struct — 映射 snake_case 到 camelCase
    attr_map = {"qualified_name": "qualifiedName"}
    attr = attr_map.get(key, key)
    return getattr(descriptor, attr, default)


class DAWorkflow:
    """
    工作流 DAG 模型

    管理工作流中的节点实例和连接关系，构成有向无环图（DAG）。
    提供节点增删、连接增删、拓扑排序验证、序列化等核心功能。

    节点实例需具有以下属性：
    - node_id: 字符串，节点的唯一标识（通常为 qualified_name + 实例后缀）
    - qualified_name: 字符串，节点类型的唯一标识（模块名.类名）
    - _node_descriptor: 字典，NodeDef 装饰器生成的描述信息

    使用示例::

        workflow = DAWorkflow()
        workflow.add_node(node_instance)
        workflow.add_connection(conn)
        if not workflow.is_valid_dag():
            print("工作流存在环，无法执行")

    :param name: 工作流名称，默认为空字符串
    """

    def __init__(self, name: str = ""):
        self.name = name
        # 以 node_id 为键，节点实例为值
        self._nodes: dict = {}
        # 以 connection_id 为键，DAConnection 实例为值
        self._connections: dict = {}

    def add_node(self, node_instance: object) -> str:
        """
        添加节点实例到工作流

        为节点自动分配 node_id（如果节点没有 node_id 属性），
        并将节点存储到内部字典中。如果节点已有 node_id 且已存在，
        将抛出 KeyError。

        自动分配的 node_id 格式为 qualified_name 加数字后缀，
        例如 "module.DataFilter_1"。

        :param node_instance: NodeDef 装饰的节点类实例
        :return: 节点的 node_id
        :raises KeyError: 如果 node_id 已存在
        :raises ValueError: 如果节点实例没有 qualified_name 属性
        """
        # 获取 qualified_name
        qualified_name = getattr(node_instance, "qualified_name", None)
        descriptor = getattr(node_instance, "_node_descriptor", None)
        if qualified_name is None and descriptor is not None:
            qualified_name = _get_desc_attr(descriptor, "qualified_name", "")
        if not qualified_name:
            raise ValueError("节点实例必须有 qualified_name 属性或 _node_descriptor.qualified_name 字段")

        # 获取或生成 node_id
        existing_node_id = getattr(node_instance, "node_id", None)
        if existing_node_id is None:
            # 自动分配 node_id: qualified_name + 数字后缀
            counter = 1
            while f"{qualified_name}_{counter}" in self._nodes:
                counter += 1
            node_id = f"{qualified_name}_{counter}"
            node_instance.node_id = node_id  # 设置到实例上
        else:
            node_id = existing_node_id

        if node_id in self._nodes:
            raise KeyError(f"节点 ID '{node_id}' 已存在")

        self._nodes[node_id] = node_instance
        return node_id

    def remove_node(self, node_id: str) -> object:
        """
        从工作流中移除节点

        移除节点时，同时移除与该节点相关的所有连接。

        :param node_id: 要移除的节点 ID
        :return: 被移除的节点实例
        :raises KeyError: 如果 node_id 不存在
        """
        if node_id not in self._nodes:
            raise KeyError(f"节点 ID '{node_id}' 不存在")

        # 先移除相关的连接
        related_conn_ids = [
            cid
            for cid, conn in self._connections.items()
            if conn.source_node_id == node_id or conn.target_node_id == node_id
        ]
        for cid in related_conn_ids:
            self._connections.pop(cid)

        return self._nodes.pop(node_id)

    def add_connection(self, connection: DAConnection) -> str:
        """
        添加连接到工作流

        验证连接的源节点和目标节点均存在于工作流中，
        且不允许重复连接同一对端口。

        :param connection: DAConnection 实例
        :return: 连接的 connection_id
        :raises KeyError: 如果源节点或目标节点不存在
        :raises ValueError: 如果同一对端口已存在连接
        """
        # 验证源节点和目标节点存在
        if connection.source_node_id not in self._nodes:
            raise KeyError(f"源节点 '{connection.source_node_id}' 不存在")
        if connection.target_node_id not in self._nodes:
            raise KeyError(f"目标节点 '{connection.target_node_id}' 不存在")

        # 检查是否已有相同端口的连接
        for existing_conn in self._connections.values():
            if (
                existing_conn.source_node_id == connection.source_node_id
                and existing_conn.source_output_channel == connection.source_output_channel
                and existing_conn.target_node_id == connection.target_node_id
                and existing_conn.target_input_channel == connection.target_input_channel
            ):
                raise ValueError(
                    f"连接已存在: {existing_conn.source_node_id}:{existing_conn.source_output_channel} -> "
                    f"{existing_conn.target_node_id}:{existing_conn.target_input_channel}"
                )

        if connection.connection_id in self._connections:
            raise KeyError(f"连接 ID '{connection.connection_id}' 已存在")

        self._connections[connection.connection_id] = connection
        return connection.connection_id

    def remove_connection(self, connection_id: str) -> DAConnection:
        """
        从工作流中移除连接

        :param connection_id: 要移除的连接 ID
        :return: 被移除的 DAConnection 实例
        :raises KeyError: 如果 connection_id 不存在
        """
        if connection_id not in self._connections:
            raise KeyError(f"连接 ID '{connection_id}' 不存在")
        return self._connections.pop(connection_id)

    def get_nodes(self) -> list:
        """
        获取所有节点实例

        :return: 节点实例列表
        """
        return list(self._nodes.values())

    def get_connections(self) -> list:
        """
        获取所有连接

        :return: DAConnection 实例列表
        """
        return list(self._connections.values())

    def get_node_by_id(self, node_id: str) -> object:
        """
        通过 node_id 获取节点实例

        :param node_id: 节点的唯一 ID
        :return: 节点实例
        :raises KeyError: 如果 node_id 不存在
        """
        if node_id not in self._nodes:
            raise KeyError(f"节点 ID '{node_id}' 不存在")
        return self._nodes[node_id]

    def get_connections_for_node(self, node_id: str) -> list:
        """
        获取与指定节点相关的所有连接

        :param node_id: 节点 ID
        :return: 包含该节点作为源或目标的连接列表
        """
        return [
            conn
            for conn in self._connections.values()
            if conn.source_node_id == node_id or conn.target_node_id == node_id
        ]

    def get_downstream_connections(self, node_id: str, output_channel: str = None) -> list:
        """
        获取指定节点的下游连接

        :param node_id: 源节点 ID
        :param output_channel: 可选，筛选指定输出端口
        :return: 下游连接列表
        """
        result = []
        for conn in self._connections.values():
            if conn.source_node_id == node_id:
                if output_channel is None or conn.source_output_channel == output_channel:
                    result.append(conn)
        return result

    def get_upstream_connections(self, node_id: str, input_channel: str = None) -> list:
        """
        获取指定节点的上游连接

        :param node_id: 目标节点 ID
        :param input_channel: 可选，筛选指定输入端口
        :return: 上游连接列表
        """
        result = []
        for conn in self._connections.values():
            if conn.target_node_id == node_id:
                if input_channel is None or conn.target_input_channel == input_channel:
                    result.append(conn)
        return result

    def is_valid_dag(self) -> bool:
        """
        检查工作流是否为有效的 DAG（无环有向图）

        使用 Kahn 算法进行拓扑排序，如果在排序过程中无法处理所有节点，
        说明存在环，工作流不是有效的 DAG。

        :return: True 表示是有效的 DAG，False 表示存在环
        """
        if not self._nodes:
            return True

        # 构建邻接表和入度计数
        adjacency: dict = defaultdict(list)  # node_id -> [下游 node_id 列表]
        in_degree: dict = defaultdict(int)   # node_id -> 入度数

        for node_id in self._nodes:
            in_degree[node_id] = 0

        for conn in self._connections.values():
            adjacency[conn.source_node_id].append(conn.target_node_id)
            in_degree[conn.target_node_id] += 1

        # Kahn 算法：从入度为0的节点开始
        queue = deque([nid for nid, deg in in_degree.items() if deg == 0])
        visited_count = 0

        while queue:
            current = queue.popleft()
            visited_count += 1
            for downstream in adjacency[current]:
                in_degree[downstream] -= 1
                if in_degree[downstream] == 0:
                    queue.append(downstream)

        # 如果处理了所有节点，说明无环
        return visited_count == len(self._nodes)

    def topological_sort(self) -> list:
        """
        对工作流节点进行拓扑排序

        返回从源节点到终端节点的有序列表。
        如果工作流存在环，将抛出 ValueError。

        :return: 拓扑排序后的 node_id 列表
        :raises ValueError: 如果工作流存在环（不是有效的 DAG）
        """
        if not self.is_valid_dag():
            raise ValueError("工作流存在环，无法进行拓扑排序")

        if not self._nodes:
            return []

        # 构建邻接表和入度计数
        adjacency: dict = defaultdict(list)
        in_degree: dict = defaultdict(int)

        for node_id in self._nodes:
            in_degree[node_id] = 0

        for conn in self._connections.values():
            adjacency[conn.source_node_id].append(conn.target_node_id)
            in_degree[conn.target_node_id] += 1

        # Kahn 算法
        queue = deque([nid for nid, deg in in_degree.items() if deg == 0])
        result = []

        while queue:
            current = queue.popleft()
            result.append(current)
            for downstream in adjacency[current]:
                in_degree[downstream] -= 1
                if in_degree[downstream] == 0:
                    queue.append(downstream)

        return result

    def get_start_nodes(self) -> list:
        """
        获取工作流的起始节点（入度为0且有出度的节点）

        :return: 起始节点 ID 列表
        """
        in_degree = defaultdict(int)
        for conn in self._connections.values():
            in_degree[conn.target_node_id] += 1

        start_nodes = []
        for node_id in self._nodes:
            if in_degree[node_id] == 0:
                # 有出度的才是真正的起始节点
                downstream = self.get_downstream_connections(node_id)
                if downstream:
                    start_nodes.append(node_id)
        return start_nodes

    def get_isolated_nodes(self) -> list:
        """
        获取孤立节点（既没有入度也没有出度的节点）

        :return: 孤立节点 ID 列表
        """
        connected_nodes = set()
        for conn in self._connections.values():
            connected_nodes.add(conn.source_node_id)
            connected_nodes.add(conn.target_node_id)

        return [nid for nid in self._nodes if nid not in connected_nodes]

    def to_dict(self) -> dict:
        """
        将工作流转换为 JSON 可序列化的字典

        生成的字典包含工作流的名称、节点信息和连接信息，
        所有值均为 JSON 兼容类型，可直接传递给 C++ 侧通过
        DAPyJsonCast (QJsonObject ↔ py::dict) 进行转换。

        节点序列化时会提取 node_id、qualified_name、_node_descriptor
        和 _input_data（运行时参数数据）信息。

        :return: JSON 可序列化的字典
        """
        nodes_data = []
        for node_id, node_instance in self._nodes.items():
            descriptor = getattr(node_instance, "_node_descriptor", {})
            qualified_name = getattr(node_instance, "qualified_name", _get_desc_attr(descriptor, "qualified_name", ""))
            node_dict = {
                "node_id": node_id,
                "qualified_name": qualified_name,
                "descriptor": descriptor,
            }
            # 如果节点实例有自定义序列化方法，优先使用
            if hasattr(node_instance, "to_dict"):
                node_dict["instance_data"] = node_instance.to_dict()
            # 序列化节点的运行时输入数据（_input_data）
            input_data = getattr(node_instance, "_input_data", None)
            if input_data is not None and isinstance(input_data, dict):
                # 仅序列化可JSON化的参数值，忽略不可序列化的Python对象引用
                try:
                    json_safe_data = {}
                    for key, value in input_data.items():
                        # 尝试判断是否为JSON安全类型
                        if isinstance(value, (str, int, float, bool, list, dict, type(None))):
                            json_safe_data[key] = value
                        else:
                            # 不可JSON化的对象标记类型，C++侧使用pickle处理
                            json_safe_data[key] = {"__type__": type(value).__name__, "__pickle__": True}
                    if json_safe_data:
                        node_dict["input_data"] = json_safe_data
                except Exception:
                    # 序列化失败时不影响整体流程
                    pass
            nodes_data.append(node_dict)

        connections_data = [conn.to_dict() for conn in self._connections.values()]

        return {
            "name": self.name,
            "nodes": nodes_data,
            "connections": connections_data,
        }

    def fill_state(self, state):
        """
        直接填充 C++ DAWorkflowState 结构体（绕过 JSON 序列化）

        遍历工作流中的节点和连接，将数据直接写入 C++ DAWorkflowState 对象。
        节点元数据通过 DANodeDescriptor.toMetaData() 传递，而非 JSON 字典。

        :param state: da_py_workflow.DAWorkflowState 实例（C++ 侧 pybind11 绑定对象）
        """
        import da_py_workflow

        state.name = self.name

        # 填充节点
        for node_id, node_instance in self._nodes.items():
            ns = da_py_workflow.DAWorkflowNodeState()
            ns.nodeId = node_id
            ns.qualifiedName = getattr(node_instance, "qualified_name", "")
            # 获取节点描述符，优先使用 C++ DANodeDescriptor 对象
            descriptor = getattr(node_instance, "_node_descriptor", None)
            if descriptor is not None:
                # DANodeDescriptor 有 toMetaData() 方法，返回 DAPyNodeMetaData
                to_meta = getattr(descriptor, "toMetaData", None)
                if to_meta is not None:
                    ns.metaData = to_meta()
            ns.position = (0, 0)  # 位置由 C++ Scene 设置
            state.nodes.append(ns)

        # 填充连接
        for conn in self._connections.values():
            cs = da_py_workflow.DAWorkflowConnectionState()
            cs.connectionId = conn.connection_id
            cs.fromNodeId = conn.source_node_id
            cs.fromChannel = conn.source_output_channel
            cs.toNodeId = conn.target_node_id
            cs.toChannel = conn.target_input_channel
            state.connections.append(cs)

    @classmethod
    def from_dict(cls, data: dict, node_registry: object = None) -> "DAWorkflow":
        """
        从字典反序列化创建 DAWorkflow 实例

        反序列化需要能够根据 qualified_name 找到对应的节点类来创建实例。
        如果提供了 node_registry（DANodeRegistry 实例），将通过注册表查找节点类；
        否则，仅存储节点元数据，不创建实例。

        加载时会恢复节点的 _input_data（运行时参数数据），
        但不会自动执行工作流。

        :param data: JSON 可序列化的字典
        :param node_registry: 可选的 DANodeRegistry 实例，用于节点类查找
        :return: DAWorkflow 实例
        :raises ValueError: 如果字典缺少必要字段
        :note 加载后不会自动执行工作流
        """
        workflow = cls(name=data.get("name", ""))

        # 反序列化节点
        nodes_data = data.get("nodes", [])
        for node_data in nodes_data:
            node_id = node_data.get("node_id", "")
            qualified_name = node_data.get("qualified_name", "")
            descriptor = node_data.get("descriptor", {})
            input_data = node_data.get("input_data", {})
            instance_data = node_data.get("instance_data", None)

            if node_registry is not None:
                # 通过注册表查找节点类并创建实例
                try:
                    node_descriptor = node_registry.get_descriptor(qualified_name)
                    # 通过描述符获取节点类
                    node_class = getattr(node_descriptor, "_node_class", None)
                    if node_class is not None:
                        node_instance = node_class()
                        node_instance.node_id = node_id
                        # 恢复运行时输入数据
                        if input_data and hasattr(node_instance, "_input_data"):
                            for key, value in input_data.items():
                                # 检查是否为需要pickle恢复的对象
                                if isinstance(value, dict) and value.get("__pickle__"):
                                    # pickle数据由C++侧处理，Python侧跳过
                                    continue
                                node_instance._input_data[key] = value
                        # 恢复自定义实例数据
                        if instance_data and hasattr(node_instance, "from_dict"):
                            node_instance.from_dict(instance_data)
                        workflow._nodes[node_id] = node_instance
                    else:
                        # 无节点类引用，存储占位
                        placeholder = type("NodePlaceholder", (), {
                            "node_id": node_id,
                            "qualified_name": qualified_name,
                            "_node_descriptor": descriptor,
                            "_input_data": input_data,
                        })()
                        workflow._nodes[node_id] = placeholder
                except KeyError:
                    # 节点类型未注册，存储占位
                    placeholder = type("NodePlaceholder", (), {
                        "node_id": node_id,
                        "qualified_name": qualified_name,
                        "_node_descriptor": descriptor,
                        "_input_data": input_data,
                    })()
                    workflow._nodes[node_id] = placeholder
            else:
                # 无注册表，存储占位
                placeholder = type("NodePlaceholder", (), {
                    "node_id": node_id,
                    "qualified_name": qualified_name,
                    "_node_descriptor": descriptor,
                    "_input_data": input_data,
                })()
                workflow._nodes[node_id] = placeholder

        # 反序列化连接
        connections_data = data.get("connections", [])
        for conn_data in connections_data:
            conn = DAConnection.from_dict(conn_data)
            workflow._connections[conn.connection_id] = conn

        return workflow

    def clear(self):
        """
        清空工作流中的所有节点和连接
        """
        self._nodes.clear()
        self._connections.clear()

    def connect_node(
        self,
        src_node_id: str,
        src_channel: str,
        dst_node_id: str,
        dst_channel: str,
    ) -> DAConnection:
        """
        通过节点 ID 和端口名创建并添加连接

        便捷方法，无需手动构造 DAConnection 实例。
        内部创建 DAConnection 并调用 add_connection 添加到工作流，
        因此同样执行节点存在性和重复连接校验。

        :param src_node_id: 源节点 ID
        :param src_channel: 源节点输出端口名称
        :param dst_node_id: 目标节点 ID
        :param dst_channel: 目标节点输入端口名称
        :return: 创建并添加的 DAConnection 实例
        :raises KeyError: 源节点或目标节点不存在
        :raises ValueError: 同一对端口已存在连接
        """
        conn = DAConnection(
            source_node_id=src_node_id,
            source_output_channel=src_channel,
            target_node_id=dst_node_id,
            target_input_channel=dst_channel,
        )
        self.add_connection(conn)
        return conn

    def __getitem__(self, node_id: str) -> NodeProxy:
        """
        通过 node_id 获取节点代理

        返回 NodeProxy 对象，支持 .outputs["channel"] / .inputs["channel"]
        访问器以及 >> 操作符自动端口匹配。

        :param node_id: 节点 ID
        :return: NodeProxy 实例
        :raises KeyError: node_id 不存在
        """
        if node_id not in self._nodes:
            raise KeyError(f"节点 ID '{node_id}' 不存在")
        return NodeProxy(self, node_id)

    def __len__(self) -> int:
        """
        工作流中节点的数量

        :return: 节点数量
        """
        return len(self._nodes)

    def __contains__(self, node_id: str) -> bool:
        """
        判断节点 ID 是否在工作流中

        :param node_id: 节点 ID
        :return: 是否存在
        """
        return node_id in self._nodes

    def __repr__(self) -> str:
        return f"DAWorkflow(name='{self.name}', nodes={len(self._nodes)}, connections={len(self._connections)})"