#include "DAPyWorkFlowSceneSerializer.h"
#include "DAPyWorkFlowScene.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAPyNode.h"
#include "DAPyWorkFlow.h"
#include "DAPyWorkFlowManager.h"
#include "DAPyGILGuard.h"
#include "DAPybind11QtCaster.hpp"
#include "DAXMLFileInterface.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include <QCoreApplication>

// DAPyWorkFlowSceneSerializer 不是 QObject，直接使用 QCoreApplication::translate 获取翻译
// （context 固定为 "DAPyWorkFlowSceneSerializer"），便于 extract_cn_comments.py 识别 //cn: 注释

namespace DA
{

/**
 * @brief 从节点图形项获取node_id字符串
 * @param item 节点图形项指针
 * @return node_id字符串，若无法获取则返回节点名称
 */
static QString getNodeItemIdFromItem(DAPyNodeGraphicsItem* item)
{
    if (!item) {
        return QString();
    }
    const DAPyNode& proxy = item->getProxy();
    if (!proxy.isNone()) {
        return proxy.getNodeId();
    }
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
 * @brief 保存场景布局到XML文档
 *
 * 仅保存场景级别的布局数据（节点位置、图元可视化属性、连接线布局），
 * 不保存Python工作流逻辑数据（节点拓扑、参数值、连接关系）。
 * Python工作流数据由DAPyWorkFlowSerializer序列化到独立的workflow-data.xml中。
 *
 * 每个节点通过node_id与Python工作流数据关联，保存信息包括：
 * - node_id：与Python数据的关联键
 * - qualified_name：节点类型标识
 * - 位置（x, y）
 * - 图元可视化属性（通过DAPyNodeGraphicsItem::saveToXml）
 *
 * 连接线保存信息包括：
 * - 源/目标节点ID和端口名
 * - 图元可视化属性（通过DAPyLinkGraphicsItem::saveToXml）
 *
 * @param scene 要保存的场景指针
 * @param doc XML文档指针
 * @param ver 版本号
 * @return 保存成功返回true，失败返回false
 */
bool DAPyWorkFlowSceneSerializer::saveSceneToXml(const DAPyWorkFlowScene* scene,
                                                 QDomDocument* doc,
                                                 const QVersionNumber& ver)
{
    DA_D(d);
    d->mLastErrorString.clear();
    if (!scene || !doc) {
        d->mLastErrorString = QCoreApplication::translate("DAPyWorkFlowSceneSerializer", "scene or doc pointer is null");  //cn:scene或doc指针为空
        return false;
    }

    // 创建根元素
    QDomElement rootEle = doc->createElement("DAPyWorkFlowScene");
    rootEle.setAttribute("version", ver.toString());
    doc->appendChild(rootEle);

    // 保存节点布局数据
    QDomElement nodesEle                     = doc->createElement("nodes");
    QList< DAPyNodeGraphicsItem* > nodeItems = scene->getPyNodeItems();
    for (DAPyNodeGraphicsItem* nodeItem : std::as_const(nodeItems)) {
        QDomElement nodeEle = doc->createElement("node");

        // 保存节点关联信息
        const DAPyNode& proxy = nodeItem->getProxy();
        if (!proxy.isNone()) {
            nodeEle.setAttribute("node_id", proxy.getNodeId());
            nodeEle.setAttribute("qualified_name", proxy.getQualifiedName());
        }

        // 保存位置
        QPointF pos = nodeItem->pos();
        DAXMLFileInterface::appendElementWithText(nodeEle, "x", DA::doubleToString(pos.x()), doc);
        DAXMLFileInterface::appendElementWithText(nodeEle, "y", DA::doubleToString(pos.y()), doc);

        // 节点图元自己保存可视化属性
        QDomElement itemEle = doc->createElement("itemData");
        if (!nodeItem->saveToXml(doc, &itemEle, ver)) {
            qWarning() << "Node item saveToXml failed:" << nodeItem->getNodeName();
        }
        nodeEle.appendChild(itemEle);

        nodesEle.appendChild(nodeEle);
    }
    rootEle.appendChild(nodesEle);

    // 保存连接线布局数据
    QDomElement linksEle                     = doc->createElement("links");
    QList< DAPyLinkGraphicsItem* > linkItems = scene->getPyNodeLinkItems();
    for (DAPyLinkGraphicsItem* linkItem : std::as_const(linkItems)) {
        QDomElement linkEle = doc->createElement("link");

        // 保存连接关系
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

        // 连接线图元自己保存可视化属性
        QDomElement itemEle = doc->createElement("itemData");
        if (!linkItem->saveToXml(doc, &itemEle, ver)) {
            qWarning() << "Link item saveToXml failed";
        }
        linkEle.appendChild(itemEle);

        linksEle.appendChild(linkEle);
    }
    rootEle.appendChild(linksEle);

    return true;
}

/**
 * @brief 从XML元素加载场景布局
 *
 * 用于独立场景序列化/反序列化（非项目级加载）。
 * 前提条件：Python workflow数据已通过manager->setWorkflow()注入。
 * 此方法仅处理节点和连线的布局恢复：
 * 1. 清空现有C++图元（保留Python workflow数据）
 * 2. 通过workflow.getNodeById()查找已有Python节点 → wrapPyNode → addItem
 * 3. 通过findNodeItemById查找图形项 → wrapPyNodeLink创建连线
 * 4. rebuildLinkConnectionIdMap确保后续UI删除能同步Python
 *
 * @param sceneElement XML场景元素
 * @param scene 目标场景指针
 * @param ver 版本号
 * @return 加载成功返回true，失败返回false
 */
bool DAPyWorkFlowSceneSerializer::loadSceneFromXml(const QDomElement* sceneElement,
                                                   DAPyWorkFlowScene* scene,
                                                   const QVersionNumber& ver)
{
    DA_D(d);
    d->mLastErrorString.clear();
    if (!sceneElement || !scene) {
        d->mLastErrorString = QCoreApplication::translate("DAPyWorkFlowSceneSerializer", "sceneElement or scene pointer is null");  //cn:sceneElement或scene指针为空
        return false;
    }

    // 清空现有图元（保留Python workflow数据）
    scene->clearSceneItems();

    DAPyGILGuard gil;  // GIL保护：wrapPyNode/wrapPyNodeLink 间接调用 Python

    // 获取manager和workflow
    DAPyWorkFlowManager* mgr = scene->getManager();
    if (!mgr || !mgr->isWorkflowValid()) {
        d->mLastErrorString = QCoreApplication::translate("DAPyWorkFlowSceneSerializer", "Manager or workflow is invalid");  //cn:Manager或workflow无效
        return false;
    }
    DAPyWorkFlow wf = mgr->getWorkflow();

    // 加载节点：getNodeById → wrapPyNode → addItem → loadFromXml
    QDomElement nodesEle = sceneElement->firstChildElement("nodes");
    QDomElement nodeEle  = nodesEle.firstChildElement("node");
    while (!nodeEle.isNull()) {
        QString nodeId = nodeEle.attribute("node_id");
        if (!nodeId.isEmpty()) {
            DAPyNode proxy = wf.getNodeById(nodeId);
            if (proxy.isNone()) {
                qWarning() << "loadSceneFromXml: node_id=" << nodeId << " not found in Python workflow";
            } else {
                double posX = 0.0, posY = 0.0;
                QDomElement xEle = nodeEle.firstChildElement("x");
                QDomElement yEle = nodeEle.firstChildElement("y");
                if (!xEle.isNull()) {
                    DA::getStringRealValue(xEle.text(), posX);
                }
                if (!yEle.isNull()) {
                    DA::getStringRealValue(yEle.text(), posY);
                }

                DAPyNodeGraphicsItem* item = scene->wrapPyNode(proxy, QPointF(posX, posY));
                if (item) {
                    scene->addItem(item);
                    item->updateLinkPoints();
                    QDomElement itemDataEle = nodeEle.firstChildElement("itemData");
                    if (!itemDataEle.isNull()) {
                        item->loadFromXml(&itemDataEle, ver);
                    }
                }
            }
        }
        nodeEle = nodeEle.nextSiblingElement("node");
    }

    // 加载连线：findNodeItemById → wrapPyNodeLink → loadFromXml
    QDomElement linksEle = sceneElement->firstChildElement("links");
    QDomElement linkEle  = linksEle.firstChildElement("link");
    while (!linkEle.isNull()) {
        QString fromNodeId = linkEle.attribute("fromNodeId");
        QString fromOutput = linkEle.attribute("fromOutput");
        QString toNodeId   = linkEle.attribute("toNodeId");
        QString toInput    = linkEle.attribute("toInput");

        DAPyNodeGraphicsItem* fromItem = scene->findNodeItemById(fromNodeId);
        DAPyNodeGraphicsItem* toItem   = scene->findNodeItemById(toNodeId);
        if (fromItem && toItem) {
            DAPyLinkGraphicsItem* link = scene->wrapPyNodeLink(fromItem, fromOutput, toItem, toInput);
            if (link) {
                QDomElement linkItemDataEle = linkEle.firstChildElement("itemData");
                if (!linkItemDataEle.isNull()) {
                    link->loadFromXml(&linkItemDataEle, ver);
                }
            }
        } else {
            qWarning() << "loadSceneFromXml: cannot find nodes for link (from=" << fromNodeId << ", to=" << toNodeId << ")";
        }

        linkEle = linkEle.nextSiblingElement("link");
    }

    // 重建连接ID映射
    scene->rebuildLinkConnectionIdMap();

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
bool DAPyWorkFlowSceneSerializer::saveSceneToFile(const DAPyWorkFlowScene* scene,
                                                  const QString& filePath,
                                                  const QVersionNumber& ver)
{
    DA_D(d);
    QDomDocument doc("DAPyWorkFlowScene");
    if (!saveSceneToXml(scene, &doc, ver)) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        d->mLastErrorString = QCoreApplication::translate("DAPyWorkFlowSceneSerializer", "Cannot open file for writing: %1").arg(filePath);  //cn:无法打开文件写入: %1
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
bool DAPyWorkFlowSceneSerializer::loadSceneFromFile(const QString& filePath,
                                                    DAPyWorkFlowScene* scene,
                                                    const QVersionNumber& ver)
{
    DA_D(d);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        d->mLastErrorString = QCoreApplication::translate("DAPyWorkFlowSceneSerializer", "Cannot open file for reading: %1").arg(filePath);  //cn:无法打开文件读取: %1
        return false;
    }

    QDomDocument doc("DAPyWorkFlowScene");
    QString errorMsg;
    int errorLine   = 0;
    int errorColumn = 0;
    if (!doc.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
        d->mLastErrorString =
            QCoreApplication::translate("DAPyWorkFlowSceneSerializer", "XML parse error: %1 (line:%2 col:%3)").arg(errorMsg).arg(errorLine).arg(errorColumn);  //cn:XML解析错误: %1 (行:%2 列:%3)
        file.close();
        return false;
    }
    file.close();

    QDomElement rootEle = doc.documentElement();
    if (rootEle.isNull()) {
        d->mLastErrorString = QCoreApplication::translate("DAPyWorkFlowSceneSerializer", "XML document has no root element");  //cn:XML文档无根元素
        return false;
    }

    return loadSceneFromXml(&rootEle, scene, ver);
}

/**
 * @brief 获取最后的错误信息
 * @return 最后的错误描述字符串
 */
QString DAPyWorkFlowSceneSerializer::getLastErrorString() const
{
    DA_DC(d);
    return d->mLastErrorString;
}

}  // namespace DA
