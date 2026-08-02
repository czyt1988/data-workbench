#ifndef DAGRAPHICSRESIZEABLEITEM_H
#define DAGRAPHICSRESIZEABLEITEM_H
#include "DAGraphicsItem.h"
#include "DAIResizableGraphicsItem.h"
#include <QColor>
#include <QBrush>
#include "DAGraphicsViewGlobal.h"
class QDomDocument;
class QDomElement;
namespace DA
{

/**
 * @brief 用于调整大小的item
 *
 * 此类保留为 body 尺寸管理基类，控制点绘制和鼠标交互已移至 DAGraphicsResizeOverlayItem（Overlay 模式）。
 *
 * 关键函数：
 * 1、@sa paintBody 这个函数必须继承，用于绘制内容
 * 2、@sa setBodySize 此函数的重载可以控制绘图区域尺寸，如图片等比例缩放就可以通过此函数控制
 */
class DAGRAPHICSVIEW_API DAGraphicsResizeableItem : public DAGraphicsItem, public DAIResizableGraphicsItem
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAGraphicsResizeableItem)
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

public:  // QGraphicsItem 接口
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    // 保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const override;
    virtual bool loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver) override;

public:  // 尺寸相关接口 — DAIResizableGraphicsItem 实现
    // 设置尺寸，尺寸不包括旋转和缩放的辅助控制，如果构造函数中需要设置默认大小，使用changeBodySize
    virtual void setBodySize(const QSizeF& s) override;
    // 获取尺寸
    QSizeF getBodySize() const override;
    // 绘图的区域（item 坐标系）
    QRectF getBodyRect() const override;
    // 设置最大最小范围
    void setBodyMinimumSize(const QSizeF& s);
    void setBodyMaximumSize(const QSizeF& s);
    QSizeF getBodyMinimumSize() const override;
    QSizeF getBodyMaximumSize() const override;
    // 是否允许缩放
    bool isResizable() const override;

public:  // 其他尺寸管理方法
    // 获取body控制矩形，已废弃，等价于 getBodyRect()
    Q_DECL_DEPRECATED_X("Use getBodyRect() instead") QRectF getBodyControlRect() const;
    // 设置控制器的大小（已废弃，控制点由 Overlay 管理）
    Q_DECL_DEPRECATED_X("Use getBodyRect() instead") void setControlerSize(const QSizeF& s);
    Q_DECL_DEPRECATED_X("Use getBodyRect() instead") QSizeF getControlerSize() const;
    // 是否允许缩放
    void setEnableResize(bool on);
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
    // 获取绘图起始位置（不在 DAIResizableGraphicsItem 接口中）
    QPointF getBodyPainterStartPos() const;

public:  // DAIResizableGraphicsItem 接口实现
    QPointF getBodyTransformOriginPoint() const override;
    QGraphicsItem* graphicsItem() override;
    const QGraphicsItem* graphicsItem() const override;

public:  // 需要用户继承的接口
    // 绘图的shape，等同于原来的QGraphicsItem::shape
    virtual QPainterPath getBodyShape() const;
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
    // 控制点的大小（非虚，仅返回 mControlPointSize）
    QSizeF controlPointSize() const;

public:
    // 下面两个函数是通过scene获取，如果没有scene，返回默认值
    // 是否允许对齐网格
    bool isSnapToGrid() const;
    // 设置网格尺寸
    QSize getGridSize() const;

protected:
    // 直接改变bodysize
    void changeBodySize(const QSizeF& s);
    // 测试尺寸是否在最大最小范围内，返回修正后的尺寸（仅供子类使用）
    QSizeF testBodySize(const QSizeF& s) const;
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;
};
}  // namespace DA
#endif  // DAGRAPHICSRESIZEABLEITEM_H
