#include "DAPyNodeGraphicsItem.h"
#include "DAPyPainterProxy.h"
#include "DAPyNodeProxy.h"
#include "DAPyNodePalette.h"
#include "DAPyLinkPoint.h"
#include "DAPyWorkFlowScene.h"
#include <memory>
#include <QPainter>
#include <QSvgRenderer>
#include <QGraphicsProxyWidget>
#include <QWidget>
#include <QDomDocument>
#include <QDomElement>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QFontMetrics>

namespace DA
{

/**
 * @def 调试打印开关
 */
#define DA_DAPYNODEGRAPHICSITEM_DEBUG_PRINT 0

//===================================================
// DAPyNodeGraphicsItem::PrivateData
//===================================================

class DAPyNodeGraphicsItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyNodeGraphicsItem)

public:
    PrivateData(DAPyNodeGraphicsItem* p);
    ~PrivateData();

    // 根据描述符生成连接点
    QList< DAPyLinkPoint > generateLinkPointsFromDescriptor() const;
    // 更新连接点位置
    void updateLinkPointPositions(const QRectF& bodyRect);
    // 清理widget
    void cleanupWidget();
    // 清理SVG
    void cleanupSvg();

public:
    std::unique_ptr< DAPyNodeProxy > mProxy;           ///< Python节点代理（独占所有权）
    RenderTemplate mRenderTemplate { RectTemplate };  ///< 当前渲染模板
    QString mNodeName;                                ///< 节点名称
    QIcon mIcon;                                      ///< 节点图标
    QString mSvgPath;                                 ///< SVG文件路径
    QSvgRenderer* mSvgRenderer { nullptr };           ///< SVG渲染器
    QGraphicsProxyWidget* mProxyWidget { nullptr };   ///< Widget代理
    QWidget* mWidget { nullptr };                     ///< 嵌入的widget
    DAPyNodeState mNodeState { Idle };                ///< 节点状态
    QJsonObject mDescriptor;                          ///< 节点描述符
    QList< DAPyLinkPoint > mInputLinkPoints;          ///< 输入连接点
    QList< DAPyLinkPoint > mOutputLinkPoints;         ///< 输出连接点
    DAPySafePyObjectHolder mPaintCallback;            ///< 自定义绘制回调（Python函数对象）
    bool mPaintCallbackError { false };               ///< 绘制回调是否发生过异常
    DANodeStyle mStyle;                               ///< 节点样式配置
};

/**
 * @brief 构造函数
 * @param[in] p 父对象指针
 */
DAPyNodeGraphicsItem::PrivateData::PrivateData(DAPyNodeGraphicsItem* p) : q_ptr(p)
{
}

/**
 * @brief 析构函数
 */
DAPyNodeGraphicsItem::PrivateData::~PrivateData()
{
    cleanupSvg();
    cleanupWidget();
}

/**
 * @brief 从描述符生成连接点
 * @return 连接点列表
 */
QList< DAPyLinkPoint > DAPyNodeGraphicsItem::PrivateData::generateLinkPointsFromDescriptor() const
{
    QList< DAPyLinkPoint > result;

    if (mDescriptor.isEmpty()) {
        return result;
    }

    // 从描述符解析输入连接点
    QJsonArray inputs = mDescriptor.value("inputs").toArray();
    int inputCount    = inputs.size();
    for (int i = 0; i < inputCount; ++i) {
        DAPyLinkPoint lp;
        lp.way               = DAPyLinkPoint::Input;
        lp.direction         = AspectDirection::West;
        QJsonObject inputObj = inputs.at(i).toObject();
        lp.name              = inputObj.value("name").toString(QString("input_%1").arg(i));
        result.append(lp);
    }

    // 从描述符解析输出连接点
    QJsonArray outputs = mDescriptor.value("outputs").toArray();
    int outputCount    = outputs.size();
    for (int i = 0; i < outputCount; ++i) {
        DAPyLinkPoint lp;
        lp.way                = DAPyLinkPoint::Output;
        lp.direction          = AspectDirection::East;
        QJsonObject outputObj = outputs.at(i).toObject();
        lp.name               = outputObj.value("name").toString(QString("output_%1").arg(i));
        result.append(lp);
    }

    return result;
}

/**
 * @brief 更新连接点位置
 * @param[in] bodyRect 节点主体矩形区域
 */
void DAPyNodeGraphicsItem::PrivateData::updateLinkPointPositions(const QRectF& bodyRect)
{
    // 计算输入连接点位置（左侧）
    int inputCount = mInputLinkPoints.size();
    if (inputCount > 0) {
        qreal spacing = bodyRect.height() / (inputCount + 1);
        for (int i = 0; i < inputCount; ++i) {
            mInputLinkPoints[ i ].position = QPointF(bodyRect.left(), bodyRect.top() + spacing * (i + 1));
        }
    }

    // 计算输出连接点位置（右侧）
    int outputCount = mOutputLinkPoints.size();
    if (outputCount > 0) {
        qreal spacing = bodyRect.height() / (outputCount + 1);
        for (int i = 0; i < outputCount; ++i) {
            mOutputLinkPoints[ i ].position = QPointF(bodyRect.right(), bodyRect.top() + spacing * (i + 1));
        }
    }
}

/**
 * @brief 清理widget资源
 */
void DAPyNodeGraphicsItem::PrivateData::cleanupWidget()
{
    if (mProxyWidget) {
        mProxyWidget->setWidget(nullptr);
        delete mProxyWidget;
        mProxyWidget = nullptr;
    }
    mWidget = nullptr;
}

/**
 * @brief 清理SVG资源
 */
void DAPyNodeGraphicsItem::PrivateData::cleanupSvg()
{
    if (mSvgRenderer) {
        delete mSvgRenderer;
        mSvgRenderer = nullptr;
    }
}

//===================================================
// DAPyNodeGraphicsItem
//===================================================

/**
 * @brief 构造函数
 * @param[in] proxy Python节点代理
 * @param[in] parent 父图形项
 */
DAPyNodeGraphicsItem::DAPyNodeGraphicsItem(DAPyNodeProxy* proxy, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->mProxy.reset(proxy);

    // 设置默认尺寸
    setBodySize(QSizeF(120, 60));

    // 设置可选中和可移动
    setSelectable(true);
    setMovable(true);

    // 如果代理有效，同步信息
    if (proxy) {
        d_ptr->mNodeName   = proxy->getNodeName();
        d_ptr->mDescriptor = proxy->getDescriptor();
        d_ptr->mNodeState  = proxy->getNodeState();
        updateLinkPoints();
    }
}

/**
 * @brief 析构函数
 */
DAPyNodeGraphicsItem::~DAPyNodeGraphicsItem()
{
}

/**
 * @brief 设置渲染模板
 * @param[in] tmpl 渲染模板类型
 */
void DAPyNodeGraphicsItem::setRenderTemplate(RenderTemplate tmpl)
{
    if (d_ptr->mRenderTemplate == tmpl) {
        return;
    }

    // 清理之前的资源
    if (d_ptr->mRenderTemplate == SvgTemplate) {
        d_ptr->cleanupSvg();
    } else if (d_ptr->mRenderTemplate == WidgetTemplate) {
        d_ptr->cleanupWidget();
    }

    d_ptr->mRenderTemplate = tmpl;

    // 初始化新的资源
    if (tmpl == WidgetTemplate && !d_ptr->mProxyWidget) {
        d_ptr->mProxyWidget = new QGraphicsProxyWidget(this);
    }

    update();
}

/**
 * @brief 通过名称设置渲染模板
 * @param[in] tmplName 模板名称（"rect"/"svg"/"widget"）
 */
void DAPyNodeGraphicsItem::setRenderTemplate(const QString& tmplName)
{
    QString lowerName = tmplName.toLower();
    if (lowerName == "svg" || lowerName == "icon") {
        setRenderTemplate(SvgTemplate);
    } else if (lowerName == "widget") {
        setRenderTemplate(WidgetTemplate);
    } else {
        setRenderTemplate(RectTemplate);
    }
}

/**
 * @brief 获取当前渲染模板
 * @return 渲染模板类型
 */
DAPyNodeGraphicsItem::RenderTemplate DAPyNodeGraphicsItem::getRenderTemplate() const
{
    return d_ptr->mRenderTemplate;
}

/**
 * @brief 获取渲染模板名称
 * @return 模板名称字符串
 */
QString DAPyNodeGraphicsItem::getRenderTemplateName() const
{
    switch (d_ptr->mRenderTemplate) {
    case SvgTemplate:
        return QString("svg");
    case WidgetTemplate:
        return QString("widget");
    case RectTemplate:
    default:
        return QString("rect");
    }
}

/**
 * @brief 获取Python节点代理
 * @return 代理指针
 */
DAPyNodeProxy* DAPyNodeGraphicsItem::getProxy() const
{
    return d_ptr->mProxy.get();
}

/**
 * @brief 设置Python节点代理
 * @param[in] proxy 代理指针
 */
void DAPyNodeGraphicsItem::setProxy(DAPyNodeProxy* proxy)
{
    d_ptr->mProxy.reset(proxy);
    if (proxy) {
        d_ptr->mNodeName   = proxy->getNodeName();
        d_ptr->mDescriptor = proxy->getDescriptor();
        d_ptr->mNodeState  = proxy->getNodeState();
        updateLinkPoints();
        update();
    }
}

/**
 * @brief 设置节点名称
 * @param[in] name 节点名称
 */
void DAPyNodeGraphicsItem::setNodeName(const QString& name)
{
    if (d_ptr->mNodeName != name) {
        d_ptr->mNodeName = name;

        // 仅 RectTemplate 模式下自适应尺寸
        if (d_ptr->mRenderTemplate == RectTemplate && !name.isEmpty()) {
            QFont font;
            font.setPointSize(9);
            QFontMetrics fm(font);

            constexpr qreal kIconSpace      = 36;   // 图标占用的左侧空间
            constexpr qreal kRightMargin    = 8;    // 右侧边距
            constexpr qreal kLinkPointSpace = 12;   // 连接点额外空间
            constexpr qreal kMinWidth       = 120;  // 最小宽度
            constexpr qreal kMaxWidth       = 280;  // 最大宽度
            constexpr qreal kHeightPadding  = 12;   // 上下间距
            constexpr qreal kMinHeight      = 60;   // 最小高度

            // 文本宽度测量（单行）
            qreal textWidth    = fm.horizontalAdvance(name);
            qreal desiredWidth = kIconSpace + textWidth + kRightMargin + kLinkPointSpace;

            // 判断是否需要换行
            qreal availableTextWidth = kMaxWidth - kIconSpace - kRightMargin - kLinkPointSpace;
            bool needsWrap           = textWidth > availableTextWidth;
            qreal finalWidth         = qBound(kMinWidth, desiredWidth, kMaxWidth);
            qreal desiredHeight      = kMinHeight;

            if (needsWrap) {
                // 使用 boundingRect 计算多行文本高度
                QRect textBounds(0, 0, qRound(availableTextWidth), 0);
                textBounds    = fm.boundingRect(textBounds, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, name);
                desiredHeight = qMax(kMinHeight, textBounds.height() + kHeightPadding);
            }

            setBodySize(QSizeF(finalWidth, desiredHeight));
            d_ptr->updateLinkPointPositions(getBodyRect());
        }

        update();
    }
}

/**
 * @brief 获取节点名称
 * @return 节点名称
 */
QString DAPyNodeGraphicsItem::getNodeName() const
{
    return d_ptr->mNodeName;
}

/**
 * @brief 设置节点样式
 * @param[in] style 节点样式配置
 */
void DAPyNodeGraphicsItem::setStyle(const DANodeStyle& style)
{
    d_ptr->mStyle = style;
    update();
}

/**
 * @brief 获取节点样式（非常量引用，允许修改）
 * @return 节点样式引用
 */
DANodeStyle& DAPyNodeGraphicsItem::getStyle()
{
    return d_ptr->mStyle;
}

/**
 * @brief 获取节点样式（常量引用）
 * @return 节点样式常量引用
 */
const DANodeStyle& DAPyNodeGraphicsItem::getStyle() const
{
    return d_ptr->mStyle;
}

/**
 * @brief 设置图标
 * @param[in] icon 图标
 */
void DAPyNodeGraphicsItem::setIcon(const QIcon& icon)
{
    d_ptr->mIcon = icon;
    update();
}

/**
 * @brief 获取图标
 * @return 图标
 */
QIcon DAPyNodeGraphicsItem::getIcon() const
{
    return d_ptr->mIcon;
}

/**
 * @brief 设置SVG文件路径
 * @param[in] path SVG文件路径
 */
void DAPyNodeGraphicsItem::setSvgPath(const QString& path)
{
    d_ptr->mSvgPath = path;
    if (!path.isEmpty()) {
        loadSvg(path);
    }
}

/**
 * @brief 获取SVG文件路径
 * @return SVG文件路径
 */
QString DAPyNodeGraphicsItem::getSvgPath() const
{
    return d_ptr->mSvgPath;
}

/**
 * @brief 加载SVG文件
 * @param[in] path SVG文件路径
 * @return 加载成功返回true
 */
bool DAPyNodeGraphicsItem::loadSvg(const QString& path)
{
    // 清理旧的渲染器
    d_ptr->cleanupSvg();

    // 创建新的渲染器
    d_ptr->mSvgRenderer = new QSvgRenderer(this);
    if (!d_ptr->mSvgRenderer->load(path)) {
        d_ptr->cleanupSvg();
        return false;
    }

    d_ptr->mSvgPath = path;

    // 如果当前不是SVG模式，切换到SVG模式
    if (d_ptr->mRenderTemplate != SvgTemplate) {
        d_ptr->mRenderTemplate = SvgTemplate;
    }

    update();
    return true;
}

/**
 * @brief 设置嵌入的Widget
 * @param[in] widget 要嵌入的widget
 */
void DAPyNodeGraphicsItem::setWidget(QWidget* widget)
{
    if (d_ptr->mWidget == widget) {
        return;
    }

    // 确保proxy widget已创建
    if (!d_ptr->mProxyWidget) {
        d_ptr->mProxyWidget = new QGraphicsProxyWidget(this);
    }

    // 设置widget
    d_ptr->mProxyWidget->setWidget(widget);
    d_ptr->mWidget = widget;

    // 如果当前不是widget模式，切换到widget模式
    if (d_ptr->mRenderTemplate != WidgetTemplate) {
        d_ptr->mRenderTemplate = WidgetTemplate;
    }

    // 更新widget几何位置
    updateWidgetGeometry();
}

/**
 * @brief 获取嵌入的Widget
 * @return widget指针
 */
QWidget* DAPyNodeGraphicsItem::getWidget() const
{
    return d_ptr->mWidget;
}

/**
 * @brief 获取节点状态
 * @return 节点状态
 */
DAPyNodeState DAPyNodeGraphicsItem::getNodeState() const
{
    return d_ptr->mNodeState;
}

/**
 * @brief 设置节点状态
 * @param[in] state 节点状态
 */
void DAPyNodeGraphicsItem::setNodeState(DAPyNodeState state)
{
    if (d_ptr->mNodeState != state) {
        d_ptr->mNodeState = state;
        update();  // 状态变化时重绘
    }
}

/**
 * @brief 设置节点描述符
 * @param[in] desc 描述符JSON对象
 */
void DAPyNodeGraphicsItem::setDescriptor(const QJsonObject& desc)
{
    d_ptr->mDescriptor = desc;
    updateLinkPoints();
    update();
}

/**
 * @brief 获取节点描述符
 * @return 描述符JSON对象
 */
QJsonObject DAPyNodeGraphicsItem::getDescriptor() const
{
    return d_ptr->mDescriptor;
}

/**
 * @brief 获取输入连接点
 * @return 输入连接点列表
 */
QList< DAPyLinkPoint > DAPyNodeGraphicsItem::getInputLinkPoints() const
{
    return d_ptr->mInputLinkPoints;
}

/**
 * @brief 获取输出连接点
 * @return 输出连接点列表
 */
QList< DAPyLinkPoint > DAPyNodeGraphicsItem::getOutputLinkPoints() const
{
    return d_ptr->mOutputLinkPoints;
}

/**
 * @brief 更新连接点
 */
void DAPyNodeGraphicsItem::updateLinkPoints()
{
    QList< DAPyLinkPoint > allPoints = generateLinkPoints();

    d_ptr->mInputLinkPoints.clear();
    d_ptr->mOutputLinkPoints.clear();

    for (const auto& lp : std::as_const(allPoints)) {
        if (lp.isInput()) {
            d_ptr->mInputLinkPoints.append(lp);
        } else {
            d_ptr->mOutputLinkPoints.append(lp);
        }
    }

    d_ptr->updateLinkPointPositions(getBodyRect());
}

/**
 * @brief 生成连接点
 * @return 连接点列表
 */
QList< DAPyLinkPoint > DAPyNodeGraphicsItem::generateLinkPoints() const
{
    // 优先从描述符生成
    if (!d_ptr->mDescriptor.isEmpty()) {
        QList<DAPyLinkPoint> descriptorPoints = d_ptr->generateLinkPointsFromDescriptor();
        if (!descriptorPoints.isEmpty()) {
            return descriptorPoints;  // 描述符有完整的I/O信息，直接使用
        }
        // 描述符为薄描述符（没有inputs/outputs数组），回退到代理路径
    }

    // 从代理节点生成
    QList< DAPyLinkPoint > result;
    if (d_ptr->mProxy) {
        QStringList inputs  = d_ptr->mProxy->getInputKeys();
        QStringList outputs = d_ptr->mProxy->getOutputKeys();

        // 生成输入连接点
        for (const QString& key : inputs) {
            DAPyLinkPoint lp;
            lp.way       = DAPyLinkPoint::Input;
            lp.direction = AspectDirection::West;
            lp.name      = key;
            result.append(lp);
        }

        // 生成输出连接点
        for (const QString& key : outputs) {
            DAPyLinkPoint lp;
            lp.way       = DAPyLinkPoint::Output;
            lp.direction = AspectDirection::East;
            lp.name      = key;
            result.append(lp);
        }
    }

    return result;
}

/**
 * @brief 保存到XML
 * @param[in] doc XML文档
 * @param[in] parentElement 父元素
 * @param[in] ver 版本号
 * @return 保存成功返回true
 */
bool DAPyNodeGraphicsItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
    if (!DAGraphicsResizeableItem::saveToXml(doc, parentElement, ver)) {
        return false;
    }

    QDomElement pyNodeEle = doc->createElement("pyNodeItem");

    // 保存渲染模板（始终保存为 nodestyle，升级旧的 rect/svg）
    pyNodeEle.setAttribute("renderTemplate", "nodestyle");

    // 保存节点名称
    pyNodeEle.setAttribute("nodeName", d_ptr->mNodeName);

    // 保存SVG路径（保留向后兼容）
    if (!d_ptr->mSvgPath.isEmpty()) {
        pyNodeEle.setAttribute("svgPath", d_ptr->mSvgPath);
    }

    // 保存状态
    pyNodeEle.setAttribute("nodeState", static_cast< int >(d_ptr->mNodeState));

    // 保存样式配置（稀疏JSON）
    const QJsonObject styleJson = DANodeStyleToJson(d_ptr->mStyle);
    if (!styleJson.isEmpty()) {
        const QJsonDocument styleDoc(styleJson);
        QDomElement styleEle = doc->createElement("style");
        QDomCDATASection cdata = doc->createCDATASection(
            QString::fromUtf8(styleDoc.toJson(QJsonDocument::Compact)));
        styleEle.appendChild(cdata);
        pyNodeEle.appendChild(styleEle);
    }

    parentElement->appendChild(pyNodeEle);
    return true;
}

/**
 * @brief 从XML加载
 * @param[in] itemElement 元素
 * @param[in] ver 版本号
 * @return 加载成功返回true
 */
bool DAPyNodeGraphicsItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
    if (!DAGraphicsResizeableItem::loadFromXml(itemElement, ver)) {
        return false;
    }

    QDomElement pyNodeEle = itemElement->firstChildElement("pyNodeItem");
    if (pyNodeEle.isNull()) {
        return false;
    }

    // 加载渲染模板（含遗留迁移）
    QString tmplName = pyNodeEle.attribute("renderTemplate", "rect");
    if (tmplName == "rect" || tmplName == "svg" || tmplName == "nodestyle") {
        // 遗留迁移：rect/svg/nodestyle → RectTemplate（NodeStyleTemplate）
        setRenderTemplate(RectTemplate);
        if (tmplName == "svg") {
            // SVG迁移：记录旧的svgPath到style
            QString svgPath = pyNodeEle.attribute("svgPath");
            if (!svgPath.isEmpty()) {
                d_ptr->mStyle.bodyIconType = BodyIconType::Svg;
                d_ptr->mStyle.bodyIconSource = svgPath;
            }
        }
    } else if (tmplName == "widget") {
        setRenderTemplate(WidgetTemplate);
    } else {
        // 未知类型，回退到矩形
        setRenderTemplate(RectTemplate);
    }

    // 加载节点名称
    d_ptr->mNodeName = pyNodeEle.attribute("nodeName");

    // 加载SVG路径（保留向后兼容）
    QString svgPath = pyNodeEle.attribute("svgPath");
    if (!svgPath.isEmpty()) {
        loadSvg(svgPath);
    }

    // 加载样式配置（如果存在）
    QDomElement styleEle = pyNodeEle.firstChildElement("style");
    if (!styleEle.isNull()) {
        QDomCDATASection cdata = styleEle.firstChild().toCDATASection();
        if (!cdata.isNull()) {
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(cdata.data().toUtf8(), &error);
            if (error.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                d_ptr->mStyle = DANodeStyleFromJson(jsonDoc.object());
            } else {
                qWarning() << "DAPyNodeGraphicsItem::loadFromXml: style JSON parse error:"
                           << error.errorString();
            }
        }
    }

    // 加载状态
    int state         = pyNodeEle.attribute("nodeState", "0").toInt();
    d_ptr->mNodeState = static_cast< DAPyNodeState >(state);

    return true;
}

/**
 * @brief 绘制主体
 *
 * 优先执行Python自定义绘制回调（paint_callback），
 * 如果回调不存在或执行失败则回退到模板渲染。
 * Python回调签名：def paint(self, painter_proxy, body_rect)
 * painter_proxy为DAPyPainterProxy实例，body_rect为(x,y,w,h)元组。
 * 回调应在50ms内完成绘制，避免阻塞GUI线程。
 *
 * @param[in] painter 画笔
 * @param[in] option 样式选项
 * @param[in] widget 窗口
 * @param[in] bodyRect 主体矩形区域
 */
void DAPyNodeGraphicsItem::paintBody(QPainter* painter,
                                     const QStyleOptionGraphicsItem* option,
                                     QWidget* widget,
                                     const QRectF& bodyRect)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // 如果有自定义绘制回调，尝试调用Python回调
    if (d_ptr->mPaintCallback && !d_ptr->mPaintCallback.isNone()) {
        // 获取GIL，创建代理，调用Python回调
        // 注意：paint回调应在50ms内完成，避免阻塞GUI线程
        DAPyGILGuard gil;
        try {
            DAPyPainterProxy proxy(painter);
            pybind11::tuple bodyTuple =
                pybind11::make_tuple(bodyRect.x(), bodyRect.y(), bodyRect.width(), bodyRect.height());
            d_ptr->mPaintCallback.object()(proxy, bodyTuple);
            d_ptr->mPaintCallbackError = false;
            // 回调成功，绘制连接点后返回
            paintLinkPoints(painter, option, widget);
            return;
        } catch (const pybind11::error_already_set& e) {
            // Python异常必须在GIL作用域内消费，否则析构时会死锁
            qWarning() << "DAPyNodeGraphicsItem paint_callback error:" << e.what();
            d_ptr->mPaintCallbackError = true;
            // 回调失败，回退到rect模板渲染并添加错误标记
        } catch (const std::exception& e) {
            qWarning() << "DAPyNodeGraphicsItem paint_callback exception:" << e.what();
            d_ptr->mPaintCallbackError = true;
        }
        // GIL在gil析构时自动释放
    }

    // 绘制状态装饰（边框颜色等）
    paintStateDecoration(painter, bodyRect);

    // 根据模板类型绘制
    switch (d_ptr->mRenderTemplate) {
    case SvgTemplate:
        paintSvgTemplate(painter, bodyRect);
        break;
    case WidgetTemplate:
        paintWidgetTemplate(painter, bodyRect);
        break;
    case RectTemplate:
    default:
        paintRectTemplate(painter, bodyRect);
        break;
    }

    // 如果paint回调发生过异常，绘制错误标记
    if (d_ptr->mPaintCallbackError) {
        painter->save();
        // 红色边框
        QPen errorPen(QColor(255, 0, 0));
        errorPen.setWidth(2);
        painter->setPen(errorPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(bodyRect, 4, 4);
        // 红色"Error"文本
        QFont font = painter->font();
        font.setPointSize(8);
        font.setBold(true);
        painter->setFont(font);
        painter->setPen(QColor(255, 0, 0));
        QRectF errorRect(bodyRect.left() + 4, bodyRect.top() + 4, bodyRect.width() - 8, 14);
        painter->drawText(errorRect, Qt::AlignLeft | Qt::AlignTop, "Error");
        painter->restore();
    }

    // 绘制连接点
    paintLinkPoints(painter, option, widget);
}

/**
 * @brief 绘制连接点
 * @param[in] painter 画笔
 * @param[in] option 样式选项
 * @param[in] widget 窗口
 *
 * 连接点根据方向绘制不同形状：
 * - East/West 方向：水平矩形 14×10
 * - North/South 方向：垂直矩形 10×14
 *
 * 输入连接点使用白色填充，输出连接点使用深灰色填充。
 * 文字标签根据方向定位在连接点的对侧。
 */
void DAPyNodeGraphicsItem::paintLinkPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();

    // 连接点尺寸常量
    const qreal cLPHWidth  = 14;  // 水平连接点宽度
    const qreal cLPHHeight = 10;  // 水平连接点高度
    const qreal spacing    = 2;   // 文字与连接点间距

    // 设置画笔样式
    QPen pen(Qt::black);
    pen.setWidth(1);
    painter->setPen(pen);

    // 设置小字体用于绘制连接点名称
    QFont smallFont = painter->font();
    smallFont.setPointSize(7);
    painter->setFont(smallFont);
    QFontMetrics fm(smallFont);

    // 绘制输入连接点（白色填充 + 深色边框）
    for (const auto& lp : std::as_const(d_ptr->mInputLinkPoints)) {
        // 根据方向确定矩形尺寸
        qreal halfW, halfH;
        if (lp.direction == AspectDirection::East || lp.direction == AspectDirection::West) {
            // 水平方向：14×10
            halfW = cLPHWidth / 2;
            halfH = cLPHHeight / 2;
        } else {
            // 垂直方向：10×14
            halfW = cLPHHeight / 2;
            halfH = cLPHWidth / 2;
        }

        // 绘制连接点矩形
        QRectF linkRect(lp.position.x() - halfW, lp.position.y() - halfH, halfW * 2, halfH * 2);
        painter->setBrush(QBrush(Qt::white));
        painter->drawRect(linkRect);

        // 绘制连接点名称（方向感知定位）
        QRect textRect = fm.boundingRect(lp.name);
        textRect.adjust(0, 0, spacing, spacing);
        QPointF textPos;

        switch (lp.direction) {
        case AspectDirection::East:
            // 文本在左侧：←◁文本
            textPos.setX(lp.position.x() - halfW - textRect.width() - spacing);
            textPos.setY(lp.position.y() - textRect.height() / 2);
            break;
        case AspectDirection::West:
            // 文本在右侧：文本▷→
            textPos.setX(lp.position.x() + halfW + spacing);
            textPos.setY(lp.position.y() - textRect.height() / 2);
            break;
        case AspectDirection::North:
            // 文本在下方：↑△文本
            textPos.setX(lp.position.x() - textRect.width() / 2);
            textPos.setY(lp.position.y() + halfH + spacing);
            break;
        case AspectDirection::South:
            // 文本在上方：文本▽↓
            textPos.setX(lp.position.x() - textRect.width() / 2);
            textPos.setY(lp.position.y() - halfH - textRect.height());
            break;
        default:
            textPos = lp.position;
            break;
        }

        textRect.moveTopLeft(textPos.toPoint());
        painter->drawText(textRect, Qt::AlignCenter, lp.name);
    }

    // 绘制输出连接点（深灰填充 + 黑色边框）
    for (const auto& lp : std::as_const(d_ptr->mOutputLinkPoints)) {
        // 根据方向确定矩形尺寸
        qreal halfW, halfH;
        if (lp.direction == AspectDirection::East || lp.direction == AspectDirection::West) {
            // 水平方向：14×10
            halfW = cLPHWidth / 2;
            halfH = cLPHHeight / 2;
        } else {
            // 垂直方向：10×14
            halfW = cLPHHeight / 2;
            halfH = cLPHWidth / 2;
        }

        // 绘制连接点矩形
        QRectF linkRect(lp.position.x() - halfW, lp.position.y() - halfH, halfW * 2, halfH * 2);
        painter->setBrush(QBrush(Qt::darkGray));
        painter->drawRect(linkRect);

        // 绘制连接点名称（方向感知定位）
        QRect textRect = fm.boundingRect(lp.name);
        textRect.adjust(0, 0, spacing, spacing);
        QPointF textPos;

        switch (lp.direction) {
        case AspectDirection::East:
            // 文本在左侧：←◁文本
            textPos.setX(lp.position.x() - halfW - textRect.width() - spacing);
            textPos.setY(lp.position.y() - textRect.height() / 2);
            break;
        case AspectDirection::West:
            // 文本在右侧：文本▷→
            textPos.setX(lp.position.x() + halfW + spacing);
            textPos.setY(lp.position.y() - textRect.height() / 2);
            break;
        case AspectDirection::North:
            // 文本在下方：↑△文本
            textPos.setX(lp.position.x() - textRect.width() / 2);
            textPos.setY(lp.position.y() + halfH + spacing);
            break;
        case AspectDirection::South:
            // 文本在上方：文本▽↓
            textPos.setX(lp.position.x() - textRect.width() / 2);
            textPos.setY(lp.position.y() - halfH - textRect.height());
            break;
        default:
            textPos = lp.position;
            break;
        }

        textRect.moveTopLeft(textPos.toPoint());
        painter->drawText(textRect, Qt::AlignCenter, lp.name);
    }

    painter->restore();
}

/**
 * @brief 绘制状态装饰
 * @param[in] painter 画笔
 * @param[in] bodyRect 主体矩形区域
 */
void DAPyNodeGraphicsItem::paintStateDecoration(QPainter* painter, const QRectF& bodyRect)
{
    QColor stateColor = getStateColor();

    if (!stateColor.isValid() || stateColor.alpha() == 0) {
        return;
    }

    painter->save();

    // 根据状态绘制不同的装饰效果
    switch (d_ptr->mNodeState) {
    case Running:
    case Waiting: {
        // 运行/等待状态：绘制半透明边框
        QPen pen(stateColor);
        pen.setWidth(3);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(bodyRect.adjusted(1, 1, -1, -1), 4, 4);
        break;
    }
    case Success:
    case Error:
    case Skipped: {
        // 完成状态：填充背景色
        QBrush brush(stateColor);
        painter->setBrush(brush);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(bodyRect, 4, 4);
        break;
    }
    case Idle:
    default:
        // 空闲状态：不绘制特殊装饰
        break;
    }

    painter->restore();
}

/**
 * @brief 绘制矩形模板
 * @param[in] painter 画笔
 * @param[in] bodyRect 主体矩形区域
 */
void DAPyNodeGraphicsItem::paintRectTemplate(QPainter* painter, const QRectF& bodyRect)
{
    painter->save();

    // 绘制背景（如果状态装饰未填充）
    if (d_ptr->mNodeState == Idle || d_ptr->mNodeState == Waiting) {
        QBrush brush(QColor(240, 240, 240));
        painter->setBrush(brush);
        QPen pen(QColor(180, 180, 180));
        painter->setPen(pen);
        painter->drawRoundedRect(bodyRect, 4, 4);
    }

    // 绘制图标（如果有）
    if (!d_ptr->mIcon.isNull()) {
        QPixmap pixmap = d_ptr->mIcon.pixmap(QSize(24, 24));
        if (!pixmap.isNull()) {
            qreal dr = pixmap.devicePixelRatio();
            QRectF iconRect(bodyRect.left() + 8,
                            bodyRect.top() + (bodyRect.height() - pixmap.height() / dr) / 2,
                            pixmap.width() / dr,
                            pixmap.height() / dr);
            painter->drawPixmap(iconRect.topLeft(), pixmap);
        }
    }

    // 绘制节点名称
    if (!d_ptr->mNodeName.isEmpty()) {
        QFont font = painter->font();
        font.setPointSize(9);
        painter->setFont(font);
        painter->setPen(Qt::black);

        // 计算文本绘制区域（留出图标空间）
        QRectF textRect = bodyRect.adjusted(36, 0, -8, 0);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, d_ptr->mNodeName);
    }

    painter->restore();
}

/**
 * @brief 绘制SVG模板
 * @param[in] painter 画笔
 * @param[in] bodyRect 主体矩形区域
 */
void DAPyNodeGraphicsItem::paintSvgTemplate(QPainter* painter, const QRectF& bodyRect)
{
    if (!d_ptr->mSvgRenderer || !d_ptr->mSvgRenderer->isValid()) {
        // SVG无效时回退到矩形模板
        paintRectTemplate(painter, bodyRect);
        return;
    }

    painter->save();

    // 计算SVG绘制区域（保持比例）
    QSizeF svgSize = d_ptr->mSvgRenderer->defaultSize();
    svgSize.scale(bodyRect.size() * 0.8, Qt::KeepAspectRatio);

    QRectF svgRect;
    svgRect.setSize(svgSize);
    svgRect.moveCenter(bodyRect.center() - QPointF(0, 8));  // 稍微上移，给名称留空间

    // 绘制SVG
    d_ptr->mSvgRenderer->render(painter, svgRect);

    // 绘制节点名称（在SVG下方）
    if (!d_ptr->mNodeName.isEmpty()) {
        QFont font = painter->font();
        font.setPointSize(8);
        painter->setFont(font);
        painter->setPen(Qt::black);

        QRectF textRect(bodyRect.left(), svgRect.bottom() + 2, bodyRect.width(), 16);
        painter->drawText(textRect, Qt::AlignCenter | Qt::AlignTop, d_ptr->mNodeName);
    }

    painter->restore();
}

/**
 * @brief 绘制Widget模板
 * @param[in] painter 画笔
 * @param[in] bodyRect 主体矩形区域
 */
void DAPyNodeGraphicsItem::paintWidgetTemplate(QPainter* painter, const QRectF& bodyRect)
{
    Q_UNUSED(painter);

    // Widget模式下，实际绘制由QGraphicsProxyWidget处理
    // 这里只绘制边框装饰
    if (d_ptr->mProxyWidget) {
        updateWidgetGeometry();
    }
}

/**
 * @brief 设置主体尺寸
 * @param[in] s 尺寸
 */
void DAPyNodeGraphicsItem::setBodySize(const QSizeF& s)
{
    DAGraphicsResizeableItem::setBodySize(s);
    d_ptr->updateLinkPointPositions(getBodyRect());
    updateWidgetGeometry();
}

/**
 * @brief 获取当前状态颜色
 * @return 状态颜色
 */
QColor DAPyNodeGraphicsItem::getStateColor() const
{
    return DAPyNodePalette::getGlobalColorForState(d_ptr->mNodeState);
}

/**
 * @brief 更新Widget几何位置
 */
void DAPyNodeGraphicsItem::updateWidgetGeometry()
{
    if (d_ptr->mProxyWidget && d_ptr->mWidget) {
        d_ptr->mProxyWidget->setGeometry(getBodyRect().toRect());
    }
}

/**
 * @brief 设置自定义绘制回调
 *
 * 设置Python函数对象作为节点自定义绘制回调。
 * Python回调签名：def paint(self, painter_proxy, body_rect)
 * painter_proxy为DAPyPainterProxy实例，body_rect为(x,y,w,h)元组。
 * 回调应在50ms内完成绘制。
 *
 * @param[in] callback Python可调用对象，若为None则清除回调
 */
void DAPyNodeGraphicsItem::setPaintCallback(const pybind11::object& callback)
{
    if (callback.is_none()) {
        clearPaintCallback();
        return;
    }
    DAPyGILGuard gil;
    try {
        d_ptr->mPaintCallback      = DAPySafePyObjectHolder(callback);
        d_ptr->mPaintCallbackError = false;
    } catch (const std::exception& e) {
        qWarning() << "DAPyNodeGraphicsItem setPaintCallback exception:" << e.what();
        d_ptr->mPaintCallback = DAPySafePyObjectHolder();
    }
    update();
}

/**
 * @brief 判断是否存在自定义绘制回调
 *
 * @return 如果mPaintCallback非None返回true，否则返回false
 */
bool DAPyNodeGraphicsItem::hasPaintCallback() const
{
    return d_ptr->mPaintCallback && !d_ptr->mPaintCallback.isNone();
}

/**
 * @brief 清除自定义绘制回调
 *
 * 重置mPaintCallback为None，并清除错误标记。
 */
void DAPyNodeGraphicsItem::clearPaintCallback()
{
    DAPyGILGuard gil;
    try {
        d_ptr->mPaintCallback = DAPySafePyObjectHolder();
    } catch (const std::exception& e) {
        qWarning() << "DAPyNodeGraphicsItem clearPaintCallback exception:" << e.what();
    }
    d_ptr->mPaintCallbackError = false;
    update();
}

/**
 * @brief 位置变化时刷新连接线
 *
 * 当节点位置发生变化后（拖拽移动），立即刷新所有连接线的端点位置，
 * 保证连接线跟随节点实时移动，而不是仅在鼠标释放后才更新。
 * 基类DAGraphicsResizeableItem::itemChange()已处理网格对齐（ItemPositionChange），
 * 本方法仅关注ItemPositionHasChanged事件，在位置确定后委托给场景刷新连接线。
 *
 * @param[in] change 图形项变更类型
 * @param[in] value 变更值
 * @return 基类处理后的返回值
 * @note ItemPositionChange不触发连接线更新，仅ItemPositionHasChanged触发
 * @see updateLinkItems() DAGraphicsResizeableItem::itemChange()
 */
QVariant DAPyNodeGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    // 先调用基类，基类处理网格对齐等逻辑
    QVariant r = DAGraphicsResizeableItem::itemChange(change, value);
    if (change == ItemPositionHasChanged && scene()) {
        updateLinkItems();
    }
    return r;
}

/**
 * @brief 刷新连接线位置
 *
 * 将连接线更新委托给DAPyWorkFlowScene::updateNodeLinkPositions()，
 * 通过场景级的mNodeLinksMap映射表查找节点关联的所有连接线并更新端点。
 * 不维护per-node的连接线列表，避免与场景映射表重复。
 *
 * @see DAPyWorkFlowScene::updateNodeLinkPositions()
 */
void DAPyNodeGraphicsItem::updateLinkItems()
{
    DAPyWorkFlowScene* sc = dynamic_cast<DAPyWorkFlowScene*>(scene());
    if (sc) {
        sc->updateNodeLinkPositions(this);
    }
}

/**
 * @brief 分组位置变化时刷新连接线
 *
 * 当节点作为分组的一部分被整体移动时，基类DAGraphicsItem::groupPositionChanged()
 * 会触发此虚函数。调用updateLinkItems()确保连接线跟随节点组同步更新。
 *
 * @param[in] pos 分组移动后的新位置
 */
void DAPyNodeGraphicsItem::groupPositionChanged(const QPointF& pos)
{
    DAGraphicsItem::groupPositionChanged(pos);
    updateLinkItems();
}

/**
 * @brief 鼠标双击事件处理
 *
 * 双击节点时发射nodeDoubleClicked信号，由DAGui层的场景负责弹出配置对话框。
 * 此方法仅负责检测有效代理并发射信号，不直接创建任何对话框，
 * 以保持DAPyWorkFlow模块不依赖QDialog/QWidget的约束。
 *
 * @param[in] event 鼠标事件
 */
void DAPyNodeGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event)

    // 调用基类实现
    DAGraphicsResizeableItem::mouseDoubleClickEvent(event);

    // 检查是否有有效的代理节点
    if (!d_ptr->mProxy) {
        return;
    }

    // 发射信号，由DAGui层（DAPyWorkFlowGraphicsScene）处理配置对话框
    Q_EMIT nodeDoubleClicked(d_ptr->mProxy.get());
}

}  // end of namespace DA
