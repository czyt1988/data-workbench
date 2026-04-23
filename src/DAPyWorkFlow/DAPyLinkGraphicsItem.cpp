#include "DAPyLinkGraphicsItem.h"
#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include "DAPyNodeGraphicsItem.h"
#include "../DAPyBindQt/DAPythonSignalHandler.h"
#include "../DAWorkFlow/DANodeLinkPoint.h"

namespace DA
{

//===============================================================
// DAPyLinkGraphicsItem::PrivateData
//===============================================================
class DAPyLinkGraphicsItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyLinkGraphicsItem)
public:
    PrivateData(DAPyLinkGraphicsItem* p);

public:
    bool mIsDataFlowing { false };                     ///< 数据流状态标志
    QTimer* mDataFlowTimer { nullptr };                ///< 数据流动画定时器
    DAPythonSignalHandler* mSignalHandler { nullptr }; ///< Python信号处理器
    DAPyNodeGraphicsItem* mFromNode { nullptr };       ///< 源节点
    DAPyNodeGraphicsItem* mToNode { nullptr };         ///< 目标节点
    QString mFromOutputName;                           ///< 源节点输出名称
    QString mToInputName;                              ///< 目标节点输入名称
};

DAPyLinkGraphicsItem::PrivateData::PrivateData(DAPyLinkGraphicsItem* p) : q_ptr(p)
{
    mDataFlowTimer = new QTimer(p);
    mDataFlowTimer->setSingleShot(true);
    mDataFlowTimer->setInterval(300);  // 300ms动画持续时间
    QObject::connect(mDataFlowTimer, &QTimer::timeout, p, &DAPyLinkGraphicsItem::onDataFlowTimer);
}

//===============================================================
// DAPyLinkGraphicsItem
//===============================================================
DAPyLinkGraphicsItem::DAPyLinkGraphicsItem(QGraphicsItem* parent) : DAGraphicsLinkItem(parent), DA_PIMPL_CONSTRUCT
{
    setFlags(flags() | ItemIsSelectable);
    setEndPointType(OrientationStart, EndPointNone);
    setEndPointType(OrientationEnd, EndPointTriangType);
    setLinkLineStyle(LinkLineBezier);
    setZValue(-1);  // 连接线在-1层，避免在节点上面
}

DAPyLinkGraphicsItem::~DAPyLinkGraphicsItem()
{
}

/**
 * @brief 设置数据流状态
 * 
 * 当数据在连接线上流动时，调用此函数启动动画效果。
 * 动画持续300ms后自动关闭。
 * 
 * @param flowing true表示数据开始流动，false表示停止流动
 */
void DAPyLinkGraphicsItem::setDataFlowing(bool flowing)
{
    if (d_ptr->mIsDataFlowing == flowing) {
        return;
    }
    d_ptr->mIsDataFlowing = flowing;
    if (flowing) {
        // 启动定时器，300ms后自动关闭动画
        d_ptr->mDataFlowTimer->start();
    }
    update();
}

/**
 * @brief 获取数据流状态
 * @return true表示数据正在流动，false表示没有数据流动
 */
bool DAPyLinkGraphicsItem::isDataFlowing() const
{
    return d_ptr->mIsDataFlowing;
}

/**
 * @brief 设置Python信号处理器
 * 
 * 信号处理器用于接收Python端的通知，当数据在节点间传递时触发动画效果。
 * 
 * @param handler Python信号处理器指针
 */
void DAPyLinkGraphicsItem::setSignalHandler(DAPythonSignalHandler* handler)
{
    d_ptr->mSignalHandler = handler;
}

/**
 * @brief 获取Python信号处理器
 * @return Python信号处理器指针，可能为nullptr
 */
DAPythonSignalHandler* DAPyLinkGraphicsItem::getSignalHandler() const
{
    return d_ptr->mSignalHandler;
}

/**
 * @brief 设置连接的源节点
 * 
 * 设置连接线的源节点和输出端口名称。
 * 
 * @param node 源节点图形项
 * @param outputName 输出端口名称
 */
void DAPyLinkGraphicsItem::setFromNode(DAPyNodeGraphicsItem* node, const QString& outputName)
{
    d_ptr->mFromNode = node;
    d_ptr->mFromOutputName = outputName;
}

/**
 * @brief 设置连接的目标节点
 * 
 * 设置连接线的目标节点和输入端口名称。
 * 
 * @param node 目标节点图形项
 * @param inputName 输入端口名称
 */
void DAPyLinkGraphicsItem::setToNode(DAPyNodeGraphicsItem* node, const QString& inputName)
{
    d_ptr->mToNode = node;
    d_ptr->mToInputName = inputName;
}

/**
 * @brief 获取源节点
 * @return 源节点图形项指针，可能为nullptr
 */
DAPyNodeGraphicsItem* DAPyLinkGraphicsItem::getFromNode() const
{
    return d_ptr->mFromNode;
}

/**
 * @brief 获取目标节点
 * @return 目标节点图形项指针，可能为nullptr
 */
DAPyNodeGraphicsItem* DAPyLinkGraphicsItem::getToNode() const
{
    return d_ptr->mToNode;
}

/**
 * @brief 获取源节点输出名称
 * @return 输出端口名称
 */
QString DAPyLinkGraphicsItem::getFromOutputName() const
{
    return d_ptr->mFromOutputName;
}

/**
 * @brief 获取目标节点输入名称
 * @return 输入端口名称
 */
QString DAPyLinkGraphicsItem::getToInputName() const
{
    return d_ptr->mToInputName;
}

/**
 * @brief 在将要结束链接的回调，验证数据类型的兼容性
 * 
 * 检查源节点输出数据类型和目标节点输入数据类型是否兼容。
 * 如果类型不兼容，返回false阻止连接完成。
 * 
 * @return true表示类型兼容，允许连接；false表示类型不兼容，阻止连接
 */
bool DAPyLinkGraphicsItem::willCompleteLink()
{
    // 如果没有源节点或目标节点，使用基类默认行为
    if (!d_ptr->mFromNode || !d_ptr->mToNode) {
        return DAGraphicsLinkItem::willCompleteLink();
    }

    // 获取节点描述符
    QJsonObject fromDesc = d_ptr->mFromNode->getDescriptor();
    QJsonObject toDesc = d_ptr->mToNode->getDescriptor();

    // 从描述符中获取输出/输入类型信息
    // 这里假设描述符中包含"outputs"和"inputs"数组，每个元素有"name"和"data_type"字段
    QString outputType;
    QString inputType;

    if (fromDesc.contains("outputs") && fromDesc["outputs"].isArray()) {
        QJsonArray outputs = fromDesc["outputs"].toArray();
        for (const QJsonValue& output : outputs) {
            if (output.isObject()) {
                QJsonObject obj = output.toObject();
                if (obj["name"].toString() == d_ptr->mFromOutputName) {
                    outputType = obj["data_type"].toString();
                    break;
                }
            }
        }
    }

    if (toDesc.contains("inputs") && toDesc["inputs"].isArray()) {
        QJsonArray inputs = toDesc["inputs"].toArray();
        for (const QJsonValue& input : inputs) {
            if (input.isObject()) {
                QJsonObject obj = input.toObject();
                if (obj["name"].toString() == d_ptr->mToInputName) {
                    inputType = obj["data_type"].toString();
                    break;
                }
            }
        }
    }

    // 检查类型兼容性
    if (!outputType.isEmpty() && !inputType.isEmpty()) {
        return checkDataTypeCompatibility(outputType, inputType);
    }

    // 如果无法获取类型信息，默认允许连接
    return true;
}

/**
 * @brief 绘图函数，添加数据流动画效果
 * 
 * 重写基类的paint函数，在绘制连接线时添加数据流高亮效果。
 * 
 * @param painter 绘图设备
 * @param option 绘图选项
 * @param widget 绘图部件
 */
void DAPyLinkGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    // 先调用基类绘制连接线
    DAGraphicsLinkItem::paint(painter, option, widget);

    // 如果数据正在流动，添加高亮效果
    if (d_ptr->mIsDataFlowing) {
        QPainterPath linkPath = getLinkLinePainterPath();
        paintDataFlowHighlight(painter, option, linkPath);
    }
}

/**
 * @brief 保存到XML
 * 
 * 保存连接线的配置信息，包括节点连接信息和自定义属性。
 * 注意：节点连接信息保存的是节点名称和端口名称，而不是指针。
 * 实际连接需要在场景恢复时通过名称重新建立。
 * 
 * @param doc XML文档
 * @param parentElement 父元素
 * @param ver 版本号
 * @return true保存成功，false保存失败
 */
bool DAPyLinkGraphicsItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
    if (!DAGraphicsLinkItem::saveToXml(doc, parentElement, ver)) {
        return false;
    }

    // 保存节点连接信息（使用节点名称而不是指针）
    QDomElement nodeInfoEle = doc->createElement("nodeInfo");
    if (d_ptr->mFromNode) {
        nodeInfoEle.setAttribute("fromNodeName", d_ptr->mFromNode->getNodeName());
    }
    if (d_ptr->mToNode) {
        nodeInfoEle.setAttribute("toNodeName", d_ptr->mToNode->getNodeName());
    }
    nodeInfoEle.setAttribute("fromOutput", d_ptr->mFromOutputName);
    nodeInfoEle.setAttribute("toInput", d_ptr->mToInputName);

    parentElement->appendChild(nodeInfoEle);
    return true;
}

/**
 * @brief 从XML加载
 * 
 * 加载连接线的配置信息，包括节点连接信息和自定义属性。
 * 注意：节点连接信息加载的是节点名称和端口名称，而不是指针。
 * 实际连接需要在场景恢复时通过名称重新建立。
 * 
 * @param parentElement 父元素
 * @param ver 版本号
 * @return true加载成功，false加载失败
 */
bool DAPyLinkGraphicsItem::loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver)
{
    if (!DAGraphicsLinkItem::loadFromXml(parentElement, ver)) {
        return false;
    }

    // 加载节点连接信息
    QDomElement nodeInfoEle = parentElement->firstChildElement("nodeInfo");
    if (!nodeInfoEle.isNull()) {
        d_ptr->mFromOutputName = nodeInfoEle.attribute("fromOutput");
        d_ptr->mToInputName = nodeInfoEle.attribute("toInput");
        // 注意：节点指针需要在场景恢复时通过节点名称重新关联
        // 这里只保存名称，实际连接由场景管理器负责恢复
    }

    return true;
}

/**
 * @brief 数据流动画定时器回调
 * 
 * 当数据流动画定时器超时时，关闭数据流状态。
 */
void DAPyLinkGraphicsItem::onDataFlowTimer()
{
    d_ptr->mIsDataFlowing = false;
    update();
}

/**
 * @brief 获取数据流高亮颜色
 * 
 * 根据当前选中状态返回不同的高亮颜色。
 * 
 * @return 高亮颜色
 */
QColor DAPyLinkGraphicsItem::getDataFlowHighlightColor() const
{
    // 如果选中，使用更亮的颜色
    if (isSelected()) {
        return QColor(255, 200, 50, 180);  // 半透明橙色
    } else {
        return QColor(50, 150, 255, 150);  // 半透明蓝色
    }
}

/**
 * @brief 绘制数据流高亮效果
 * 
 * 在连接线上绘制高亮效果，表示数据正在流动。
 * 
 * @param painter 绘图设备
 * @param option 绘图选项
 * @param linkPath 连接线路径
 */
void DAPyLinkGraphicsItem::paintDataFlowHighlight(QPainter* painter, const QStyleOptionGraphicsItem* option, const QPainterPath& linkPath)
{
    painter->save();
    
    // 设置高亮画笔
    QPen highlightPen = getDataFlowHighlightColor();
    highlightPen.setWidth(getLinePen().width() + 4);  // 比原线宽4像素
    highlightPen.setStyle(Qt::SolidLine);
    highlightPen.setCapStyle(Qt::RoundCap);
    highlightPen.setJoinStyle(Qt::RoundJoin);
    
    painter->setPen(highlightPen);
    painter->setBrush(Qt::NoBrush);
    painter->setRenderHint(QPainter::Antialiasing, true);
    
    // 绘制高亮线
    painter->drawPath(linkPath);
    
    painter->restore();
}

/**
 * @brief 检查数据类型兼容性
 * 
 * 检查源节点输出数据类型和目标节点输入数据类型是否兼容。
 * 这是一个简化的实现，实际项目中可能需要更复杂的类型系统。
 * 
 * @param outputType 输出数据类型
 * @param inputType 输入数据类型
 * @return true表示类型兼容，false表示不兼容
 */
bool DAPyLinkGraphicsItem::checkDataTypeCompatibility(const QString& outputType, const QString& inputType) const
{
    // 简化实现：如果类型相同或输入类型为"any"，则兼容
    if (outputType == inputType || inputType == "any" || inputType.isEmpty()) {
        return true;
    }
    
    // 检查数字类型兼容性
    static const QStringList numericTypes = { "int", "float", "double", "number" };
    if (numericTypes.contains(outputType) && numericTypes.contains(inputType)) {
        return true;
    }
    
    // 检查字符串类型兼容性
    if ((outputType == "str" || outputType == "string") && 
        (inputType == "str" || inputType == "string" || inputType == "text")) {
        return true;
    }
    
    // 检查布尔类型兼容性
    if ((outputType == "bool" || outputType == "boolean") && 
        (inputType == "bool" || inputType == "boolean")) {
        return true;
    }
    
    // 检查列表/数组类型兼容性
    if ((outputType.startsWith("list[") || outputType.startsWith("array[")) &&
        (inputType.startsWith("list[") || inputType.startsWith("array["))) {
        // 简化：只要都是列表/数组类型就认为兼容
        return true;
    }
    
    // 检查DataFrame类型兼容性
    if ((outputType == "DataFrame" || outputType == "dataframe") &&
        (inputType == "DataFrame" || inputType == "dataframe")) {
        return true;
    }
    
    // 默认不兼容
    qDebug() << "Data type incompatibility: outputType=" << outputType << ", inputType=" << inputType;
    return false;
}

}  // end DA