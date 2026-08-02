#include "DAPyNodeGraphicsItem.h"
#include "DAPyPainterProxy.h"
#include "DAPyNode.h"
#include "DAPyNodeParameter.h"
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
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QFontMetrics>
#include <QPolygonF>
#include "DAPyGILGuard.h"
#include "DAPybind11QtCaster.hpp"
#include "DAGraphicsViewGlobal.h"
#include "DAPyObjectWrapper.h"
#include "DAPybind11InQt.h"
#include "DAPyWorkFlowEnumStringUtils.h"
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
    // 缓存的参数信息（避免paint时获取GIL）
    struct CachedParameter {
        QString name;        ///< 参数名
        QString typeLabel;   ///< 类型标签
        QVariant value;      ///< 当前值
        bool showOnNode;     ///< 是否在节点上显示
    };

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
    // 计算可见参数数量（showOnNode=true 且不超过 maxDisplayParams）
    int countVisibleParameters() const;
    // 构建tooltip文本
    QString buildTooltip() const;

public:
    DAPyNode mProxy;  ///< Python节点代理（值持有）
    // 缓存字段：从DAPyNode一次性读取，避免paint时GIL开销
    QString mName;                                   ///< 缓存的节点名称
    QString mQualifiedName;                          ///< 缓存的限定名
    QString mIconPath;                               ///< 缓存的图标路径
    QList< QString > mInputKeys;                     ///< 缓存的输入端口key列表
    QList< QString > mOutputKeys;                    ///< 缓存的输出端口key列表
    DAPyNodeStyle mStyle;                            ///< 缓存的节点样式
    QIcon mIcon;                                     ///< 节点图标
    QSvgRenderer* mSvgRenderer { nullptr };          ///< SVG渲染器
    QGraphicsProxyWidget* mProxyWidget { nullptr };  ///< Widget代理
    QWidget* mWidget { nullptr };                    ///< 嵌入的widget
    DAPyNodeState mNodeState { Idle };               ///< 节点状态
    QList< DAPyLinkPoint > mInputLinkPoints;         ///< 输入连接点
    QList< DAPyLinkPoint > mOutputLinkPoints;        ///< 输出连接点
    qreal linkPointDrawWidth { 14 };                 ///< 连接点的绘制宽度（宽度相对于东西方向的宽度）
    qreal linkPointDrawHeight { 10 };                ///< 连接点的绘制高度（高度相对于东西方向的高度）
    static constexpr qreal kPortMinGap { 4.0 };      ///< 连接点之间的最小间隔
    DAPyObjectWrapper mPaintCallback;                ///< 自定义绘制回调（Python函数对象）
    bool mPaintCallbackError { false };              ///< 绘制回调是否发生过异常
    QRectF mIconRect;                                ///< 绘制Icon的区域，仅仅有icon时才有用
    QRectF mTextRect;                                ///< 绘制text的区域
    QRectF mParamRect;                               ///< 绘制参数的区域
    QPixmap mIconPixmap;                             ///< 记录图标的pixmap
    int smallFontSize { 7 };                         ///< 小字体大小（用于渲染节点的名字）
    int normalFontSize { 9 };                        ///< 普通字体大小（用于渲染节点名称）
    // 参数显示控制
    QList< CachedParameter > mCachedParameters;      ///< 缓存的参数列表
    bool mShowParameters { true };                   ///< 是否在节点上显示参数
    int maxDisplayParams { 10 };                      ///< 最多显示的参数行数
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
 * @brief 计算可见参数数量
 * @return showOnNode=true 且不超过 maxDisplayParams 的参数个数
 */
int DAPyNodeGraphicsItem::PrivateData::countVisibleParameters() const
{
    int count = 0;
    for (const auto& cp : std::as_const(mCachedParameters)) {
        if (cp.showOnNode) {
            count++;
        }
    }
    return qMin(count, maxDisplayParams);
}

/**
 * @brief 构建tooltip文本
 * @return HTML格式tooltip，包含节点名和全部参数（不截断）
 */
QString DAPyNodeGraphicsItem::PrivateData::buildTooltip() const
{
    QString tip = QString("<b>%1</b>").arg(mName);
    if (!mCachedParameters.isEmpty()) {
        tip += "<hr>";
        for (const auto& cp : std::as_const(mCachedParameters)) {
            tip += QString("%1: %2<br>").arg(cp.name, cp.value.toString());
        }
    }
    return tip;
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
    const DAPyNodeStyle& st = mStyle;
    int inputCount          = mInputLinkPoints.size();
    if (inputCount > 0) {
        const DAPyNodeStyle::PortSide side = st.inputPortSide;
        for (int i = 0; i < inputCount; ++i) {
            mInputLinkPoints[ i ].direction = side;
            if (side == DAPyNodeStyle::PortSide::West || side == DAPyNodeStyle::PortSide::East) {
                // 垂直均匀分布
                const qreal spacing = bodyRect.height() / (inputCount + 1);
                const qreal x       = (side == DAPyNodeStyle::PortSide::West) ? bodyRect.left() : bodyRect.right();
                mInputLinkPoints[ i ].position = QPointF(x, bodyRect.top() + spacing * (i + 1));
            } else {
                // North/South：水平均匀分布
                const qreal spacing = bodyRect.width() / (inputCount + 1);
                const qreal y       = (side == DAPyNodeStyle::PortSide::North) ? bodyRect.top() : bodyRect.bottom();
                mInputLinkPoints[ i ].position = QPointF(bodyRect.left() + spacing * (i + 1), y);
            }
        }
    }

    // 更新输出连接点位置
    int outputCount = mOutputLinkPoints.size();
    if (outputCount > 0) {
        const DAPyNodeStyle::PortSide side = st.outputPortSide;
        for (int i = 0; i < outputCount; ++i) {
            mOutputLinkPoints[ i ].direction = side;
            if (side == DAPyNodeStyle::PortSide::West || side == DAPyNodeStyle::PortSide::East) {
                // 垂直均匀分布
                const qreal spacing = bodyRect.height() / (outputCount + 1);
                const qreal x       = (side == DAPyNodeStyle::PortSide::West) ? bodyRect.left() : bodyRect.right();
                mOutputLinkPoints[ i ].position = QPointF(x, bodyRect.top() + spacing * (i + 1));
            } else {
                // North/South：水平均匀分布
                const qreal spacing = bodyRect.width() / (outputCount + 1);
                const qreal y       = (side == DAPyNodeStyle::PortSide::North) ? bodyRect.top() : bodyRect.bottom();
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
    const DAPyNodeStyle& s = mStyle;
    // 根据端口方向计算各方向的连接点预留偏移量
    // 使用完整 linkPointDrawWidth 而非 halfLpW，确保文字区域不与端口矩形重叠
    const qreal lpW = linkPointDrawWidth;
    qreal lpLeft = 0, lpRight = 0, lpTop = 0, lpBottom = 0;
    if (!mInputLinkPoints.isEmpty()) {
        switch (s.inputPortSide) {
        case DAAspectDirection::West:
            lpLeft += lpW;
            break;
        case DAAspectDirection::East:
            lpRight += lpW;
            break;
        case DAAspectDirection::North:
            lpTop += lpW;
            break;
        case DAAspectDirection::South:
            lpBottom += lpW;
            break;
        }
    }
    if (!mOutputLinkPoints.isEmpty()) {
        switch (s.outputPortSide) {
        case DAAspectDirection::West:
            lpLeft += lpW;
            break;
        case DAAspectDirection::East:
            lpRight += lpW;
            break;
        case DAAspectDirection::North:
            lpTop += lpW;
            break;
        case DAAspectDirection::South:
            lpBottom += lpW;
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

        // 判断是否需要为参数留出空间
        bool hasPaintCb = (mPaintCallback && !mPaintCallback.isNone());
        bool showParams = mShowParameters && !hasPaintCb && !mCachedParameters.isEmpty();
        int paramVisibleCount = showParams ? countVisibleParameters() : 0;
        // 如果参数总数超过maxDisplayParams，额外留一行给"..."
        int totalVisible = 0;
        for (const auto& cp : std::as_const(mCachedParameters)) {
            if (cp.showOnNode) totalVisible++;
        }
        bool hasOverflow = showParams && totalVisible > maxDisplayParams;

        // 标题区域高度（参数显示时仅占顶部，否则占整个body）
        QFont titleFont;
        titleFont.setPointSize(normalFontSize);
        QFontMetricsF titleFm(titleFont);
        qreal titleAreaHeight = showParams
                                     ? (qMax(iconSize, titleFm.height()) + 2 * static_cast< qreal >(qMin(4.0, s.cornerRadius)))
                                     : (bodyRect.height() - lpTop - lpBottom);

        const int space = qMin(4.0, s.cornerRadius);
        if (s.isIconLeftOfText()) {
            // icon在左文字在右
            // 定位icon位置，icon位于最左边
            mIconRect.setLeft(bodyRect.left() + space + lpLeft);
            mIconRect.setTop(bodyRect.top() + (titleAreaHeight - iconSize) / 2.0 + lpTop);
            mIconRect.setWidth(iconSize);
            mIconRect.setHeight(iconSize);
            // 剩下的为文字区域
            mTextRect.setLeft(mIconRect.right() + space);
            mTextRect.setTop(bodyRect.top() + space + lpTop);
            mTextRect.setWidth(bodyRect.right() - mIconRect.right() - 2 * space - lpRight);
            mTextRect.setHeight(titleAreaHeight - 2 * space);
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
            mTextRect.setHeight(titleAreaHeight - iconSize - 2 * space);
        }

        // 参数区域
        if (showParams && paramVisibleCount > 0) {
            QFont paramFont;
            paramFont.setPointSize(smallFontSize);
            QFontMetricsF paramFm(paramFont);
            qreal paramStartY = bodyRect.top() + titleAreaHeight + lpTop;
            int paramLines = paramVisibleCount + (hasOverflow ? 1 : 0);
            qreal paramHeight = paramLines * (paramFm.height() + 2) + 8;  // 8 = separator gap
            mParamRect.setLeft(bodyRect.left() + space + lpLeft);
            mParamRect.setTop(paramStartY);
            mParamRect.setWidth(bodyRect.width() - 2 * space - lpRight - lpLeft);
            mParamRect.setHeight(paramHeight);
        } else {
            mParamRect = QRectF();
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
        if (s.bodyIconType == DAPyNodeStyle::SvgBodyIcon) {
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
 * @param[in] proxy Python节点代理（const引用，值拷贝持有）
 * @param[in] parent 父图形项
 */
DAPyNodeGraphicsItem::DAPyNodeGraphicsItem(const DAPyNode& proxy, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), DA_PIMPL_CONSTRUCT
{
    // 设置可选中和可移动
    setSelectable(true);
    setMovable(true);
    setZValue(DA::ZValue_NodeItem);  // 显式设置节点 z-value，确保高于连接线
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
void DAPyNodeGraphicsItem::setRenderTemplate(DAPyNodeStyle::NodeRenderTemplate tmpl)
{
    DA_D(d);
    if (d->mStyle.renderTemplate == tmpl) {
        return;
    }

    // 清理之前的资源
    if (d->mStyle.renderTemplate == DAPyNodeStyle::RenderDefaultTemplate) {
        d->cleanupSvg();
    } else if (d_ptr->mStyle.renderTemplate == DAPyNodeStyle::RenderWidgetTemplate) {
        d->cleanupWidget();
    }

    d->mStyle.renderTemplate = tmpl;

    // 初始化新的资源
    if (tmpl == DAPyNodeStyle::RenderWidgetTemplate && !d->mProxyWidget) {
        d->mProxyWidget = new QGraphicsProxyWidget(this);
    } else {
        d->updateNodeStyle(getBodyRect());
    }
    update();
}

/**
 * @brief 获取Python节点代理
 * @return 代理const引用
 */
const DAPyNode& DAPyNodeGraphicsItem::getProxy() const
{
    return d_ptr->mProxy;
}

/**
 * @brief 设置Python节点代理
 * @param[in] proxy 代理const引用（值拷贝持有）
 */
void DAPyNodeGraphicsItem::setProxy(const DAPyNode& proxy)
{
    DAPyGILGuard gil;
    d_ptr->mProxy = proxy;
    if (!proxy.isNone()) {
        d_ptr->mNodeState     = proxy.getNodeState();
        d_ptr->mName          = proxy.getNodeName();
        d_ptr->mQualifiedName = proxy.getQualifiedName();
        d_ptr->mIconPath      = proxy.getIcon();
        d_ptr->mInputKeys     = proxy.getInputKeys();
        d_ptr->mOutputKeys    = proxy.getOutputKeys();
        d_ptr->mStyle         = proxy.getNodeStyle();

        // 如果 Python 节点实例定义了 paint 方法，自动注册为自定义绘制回调
        try {
            pybind11::object pyObj = proxy.object();
            if (pybind11::hasattr(pyObj, "paint")) {
                pybind11::object paintAttr = pyObj.attr("paint");
                // 检查是否可调用（pybind11 没有 callable 自由函数，使用 __call__ 属性判断）
                if (pybind11::hasattr(paintAttr, "__call__")) {
                    d_ptr->mPaintCallback      = DAPyObjectWrapper(paintAttr);
                    d_ptr->mPaintCallbackError = false;
                } else {
                    d_ptr->mPaintCallback      = DAPyObjectWrapper();
                    d_ptr->mPaintCallbackError = false;
                }
            } else {
                d_ptr->mPaintCallback      = DAPyObjectWrapper();
                d_ptr->mPaintCallbackError = false;
            }
        } catch (const std::exception& e) {
            qWarning() << "DAPyNodeGraphicsItem setProxy paint callback exception:" << e.what();
            d_ptr->mPaintCallback      = DAPyObjectWrapper();
            d_ptr->mPaintCallbackError = false;
        }

        // 缓存参数信息（名称/类型/值/show_on_node）
        try {
            QList< DAPyNodeParameter > params = proxy.getParameters();
            d_ptr->mCachedParameters.clear();
            for (const auto& p : std::as_const(params)) {
                PrivateData::CachedParameter cp;
                cp.name      = p.name();
                cp.typeLabel = p.typeLabel();
                cp.value     = proxy.getParameterValue(p.name());
                QVariantHash props = p.properties();
                cp.showOnNode      = props.value("show_on_node", true).toBool();
                d_ptr->mCachedParameters.append(cp);
            }
        } catch (const std::exception& e) {
            qWarning() << "DAPyNodeGraphicsItem setProxy parameter cache exception:" << e.what();
            d_ptr->mCachedParameters.clear();
        }
    } else {
        d_ptr->mPaintCallback      = DAPyObjectWrapper();
        d_ptr->mPaintCallbackError = false;
        d_ptr->mCachedParameters.clear();
    }
    updateLinkPoints();
    // 构建并设置tooltip（包含完整参数信息，不截断）
    setToolTip(d_ptr->buildTooltip());
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
    if (d->mName == name) {
        return;
    }
    d->mName = name;
    updateNodeBody();
}

/**
 * @brief 获取节点名称
 * @return 节点名称
 */
QString DAPyNodeGraphicsItem::getNodeName() const
{
    return d_ptr->mName;
}

/**
 * @brief 设置节点样式
 * @param[in] style 节点样式配置
 */
void DAPyNodeGraphicsItem::setNodeStyle(const DAPyNodeStyle& style)
{
    d_ptr->mStyle = style;
    d_ptr->updateNodeStyle(getBodyRect());
    update();
}

/**
 * @brief 获取节点样式（非常量引用，允许修改）
 * @return 节点样式引用
 */
DAPyNodeStyle& DAPyNodeGraphicsItem::nodeStyle()
{
    return d_ptr->mStyle;
}

/**
 * @brief 获取节点样式（常量引用）
 * @return 节点样式常量引用
 */
const DAPyNodeStyle& DAPyNodeGraphicsItem::nodeStyle() const
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
 * @brief 设置嵌入的Widget
 * @param[in] widget 要嵌入的widget
 */
void DAPyNodeGraphicsItem::setWidget(QWidget* widget)
{
    if (d_ptr->mWidget == widget) {
        return;
    }
    DA_D(d);
    // 确保proxy widget已创建
    if (!d->mProxyWidget) {
        d->mProxyWidget = new QGraphicsProxyWidget(this);
    }

    // 设置widget
    d->mProxyWidget->setWidget(widget);
    d->mWidget = widget;

    // 如果当前不是widget模式，切换到widget模式
    if (d->mStyle.renderTemplate != DAPyNodeStyle::RenderWidgetTemplate) {
        d->mStyle.renderTemplate = DAPyNodeStyle::RenderWidgetTemplate;
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
 *
 * 无条件设置节点状态并触发重绘。
 * 即使新状态与当前状态相同也会调用 update()，
 * 因为节点执行完成后 Python 端的缓存数据（如 paint 回调依赖的 _last_text）
 * 可能已更新，需要重绘以反映最新数据。
 *
 * @param[in] state 节点状态
 */
void DAPyNodeGraphicsItem::setNodeState(DAPyNodeState state)
{
    d_ptr->mNodeState = state;
    update();
}

/**
 * @brief 从代理更新缓存字段
 *
 * 一次性从已持有的代理读取所有属性并缓存到PrivateData字段，
 * 避免每次paint时都需要获取GIL调用attr()。
 */
void DAPyNodeGraphicsItem::updateFromProxy()
{
    setProxy(d_ptr->mProxy);
}

/**
 * @brief 从缓存字段更新连接点
 *
 * 根据 mInputKeys 生成输入连接点，
 * 根据 mOutputKeys 生成输出连接点，
 * 使用 mStyle.inputPortSide/outputPortSide 设置连接点方向。
 */
void DAPyNodeGraphicsItem::updateLinkPoints()
{
    DA_D(d);
    const DAPyNodeStyle::PortSide inputSide  = d->mStyle.inputPortSide;
    const DAPyNodeStyle::PortSide outputSide = d->mStyle.outputPortSide;

    d->mInputLinkPoints.clear();
    d->mOutputLinkPoints.clear();

    // 从缓存输入key列表生成输入连接点
    for (int i = 0; i < d->mInputKeys.size(); ++i) {
        DAPyLinkPoint lp;
        lp.way       = DAPyLinkPoint::Input;
        lp.direction = inputSide;
        lp.name      = d->mInputKeys[ i ].isEmpty() ? QString("input_%1").arg(i) : d->mInputKeys[ i ];
        d->mInputLinkPoints.append(lp);
    }

    // 从缓存输出key列表生成输出连接点
    for (int i = 0; i < d->mOutputKeys.size(); ++i) {
        DAPyLinkPoint lp;
        lp.way       = DAPyLinkPoint::Output;
        lp.direction = outputSide;
        lp.name      = d->mOutputKeys[ i ].isEmpty() ? QString("output_%1").arg(i) : d->mOutputKeys[ i ];
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
    DA_DC(d);
    QList< DAPyLinkPoint > result;
    const DAPyNodeStyle::PortSide inputSide  = d->mStyle.inputPortSide;
    const DAPyNodeStyle::PortSide outputSide = d->mStyle.outputPortSide;

    // 从缓存输入key列表生成连接点
    for (const QString& key : std::as_const(d->mInputKeys)) {
        DAPyLinkPoint lp;
        lp.way       = DAPyLinkPoint::Input;
        lp.direction = inputSide;
        lp.name      = key;
        result.append(lp);
    }
    // 从缓存输出key列表生成连接点
    for (const QString& key : std::as_const(d->mOutputKeys)) {
        DAPyLinkPoint lp;
        lp.way       = DAPyLinkPoint::Output;
        lp.direction = outputSide;
        lp.name      = key;
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
    // 保存节点ID（用于与Python工作流逻辑数据关联）
    if (!d_ptr->mProxy.isNone()) {
        pyNodeEle.setAttribute("node_id", d_ptr->mProxy.getNodeId());
    }
    // 保存渲染模板
    pyNodeEle.setAttribute("renderTemplate", enumToString(d_ptr->mStyle.renderTemplate));
    // 保存节点状态
    pyNodeEle.setAttribute("nodeState", enumToString(d_ptr->mNodeState));

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
    // 加载渲染模板
    QString tmplStr = pyNodeEle.attribute("renderTemplate");
    if (!tmplStr.isEmpty()) {
        d_ptr->mStyle.renderTemplate = stringToEnum(tmplStr, DAPyNodeStyle::RenderDefaultTemplate);
    }
    // 加载节点状态
    QString stateStr = pyNodeEle.attribute("nodeState");
    if (!stateStr.isEmpty()) {
        d_ptr->mNodeState = stringToEnum(stateStr, DA::Idle);
    }
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
            // 确保 da_py_workflow 模块已加载，使 DAPyPainterProxy 类型转换器可用
            pybind11::module_::import("da_py_workflow");
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
            // Idle 状态下不显示错误覆盖层（首次拖入未运行工作流时静默回退）
            d_ptr->mPaintCallbackError = (d_ptr->mNodeState != Idle);
        } catch (const std::exception& e) {
            qWarning() << "DAPyNodeGraphicsItem paint_callback exception:" << e.what();
            d_ptr->mPaintCallbackError = (d_ptr->mNodeState != Idle);
        }
        // GIL在gil析构时自动释放
    }

    // 绘制状态装饰（边框颜色等）
    paintStateDecoration(painter, bodyRect);

    // 根据模板类型绘制
    switch (d_ptr->mStyle.renderTemplate) {
    case DAPyNodeStyle::RenderDefaultTemplate:
        paintNodeStyleBody(painter, bodyRect);
        break;
    case DAPyNodeStyle::RenderWidgetTemplate:
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
        if (lp.direction == DAAspectDirection::East || lp.direction == DAAspectDirection::West) {
            halfW = linkPointDrawWidth / 2;
            halfH = linkPointDrawHeight / 2;
        } else {
            halfW = linkPointDrawHeight / 2;
            halfH = linkPointDrawWidth / 2;
        }

        QRectF linkRect(lp.position.x() - halfW, lp.position.y() - halfH, halfW * 2, halfH * 2);

        // 绘制连接点形状
        switch (portStyle.shape) {
        case DAPyLinkPointStyle::Circle:
            painter->drawEllipse(linkRect);
            break;
        case DAPyLinkPointStyle::Diamond: {
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
        case DAPyLinkPointStyle::Rect:
        default:
            painter->drawRect(linkRect);
            break;
        }

        // 绘制连接点名称（方向感知定位）
        QRectF textRect = fm.boundingRect(lp.name);
        textRect.adjust(0, 0, spacing, spacing);

        switch (lp.direction) {
        case DAAspectDirection::East: {
            QPointF textPos(lp.position.x() + halfW + spacing, lp.position.y() - textRect.height() / 2);
            textRect.moveTopLeft(textPos);
            painter->drawText(textRect, Qt::AlignCenter, lp.name);
        } break;
        case DAAspectDirection::West: {
            QPointF textPos(lp.position.x() - halfW - spacing - textRect.width(), lp.position.y() - textRect.height() / 2);
            textRect.moveTopLeft(textPos);
            painter->drawText(textRect, Qt::AlignCenter, lp.name);
        } break;
        case DAAspectDirection::North: {
            // 顺时针旋转90度绘制文字
            painter->save();
            QTransform transform;
            transform.translate(lp.position.x(),  // + textRect.height() / 2
                                lp.position.y() - (halfH + spacing + textRect.width()));
            transform.rotate(90);
            painter->setTransform(transform, true);
            // QRectF rotatedRect(-textRect.height() / 2, -textRect.width() / 2, textRect.height(), textRect.width());
            // painter->drawText(rotatedRect, Qt::AlignCenter, lp.name);
            painter->drawText(textRect, Qt::AlignCenter, lp.name);
            painter->restore();
        } break;
        case DAAspectDirection::South: {
            // 顺时针旋转90度绘制文字
            painter->save();
            QTransform transform;
            transform.translate(lp.position.x(), lp.position.y() + (halfH + spacing));
            transform.rotate(90);
            painter->setTransform(transform, true);

            // QRectF rotatedRect(-textRect.height() / 2, -textRect.width() / 2, textRect.height(), textRect.width());
            painter->drawText(textRect, Qt::AlignCenter, lp.name);
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
    const DAPyNodeStyle& st = d->mStyle;
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
    case Waiting:
    case Success:
    case Error:
    case Skipped: {
        // 运行/等待状态：绘制半透明边框
        QPen pen(stateColor);
        pen.setWidth(1);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        // 边框始终跟随 bodyShape
        switch (d_ptr->mStyle.bodyShape) {
        case DAPyNodeStyle::EllipseShape:
            painter->drawEllipse(bodyRect.adjusted(1, 1, -1, -1));
            break;
        case DAPyNodeStyle::DiamondShape: {
            QPolygonF diamond;
            QRectF r = bodyRect.adjusted(1, 1, -1, -1);
            diamond << QPointF(r.center().x(), r.top()) << QPointF(r.right(), r.center().y())
                    << QPointF(r.center().x(), r.bottom()) << QPointF(r.left(), r.center().y());
            painter->drawPolygon(diamond);
            break;
        }
        case DAPyNodeStyle::RoundedRectShape:
        default:
            painter->drawRoundedRect(bodyRect.adjusted(1, 1, -1, -1), 4, 4);
            break;
        }
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

    const DAPyNodeStyle& style = d->mStyle;

    // 确定背景色（无效时使用默认值）
    QColor bgColor  = style.backgroundColor.isValid() ? style.backgroundColor : QColor(240, 240, 240);
    QColor bdrColor = style.borderColor.isValid() ? style.borderColor : QColor(180, 180, 180);

    // 绘制主体形状
    painter->setBrush(QBrush(bgColor));
    QPen pen(bdrColor);
    pen.setWidthF(style.borderWidth);
    painter->setPen(pen);

    switch (style.bodyShape) {
    case DAPyNodeStyle::EllipseShape:
        painter->drawEllipse(bodyRect);
        break;
    case DAPyNodeStyle::DiamondShape: {
        // 菱形：四边中点连线
        QPolygonF diamond;
        diamond << QPointF(bodyRect.center().x(), bodyRect.top()) << QPointF(bodyRect.right(), bodyRect.center().y())
                << QPointF(bodyRect.center().x(), bodyRect.bottom()) << QPointF(bodyRect.left(), bodyRect.center().y());
        painter->drawPolygon(diamond);
        break;
    }
    case DAPyNodeStyle::RoundedRectShape:
    default:
        painter->drawRoundedRect(bodyRect, style.cornerRadius, style.cornerRadius);
        break;
    }

    // 绘制pixmap
    painter->drawPixmap(d->mIconRect.toRect(), d->mIconPixmap);
    // 绘制文字
    if (!d->mName.isEmpty()) {
        QFont font = painter->font();
        font.setPointSize(d->normalFontSize);
        painter->setFont(font);
        painter->setPen(Qt::black);
        // 若文字超出mTextRect宽度，自动省略显示，避免裁剪
        QFontMetricsF fm(font);
        QString displayName = d->mName;
        if (fm.horizontalAdvance(displayName) > d->mTextRect.width()) {
            displayName = fm.elidedText(displayName, Qt::ElideRight, d->mTextRect.width());
        }
        painter->drawText(d->mTextRect, Qt::AlignCenter, displayName);
    }
    painter->restore();

    // 参数渲染（仅默认模板且无自定义paint回调时）
    paintParameters(painter, bodyRect, d->mTextRect);
}

/**
 * @brief 绘制节点参数
 *
 * 在节点标题区域下方绘制分割线和参数列表，每行一个参数：
 *   参数名:参数值
 * 超出宽度的行末尾显示省略号。参数数量超过 maxDisplayParams 时显示 "..."。
 * 仅在 mShowParameters 为 true 且无自定义paint回调时生效。
 *
 * @param[in] painter 画笔
 * @param[in] bodyRect 节点body矩形
 * @param[in] titleRect 标题文字区域（分割线位于其下方）
 */
void DAPyNodeGraphicsItem::paintParameters(QPainter* painter, const QRectF& bodyRect, const QRectF& titleRect)
{
    DA_D(d);
    // 无参数、关闭显示、有自定义paint回调时跳过
    if (!d->mShowParameters || d->mCachedParameters.isEmpty()) {
        return;
    }
    if (d->mPaintCallback && !d->mPaintCallback.isNone()) {
        return;
    }

    painter->save();
    QFont paramFont;
    paramFont.setPointSize(d->smallFontSize);
    painter->setFont(paramFont);
    QFontMetricsF fm(paramFont);

    const qreal margin    = 4;
    const qreal separatorGap = 4;  // 分割线上下间距
    const qreal lineStep  = fm.height() + 2;

    // 分割线位于标题区域下方
    qreal separatorY = titleRect.bottom() + separatorGap;
    QPen sepPen(d->mStyle.borderColor);
    sepPen.setWidthF(1.0);
    painter->setPen(sepPen);
    painter->drawLine(QPointF(bodyRect.left() + margin, separatorY),
                      QPointF(bodyRect.right() - margin, separatorY));

    // 参数行
    painter->setPen(QColor(60, 60, 60));
    qreal drawY = separatorY + separatorGap + fm.ascent();
    qreal textWidth = bodyRect.width() - 2 * margin;
    int shown = 0;

    for (const auto& cp : std::as_const(d->mCachedParameters)) {
        if (!cp.showOnNode) {
            continue;
        }
        if (shown >= d->maxDisplayParams) {
            painter->drawText(QRectF(bodyRect.left() + margin, drawY - fm.ascent(),
                                     textWidth, fm.height()),
                              Qt::AlignLeft | Qt::AlignVCenter, "...");
            break;
        }
        QString valueStr = cp.value.toString();
        QString line     = cp.name + ": " + valueStr;
        line = fm.elidedText(line, Qt::ElideRight, textWidth);
        painter->drawText(QPointF(bodyRect.left() + margin, drawY), line);
        drawY += lineStep;
        shown++;
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
    if (d->mStyle.namePosition == DAPyNodeStyle::NameBelowBody && !d_ptr->mName.isEmpty()) {
        QFont font;
        font.setPointSize(d->normalFontSize);
        QFontMetricsF fm(font);
        const qreal textHeight = fm.height() + 4;  // 额外4px间距
        const qreal textWidth  = fm.horizontalAdvance(d->mName) + 4;
        // 如果文字比body宽，水平扩展
        qreal extraWidth = qMax(0.0, textWidth - rect.width());
        rect.adjust(0, 0, extraWidth, textHeight);
    }

    // 端口扩展：需要覆盖端口矩形 + 标签文字
    const DAPyNodeStyle::PortSide& inputSide  = d->mStyle.inputPortSide;
    const DAPyNodeStyle::PortSide& outputSide = d->mStyle.outputPortSide;

    // 计算端口标签最大宽度
    QFont smallFont;
    smallFont.setPointSize(d->smallFontSize);
    QFontMetricsF smallFm(smallFont);
    qreal maxLabelW = 0;
    for (const QString& key : std::as_const(d->mInputKeys)) {
        maxLabelW = qMax(maxLabelW, smallFm.horizontalAdvance(key));
    }
    for (const QString& key : std::as_const(d->mOutputKeys)) {
        maxLabelW = qMax(maxLabelW, smallFm.horizontalAdvance(key));
    }
    const qreal labelSpacing = 2.0;
    // East/West 端口偏移 = 端口半宽 + 标签间距 + 标签宽度 + 安全余量
    const qreal ewOffset = d->linkPointDrawWidth / 2 + labelSpacing + maxLabelW + 4;
    // North/South 端口偏移 = 端口半高 + 标签间距 + 标签高度 + 安全余量
    const qreal nsOffset = d->linkPointDrawWidth / 2 + labelSpacing + smallFm.height() + 4;

    // 计算各方向扩展量（同一方向有 input+output 时累加）
    qreal leftOff = 0, rightOff = 0, topOff = 0, bottomOff = 0;
    if (!d->mInputLinkPoints.isEmpty()) {
        if (inputSide == DAPyNodeStyle::PortSide::West)
            leftOff += ewOffset;
        else if (inputSide == DAPyNodeStyle::PortSide::East)
            rightOff += ewOffset;
        else if (inputSide == DAPyNodeStyle::PortSide::North)
            topOff += nsOffset;
        else if (inputSide == DAPyNodeStyle::PortSide::South)
            bottomOff += nsOffset;
    }
    if (!d->mOutputLinkPoints.isEmpty()) {
        if (outputSide == DAPyNodeStyle::PortSide::West)
            leftOff += ewOffset;
        else if (outputSide == DAPyNodeStyle::PortSide::East)
            rightOff += ewOffset;
        else if (outputSide == DAPyNodeStyle::PortSide::North)
            topOff += nsOffset;
        else if (outputSide == DAPyNodeStyle::PortSide::South)
            bottomOff += nsOffset;
    }

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

    if (d->mStyle.bodyShape == DAPyNodeStyle::EllipseShape) {
        path.addEllipse(getBodyRect());
    } else if (d->mStyle.bodyShape == DAPyNodeStyle::DiamondShape) {
        QRectF r = getBodyRect();
        QPolygonF diamond;
        diamond << QPointF(r.center().x(), r.top()) << QPointF(r.right(), r.center().y())
                << QPointF(r.center().x(), r.bottom()) << QPointF(r.left(), r.center().y());
        path.addPolygon(diamond);
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
    if (d->mStyle.renderTemplate == DAPyNodeStyle::RenderDefaultTemplate) {
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
        d_ptr->mPaintCallback      = DAPyObjectWrapper(callback);
        d_ptr->mPaintCallbackError = false;
    } catch (const std::exception& e) {
        qWarning() << "DAPyNodeGraphicsItem setPaintCallback exception:" << e.what();
        d_ptr->mPaintCallback = DAPyObjectWrapper();
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
        d_ptr->mPaintCallback = DAPyObjectWrapper();
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
    if (d_ptr->mProxy.isNone()) {
        return;
    }

    // 发射信号，由DAGui层（DAPyWorkFlowGraphicsScene）处理配置对话框
    Q_EMIT nodeDoubleClicked(d_ptr->mProxy);
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
    // 使用 horizontalAdvance 计算文字宽度（boundingRect 返回紧密包围盒，
    // 略小于 drawText 实际占用的前进宽度，会导致文字截断显示 "..."）
    const qreal textAdvanceWidth = fm.horizontalAdvance(d->mName);
    QRectF textBoundRect         = fm.boundingRect(d->mName);
    // 计算推荐
    qreal bodyWidth        = 0.0;
    qreal bodyHeight       = 0.0;
    const DAPyNodeStyle& s = d->mStyle;
    const int space        = qMin(4.0, s.cornerRadius);
    qreal iconSize         = s.iconSize;
    if (s.bodyIconSource.isEmpty()) {
        iconSize = 0.0;
    }
    if (s.isNameInside()) {
        if (s.isIconLeftOfText()) {
            // icon在左文字在右
            bodyWidth  = iconSize + 3 * space + textAdvanceWidth;
            bodyHeight = qMax(iconSize + 2 * space, textBoundRect.height() + 2 * space);
        } else {
            // icon在上文字在下
            bodyWidth  = qMax(textAdvanceWidth + 2 * space, iconSize + 2 * space);
            bodyHeight = iconSize + textBoundRect.height() + 3 * space;
        }
    } else {
        // name在外面(Below)，body宽度需容纳文字宽度（取icon和文字宽度的最大值）
        qreal textContentWidth = textAdvanceWidth + 2 * space;
        qreal iconContentWidth = (s.bodyIconSource.isEmpty()) ? 2 * space : iconSize + 2 * space;
        bodyWidth              = qMax(textContentWidth, iconContentWidth);
        bodyHeight             = qMax(iconSize + 2 * space, 2.0 * space);  // 最小高度保障
    }

    // 预留连接点矩形的位置（仅端口矩形宽度，标签文字朝外绘制，不影响 body 尺寸）
    const int inputCount  = d->mInputLinkPoints.size();
    const int outputCount = d->mOutputLinkPoints.size();
    if (inputCount > 0) {
        if (s.inputPortSide == DAAspectDirection::East || s.inputPortSide == DAAspectDirection::West) {
            bodyWidth += d->linkPointDrawWidth;
        } else {
            bodyHeight += d->linkPointDrawWidth;
        }
    }
    if (outputCount > 0) {
        if (s.outputPortSide == DAAspectDirection::East || s.outputPortSide == DAAspectDirection::West) {
            bodyWidth += d->linkPointDrawWidth;
        } else {
            bodyHeight += d->linkPointDrawWidth;
        }
    }

    // 确保body高度足够容纳East/West方向的连接点（避免端口过多时拥挤叠加）
    auto calcMinPortSpan = [ & ](int count) -> qreal {
        if (count <= 0)
            return 0.0;
        const qreal slotH = d->linkPointDrawHeight + d->kPortMinGap;
        return count * slotH + d->kPortMinGap;
    };
    if (inputCount > 0 && (s.inputPortSide == DAAspectDirection::East || s.inputPortSide == DAAspectDirection::West)) {
        bodyHeight = qMax(bodyHeight, calcMinPortSpan(inputCount));
    }
    if (outputCount > 0 && (s.outputPortSide == DAAspectDirection::East || s.outputPortSide == DAAspectDirection::West)) {
        bodyHeight = qMax(bodyHeight, calcMinPortSpan(outputCount));
    }

    // 确保body宽度足够容纳North/South方向的连接点
    auto calcMinPortSpanW = [ & ](int count) -> qreal {
        if (count <= 0)
            return 0.0;
        const qreal slotW = d->linkPointDrawWidth + d->kPortMinGap;
        return count * slotW + d->kPortMinGap;
    };
    if (inputCount > 0 && (s.inputPortSide == DAAspectDirection::North || s.inputPortSide == DAAspectDirection::South)) {
        bodyWidth = qMax(bodyWidth, calcMinPortSpanW(inputCount));
    }
    if (outputCount > 0 && (s.outputPortSide == DAAspectDirection::North || s.outputPortSide == DAAspectDirection::South)) {
        bodyWidth = qMax(bodyWidth, calcMinPortSpanW(outputCount));
    }

    // 参数渲染预留空间（仅默认模板且无自定义paint回调时）
    bool hasPaintCallback = (d->mPaintCallback && !d->mPaintCallback.isNone());
    if (d->mShowParameters && !hasPaintCallback && !d->mCachedParameters.isEmpty()) {
        QFont paramFont;
        paramFont.setPointSize(d->smallFontSize);
        QFontMetricsF paramFm(paramFont);
        const qreal paramMargin = 4;
        const qreal separatorGap = 8;  // 分割线 + 上下间距

        // 分割线高度
        bodyHeight += separatorGap;

        // 每个可见参数一行
        int visibleCount = d->countVisibleParameters();
        bodyHeight += visibleCount * (paramFm.height() + 2);
        // 如果超过 maxDisplayParams，额外一行 "..."
        int totalVisible = 0;
        for (const auto& cp : std::as_const(d->mCachedParameters)) {
            if (cp.showOnNode) totalVisible++;
        }
        if (totalVisible > d->maxDisplayParams) {
            bodyHeight += paramFm.height() + 2;
        }

        // 宽度：取最长的 "name: value" 行
        for (const auto& cp : std::as_const(d->mCachedParameters)) {
            if (!cp.showOnNode) continue;
            QString line = cp.name + ": " + cp.value.toString();
            qreal lineWidth = paramFm.horizontalAdvance(line);
            bodyWidth = qMax(bodyWidth, lineWidth + 2 * paramMargin);
        }
    }

    // 应用最小 body 尺寸限制
    if (s.minBodyWidth > 0) {
        bodyWidth = qMax(bodyWidth, s.minBodyWidth);
    }
    if (s.minBodyHeight > 0) {
        bodyHeight = qMax(bodyHeight, s.minBodyHeight);
    }

    setBodySize(QSizeF(bodyWidth, bodyHeight));  // 内部会调用updateNodeStyleGeometry
    update();
}

/**
 * @brief 是否在节点上显示参数
 * @return true表示参数渲染开启
 */
bool DAPyNodeGraphicsItem::isShowParameters() const
{
    return d_ptr->mShowParameters;
}

/**
 * @brief 设置是否在节点上显示参数
 * @param[in] show true开启参数渲染，false关闭（恢复最简显示状态）
 */
void DAPyNodeGraphicsItem::setShowParameters(bool show)
{
    if (d_ptr->mShowParameters == show) {
        return;
    }
    d_ptr->mShowParameters = show;
    updateNodeBody();
}

/**
 * @brief 获取最多显示的参数行数
 * @return 最大行数
 */
int DAPyNodeGraphicsItem::getMaxDisplayParams() const
{
    return d_ptr->maxDisplayParams;
}

/**
 * @brief 设置最多显示的参数行数
 * @param[in] max 最大行数
 */
void DAPyNodeGraphicsItem::setMaxDisplayParams(int max)
{
    d_ptr->maxDisplayParams = qMax(1, max);
    updateNodeBody();
}

/**
 * @brief 刷新参数缓存
 *
 * 从Python节点重新读取参数名/值/show_on_node，并重建tooltip。
 * 在参数被修改后调用以更新节点显示。
 */
void DAPyNodeGraphicsItem::refreshParameterCache()
{
    DAPyGILGuard gil;
    if (d_ptr->mProxy.isNone()) {
        d_ptr->mCachedParameters.clear();
        return;
    }
    try {
        QList< DAPyNodeParameter > params = d_ptr->mProxy.getParameters();
        d_ptr->mCachedParameters.clear();
        for (const auto& p : std::as_const(params)) {
            PrivateData::CachedParameter cp;
            cp.name      = p.name();
            cp.typeLabel = p.typeLabel();
            cp.value     = d_ptr->mProxy.getParameterValue(p.name());
            QVariantHash props = p.properties();
            cp.showOnNode      = props.value("show_on_node", true).toBool();
            d_ptr->mCachedParameters.append(cp);
        }
    } catch (const std::exception& e) {
        qWarning() << "DAPyNodeGraphicsItem refreshParameterCache exception:" << e.what();
        d_ptr->mCachedParameters.clear();
    }
    setToolTip(d_ptr->buildTooltip());
    updateNodeBody();
}

}  // end of namespace DA
