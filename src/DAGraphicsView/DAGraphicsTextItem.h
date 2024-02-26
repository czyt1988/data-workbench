#ifndef DAGRAPHICSTEXTITEM_H
#define DAGRAPHICSTEXTITEM_H
#include <QGraphicsTextItem>
#include "DAGraphicsResizeableItem.h"
namespace DA
{
class DAGraphicsStandardTextItem;
/**
 * @brief 支持缩放编辑的文本框Item
 */
class DAGRAPHICSVIEW_API DAGraphicsTextItem : public DAGraphicsResizeableItem
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAGraphicsTextItem)
public:
    /**
     * @brief 适用qgraphicsitem_cast
     */
    enum
    {
        Type = DA::ItemType_DAGraphicsTextItem
    };
    int type() const override
    {
        return (Type);
    }

public:
    DAGraphicsTextItem(QGraphicsItem* parent = nullptr);
    DAGraphicsTextItem(const QFont& f, QGraphicsItem* parent = nullptr);
    DAGraphicsTextItem(const QString& str, const QFont& f, QGraphicsItem* parent = nullptr);
    ~DAGraphicsTextItem();

    // 设置编辑模式
    void setEditMode(bool on = true);
    // 保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement) const override;
    virtual bool loadFromXml(const QDomElement* itemElement) override;

    // 获取内部的文本item
    DAGraphicsStandardTextItem* textItem() const;
    // 设置尺寸这里的尺寸是不包括旋转和缩放的辅助控制，如果构造函数中需要设置默认大小，使用changeBodySize
    void setBodySize(const QSizeF& s) override;

    // 文本
    void setText(const QString& v);
    QString getText() const;

    // 字体
    void setFont(const QFont& v);
    QFont getFont() const;

    // 设置编辑模式
    void setEditable(bool on = true);
    bool isEditable() const;

    // 设置是否开启相对定位
    void setEnableRelativePosition(bool on);
    bool isEnableRelativePosition() const;

    // 自动调整大小
    void setAutoAdjustSize(bool on);
    bool isAutoAdjustSize() const;

    // 设置相对父窗口的相对定位
    void setRelativePosition(qreal xp, qreal yp);
    QPointF getRelativePosition() const;

    // 设置对齐的锚点,RelativePosition=(0,0)，那么就是描点之间的对齐
    void setRelativeAnchorPoint(ShapeKeyPoint kParentAnchorPoint, ShapeKeyPoint thisAnchorPoint);
    ShapeKeyPoint getParentRelativeAnchorPoint() const;
    ShapeKeyPoint getItemRelativeAnchorPoint() const;

    // 更新相对位置
    void updateRelativePosition();
    void updateRelativePosition(const QRectF& parentRect, const QRectF& itemRect);

protected:
    // 绘制具体内容
    virtual void paintBody(QPainter* painter,
                           const QStyleOptionGraphicsItem* option,
                           QWidget* widget,
                           const QRectF& bodyRect) override;

private:
    void init();
};
}
#endif  // DAGRAPHICSTEXTITEM_H
