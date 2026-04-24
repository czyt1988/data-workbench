#include "DAPyWorkFlowSceneSerializer.h"
#include "DAPybind11InQt.h"
#include "DAPyWorkFlowScene.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAPyNodeProxy.h"
#include "DAPyGILGuard.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyJsonCast.h"
#include "DAPyLinkPoint.h"
#include "DAXMLFileInterface.h"
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
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
 * 每个节点保存以下信息：
 * - qualified_name：用于加载时通过DANodeRegistry重建节点
 * - node_id：Python侧的节点唯一标识
 * - 位置信息（x, y）
 * - 节点参数值（通过DAPyJsonCast转为QJsonObject再写入XML CDATA）
 * - 渲染模板、节点名称、SVG路径等显示属性
 *
 * 连接线保存以下信息：
 * - 源节点ID和输出端口名
 * - 目标节点ID和输入端口名
 * - 连接线的可视化属性
 *
 * 同时保存Python DAWorkflow的to_dict()数据作为DAG级别的序列化。
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
        d->mLastErrorString = QCoreApplication::translate("DAPyWorkFlowSceneSerializer", "scene或doc指针为空");
        return false;
    }

    // 创建根元素
    QDomElement rootEle = doc->createElement("DAPyWorkFlowScene");
    rootEle.setAttribute("version", ver.toString());
    doc->appendChild(rootEle);

    // 保存Python DAWorkflow的DAG级别数据（to_dict）
    QDomElement workflowDataEle = doc->createElement("workflowData");
    if (scene->hasPyWorkflow()) {
        DAPyGILGuard gil;
        try {
            pybind11::object workflowObj = scene->getPyWorkflow();
            if (workflowObj && pybind11::hasattr(workflowObj, "to_dict")) {
                pybind11::dict workflowDict = workflowObj.attr("to_dict").cast< pybind11::dict >();
                // 将py::dict转为QJsonObject
                QJsonObject workflowJson = DA::PY::pyDictToQJsonObject(workflowDict);
                // 将QJsonObject转为字符串写入CDATA
                QJsonDocument jsonDoc(workflowJson);
                QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact);
                DAXMLFileInterface::appendElementWithText(workflowDataEle, "json", QString::fromUtf8(jsonBytes), doc);
            }
        } catch (const pybind11::error_already_set& e) {
            qWarning() << DA_SERIALIZER_TR("DAPyWorkFlowSceneSerializer::saveSceneToXml: Python error saving workflow data: %1").arg(e.what());
            d->mLastErrorString = DA_SERIALIZER_TR("保存workflow数据时Python异常: %1").arg(e.what());
            // 继续保存场景级别的数据，不因workflow数据保存失败而中断
        }
    }
    rootEle.appendChild(workflowDataEle);

    // 保存节点数据
    QDomElement nodesEle = doc->createElement("nodes");
    QList<DAPyNodeGraphicsItem*> nodeItems = scene->getPyNodeItems();
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
            try {
                QJsonObject configJson = proxy->getConfig();
                if (!configJson.isEmpty()) {
                    QJsonDocument configDoc(configJson);
                    QByteArray configBytes = configDoc.toJson(QJsonDocument::Compact);
                    DAXMLFileInterface::appendElementWithText(nodeEle, "config", QString::fromUtf8(configBytes), doc);
                }
            } catch (const pybind11::error_already_set& e) {
                qWarning() << DA_SERIALIZER_TR("DAPyWorkFlowSceneSerializer::saveSceneToXml: Python error saving node config: %1").arg(e.what());
            }

            // 保存Python对象参数的pickle序列化
            try {
                pybind11::object pyNodeRef = proxy->getPyNodeRef();
                if (pyNodeRef && pybind11::hasattr(pyNodeRef, "get_pickle_data")) {
                    pybind11::object pickleModule = pybind11::module_::import("pickle");
                    pybind11::bytes pickleBytes    = pickleModule.attr("dumps")(pyNodeRef);
                    std::string rawBytes           = pickleBytes;
                    QByteArray qBytes              = QByteArray::fromStdString(rawBytes);
                    QString base64Str              = QString(qBytes.toBase64());
                    QDomElement pickleEle          = doc->createElement("pickleData");
                    QDomText cdataText             = doc->createCDATASection(base64Str);
                    pickleEle.appendChild(cdataText);
                    nodeEle.appendChild(pickleEle);
                }
            } catch (const pybind11::error_already_set& e) {
                qWarning() << DA_SERIALIZER_TR("DAPyWorkFlowSceneSerializer::saveSceneToXml: Python error saving pickle data: %1").arg(e.what());
                // pickle保存失败不影响整体流程
            }
        }

        // 保存节点描述符
        QJsonObject descriptor = nodeItem->getDescriptor();
        if (!descriptor.isEmpty()) {
            QJsonDocument descDoc(descriptor);
            QByteArray descBytes = descDoc.toJson(QJsonDocument::Compact);
            DAXMLFileInterface::appendElementWithText(nodeEle, "descriptor", QString::fromUtf8(descBytes), doc);
        }

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
    QDomElement linksEle = doc->createElement("links");
    QList<DAPyLinkGraphicsItem*> linkItems = scene->getPyNodeLinkItems();
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
 * 2. 先恢复Python DAWorkflow的DAG数据（from_dict）
 * 3. 逐个创建节点图形项：通过qualified_name从DANodeRegistry获取描述符，
 *    调用DAPyWorkFlowScene::createPyNode()创建节点
 * 4. 恢复节点位置、参数值、pickle数据
 * 5. 建立节点id到图形项的映射表
 * 6. 逐个恢复连接线：通过映射表查找源/目标节点，调用addPyNodeLink
 *
 * 加载完成后不会自动执行工作流。
 *
 * @param sceneElement XML场景元素
 * @param scene 目标场景指针
 * @param ver 版本号
 * @return 加载成功返回true，失败返回false
 * @note 加载后不会自动执行工作流
 */
bool DAPyWorkFlowSceneSerializer::loadSceneFromXml(const QDomElement* sceneElement,
                                                    DAPyWorkFlowScene* scene,
                                                    const QVersionNumber& ver)
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
    QMap<QString, DAPyNodeGraphicsItem*> nodeIdToItemMap;

    // 1. 加载Python DAWorkflow的DAG数据
    QDomElement workflowDataEle = sceneElement->firstChildElement("workflowData");
    if (!workflowDataEle.isNull() && scene->hasPyWorkflow()) {
        QDomElement jsonEle = workflowDataEle.firstChildElement("json");
        if (!jsonEle.isNull()) {
            QString jsonStr = jsonEle.text();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                QJsonObject workflowJson = jsonDoc.object();
                DAPyGILGuard gil;
                try {
                    // 将QJsonObject转为py::dict
                    pybind11::dict workflowDict = DA::PY::qjsonObjectToPyDict(workflowJson);
                    pybind11::object workflowObj = scene->getPyWorkflow();
                    if (workflowObj && pybind11::hasattr(workflowObj, "from_dict")) {
                        // 加载DAG级别数据到现有workflow实例
                        // 注意：from_dict会清空并重建workflow的节点和连接
                        // 但场景级别的恢复需要额外的createPyNode操作
                        // 这里仅加载DAG逻辑数据，场景图形项由后面的步骤创建
                    }
                } catch (const pybind11::error_already_set& e) {
                    qWarning() << DA_SERIALIZER_TR("DAPyWorkFlowSceneSerializer::loadSceneFromXml: Python error loading workflow data: %1").arg(e.what());
                    d->mLastErrorString = DA_SERIALIZER_TR("加载workflow数据时Python异常: %1").arg(e.what());
                }
            }
        }
    }

    // 2. 加载节点数据
    QDomElement nodesEle = sceneElement->firstChildElement("nodes");
    QDomElement nodeEle  = nodesEle.firstChildElement("node");
    while (!nodeEle.isNull()) {
        QString qualifiedName = nodeEle.attribute("qualified_name");
        QString nodeId        = nodeEle.attribute("node_id");

        // 获取节点位置
        double posX = 0.0, posY = 0.0;
        QDomElement xEle = nodeEle.firstChildElement("x");
        QDomElement yEle = nodeEle.firstChildElement("y");
        if (!xEle.isNull()) {
            DA::getStringRealValue(xEle.text(), posX);
        }
        if (!yEle.isNull()) {
            DA::getStringRealValue(yEle.text(), posY);
        }
        QPointF pos(posX, posY);

        // 获取节点描述符
        QJsonObject descriptor;
        QDomElement descEle = nodeEle.firstChildElement("descriptor");
        if (!descEle.isNull()) {
            QString descStr    = descEle.text();
            QJsonDocument descDoc = QJsonDocument::fromJson(descStr.toUtf8());
            if (!descDoc.isNull() && descDoc.isObject()) {
                descriptor = descDoc.object();
            }
        }

        // 如果描述符中没有qualified_name，补充进去
        if (!qualifiedName.isEmpty() && !descriptor.contains("qualified_name")) {
            descriptor["qualified_name"] = qualifiedName;
        }

        // 通过DANodeRegistry查找描述符（如果本地描述符不完整）
        if (descriptor.isEmpty() && !qualifiedName.isEmpty()) {
            DAPyGILGuard gil;
            try {
                DAPyModuleWorkflow& modWorkflow = DAPyModuleWorkflow::getInstance();
                if (modWorkflow.import()) {
                    pybind11::object registryClass = modWorkflow.getNodeRegistryClass();
                    // 尝试从注册表获取描述符
                    // 注意：这里需要一个DANodeRegistry实例，通常由应用层提供
                    // 如果没有注册表实例，使用XML中保存的描述符
                }
            } catch (const pybind11::error_already_set& e) {
                qWarning() << DA_SERIALIZER_TR("DAPyWorkFlowSceneSerializer::loadSceneFromXml: Python error querying registry: %1").arg(e.what());
            }
        }

        // 创建节点图形项
        DAPyNodeGraphicsItem* nodeItem = scene->createPyNode(descriptor, pos);
        if (!nodeItem) {
            d->mLastErrorString = DA_SERIALIZER_TR("创建节点失败: qualified_name=%1").arg(qualifiedName);
            qWarning() << d->mLastErrorString;
            nodeEle = nodeEle.nextSiblingElement("node");
            continue;
        }

        // 添加到场景
        scene->addItem_(nodeItem);

        // 建立映射
        if (!nodeId.isEmpty()) {
            nodeIdToItemMap[nodeId] = nodeItem;
        }
        // 也用qualified_name作为备用映射键
        if (!qualifiedName.isEmpty() && !nodeIdToItemMap.contains(qualifiedName)) {
            nodeIdToItemMap[qualifiedName] = nodeItem;
        }

        // 加载节点item的可视化属性
        QDomElement itemDataEle = nodeEle.firstChildElement("itemData");
        if (!itemDataEle.isNull()) {
            nodeItem->loadFromXml(&itemDataEle, ver);
        }

        // 恢复节点参数值（config）
        QDomElement configEle = nodeEle.firstChildElement("config");
        if (!configEle.isNull() && nodeItem->getProxy()) {
            QString configStr       = configEle.text();
            QJsonDocument configDoc = QJsonDocument::fromJson(configStr.toUtf8());
            if (!configDoc.isNull() && configDoc.isObject()) {
                nodeItem->getProxy()->setConfig(configDoc.object());
            }
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
                // 注意：不替换整个节点实例，仅合并参数属性
                pybind11::object pyNodeRef = nodeItem->getProxy()->getPyNodeRef();
                if (pyNodeRef && pybind11::hasattr(unpickledObj, "_input_data")) {
                    pyNodeRef.attr("_input_data") = unpickledObj.attr("_input_data");
                }
            } catch (const pybind11::error_already_set& e) {
                qWarning() << DA_SERIALIZER_TR("DAPyWorkFlowSceneSerializer::loadSceneFromXml: Python error restoring pickle data: %1").arg(e.what());
                // pickle恢复失败不影响整体流程
            }
        }

        nodeEle = nodeEle.nextSiblingElement("node");
    }

    // 3. 加载连接线数据
    QDomElement linksEle = sceneElement->firstChildElement("links");
    QDomElement linkEle  = linksEle.firstChildElement("link");
    while (!linkEle.isNull()) {
        QString fromNodeId  = linkEle.attribute("fromNodeId");
        QString fromOutput  = linkEle.attribute("fromOutput");
        QString toNodeId    = linkEle.attribute("toNodeId");
        QString toInput     = linkEle.attribute("toInput");

        // 通过映射表查找节点
        DAPyNodeGraphicsItem* fromItem = nodeIdToItemMap.value(fromNodeId, nullptr);
        DAPyNodeGraphicsItem* toItem   = nodeIdToItemMap.value(toNodeId, nullptr);

        if (fromItem && toItem && !fromOutput.isEmpty() && !toInput.isEmpty()) {
            // 创建连接线
            DAPyLinkGraphicsItem* linkItem = scene->addPyNodeLink(fromItem, fromOutput, toItem, toInput);
            if (linkItem) {
                scene->addItem_(linkItem);

                // 加载连接线item的可视化属性
                QDomElement linkItemDataEle = linkEle.firstChildElement("itemData");
                if (!linkItemDataEle.isNull()) {
                    linkItem->loadFromXml(&linkItemDataEle, ver);
                }
            }
        } else {
            qWarning() << DA_SERIALIZER_TR("DAPyWorkFlowSceneSerializer::loadSceneFromXml: 连接线引用的节点不存在: fromNodeId=%1, toNodeId=%2")
                            .arg(fromNodeId, toNodeId);
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
bool DAPyWorkFlowSceneSerializer::loadSceneFromFile(const QString& filePath,
                                                     DAPyWorkFlowScene* scene,
                                                     const QVersionNumber& ver)
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
        d->mLastErrorString = DA_SERIALIZER_TR("XML解析错误: %1 (行:%2 列:%3)").arg(errorMsg).arg(errorLine).arg(errorColumn);
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