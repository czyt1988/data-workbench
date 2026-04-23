"""
工作流连接模型模块

本模块定义了 DAConnection 类，用于描述工作流中两个节点之间的数据连接关系。
连接由源节点的输出端口指向目标节点的输入端口，构成有向图的边。

DAConnection 是纯 Python 实现，不依赖 Qt，支持序列化（to_dict / from_dict），
序列化字典兼容 DAPyJsonCast（QJsonObject ↔ py::dict）转换。

使用示例::

    conn = DAConnection(
        source_node_id="node_1",
        source_output_channel="result",
        target_node_id="node_2",
        target_input_channel="data",
    )
    # 序列化
    d = conn.to_dict()
    # 反序列化
    conn2 = DAConnection.from_dict(d)
"""

import uuid


class DAConnection:
    """
    工作流节点连接

    描述从源节点的输出端口到目标节点的输入端口的数据传递关系。
    每条连接拥有唯一的 ID，支持序列化和反序列化。

    使用示例::

        conn = DAConnection(
            source_node_id="module.DataFilter_1",
            source_output_channel="filtered",
            target_node_id="module.DataPlot_1",
            target_input_channel="data",
        )

    :param source_node_id: 源节点 ID（字符串）
    :param source_output_channel: 源节点输出端口名称
    :param target_node_id: 目标节点 ID（字符串）
    :param target_input_channel: 目标节点输入端口名称
    :param connection_id: 连接的唯一 ID，默认自动生成
    """

    def __init__(
        self,
        source_node_id: str,
        source_output_channel: str,
        target_node_id: str,
        target_input_channel: str,
        connection_id: str = None,
    ):
        if not source_node_id:
            raise ValueError("source_node_id 不能为空")
        if not source_output_channel:
            raise ValueError("source_output_channel 不能为空")
        if not target_node_id:
            raise ValueError("target_node_id 不能为空")
        if not target_input_channel:
            raise ValueError("target_input_channel 不能为空")
        if source_node_id == target_node_id:
            raise ValueError("源节点和目标节点不能相同（不允许自连接）")

        self.source_node_id = source_node_id
        self.source_output_channel = source_output_channel
        self.target_node_id = target_node_id
        self.target_input_channel = target_input_channel
        # 生成唯一 ID，若未指定则自动生成
        self.connection_id = connection_id if connection_id is not None else str(uuid.uuid4())

    def to_dict(self) -> dict:
        """
        将连接转换为 JSON 可序列化的字典

        生成的字典所有值均为 JSON 兼容类型（str、list、dict、None），
        可直接传递给 C++ 侧通过 DAPyJsonCast (QJsonObject ↔ py::dict) 进行转换。

        :return: JSON 可序列化的字典，包含连接的完整信息
        """
        return {
            "connection_id": self.connection_id,
            "source_node_id": self.source_node_id,
            "source_output_channel": self.source_output_channel,
            "target_node_id": self.target_node_id,
            "target_input_channel": self.target_input_channel,
        }

    @classmethod
    def from_dict(cls, data: dict) -> "DAConnection":
        """
        从字典反序列化创建 DAConnection 实例

        字典格式应与 to_dict() 输出一致，必须包含以下键：
        connection_id, source_node_id, source_output_channel,
        target_node_id, target_input_channel。

        :param data: JSON 可序列化的字典
        :return: DAConnection 实例
        :raises ValueError: 如果字典缺少必要字段
        """
        required_keys = [
            "source_node_id",
            "source_output_channel",
            "target_node_id",
            "target_input_channel",
        ]
        for key in required_keys:
            if key not in data:
                raise ValueError(f"缺少必要字段 '{key}'")

        return cls(
            source_node_id=data["source_node_id"],
            source_output_channel=data["source_output_channel"],
            target_node_id=data["target_node_id"],
            target_input_channel=data["target_input_channel"],
            connection_id=data.get("connection_id", None),
        )

    def __eq__(self, other: object) -> bool:
        """
        判断两个连接是否相等

        以 connection_id 作为相等判据。

        :param other: 另一个对象
        :return: 是否相等
        """
        if not isinstance(other, DAConnection):
            return NotImplemented
        return self.connection_id == other.connection_id

    def __hash__(self) -> int:
        """
        以 connection_id 作为哈希值

        :return: 哈希值
        """
        return hash(self.connection_id)

    def __repr__(self) -> str:
        return (
            f"DAConnection("
            f"source='{self.source_node_id}:{self.source_output_channel}', "
            f"target='{self.target_node_id}:{self.target_input_channel}', "
            f"id='{self.connection_id}')"
        )