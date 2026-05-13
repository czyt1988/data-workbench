"""
连接语法糖模块

本模块提供 NodeProxy、NodeOutputProxy、NodeInputProxy 类，
用于简化工作流节点间的连接创建。

语法糖不改变底层 DAConnection 的结构和行为，仅提供更简洁的连接方式：

1. 显式端口连接：workflow["node_a"].outputs["data"] >> workflow["node_b"].inputs["data"]
2. 自动端口匹配：workflow["node_a"] >> workflow["node_b"]（单端口节点自动匹配）
3. 方法连接：workflow.connect_node(src_id, src_channel, dst_id, dst_channel)

使用示例::

    wf = DAWorkflow()
    wf.add_node(src)
    wf.add_node(dst)
    # 显式端口
    conn = wf[src.node_id].outputs["data"] >> wf[dst.node_id].inputs["data"]
    # 自动匹配（源和目标都只有 1 个端口时）
    conn = wf[src.node_id] >> wf[dst.node_id]
    # 方法调用
    conn = wf.connect_node(src.node_id, "data", dst.node_id, "data")
"""


def _get_desc_attr(descriptor, key, default=None):
    """安全获取 DANodeDescriptor C++ 结构体属性，自动映射 snake_case 到 camelCase。"""
    attr_map = {"qualified_name": "qualifiedName"}
    attr = attr_map.get(key, key)
    return getattr(descriptor, attr, default)


class NodeOutputProxy:
    """
    节点输出端口代理

    封装 (workflow, node_id, channel) 三元组，支持 >> 操作符
    连接到 NodeInputProxy，自动创建 DAConnection 并添加到工作流。

    :param workflow: DAWorkflow 实例
    :param node_id: 节点 ID
    :param channel: 输出端口名称
    """

    def __init__(self, workflow, node_id: str, channel: str):
        self.workflow = workflow
        self.node_id = node_id
        self.channel = channel

    def __rshift__(self, other):
        """
        >> 操作符：连接到目标输入端口

        支持：
        - NodeOutputProxy >> NodeInputProxy：显式端口连接
        - NodeOutputProxy >> NodeProxy：自动选择目标端口（目标仅有 1 个输入时）

        :param other: NodeInputProxy 或 NodeProxy 实例
        :return: 创建的 DAConnection 实例
        :raises ValueError: 端口歧义时（目标有多个输入端口）
        """
        if isinstance(other, NodeInputProxy):
            return self.workflow.connect_node(
                self.node_id, self.channel,
                other.node_id, other.channel,
            )
        if isinstance(other, NodeProxy):
            # 自动匹配：目标节点只有 1 个输入端口时自动选择
            node = self.workflow.get_node_by_id(other.node_id)
            descriptor = getattr(node, "_node_descriptor", {})
            inputs = _get_desc_attr(descriptor, "inputs", [])
            if len(inputs) != 1:
                raise ValueError(
                    f"ambiguous ports: target node '{other.node_id}' has "
                    f"{len(inputs)} input ports, cannot auto-match"
                )
            target_channel = getattr(inputs[0], "name", "")
            return self.workflow.connect_node(
                self.node_id, self.channel,
                other.node_id, target_channel,
            )
        return NotImplemented

    def __repr__(self) -> str:
        return f"NodeOutputProxy(node_id='{self.node_id}', channel='{self.channel}')"


class NodeInputProxy:
    """
    节点输入端口代理

    封装 (workflow, node_id, channel) 三元组，作为 >> 操作符的右操作数。

    :param workflow: DAWorkflow 实例
    :param node_id: 节点 ID
    :param channel: 输入端口名称
    """

    def __init__(self, workflow, node_id: str, channel: str):
        self.workflow = workflow
        self.node_id = node_id
        self.channel = channel

    def __repr__(self) -> str:
        return f"NodeInputProxy(node_id='{self.node_id}', channel='{self.channel}')"


class _PortAccessor:
    """
    端口访问器基类

    通过端口名 channel 从 C++ DANodeDescriptor 结构体的 port 列表中查找对应代理对象。

    :param workflow: DAWorkflow 实例
    :param node_id: 节点 ID
    :param port_list: DANodeDescriptor 端口描述列表（inputs 或 outputs），元素为 C++ struct
    :param proxy_class: 代理类（NodeOutputProxy 或 NodeInputProxy）
    """

    def __init__(self, workflow, node_id: str, port_list: list, proxy_class: type):
        self._workflow = workflow
        self._node_id = node_id
        self._port_list = port_list
        self._proxy_class = proxy_class

    def __getitem__(self, channel: str):
        """
        通过端口名获取代理对象

        :param channel: 端口名称
        :return: NodeOutputProxy 或 NodeInputProxy
        :raises KeyError: 端口不存在
        """
        for port in self._port_list:
            if port.name == channel:
                return self._proxy_class(self._workflow, self._node_id, channel)
        available = [p.name for p in self._port_list]
        raise KeyError(
            f"端口 '{channel}' 不存在于节点 '{self._node_id}'，"
            f"可用端口: {available}"
        )

    def __repr__(self) -> str:
        names = [p.name for p in self._port_list]
        cls_name = self._proxy_class.__name__.replace("Proxy", "Accessor")
        return f"{cls_name}(node_id='{self._node_id}', ports={names})"


class NodeProxy:
    """
    节点代理

    封装 (workflow, node_id) 二元组，提供 .outputs / .inputs 访问器
    和 >> 操作符自动端口匹配。

    :param workflow: DAWorkflow 实例
    :param node_id: 节点 ID
    """

    def __init__(self, workflow, node_id: str):
        self.workflow = workflow
        self.node_id = node_id
        node = workflow.get_node_by_id(node_id)
        descriptor = getattr(node, "_node_descriptor", {})
        self.outputs = _PortAccessor(
            workflow, node_id,
            _get_desc_attr(descriptor, "outputs", []),
            NodeOutputProxy,
        )
        self.inputs = _PortAccessor(
            workflow, node_id,
            _get_desc_attr(descriptor, "inputs", []),
            NodeInputProxy,
        )

    def __rshift__(self, other):
        """
        >> 操作符：自动端口匹配连接

        当源节点只有 1 个输出端口且目标节点只有 1 个输入端口时，
        自动匹配端口名称，创建 DAConnection。

        :param other: NodeProxy 实例
        :return: 创建的 DAConnection 实例
        :raises ValueError: 端口歧义（源或目标有多个端口）
        """
        if isinstance(other, NodeProxy):
            node = self.workflow.get_node_by_id(self.node_id)
            descriptor = getattr(node, "_node_descriptor", {})
            outputs = _get_desc_attr(descriptor, "outputs", [])
            if len(outputs) != 1:
                raise ValueError(
                    f"ambiguous ports: source node '{self.node_id}' has "
                    f"{len(outputs)} output ports, cannot auto-match"
                )
            src_channel = getattr(outputs[0], "name", "")
            # 利用 NodeOutputProxy 的 >> 处理目标端口匹配
            out_proxy = NodeOutputProxy(self.workflow, self.node_id, src_channel)
            return out_proxy >> other
        return NotImplemented

    def __repr__(self) -> str:
        return f"NodeProxy(node_id='{self.node_id}')"