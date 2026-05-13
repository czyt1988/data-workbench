#include "DAPyWorkFlowSceneSerializer.h"
#include "DAPybind11InQt.h"
#include "DAPyWorkFlowScene.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAPyNodeProxy.h"
#include "DAPyGILGuard.h"
#include "DAWorkflowState.h"
#include "DAXMLFileInterface.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include <QCoreApplication>

// DAPyWorkFlowSceneSerializer不是QObject，使用此宏替代tr()
#define DA_SERIALIZER_TR(sourceText) QCoreApplication::translate("DAPyWorkFlowSceneSerializer", sourceText)

namespace DA
{

/**
 * @brief 从节点图形项获取node_id字符串
 *
 * 用于序列化连接线时引用节点。
 * 优先从Python侧获取node_id属性，其次使用节点名称。
 *
 * @param item 节点图形项指针
 * @return node_id字符串
 * @note 此为内部辅助函数
 */
static QString getNodeItemIdFromItem(DAPyNodeGraphicsItem* item)
{
    if (!item) {
        return QString();
    }
    DAPyNodeProxy* proxy = item->getProxy();
    if (proxy && proxy->hasPyNodeRef()) {
        DAPyGILGuard gil;
        try {
            pybind11::object pyNodeRef = proxy->getPyNodeRef();
            if (pyNodeRef && pybind11::hasattr(pyNodeRef, "id")) {
                std::string idStr = pybind11::str(pyNodeRef.attr("id"));
                return QString::fromStdString(idStr);
            }
        } catch (const pybind11::error_already_set&) {
            qWarning() << "DAPyWorkFlowSceneSerializer: Python exception ignored while getting node ID";
        }
    }
    // 回退到节点名称
    return item->getNodeName();
}

class DAPyWorkFlowSceneSerializer::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlowSceneSerializer)
public:
    PrivateData(DAPyWorkFlowSceneSerializer* p);
    QString mLastErrorString;
};

DAPyWorkFlowSceneSerializer::PrivateData::PrivateData(DAPyWorkFlowSceneSerializer* p) : q_ptr(p)
{
}

////////////////////////////////////////////////////
/// DAPyWorkFlowSceneSerializer
////////////////////////////////////////////////////

/**
 * @brief 构造函数
 */
DAPyWorkFlowSceneSerializer::DAPyWorkFlowSceneSerializer() : DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构函数
 */
DAPyWorkFlowSceneSerializer::~DAPyWorkFlowSceneSerializer()
{
}

/**
 * @brief 保存场景到XML文档
 *
 * 将DAPyWorkFlowScene中的所有节点和连接线序列化为XML格式。
 * workflow级别数据通过DAWorkflowState结构体保存（替代原来的JSON桥接链），
 * 节点位置从C++图形项获取后写入DAWorkflowState。
 * 每个节点图形项还保存以下独立信息：
 * - qualified_name：用于加载时通过DANodeRegistry重建节点
 * - node_id：Python侧的节点唯一标识
 * - 位置信息（x, y）
 * - pickle数据（Python对象序列化）
 * - 渲染模板、节点名称等可视化属性
 *
 * 连接线保存以下信息：
 * - 源节点ID和输出端口名
 * - 目标节点ID和输入端口名
 * - 连接线的可视化属性
 *
 * @param scene 要保存的场景指针
 * @param doc XML文档指针
 * @param ver 版本号
 * @return 保存成功返回true，失败返回false
 */
bool DAPyWorkFlowSceneSerializer::saveSceneToXml(const DAPyWorkFlowScene* scene, QDomDocument* doc, const QVersionNumber& ver)
{
    DA_D(d);
    d->mLastErrorString.clear();
    if (!scene || !doc) {
        d->mLastErrorString = QCoreApplication::translate("DAPyWorkFlowSceneSerializer", "scene或doc指针为空");
        return false;
    }

    // 创建根元素
    QDomElement rootEle = doc->createElement("DAPyWorkFlowScene");
    rootEle.setAttribute("version", ver.toString());
    doc->appendChild(rootEle);

    // 保存Python DAWorkflow的DAG级别数据（使用DAWorkflowState替代JSON桥接链）
    QDomElement workflowDataEle = doc->createElement("workflowData");
    if (scene->hasPyWorkflow()) {
        DAPyGILGuard gil;
        try {
            pybind11::object workflowObj = scene->getPyWorkflow();

            DAWorkflowState state;
            if (workflowObj && pybind11::hasattr(workflowObj, "name")) {
                std::string nameStr = workflowObj.attr("name").cast< std::string >();
                state.name          = QString::fromStdString(nameStr);
            }

            // 从Python workflow获取节点数据
            if (workflowObj && pybind11::hasattr(workflowObj, "_nodes")) {
                pybind11::dict nodesDict = workflowObj.attr("_nodes").cast< pybind11::dict >();
                for (auto item : nodesDict) {
                    DAWorkflowNodeState ns;
                    std::string nodeIdStr   = pybind11::str(item.first);
                    ns.nodeId               = QString::fromStdString(nodeIdStr);
                    pybind11::object nodeInst = item.second.cast< pybind11::object >();
                    if (pybind11::hasattr(nodeInst, "qualified_name")) {
                        std::string qnameStr = pybind11::str(nodeInst.attr("qualified_name"));
                        ns.qualifiedName     = QString::fromStdString(qnameStr);
                    }
                    pybind11::object descriptor = pybind11::getattr(nodeInst, "_node_descriptor", pybind11::none());
                    if (!descriptor.is_none() && pybind11::hasattr(descriptor, "toMetaData")) {
                        pybind11::object metaDataObj = descriptor.attr("toMetaData");
                        ns.metaData                  = metaDataObj.cast< DAPyNodeMetaData >();
                    }
                    state.nodes.append(ns);
                }
            }

            // 从Python workflow获取连接数据
            if (workflowObj && pybind11::hasattr(workflowObj, "_connections")) {
                pybind11::dict connsDict = workflowObj.attr("_connections").cast< pybind11::dict >();
                for (auto item : connsDict) {
                    DAWorkflowConnectionState cs;
                    pybind11::object conn     = item.second.cast< pybind11::object >();
                    if (pybind11::hasattr(conn, "connection_id")) {
                        cs.connectionId = QString::fromStdString(pybind11::str(conn.attr("connection_id")));
                    }
                    if (pybind11::hasattr(conn, "source_node_id")) {
                        cs.fromNodeId = QString::fromStdString(pybind11::str(conn.attr("source_node_id")));
                    }
                    if (pybind11::hasattr(conn, "source_output_channel")) {
                        cs.fromChannel = conn.attr("source_output_channel").cast< int >();
                    }
                    if (pybind11::hasattr(conn, "target_node_id")) {
                        cs.toNodeId = QString::fromStdString(pybind11::str(conn.attr("target_node_id")));
                    }
                    if (pybind11::hasattr(conn, "target_input_channel")) {
                        cs.toChannel = conn.attr("target_input_channel").cast< int >();
                    }
                    state.connections.append(cs);
                }
            }

            // 从C++图形项获取位置并写入DAWorkflowState的节点
            QList< DAPyNodeGraphicsItem* > nodeItems = scene->getPyNodeItems();
            for (DAPyNodeGraphicsItem* nodeItem : nodeItems) {
                DAPyNodeProxy* proxy = nodeItem->getProxy();
                if (proxy && proxy->hasPyNodeRef()) {
                    try {
                        pybind11::object pyNodeRef = proxy->getPyNodeRef();
                        if (pyNodeRef && pybind11::hasattr(pyNodeRef, "id")) {
                            std::string idStr = pybind11::str(pyNodeRef.attr("id"));
                            QString nodeId    = QString::fromStdString(idStr);
                            // 在state.nodes中查找对应节点并更新位置
                            for (DAWorkflowNodeState& ns : state.nodes) {
                                if (ns.nodeId == nodeId) {
                                    ns.position = nodeItem->pos();
                                    break;
                                }
                            }
                        }
                    } catch (const pybind11::error_already_set&) {
                        qWarning() << "DAPyWorkFlowSceneSerializer: Python exception ignored while transferring node position";
                    }
                }
            }

            // 序列化DAWorkflowState到XML
            // toXml()会创建WorkflowState元素并追加到doc（临时成为doc的第二个子元素）
            state.toXml(*doc);

            // 从doc中找到WorkflowState元素并移到workflowDataEle下
            QDomElement workflowStateEle;
            QDomNode child = doc->firstChild();
            while (!child.isNull()) {
                if (child.isElement()) {
                    QDomElement ele = child.toElement();
                    if (ele.tagName() == "WorkflowState") {
                        workflowStateEle = ele;
                        break;
                    }
                }
                child = child.nextSibling();
            }
            if (!workflowStateEle.isNull()) {
                // QDomNode::appendChild会自动从原父节点移除后追加到新父节点
                workflowDataEle.appendChild(workflowStateEle);
            }

        } catch (const pybind11::error_already_set& e) {
            qWarning() << DA_SERIALIZER_TR("DAPyWorkFlowSceneSerializer::saveSceneToXml: Python error saving workflow data: %1")
                              .arg(e.what());
            d->mLastErrorString = DA_SERIALIZER_TR("保存workflow数据时Python异常: %1").arg(e.what());
            // 继续保存场景级别的数据，不因workflow数据保存失败而中断
        }
    }
    rootEle.appendChild(workflowDataEle);

    // 保存节点数据
    QDomElement nodesEle                     = doc->createElement("nodes");
    QList< DAPyNodeGraphicsItem* > nodeItems = scene->getPyNodeItems();
    for (DAPyNodeGraphicsItem* nodeItem : nodeItems) {
        QDomElement nodeEle = doc->createElement("node");

        // 保存节点基本信息
        DAPyNodeProxy* proxy = nodeItem->getProxy();
        if (proxy) {
            nodeEle.setAttribute("qualified_name", proxy->getQualifiedName());
            // 保存Python侧的node_id
            DAPyGILGuard gil;
            try {
                pybind11::object pyNodeRef = proxy->getPyNodeRef();
                if (pyNodeRef && pybind11::hasattr(pyNodeRef, "id")) {
                    std::string idStr = pybind11::str(pyNodeRef.attr("id"));
                    nodeEle.setAttribute("node_id", QString::fromStdString(idStr));
                }
            } catch (const pybind11::error_already_set&) {
                qWarning() << "DAPyWorkFlowSceneSerializer: Python exception ignored while saving node_id";
            }
        }

        // 保存位置信息
        QPointF pos = nodeItem->pos();
        DAXMLFileInterface::appendElementWithText(nodeEle, "x", DA::doubleToString(pos.x()), doc);
        DAXMLFileInterface::appendElementWithText(nodeEle, "y", DA::doubleToString(pos.y()), doc);

        // 保存节点参数值（从Python侧获取config）
        if (proxy && proxy->hasPyNodeRef()) {
            DAPyGILGuard gil;


            // 保存Python对象参数的pickle序列化
            try {
                pybind11::object pyNodeRef = proxy->getPyNodeRef();
                if (pyNodeRef && pybind11::hasattr(pyNodeRef, "get_pickle_data")) {
                    pybind11::object pickleModule = pybind11::module_::import("pickle");
                    pybind11::bytes pickleBytes   = pickleModule.attr("dumps")(pyNodeRef);
                    std::string rawBytes          = pickleBytes;
                    QByteArray qBytes             = QByteArray::fromStdString(rawBytes);
                    QString base64Str             = QString(qBytes.toBase64());
                    QDomElement pickleEle         = doc->createElement("pickleData");
                    QDomText cdataText            = doc->createCDATASection(base64Str);
                    pickleEle.appendChild(cdataText);
                    nodeEle.appendChild(pickleEle);
                }
            } catch (const pybind11::error_already_set& e) {
                qWarning(
                ) << DA_SERIALIZER_TR("DAPyWorkFlowSceneSerializer::saveSceneToXml: Python error saving pickle data: %1")
                         .arg(e.what());
                // pickle保存失败不影响整体流程
            }
        }

        // 保存节点描述符
        // todo

        // 让节点item自己保存可视化属性
        QDomElement itemEle = doc->createElement("itemData");
        if (!nodeItem->saveToXml(doc, &itemEle, ver)) {
            d->mLastErrorString = DA_SERIALIZER_TR("节点item saveToXml失败: %1").arg(nodeItem->getNodeName());
        }
        nodeEle.appendChild(itemEle);

        nodesEle.appendChild(nodeEle);
    }
    rootEle.appendChild(nodesEle);

    // 保存连接线数据
    QDomElement linksEle                     = doc->createElement("links");
    QList< DAPyLinkGraphicsItem* > linkItems = scene->getPyNodeLinkItems();
    for (DAPyLinkGraphicsItem* linkItem : linkItems) {
        QDomElement linkEle = doc->createElement("link");

        // 保存连接的节点信息
        DAPyNodeGraphicsItem* fromNode = linkItem->getFromNode();
        DAPyNodeGraphicsItem* toNode   = linkItem->getToNode();

        if (fromNode) {
            linkEle.setAttribute("fromNodeId", getNodeItemIdFromItem(fromNode));
            linkEle.setAttribute("fromOutput", linkItem->getFromOutputName());
        }
        if (toNode) {
            linkEle.setAttribute("toNodeId", getNodeItemIdFromItem(toNode));
            linkEle.setAttribute("toInput", linkItem->getToInputName());
        }

        // 让连接线item自己保存可视化属性
        QDomElement itemEle = doc->createElement("itemData");
        if (!linkItem->saveToXml(doc, &itemEle, ver)) {
            d->mLastErrorString = DA_SERIALIZER_TR("连接线item saveToXml失败");
        }
        linkEle.appendChild(itemEle);

        linksEle.appendChild(linkEle);
    }
    rootEle.appendChild(linksEle);

    return true;
}

/**
 * @brief 从XML元素加载场景
 *
 * 从XML数据恢复DAPyWorkFlowScene中的节点和连接线。
 * 加载过程：
 * 1. 清空当前场景
 * 2. 从workflowData/WorkflowState解析DAWorkflowState（替代原JSON桥接链）
 * 3. 从nodes XML节构建位置映射表，遍历DAWorkflowState.nodes创建节点图形项
 * 4. 恢复节点pickle数据和可视化属性
 * 5. 遍历DAWorkflowState.connections创建连接线（通道编号→端口名称转换）
 * 6. 恢复连接线可视化属性
 *
 * 加载完成后不会自动执行工作流。
 *
 * @param sceneElement XML场景元素
 * @param scene 目标场景指针
 * @param ver 版本号
 * @return 加载成功返回true，失败返回false
 * @note 加载后不会自动执行工作流
 */
bool DAPyWorkFlowSceneSerializer::loadSceneFromXml(
    const QDomElement* sceneElement, DAPyWorkFlowScene* scene, const QVersionNumber& ver
)
{
    DA_D(d);
    d->mLastErrorString.clear();
    if (!sceneElement || !scene) {
        d->mLastErrorString = DA_SERIALIZER_TR("sceneElement或scene指针为空");
        return false;
    }

    // 清空当前场景
    scene->clearPyScene();

    // 建立node_id到图形项的映射表
    QMap< QString, DAPyNodeGraphicsItem* > nodeIdToItemMap;

    // 1. 从workflowData加载DAWorkflowState（替代原JSON桥接链）
    DAWorkflowState workflowState;
    QDomElement workflowDataEle = sceneElement->firstChildElement("workflowData");
    if (!workflowDataEle.isNull()) {
        QDomElement workflowStateEle = workflowDataEle.firstChildElement("WorkflowState");
        if (!workflowStateEle.isNull()) {
            // 创建临时QDomDocument以调用DAWorkflowState::fromXml
            QDomDocument tempDoc;
            tempDoc.appendChild(tempDoc.importNode(workflowStateEle, true));
            workflowState = DAWorkflowState::fromXml(tempDoc);
        }

        // 设置workflow名称
        if (scene->hasPyWorkflow() && !workflowState.name.isEmpty()) {
            DAPyGILGuard gil;
            try {
                pybind11::object workflowObj = scene->getPyWorkflow();
                if (workflowObj && pybind11::hasattr(workflowObj, "name")) {
                    workflowObj.attr("name") = workflowState.name.toStdString();
                }
            } catch (const pybind11::error_already_set& e) {
                qWarning() << DA_SERIALIZER_TR(
                                  "DAPyWorkFlowSceneSerializer::loadSceneFromXml: Python error setting workflow name: %1"
                )
                                  .arg(e.what());
            }
        }
    }

    // 2. 构建位置映射表（从nodes XML节获取位置，遵循"位置来自node data"原则）
    QMap< QString, QPointF > nodeIdToPosMap;
    QDomElement nodesEle = sceneElement->firstChildElement("nodes");
    QDomElement nodeEle  = nodesEle.firstChildElement("node");
    while (!nodeEle.isNull()) {
        QString nodeId = nodeEle.attribute("node_id");
        double posX = 0.0, posY = 0.0;
        QDomElement xEle = nodeEle.firstChildElement("x");
        QDomElement yEle = nodeEle.firstChildElement("y");
        if (!xEle.isNull()) {
            DA::getStringRealValue(xEle.text(), posX);
        }
        if (!yEle.isNull()) {
            DA::getStringRealValue(yEle.text(), posY);
        }
        if (!nodeId.isEmpty()) {
            nodeIdToPosMap[ nodeId ] = QPointF(posX, posY);
        }
        nodeEle = nodeEle.nextSiblingElement("node");
    }

    // 建立node_id到metaData的映射表（用于连接线通道→端口名转换）
    QMap< QString, DAPyNodeMetaData > nodeIdToMetaDataMap;

    // 3. 创建节点：遍历DAWorkflowState.nodes
    for (const DAWorkflowNodeState& ns : workflowState.nodes) {
        DAPyNodeMetaData metaData = ns.metaData;
        // 如果metaData无效但qualifiedName存在，补全qualifiedName
        if (!metaData.isValid() && !ns.qualifiedName.isEmpty()) {
            metaData.qualifiedName = ns.qualifiedName;
        }

        // 获取位置：优先从nodes XML节获取，其次从state获取
        QPointF pos = ns.position;
        if (nodeIdToPosMap.contains(ns.nodeId)) {
            pos = nodeIdToPosMap[ ns.nodeId ];
        }

        // 创建节点图形项
        DAPyNodeGraphicsItem* nodeItem = scene->createPyNode(metaData, pos);
        if (!nodeItem) {
            d->mLastErrorString = DA_SERIALIZER_TR("创建节点失败: qualified_name=%1").arg(ns.qualifiedName);
            qWarning() << d->mLastErrorString;
            continue;
        }

        // 添加到场景（createPyNode不自动添加到场景）
        scene->addItem_(nodeItem);

        // 建立映射
        nodeIdToItemMap[ ns.nodeId ] = nodeItem;
        nodeIdToMetaDataMap[ ns.nodeId ] = metaData;
    }

    // 4. 恢复节点的pickle数据和可视化属性（从nodes XML节）
    nodeEle = nodesEle.firstChildElement("node");
    while (!nodeEle.isNull()) {
        QString nodeId = nodeEle.attribute("node_id");
        DAPyNodeGraphicsItem* nodeItem = nodeIdToItemMap.value(nodeId, nullptr);
        if (nodeItem) {
            // 加载节点item的可视化属性
            QDomElement itemDataEle = nodeEle.firstChildElement("itemData");
            if (!itemDataEle.isNull()) {
                nodeItem->loadFromXml(&itemDataEle, ver);
            }

            // 恢复pickle数据
            QDomElement pickleEle = nodeEle.firstChildElement("pickleData");
            if (!pickleEle.isNull() && nodeItem->getProxy() && nodeItem->getProxy()->hasPyNodeRef()) {
                DAPyGILGuard gil;
                try {
                    QString base64Str = pickleEle.text();
                    QByteArray qBytes = QByteArray::fromBase64(base64Str.toUtf8());
                    std::string rawBytes(qBytes.constData(), qBytes.size());

                    pybind11::object pickleModule = pybind11::module_::import("pickle");
                    pybind11::bytes pickleBytesObj(rawBytes);
                    pybind11::object unpickledObj = pickleModule.attr("loads")(pickleBytesObj);

                    // 将pickle恢复的数据合并到节点实例
                    pybind11::object pyNodeRef = nodeItem->getProxy()->getPyNodeRef();
                    if (pyNodeRef && pybind11::hasattr(unpickledObj, "_input_data")) {
                        pyNodeRef.attr("_input_data") = unpickledObj.attr("_input_data");
                    }
                } catch (const pybind11::error_already_set& e) {
                    qWarning()
                        << DA_SERIALIZER_TR(
                               "DAPyWorkFlowSceneSerializer::loadSceneFromXml: Python error restoring pickle data: %1")
                               .arg(e.what());
                    // pickle恢复失败不影响整体流程
                }
            }
        }
        nodeEle = nodeEle.nextSiblingElement("node");
    }

    // 5. 创建连接线：遍历DAWorkflowState.connections
    for (const DAWorkflowConnectionState& cs : workflowState.connections) {
        DAPyNodeGraphicsItem* fromItem = nodeIdToItemMap.value(cs.fromNodeId, nullptr);
        DAPyNodeGraphicsItem* toItem   = nodeIdToItemMap.value(cs.toNodeId, nullptr);

        if (!fromItem || !toItem) {
            qWarning() << DA_SERIALIZER_TR(
                "DAPyWorkFlowSceneSerializer::loadSceneFromXml: 连接线引用的节点不存在: "
                "fromNodeId=%1, toNodeId=%2").arg(cs.fromNodeId, cs.toNodeId);
            continue;
        }

        // 将通道编号转换为端口名称
        DAPyNodeMetaData fromMetaData = nodeIdToMetaDataMap.value(cs.fromNodeId);
        DAPyNodeMetaData toMetaData   = nodeIdToMetaDataMap.value(cs.toNodeId);

        QString fromOutput;
        if (cs.fromChannel >= 0 && cs.fromChannel < fromMetaData.outputKeys.size()) {
            fromOutput = fromMetaData.outputKeys[ cs.fromChannel ];
        } else {
            fromOutput = QString::number(cs.fromChannel);
        }

        QString toInput;
        if (cs.toChannel >= 0 && cs.toChannel < toMetaData.inputKeys.size()) {
            toInput = toMetaData.inputKeys[ cs.toChannel ];
        } else {
            toInput = QString::number(cs.toChannel);
        }

        // 创建连接线（addPyNodeLink已自动添加到场景）
        scene->addPyNodeLink(fromItem, fromOutput, toItem, toInput);
    }

    // 6. 恢复连接线可视化属性（从links XML节）
    QDomElement linksEle = sceneElement->firstChildElement("links");
    QDomElement linkEle  = linksEle.firstChildElement("link");
    while (!linkEle.isNull()) {
        QString fromNodeId = linkEle.attribute("fromNodeId");
        QString fromOutput = linkEle.attribute("fromOutput");
        QString toNodeId   = linkEle.attribute("toNodeId");
        QString toInput    = linkEle.attribute("toInput");

        // 查找源节点和目标节点图形项
        DAPyNodeGraphicsItem* fromItem = nodeIdToItemMap.value(fromNodeId, nullptr);
        DAPyNodeGraphicsItem* toItem   = nodeIdToItemMap.value(toNodeId, nullptr);
        if (fromItem && toItem) {
            // 在场景中查找匹配的连接线图形项
            QList< DAPyLinkGraphicsItem* > allLinks = scene->getPyNodeLinkItems();
            for (DAPyLinkGraphicsItem* existingLink : allLinks) {
                if (existingLink->getFromNode() == fromItem
                    && existingLink->getFromOutputName() == fromOutput
                    && existingLink->getToNode() == toItem
                    && existingLink->getToInputName() == toInput) {
                    // 加载连接线item的可视化属性
                    QDomElement linkItemDataEle = linkEle.firstChildElement("itemData");
                    if (!linkItemDataEle.isNull()) {
                        existingLink->loadFromXml(&linkItemDataEle, ver);
                    }
                    break;
                }
            }
        }

        linkEle = linkEle.nextSiblingElement("link");
    }

    // 清空undo栈（加载后的操作不应受之前undo影响）
    scene->undoStack().clear();

    return true;
}

/**
 * @brief 保存场景到XML文件
 *
 * 先调用saveSceneToXml生成XML文档，再写入文件。
 *
 * @param scene 要保存的场景指针
 * @param filePath 文件路径
 * @param ver 版本号
 * @return 保存成功返回true，失败返回false
 */
bool DAPyWorkFlowSceneSerializer::saveSceneToFile(const DAPyWorkFlowScene* scene, const QString& filePath, const QVersionNumber& ver)
{
    DA_D(d);
    QDomDocument doc("DAPyWorkFlowScene");
    if (!saveSceneToXml(scene, &doc, ver)) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        d->mLastErrorString = DA_SERIALIZER_TR("无法打开文件写入: %1").arg(filePath);
        return false;
    }

    QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF-8");
#else
    stream.setEncoding(QStringConverter::Utf8);
#endif
    stream << doc.toString();
    file.close();

    return true;
}

/**
 * @brief 从XML文件加载场景
 *
 * 读取文件并解析XML，再调用loadSceneFromXml恢复场景。
 *
 * @param filePath 文件路径
 * @param scene 目标场景指针
 * @param ver 版本号
 * @return 加载成功返回true，失败返回false
 */
bool DAPyWorkFlowSceneSerializer::loadSceneFromFile(const QString& filePath, DAPyWorkFlowScene* scene, const QVersionNumber& ver)
{
    DA_D(d);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        d->mLastErrorString = DA_SERIALIZER_TR("无法打开文件读取: %1").arg(filePath);
        return false;
    }

    QDomDocument doc("DAPyWorkFlowScene");
    QString errorMsg;
    int errorLine   = 0;
    int errorColumn = 0;
    if (!doc.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
        d->mLastErrorString =
            DA_SERIALIZER_TR("XML解析错误: %1 (行:%2 列:%3)").arg(errorMsg).arg(errorLine).arg(errorColumn);
        file.close();
        return false;
    }
    file.close();

    QDomElement rootEle = doc.documentElement();
    return loadSceneFromXml(&rootEle, scene, ver);
}

/**
 * @brief 获取最后的错误信息
 *
 * @return 最后的错误信息字符串
 */
QString DAPyWorkFlowSceneSerializer::getLastErrorString() const
{
    DA_DC(d);
    return d->mLastErrorString;
}

}  // namespace DA
