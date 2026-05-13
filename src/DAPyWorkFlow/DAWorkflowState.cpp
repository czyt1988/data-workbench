#include "DAWorkflowState.h"
#include <QDomElement>

namespace DA
{

////////////////////////////////////////////////////
/// DAWorkflowNodeState XML 辅助
////////////////////////////////////////////////////

/**
 * @brief 将 DAWorkflowNodeState 写入 QDomElement
 *
 * 创建 "Node" 子元素，设置 nodeId/qualifiedName 属性，
 * 写入 metaData 子元素和 position 属性。
 *
 * @param[in] parentEle 父元素
 * @param[in] ns 节点状态数据
 * @param[in] doc XML文档指针
 */
static void writeNodeStateToElement(QDomElement& parentEle, const DAWorkflowNodeState& ns, QDomDocument* doc)
{
    QDomElement nodeEle = doc->createElement("Node");
    nodeEle.setAttribute("nodeId", ns.nodeId);
    nodeEle.setAttribute("qualifiedName", ns.qualifiedName);

    // 写入元数据
    QDomElement metaEle = doc->createElement("MetaData");
    metaEle.setAttribute("name", ns.metaData.name);
    metaEle.setAttribute("qualifiedName", ns.metaData.qualifiedName);
    metaEle.setAttribute("group", ns.metaData.group);
    metaEle.setAttribute("iconPath", ns.metaData.iconPath);
    metaEle.setAttribute("tooltip", ns.metaData.tooltip);
    // 输出key列表
    QDomElement outputKeysEle = doc->createElement("OutputKeys");
    for (const QString& key : ns.metaData.outputKeys) {
        QDomElement keyEle = doc->createElement("Key");
        keyEle.setAttribute("value", key);
        outputKeysEle.appendChild(keyEle);
    }
    metaEle.appendChild(outputKeysEle);
    // 输入key列表
    QDomElement inputKeysEle = doc->createElement("InputKeys");
    for (const QString& key : ns.metaData.inputKeys) {
        QDomElement keyEle = doc->createElement("Key");
        keyEle.setAttribute("value", key);
        inputKeysEle.appendChild(keyEle);
    }
    metaEle.appendChild(inputKeysEle);

    nodeEle.appendChild(metaEle);

    // 写入位置
    nodeEle.setAttribute("x", ns.position.x());
    nodeEle.setAttribute("y", ns.position.y());

    parentEle.appendChild(nodeEle);
}

/**
 * @brief 从 QDomElement 解析 DAWorkflowNodeState
 *
 * 从 "Node" 元素读取 nodeId/qualifiedName 属性，
 * 解析 metaData 子元素和 position 属性。
 *
 * @param[in] nodeEle "Node" 元素
 * @return 解析得到的节点状态
 */
static DAWorkflowNodeState readNodeStateFromElement(const QDomElement& nodeEle)
{
    DAWorkflowNodeState ns;
    ns.nodeId        = nodeEle.attribute("nodeId");
    ns.qualifiedName = nodeEle.attribute("qualifiedName");
    ns.position      = QPointF(nodeEle.attribute("x").toDouble(), nodeEle.attribute("y").toDouble());

    // 解析元数据
    QDomElement metaEle = nodeEle.firstChildElement("MetaData");
    if (!metaEle.isNull()) {
        ns.metaData.name           = metaEle.attribute("name");
        ns.metaData.qualifiedName  = metaEle.attribute("qualifiedName");
        ns.metaData.group          = metaEle.attribute("group");
        ns.metaData.iconPath       = metaEle.attribute("iconPath");
        ns.metaData.tooltip        = metaEle.attribute("tooltip");
        // 输出key列表
        QDomElement outputKeysEle = metaEle.firstChildElement("OutputKeys");
        QDomElement keyEle        = outputKeysEle.firstChildElement("Key");
        while (!keyEle.isNull()) {
            ns.metaData.outputKeys.append(keyEle.attribute("value"));
            keyEle = keyEle.nextSiblingElement("Key");
        }
        // 输入key列表
        QDomElement inputKeysEle = metaEle.firstChildElement("InputKeys");
        keyEle                   = inputKeysEle.firstChildElement("Key");
        while (!keyEle.isNull()) {
            ns.metaData.inputKeys.append(keyEle.attribute("value"));
            keyEle = keyEle.nextSiblingElement("Key");
        }
    }
    return ns;
}

////////////////////////////////////////////////////
/// DAWorkflowConnectionState XML 辅助
////////////////////////////////////////////////////

/**
 * @brief 将 DAWorkflowConnectionState 写入 QDomElement
 *
 * 创建 "Connection" 子元素，设置 connectionId/fromNodeId/fromChannel/
 * toNodeId/toChannel 属性。
 *
 * @param[in] parentEle 父元素
 * @param[in] cs 连接线状态数据
 * @param[in] doc XML文档指针
 */
static void writeConnectionStateToElement(QDomElement& parentEle, const DAWorkflowConnectionState& cs, QDomDocument* doc)
{
    QDomElement connEle = doc->createElement("Connection");
    connEle.setAttribute("connectionId", cs.connectionId);
    connEle.setAttribute("fromNodeId", cs.fromNodeId);
    connEle.setAttribute("fromChannel", cs.fromChannel);
    connEle.setAttribute("toNodeId", cs.toNodeId);
    connEle.setAttribute("toChannel", cs.toChannel);
    parentEle.appendChild(connEle);
}

/**
 * @brief 从 QDomElement 解析 DAWorkflowConnectionState
 *
 * 从 "Connection" 元素读取 connectionId/fromNodeId/fromChannel/
 * toNodeId/toChannel 属性。
 *
 * @param[in] connEle "Connection" 元素
 * @return 解析得到的连接线状态
 */
static DAWorkflowConnectionState readConnectionStateFromElement(const QDomElement& connEle)
{
    DAWorkflowConnectionState cs;
    cs.connectionId = connEle.attribute("connectionId");
    cs.fromNodeId   = connEle.attribute("fromNodeId");
    cs.fromChannel  = connEle.attribute("fromChannel").toInt();
    cs.toNodeId     = connEle.attribute("toNodeId");
    cs.toChannel    = connEle.attribute("toChannel").toInt();
    return cs;
}

////////////////////////////////////////////////////
/// DAWorkflowState
////////////////////////////////////////////////////

/**
 * @brief 默认构造函数
 */
DAWorkflowState::DAWorkflowState()
{
}

/**
 * @brief 序列化为XML文档
 *
 * 将工作流状态写入 QDomDocument，创建 "WorkflowState" 根元素，
 * 包含版本号、名称、节点列表和连接线列表子元素。
 * 每个节点为 "Node" 子元素，每条连接线为 "Connection" 子元素。
 *
 * @param[in] doc 目标XML文档
 * @note XML格式与 DAPyWorkFlowSceneSerializer 保持一致，使用属性存储简单值
 */
void DAWorkflowState::toXml(QDomDocument& doc) const
{
    QDomElement rootEle = doc.createElement("WorkflowState");
    rootEle.setAttribute("version", QString::number(Version_1_0_0));

    // 名称
    QDomElement nameEle = doc.createElement("name");
    nameEle.appendChild(doc.createTextNode(this->name));
    rootEle.appendChild(nameEle);

    // 节点列表
    QDomElement nodesEle = doc.createElement("Nodes");
    for (const DAWorkflowNodeState& ns : this->nodes) {
        writeNodeStateToElement(nodesEle, ns, &doc);
    }
    rootEle.appendChild(nodesEle);

    // 连接线列表
    QDomElement connectionsEle = doc.createElement("Connections");
    for (const DAWorkflowConnectionState& cs : this->connections) {
        writeConnectionStateToElement(connectionsEle, cs, &doc);
    }
    rootEle.appendChild(connectionsEle);

    doc.appendChild(rootEle);
}

/**
 * @brief 从XML文档反序列化
 *
 * 从 QDomDocument 中解析 "WorkflowState" 根元素，
 * 读取版本号、名称、节点列表和连接线列表，重建 DAWorkflowState 对象。
 *
 * @param[in] doc 源XML文档
 * @return 解析得到的工作流状态对象
 * @note 如果根元素不存在或版本不匹配，返回空的 DAWorkflowState
 */
DAWorkflowState DAWorkflowState::fromXml(const QDomDocument& doc)
{
    DAWorkflowState state;

    QDomElement rootEle = doc.firstChildElement("WorkflowState");
    if (rootEle.isNull()) {
        return state;
    }

    // 名称
    QDomElement nameEle = rootEle.firstChildElement("name");
    if (!nameEle.isNull()) {
        state.name = nameEle.text();
    }

    // 节点列表
    QDomElement nodesEle = rootEle.firstChildElement("Nodes");
    QDomElement nodeEle  = nodesEle.firstChildElement("Node");
    while (!nodeEle.isNull()) {
        state.nodes.append(readNodeStateFromElement(nodeEle));
        nodeEle = nodeEle.nextSiblingElement("Node");
    }

    // 连接线列表
    QDomElement connectionsEle = rootEle.firstChildElement("Connections");
    QDomElement connEle        = connectionsEle.firstChildElement("Connection");
    while (!connEle.isNull()) {
        state.connections.append(readConnectionStateFromElement(connEle));
        connEle = connEle.nextSiblingElement("Connection");
    }

    return state;
}

}  // namespace DA