#ifndef DAGRAPHICSRESIZEABLEITEM_H
#define DAGRAPHICSRESIZEABLEITEM_H
#include "DAGraphicsItem.h"
#include <QColor>
#include <QBrush>
#include "DAGraphicsViewGlobal.h"
class QDomDocument;
class QDomElement;
namespace DA
{

/**
 * @brief DAGraphicsResizeableItem对应的样式
 *
 * 关键函数：
 * 1、@sa paintBody 这个函数必须继承，用于绘制内容
 * 2、@sa setBodySize 此函数的重载可以控制绘图区域尺寸，如图片等比例缩放就可以通过此函数控制
 * 3、@sa prepareControlInfoChange 当尺寸发生改变时要调用此函数刷新控制点信息
 */
class DAGRAPHICSVIEW_API DAGraphicsResizeableItemPalette
{
public:
    DAGraphicsResizeableItemPalette()
        : resizeControlPointBorderColor(128, 128, 147)
        , resizeBorderColor(32, 128, 240)
        , resizeControlPointBrush(QColor(32, 128, 240))
    {
    }
    QColor resizeControlPointBorderColor;  ///< 缩放时，用于调整尺寸的8个点的边框颜色
    QColor resizeBorderColor;              ///< 缩放时，缩放边框颜色
    QBrush resizeControlPointBrush;        ///< 缩放时，用于调整尺寸的8个点的颜色
};

/**
 * @brief 用于调整大小的item
 *
 * TODO:DAGraphicsResizeableItem改为DAGraphicsResizeOverlayItem
 *
 * @todo 这里作为一个item的基类会有以下问题：
 *   - 1.调整尺寸的控制点会被上层的item覆盖
 *   - 2.再嵌套的子item中，此方案需要考虑多样化
 *   - 3.必须继承DAGraphicsResizeableItem
 *  为了解决上述问题，需要把DAGraphicsResizeableItem作为一个mask或者叫Overlay（覆盖层），把DAGraphicsResizeableItem改为DAGraphicsResizeOverlayItem，
 *  在用户需要进行尺寸变换时，由scene生成DAGraphicsResizeOverlayItem，附着在要变换的item上面，
 *  在变换完成后向scene发送requestResize信号，由scene改变对应item的大小
 */
class DAGRAPHICSVIEW_API DAGraphicsResizeableItem : public DAGraphicsItem
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAGraphicsResizeableItem)
public:
    /**
     * @brief 表征控制点和控制线的类型
     */
    enum ControlType
    {
        NotUnderAnyControlType = 0,  ///< 不在控制点上
        ControlPointTopLeft,         ///< 左上控制点
        ControlPointTopMid,          ///< 顶部中间控制点
        ControlPointTopRight,        ///< 右上控制点
        ControlPointRightMid,        ///< 右边中间控制点
        ControlPointBottomRight,     ///< 右下控制点
        ControlPointBottomMid,       ///< 底部中间控制点
        ControlPointBottomLeft,      ///< 左下控制点
        ControlPointLeftMid,         ///< 左面中间控制点
        ControlLineLeft,             ///< 左边控制线
        ControlLineTop,              ///< 顶部控制线
        ControlLineRight,            ///< 右边控制线
        ControlLineBottom            ///< 底部控制线
    };
    Q_ENUM(ControlType)
public:
    DAGraphicsResizeableItem(QGraphicsItem* parent = nullptr);
    virtual ~DAGraphicsResizeableItem();
    /**
     * @brief 适用qgraphicsitem_cast
     */
    enum
    {
        Type = DA::ItemType_DAGraphicsResizeableItem
    };
    int type() const override
    {
        return (Type);
    }

public:
    // QGraphicsItem需要继承接口
    QRectF boundingRect() const override;
    //
    QPainterPath shape() const override;
    //
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    // 保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement,const QVersionNumber& ver) const override;
    virtual bool loadFromXml(const QDomElement* itemElement,const QVersionNumber& ver) override;

public:  // 尺寸相关接口
    // 测试一下setBodySize之后getBodySize能得到的尺寸
    QSizeF testBodySize(const QSizeF& s);
    // 设置尺寸这里的尺寸是不包括旋转和缩放的辅助控制，如果构造函数中需要设置默认大小，使用changeBodySize
    virtual void setBodySize(const QSizeF& s);
    // 绘图的shape，等同于原来的QGraphicsItem::boundingRect
    QRectF getBodyRect() const;
    // 获取尺寸，
    QSizeF getBodySize() const;
    // 获取body包含控制窗口大小，就是在改变尺寸时包含那8个控制点的最大尺寸
    QRectF getBodyControlRect() const;
    // 设置最大最小范围
    void setBodyMinimumSize(const QSizeF& s);
    void setBodyMaximumSize(const QSizeF& s);
    QSizeF getBodyMinimumSize() const;
    QSizeF getBodyMaximumSize() const;
    // 设置控制器的大小
    void setControlerSize(const QSizeF& s);
    QSizeF getControlerSize() const;
    // 是否允许
    void setEnableResize(bool on);
    bool isResizable() const;
    // 设置body的位置
    void setBodyPos(const QPointF& p);
    void setBodyScenePos(const QPointF& p);
    // 返回body中心点
    QPointF getBodyCenterPoint() const;
    // 获取body中心的位置
    QPointF getBodyCenterPos() const;
    // 设置body中心到scene的位置
    void setBodyCenterPos(const QPointF& p);
    // 设置TransformOriginPoint自动设置为bodysize的中心,否则为用户自己指定
    void setAutoCenterTransformOriginPoint(bool on = true);
    // 更新TransformOriginPoint，前提是setAutoCenterTransformOriginPoint(true)
    void updateTransformOriginPoint();
    // 执行变换，返回需要移动的位置和尺寸，尺寸会考虑最大最小
    QPair< QPointF, QSizeF > doItemResize(const QPointF& mousescenePos);

public:
    // 需要用户继承的接口
    // 绘图的shape，等同于原来的QGraphicsItem::shape
    virtual QPainterPath getBodyShape() const;
    // 绘制resize边框
    virtual void paintSelectedBorder(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    // 绘制resize控制点
    virtual void paintResizeControlPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    // 绘制背景
    virtual void paintBackground(QPainter* painter,
                                 const QStyleOptionGraphicsItem* option,
                                 QWidget* widget,
                                 const QRectF& bodyRect);
    // 绘制边框
    virtual void paintBorder(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect);
    // 绘制具体内容
    virtual void paintBody(QPainter* painter,
                           const QStyleOptionGraphicsItem* option,
                           QWidget* widget,
                           const QRectF& bodyRect) = 0;
    // 生成controlPoints
    virtual QRectF controlPointRect(ControlType tp, const QRectF& bodyRect) const;
    // 控制点的大小
    virtual QSizeF controlPointSize() const;
    // 在尺寸变化时调用，例如prepareGeometryChange之后调用
    void prepareControlInfoChange();
    // 检测点在哪个控制点上
    ControlType getControlPointByPos(const QPointF& pos) const;
    // 判断是否在调整大小中
    bool isResizing() const;

public:
    // 下面四个函数是通过scene获取，如果没有scene，返回默认值
    // 是否允许对齐网格
    bool isSnapToGrid() const;
    // 设置网格尺寸
    QSize getGridSize() const;

protected:
    // 直接改变bodysize
    void changeBodySize(const QSizeF& s);
#if DA_USE_QGRAPHICSOBJECT
signals:
    /**
     * @brief itemBodySizeChanged
     * @param oldsize
     * @param newsize
     */
    void itemBodySizeChanged(const QSizeF& oldsize, const QSizeF& newsize);
#endif
protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;
};
}
Q_GLOBAL_STATIC(DA::DAGraphicsResizeableItemPalette, daGlobalGraphicsResizeableItemPalette)
#endif  // DAGRAPHICSRESIZEABLEITEM_H
