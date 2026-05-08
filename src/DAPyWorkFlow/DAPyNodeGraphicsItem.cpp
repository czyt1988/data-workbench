#include "DAPyNodeGraphicsItem.h"
#include "DANodeDescriptor.h"
#include "DAPyPainterProxy.h"
#include "DAPyNodeProxy.h"
#include "DAPyNodePalette.h"
#include "DAPyLinkPoint.h"
#include "DAPyWorkFlowScene.h"
#include <memory>
#include <QImage>
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
#include "DAGraphicsViewGlobal.h"
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
    // 更新连接点位置
    void updateLinkPointPositions(const QRectF& bodyRect);
    // 清理widget
    void cleanupWidget();
    // 清理SVG
    void cleanupSvg();
    // 准备nodestyle需要的数据，包括预加载图标，预计算好位置
    void updateNodeStyle(const QRectF& bodyRect);

public:
    std::unique_ptr< DAPyNodeProxy > mProxy;                               ///< Python节点代理（独占所有权）
    RenderTemplate mRenderTemplate { RenderTemplate::NodeStyleTemplate };  ///< 当前渲染模板
    QIcon mIcon;                                                           ///< 节点图标
    QSvgRenderer* mSvgRenderer { nullptr };                                ///< SVG渲染器
    QGraphicsProxyWidget* mProxyWidget { nullptr };                        ///< Widget代理
    QWidget* mWidget { nullptr };                                          ///< 嵌入的widget
    DAPyNodeState mNodeState { Idle };                                     ///< 节点状态
    QJsonObject mDescriptor;                                               ///< 节点描述符
    DANodeDescriptor mDescriptorStruct;                                    ///< 节点描述符结构体
    QList< DAPyLinkPoint > mInputLinkPoints;                               ///< 输入连接点
    QList< DAPyLinkPoint > mOutputLinkPoints;                              ///< 输出连接点
    qreal linkPointDrawWidth { 14 };        ///< 连接点的绘制宽度（宽度相对于东西方向的宽度）
    qreal linkPointDrawHeight { 10 };       ///< 连接点的绘制高度（高度相对于东西方向的高度）
    DAPySafePyObjectHolder mPaintCallback;  ///< 自定义绘制回调（Python函数对象）
    bool mPaintCallbackError { false };     ///< 绘制回调是否发生过异常
    QRectF mIconRect;                       ///< 绘制Icon的区域，仅仅有icon时才有用
    QRectF mTextRect;                       ///< 绘制text的区域
    QPixmap mIconPixmap;                    ///< 记录图标的pixmap
    int smallFontSize { 7 };                ///< 小字体大小（用于渲染节点的名字）
    int normalFontSize { 9 };               ///< 普通字体大小（用于渲染节点名称）
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
 * @brief 更新连接点位置
 * @param[in] bodyRect 节点主体矩形区域
 *
 * 根据 d_ptr->mStyle.inputPortSide / outputPortSide 在四个方向上定位连接点。
 * West/East：沿垂直方向均匀分布；North/South：沿水平方向均匀分布。
 */
void DAPyNodeGraphicsItem::PrivateData::updateLinkPointPositions(const QRectF& bodyRect)
{
    // 更新输入连接点位置
    const DANodeStyle& st = mDescriptorStruct.style;
    int inputCount        = mInputLinkPoints.size();
    if (inputCount > 0) {
        const PortSide side = st.inputPortSide;
        for (int i = 0; i < inputCount; ++i) {
            mInputLinkPoints[ i ].direction = side;
            if (side == PortSide::West || side == PortSide::East) {
                // 垂直均匀分布
                const qreal spacing            = bodyRect.height() / (inputCount + 1);
                const qreal x                  = (side == PortSide::West) ? bodyRect.left() : bodyRect.right();
                mInputLinkPoints[ i ].position = QPointF(x, bodyRect.top() + spacing * (i + 1));
            } else {
                // North/South：水平均匀分布
                const qreal spacing            = bodyRect.width() / (inputCount + 1);
                const qreal y                  = (side == PortSide::North) ? bodyRect.top() : bodyRect.bottom();
                mInputLinkPoints[ i ].position = QPointF(bodyRect.left() + spacing * (i + 1), y);
            }
        }
    }

    // 更新输出连接点位置
    int outputCount = mOutputLinkPoints.size();
    if (outputCount > 0) {
        const PortSide side = st.outputPortSide;
        for (int i = 0; i < outputCount; ++i) {
            mOutputLinkPoints[ i ].direction = side;
            if (side == PortSide::West || side == PortSide::East) {
                // 垂直均匀分布
                const qreal spacing             = bodyRect.height() / (outputCount + 1);
                const qreal x                   = (side == PortSide::West) ? bodyRect.left() : bodyRect.right();
                mOutputLinkPoints[ i ].position = QPointF(x, bodyRect.top() + spacing * (i + 1));
            } else {
                // North/South：水平均匀分布
                const qreal spacing             = bodyRect.width() / (outputCount + 1);
                const qreal y                   = (side == PortSide::North) ? bodyRect.top() : bodyRect.bottom();
                mOutputLinkPoints[ i ].position = QPointF(bodyRect.left() + spacing * (i + 1), y);
            }
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

void DAPyNodeGraphicsItem::PrivateData::updateNodeStyle(const QRectF& bodyRect)
{
    const DANodeStyle& s = mDescriptorStruct.style;
    // 根据端口方向计算各方向的连接点预留偏移量
    const qreal halfLpW = linkPointDrawWidth / 2;
    qreal lpLeft = 0, lpRight = 0, lpTop = 0, lpBottom = 0;
    if (!mInputLinkPoints.isEmpty()) {
        switch (s.inputPortSide) {
        case AspectDirection::West:
            lpLeft = halfLpW;
            break;
        case AspectDirection::East:
            lpRight = halfLpW;
            break;
        case AspectDirection::North:
            lpTop = halfLpW;
            break;
        case AspectDirection::South:
            lpBottom = halfLpW;
            break;
        }
    }
    if (!mOutputLinkPoints.isEmpty()) {
        switch (s.outputPortSide) {
        case AspectDirection::West:
            lpLeft = halfLpW;
            break;
        case AspectDirection::East:
            lpRight = halfLpW;
            break;
        case AspectDirection::North:
            lpTop = halfLpW;
            break;
        case AspectDirection::South:
            lpBottom = halfLpW;
            break;
        }
    }
    qreal iconSize = s.iconSize;
    if (s.bodyIconSource.isEmpty()) {
        iconSize = 0.0;
    }
    if (s.isNameInside()) {
        // 如果名字是在里面，iconPosition才有用
        // 布局icon位置和text位置，存入mIconRect和mTextRect中

        const int space = qMin(4.0, s.cornerRadius);
        if (s.isIconLeftOfText()) {
            // icon在左文字在右
            // 定位icon位置，icon位于最左边
            mIconRect.setLeft(bodyRect.left() + space + lpLeft);
            mIconRect.setTop(bodyRect.top() + (bodyRect.height() - iconSize) / 2.0 + lpTop);
            mIconRect.setWidth(iconSize);
            mIconRect.setHeight(iconSize);
            // 剩下的为文字区域
            mTextRect.setLeft(mIconRect.right() + space);
            mTextRect.setTop(bodyRect.top() + space + lpTop);
            mTextRect.setWidth(bodyRect.right() - mIconRect.right() - 2 * space - lpRight);
            mTextRect.setHeight(bodyRect.height() - 2 * space - lpTop - lpBottom);
        } else {
            // icon在上文字在下
            mIconRect.setLeft(bodyRect.left() + (bodyRect.width() - iconSize - lpRight - lpLeft) / 2.0 + lpLeft);
            mIconRect.setTop(bodyRect.top() + space + lpTop);
            mIconRect.setWidth(iconSize);
            mIconRect.setHeight(iconSize);
            // 布局text
            mTextRect.setLeft(bodyRect.left() + space + lpLeft);
            mTextRect.setTop(mIconRect.bottom() + space);
            mTextRect.setWidth(bodyRect.width() - 2 * space - lpRight - lpLeft);
            mTextRect.setHeight(bodyRect.bottom() - mIconRect.bottom() - 2 * space - lpBottom);
        }
    } else {
        // 文字放外面，icon居中布局
        // 布局icon位置和text位置，存入mIconRect和mTextRect中
        mIconRect.setLeft(bodyRect.left() + (bodyRect.width() - iconSize - lpRight - lpLeft) / 2.0 + lpLeft);
        mIconRect.setTop(bodyRect.top() + (bodyRect.height() - iconSize - lpTop - lpBottom) / 2.0 + lpTop);
        mIconRect.setWidth(iconSize);
        mIconRect.setHeight(iconSize);
        // 文字
        QFont font;
        font.setPointSize(normalFontSize);
        QFontMetricsF fm(font);
        mTextRect.setTop(bodyRect.bottom() + 2);
        mTextRect.setLeft(bodyRect.left());
        mTextRect.setWidth(bodyRect.width());
        mTextRect.setHeight(fm.height() + 2);
    }
    // 获取图标，转换为pixmap，存入mIconPixmap中（仅在尺寸变化时重渲染）
    if (!s.bodyIconSource.isEmpty()) {
        if (s.bodyIconType == BodyIconType::Svg) {
            if (!mSvgRenderer) {
                mSvgRenderer = new QSvgRenderer(q_ptr);
                mSvgRenderer->load(s.bodyIconSource);
            }
            if (!mSvgRenderer->isValid()) {
                mSvgRenderer->load(s.bodyIconSource);
            }
            if (mSvgRenderer->isValid()) {
                // 仅在mIconRect尺寸变化或pixmap无效时重渲染，避免频繁开销
                QSize targetSize = mIconRect.size().toSize();
                if (mIconPixmap.isNull() || mIconPixmap.size() != targetSize) {
                    QPixmap pixmap(targetSize);
                    pixmap.fill(Qt::transparent);
                    QPainter painter(&pixmap);
                    mSvgRenderer->render(&painter, pixmap.rect());
                    mIconPixmap = pixmap;
                }
            }
        } else {
            // TODO 通过QImage加载到QPixmap
        }
    } else {
        mIconPixmap = QPixmap();
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
    // 设置可选中和可移动
    setSelectable(true);
    setMovable(true);
    // 设置默认尺寸
    setProxy(proxy);
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
    if (d_ptr->mRenderTemplate == RenderTemplate::NodeStyleTemplate) {
        d_ptr->cleanupSvg();
    } else if (d_ptr->mRenderTemplate == RenderTemplate::WidgetTemplate) {
        d_ptr->cleanupWidget();
    }

    d_ptr->mRenderTemplate = tmpl;

    // 初始化新的资源
    if (tmpl == RenderTemplate::WidgetTemplate && !d_ptr->mProxyWidget) {
        d_ptr->mProxyWidget = new QGraphicsProxyWidget(this);
    } else {
        d_ptr->updateNodeStyle(getBodyRect());
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
    if (lowerName == "widget") {
        setRenderTemplate(RenderTemplate::WidgetTemplate);
    } else {
        setRenderTemplate(RenderTemplate::NodeStyleTemplate);
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
    case RenderTemplate::NodeStyleTemplate:
        return QString("nodestyle");
    case RenderTemplate::WidgetTemplate:
        return QString("widget");
    default:
        break;
    }
    return QString("nodestyle");
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
        d_ptr->mNodeState = proxy->getNodeState();
    }
    updateLinkPoints();
    update();
}

/**
 * @brief 设置节点名称
 * @param[in] name 节点名称
 *
 * 设置名称后自动调用updateNodeBody()重新估算节点尺寸，
 * 确保名称文字不会被裁剪。
 */
void DAPyNodeGraphicsItem::setNodeName(const QString& name)
{
    DA_D(d);
    if (d->mDescriptorStruct.name == name) {
        return;
    }
    d->mDescriptorStruct.name = name;
    updateNodeBody();
}

/**
 * @brief 获取节点名称
 * @return 节点名称
 */
QString DAPyNodeGraphicsItem::getNodeName() const
{
    return d_ptr->mDescriptorStruct.name;
}

/**
 * @brief 设置节点样式
 * @param[in] style 节点样式配置
 */
void DAPyNodeGraphicsItem::setNodeStyle(const DANodeStyle& style)
{
    d_ptr->mDescriptorStruct.style = style;
    d_ptr->updateNodeStyle(getBodyRect());
    update();
}

/**
 * @brief 获取节点样式（非常量引用，允许修改）
 * @return 节点样式引用
 */
DANodeStyle& DAPyNodeGraphicsItem::nodeStyle()
{
    return d_ptr->mDescriptorStruct.style;
}

/**
 * @brief 获取节点样式（常量引用）
 * @return 节点样式常量引用
 */
const DANodeStyle& DAPyNodeGraphicsItem::nodeStyle() const
{
    return d_ptr->mDescriptorStruct.style;
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
    if (d_ptr->mRenderTemplate != RenderTemplate::WidgetTemplate) {
        d_ptr->mRenderTemplate = RenderTemplate::WidgetTemplate;
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
 * @brief 设置节点描述符结构体
 * @param[in] desc 描述符结构体
 *
 * 存储 C++ 原生描述符结构体，并根据 inputs/outputs 生成连接点，
 * 同时更新节点名称、渲染模板和样式等属性。
 */
void DAPyNodeGraphicsItem::setDescriptorStruct(const DANodeDescriptor& desc)
{
    d_ptr->mDescriptorStruct = desc;
    // 同步渲染模板
    setRenderTemplate(desc.renderTemplate);
    // 同步节点样式
    setNodeStyle(desc.style);
    // 从结构体生成连接点
    updateLinkPoints();
    update();
}

/**
 * @brief 获取节点描述符结构体
 * @return 描述符结构体常量引用
 */
const DANodeDescriptor& DAPyNodeGraphicsItem::getDescriptorStruct() const
{
    return d_ptr->mDescriptorStruct;
}

/**
 * @brief 从DANodeDescriptor结构体更新连接点
 *
 * 根据 mDescriptorStruct.inputs 生成输入连接点，
 * 根据 mDescriptorStruct.outputs 生成输出连接点，
 * 使用 mStyle.inputPortSide/outputPortSide 设置连接点方向。
 */
void DAPyNodeGraphicsItem::updateLinkPoints()
{
    DA_D(d);
    const PortSide inputSide  = d->mDescriptorStruct.style.inputPortSide;
    const PortSide outputSide = d->mDescriptorStruct.style.outputPortSide;

    d->mInputLinkPoints.clear();
    d->mOutputLinkPoints.clear();

    // 从描述符输入端口生成输入连接点
    for (int i = 0; i < d->mDescriptorStruct.inputs.size(); ++i) {
        DAPyLinkPoint lp;
        lp.way       = DAPyLinkPoint::Input;
        lp.direction = inputSide;
        lp.name      = d_ptr->mDescriptorStruct.inputs[ i ].name.isEmpty() ? QString("input_%1").arg(i)
                                                                           : d_ptr->mDescriptorStruct.inputs[ i ].name;
        d->mInputLinkPoints.append(lp);
    }

    // 从描述符输出端口生成输出连接点
    for (int i = 0; i < d_ptr->mDescriptorStruct.outputs.size(); ++i) {
        DAPyLinkPoint lp;
        lp.way       = DAPyLinkPoint::Output;
        lp.direction = outputSide;
        lp.name      = d_ptr->mDescriptorStruct.outputs[ i ].name.isEmpty() ? QString("output_%1").arg(i)
                                                                            : d_ptr->mDescriptorStruct.outputs[ i ].name;
        d->mOutputLinkPoints.append(lp);
    }

    d->updateLinkPointPositions(getBodyRect());
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
 * @brief 生成连接点
 * @return 连接点列表
 */
QList< DAPyLinkPoint > DAPyNodeGraphicsItem::generateLinkPoints() const
{
    // 优先从描述符生成
    DA_DC(d);
    QList< DAPyLinkPoint > result;
    const PortSide inputSide  = d->mDescriptorStruct.style.inputPortSide;
    const PortSide outputSide = d->mDescriptorStruct.style.outputPortSide;

    // 从描述符解析输入连接点
    for (const DAPortDescriptor& pd : std::as_const(d->mDescriptorStruct.inputs)) {
        DAPyLinkPoint lp;
        lp.way       = DAPyLinkPoint::Input;
        lp.direction = inputSide;
        lp.name      = pd.name;
        result.append(lp);
    }
    // 从描述符解析输出连接点
    for (const DAPortDescriptor& pd : std::as_const(d->mDescriptorStruct.outputs)) {
        DAPyLinkPoint lp;
        lp.way       = DAPyLinkPoint::Output;
        lp.direction = outputSide;
        lp.name      = pd.name;
        result.append(lp);
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
    // TODO

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
    // TODO
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
    case RenderTemplate::NodeStyleTemplate:
        paintNodeStyleBody(painter, bodyRect);
        break;
    case RenderTemplate::WidgetTemplate:
        paintWidgetTemplate(painter, bodyRect);
        break;
    default:
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
 * @brief 绘制一组连接点（输入或输出）
 *
 * 根据端口样式绘制连接点形状和方向感知的文字标签。
 * East/West方向文字水平绘制；North/South方向文字旋转90度绘制。
 *
 * @param[in] painter 画笔
 * @param[in] points 连接点列表
 * @param[in] portStyle 端口样式配置
 * @param[in] defaultFillColor 默认填充色（输入为白色，输出为深灰色）
 * @param[in] linkPointDrawWidth 连接点绘制宽度
 * @param[in] linkPointDrawHeight 连接点绘制高度
 * @param[in] smallFontSize 连接点标签字体大小
 */
static void drawLinkPointGroup(QPainter* painter,
                               const QList< DAPyLinkPoint >& points,
                               const DAPyLinkPointStyle& portStyle,
                               const QColor& defaultFillColor,
                               qreal linkPointDrawWidth,
                               qreal linkPointDrawHeight,
                               int smallFontSize)
{
    const qreal spacing = 2;  // 文字与连接点间距

    QBrush fillBrush(portStyle.isFillColorValid() ? portStyle.fillColor : defaultFillColor);
    QPen borderPen(portStyle.isBorderColorValid() ? portStyle.borderColor : Qt::black);
    borderPen.setWidthF(portStyle.borderWidth);
    painter->setBrush(fillBrush);
    painter->setPen(borderPen);

    QFont smallFont = painter->font();
    smallFont.setPointSize(smallFontSize);
    QFontMetricsF fm(smallFont);

    for (const auto& lp : std::as_const(points)) {
        // 根据方向确定矩形尺寸（East/West为水平矩形，North/South为垂直矩形）
        qreal halfW, halfH;
        if (lp.direction == AspectDirection::East || lp.direction == AspectDirection::West) {
            halfW = linkPointDrawWidth / 2;
            halfH = linkPointDrawHeight / 2;
        } else {
            halfW = linkPointDrawHeight / 2;
            halfH = linkPointDrawWidth / 2;
        }

        QRectF linkRect(lp.position.x() - halfW, lp.position.y() - halfH, halfW * 2, halfH * 2);

        // 绘制连接点形状
        switch (portStyle.shape) {
        case PortShape::Circle:
            painter->drawEllipse(linkRect);
            break;
        case PortShape::Diamond: {
            QPainterPath diamond;
            const qreal cx = linkRect.center().x();
            const qreal cy = linkRect.center().y();
            const qreal hw = linkRect.width() / 2;
            const qreal hh = linkRect.height() / 2;
            diamond.moveTo(cx, cy - hh);
            diamond.lineTo(cx + hw, cy);
            diamond.lineTo(cx, cy + hh);
            diamond.lineTo(cx - hw, cy);
            diamond.closeSubpath();
            painter->drawPath(diamond);
            break;
        }
        case PortShape::Rect:
        default:
            painter->drawRect(linkRect);
            break;
        }

        // 绘制连接点名称（方向感知定位）
        QRectF textRect = fm.boundingRect(lp.name);
        textRect.adjust(0, 0, spacing, spacing);

        switch (lp.direction) {
        case AspectDirection::East: {
            QPointF textPos(lp.position.x() + halfW + spacing, lp.position.y() - textRect.height() / 2);
            textRect.moveTopLeft(textPos);
            painter->drawText(textRect, Qt::AlignCenter, lp.name);
        } break;
        case AspectDirection::West: {
            QPointF textPos(lp.position.x() - halfW - spacing - textRect.width(), lp.position.y() - textRect.height() / 2);
            textRect.moveTopLeft(textPos);
            painter->drawText(textRect, Qt::AlignCenter, lp.name);
        } break;
        case AspectDirection::North: {
            // 顺时针旋转90度绘制文字
            painter->save();
            QTransform transform;
            transform.translate(lp.position.x(), lp.position.y() + halfH + spacing + textRect.width() / 2);
            transform.rotate(90);
            painter->setTransform(transform, true);
            QRectF rotatedRect(-textRect.height() / 2, -textRect.width() / 2, textRect.height(), textRect.width());
            painter->drawText(rotatedRect, Qt::AlignCenter, lp.name);
            painter->restore();
        } break;
        case AspectDirection::South: {
            // 顺时针旋转90度绘制文字
            painter->save();
            QTransform transform;
            transform.translate(lp.position.x(), lp.position.y() - halfH - spacing - textRect.width() / 2);
            transform.rotate(90);
            painter->setTransform(transform, true);

            QRectF rotatedRect(-textRect.height() / 2, -textRect.width() / 2, textRect.height(), textRect.width());
            painter->drawText(rotatedRect, Qt::AlignCenter, lp.name);
            painter->restore();
        } break;
        default:
            textRect.moveTopLeft(lp.position);
            painter->drawText(textRect, Qt::AlignCenter, lp.name);
            break;
        }
    }
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
    DA_D(d);

    painter->save();

    // 设置字体用于连接点标签
    QFont smallFont = painter->font();
    smallFont.setPointSize(d->smallFontSize);
    painter->setFont(smallFont);
    const DANodeStyle& st = d->mDescriptorStruct.style;
    // 绘制输入连接点（默认白色填充）
    drawLinkPointGroup(
        painter, d->mInputLinkPoints, st.inputPortStyle, Qt::white, d->linkPointDrawWidth, d->linkPointDrawHeight, d->smallFontSize);

    // 绘制输出连接点（默认深灰色填充）
    drawLinkPointGroup(painter,
                       d->mOutputLinkPoints,
                       st.outputPortStyle,
                       Qt::darkGray,
                       d->linkPointDrawWidth,
                       d->linkPointDrawHeight,
                       d->smallFontSize);

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
        // 边框始终跟随 bodyShape
        if (d_ptr->mDescriptorStruct.style.bodyShape == BodyShape::Ellipse) {
            painter->drawEllipse(bodyRect.adjusted(1, 1, -1, -1));
        } else {
            painter->drawRoundedRect(bodyRect.adjusted(1, 1, -1, -1), 4, 4);
        }
        break;
    }
    case Success:
    case Error:
    case Skipped: {
        // 完成状态：填充背景色
        QBrush brush(stateColor);
        painter->setBrush(brush);
        painter->setPen(Qt::NoPen);

        // 根据 bodyShape 裁剪填充区域
        if (d_ptr->mDescriptorStruct.style.bodyShape == BodyShape::Ellipse) {
            QPainterPath clipPath;
            clipPath.addEllipse(bodyRect);
            painter->setClipPath(clipPath);
        }

        painter->drawRect(bodyRect);
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
 * @brief 绘制统一节点样式模板
 * @param[in] painter 画笔
 * @param[in] bodyRect 主体矩形区域
 *
 * 根据 d_ptr->mStyle (DANodeStyle) 配置绘制节点主体，
 * 包括形状、背景色、边框、图标和名称。
 */
void DAPyNodeGraphicsItem::paintNodeStyleBody(QPainter* painter, const QRectF& bodyRect)
{
    DA_D(d);
    painter->save();

    const DANodeStyle& style = d->mDescriptorStruct.style;

    // 确定背景色（无效时使用默认值）
    QColor bgColor  = style.backgroundColor.isValid() ? style.backgroundColor : QColor(240, 240, 240);
    QColor bdrColor = style.borderColor.isValid() ? style.borderColor : QColor(180, 180, 180);

    // 绘制主体形状
    painter->setBrush(QBrush(bgColor));
    QPen pen(bdrColor);
    pen.setWidthF(style.borderWidth);
    painter->setPen(pen);

    switch (style.bodyShape) {
    case BodyShape::Ellipse:
        painter->drawEllipse(bodyRect);
        break;
    case BodyShape::RoundedRect:
    default:
        painter->drawRoundedRect(bodyRect, style.cornerRadius, style.cornerRadius);
        break;
    }

    // 绘制pixmap
    painter->drawPixmap(d->mIconRect.toRect(), d->mIconPixmap);
    // 绘制文字
    if (!d->mDescriptorStruct.name.isEmpty()) {
        QFont font = painter->font();
        font.setPointSize(d->normalFontSize);
        painter->setFont(font);
        painter->setPen(Qt::black);
        // 若文字超出mTextRect宽度，自动省略显示，避免裁剪
        QFontMetricsF fm(font);
        QString displayName = d->mDescriptorStruct.name;
        if (fm.horizontalAdvance(displayName) > d->mTextRect.width()) {
            displayName = fm.elidedText(displayName, Qt::ElideRight, d->mTextRect.width());
        }
        painter->drawText(d->mTextRect, Qt::AlignCenter, displayName);
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
 * @brief 计算边界矩形
 * @return 边界矩形
 *
 * 当名称位置为 Below 时，向下扩展以容纳名称文本，
 * 且当文字宽度超出body时水平扩展以确保文字完整可见。
 * 当输入或输出端口方位为 North/South 时，垂直扩展以容纳连接点。
 */
QRectF DAPyNodeGraphicsItem::boundingRect() const
{
    QRectF rect = DAGraphicsResizeableItem::boundingRect();
    DA_DC(d);
    // 名称位置扩展（Below 模式）
    if (d->mDescriptorStruct.style.namePosition == NamePosition::Below && !d_ptr->mDescriptorStruct.name.isEmpty()) {
        QFont font;
        font.setPointSize(d->normalFontSize);
        QFontMetricsF fm(font);
        const qreal textHeight = fm.height() + 4;  // 额外4px间距
        const qreal textWidth  = fm.horizontalAdvance(d->mDescriptorStruct.name) + 4;
        // 如果文字比body宽，水平扩展
        qreal extraWidth = qMax(0.0, textWidth - rect.width());
        rect.adjust(0, 0, extraWidth, textHeight);
    }

    // 端口扩展
    const PortSide inputSide  = d->mDescriptorStruct.style.inputPortSide;
    const PortSide outputSide = d->mDescriptorStruct.style.outputPortSide;

    // 端口突出间距常量
    constexpr qreal kPortOffset = 8.0;

    // 计算各方向扩展量（独立计算左右上下）
    qreal leftOff   = (inputSide == PortSide::West || outputSide == PortSide::West) ? kPortOffset : 0;
    qreal rightOff  = (inputSide == PortSide::East || outputSide == PortSide::East) ? kPortOffset : 0;
    qreal topOff    = (inputSide == PortSide::North || outputSide == PortSide::North) ? kPortOffset : 0;
    qreal bottomOff = (inputSide == PortSide::South || outputSide == PortSide::South) ? kPortOffset : 0;

    rect.adjust(-leftOff, -topOff, rightOff, bottomOff);

    return rect;
}

/**
 * @brief 计算碰撞形状
 * @return 碰撞路径
 *
 * 当 bodyShape 为 Ellipse 时，返回椭圆路径而非矩形路径，
 * 确保矩形角落的点击不会被误检测为命中。
 */
QPainterPath DAPyNodeGraphicsItem::shape() const
{
    DA_DC(d);
    QPainterPath path;

    if (d->mDescriptorStruct.style.bodyShape == BodyShape::Ellipse) {
        path.addEllipse(getBodyControlRect());
    } else {
        // RoundedRect 等保持默认矩形路径（复用基类行为）
        path = DAGraphicsResizeableItem::shape();
    }

    return path;
}

/**
 * @brief 设置主体尺寸
 * @param[in] s 尺寸
 */
void DAPyNodeGraphicsItem::setBodySize(const QSizeF& s)
{
    DA_D(d);
    DAGraphicsResizeableItem::setBodySize(s);
    d->updateLinkPointPositions(getBodyRect());
    if (d->mRenderTemplate == RenderTemplate::NodeStyleTemplate) {
        updateNodeStyleGeometry();
    } else {
        updateWidgetGeometry();
    }
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
 * @brief 更新样式的几何位置
 */
void DAPyNodeGraphicsItem::updateNodeStyleGeometry()
{
    d_ptr->updateNodeStyle(getBodyRect());
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
    DAPyWorkFlowScene* sc = dynamic_cast< DAPyWorkFlowScene* >(scene());
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
    // 调用基类实现
    DAGraphicsResizeableItem::mouseDoubleClickEvent(event);

    // 检查是否有有效的代理节点
    if (!d_ptr->mProxy) {
        return;
    }

    // 发射信号，由DAGui层（DAPyWorkFlowGraphicsScene）处理配置对话框
    Q_EMIT nodeDoubleClicked(d_ptr->mProxy.get());
}

/**
 * @brief  估算一个最优的body尺寸
 */
void DAPyNodeGraphicsItem::updateNodeBody()
{
    DA_D(d);
    QFont font;
    font.setPointSize(d->normalFontSize);
    QFontMetricsF fm(font);
    // 文本信息
    QRectF textBoundRect = fm.boundingRect(d->mDescriptorStruct.name);
    // 计算推荐
    qreal bodyWidth      = 0.0;
    qreal bodyHeight     = 0.0;
    const DANodeStyle& s = d->mDescriptorStruct.style;
    const int space      = qMin(4.0, s.cornerRadius);
    qreal iconSize       = s.iconSize;
    if (s.bodyIconSource.isEmpty()) {
        iconSize = 0.0;
    }
    if (s.isNameInside()) {
        if (s.isIconLeftOfText()) {
            // icon在左文字在右
            bodyWidth  = iconSize + 3 * space + textBoundRect.width();
            bodyHeight = qMax(iconSize + 2 * space, textBoundRect.height() + 2 * space);
        } else {
            // icon在上文字在下
            bodyWidth  = qMax(textBoundRect.width() + 2 * space, iconSize + 2 * space);
            bodyHeight = iconSize + textBoundRect.height() + 3 * space;
        }
    } else {
        // name在外面(Below)，body宽度需容纳文字宽度（取icon和文字宽度的最大值）
        qreal textContentWidth = textBoundRect.width() + 2 * space;
        qreal iconContentWidth = (s.bodyIconSource.isEmpty()) ? 2 * space : iconSize + 2 * space;
        bodyWidth              = qMax(textContentWidth, iconContentWidth);
        bodyHeight             = qMax(iconSize + 2 * space, 2.0 * space);  // 最小高度保障
    }
    // 还需要预留连接点的位置
    if (d->mInputLinkPoints.size() > 0) {
        if (s.inputPortSide == AspectDirection::East || s.inputPortSide == AspectDirection::West) {
            // 输入在水平方向
            bodyWidth += d->linkPointDrawWidth / 2;
        } else {
            bodyHeight += d->linkPointDrawWidth / 2;
        }
    }
    if (d->mOutputLinkPoints.size() > 0) {
        if (s.outputPortSide == AspectDirection::East || s.outputPortSide == AspectDirection::West) {
            // 输入在水平方向
            bodyWidth += d->linkPointDrawWidth / 2;
        } else {
            bodyHeight += d->linkPointDrawWidth / 2;
        }
    }
    setBodySize(QSizeF(bodyWidth, bodyHeight));
    d->updateNodeStyle(getBodyRect());
    update();
}

}  // end of namespace DA
