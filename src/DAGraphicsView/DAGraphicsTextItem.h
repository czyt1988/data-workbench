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
    // 设置编辑模式
    void setEditMode(bool on = true);
    // 保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement) const override;
    virtual bool loadFromXml(const QDomElement* itemElement) override;

    // 获取内部的文本item
    DAGraphicsStandardTextItem* textItem() const;
    // 设置尺寸这里的尺寸是不包括旋转和缩放的辅助控制，如果构造函数中需要设置默认大小，使用changeBodySize
    void setBodySize(const QSizeF& s) override;

    /**
     * @brief 设置文本
     * @param v
     */
    void setText(const QString& v);

    /**
     * @brief 文本
     * @return
     */
    QString getText() const;

    /**
     * @brief 设置相对父窗口的相对定位
     * @param xp x位置相对父级item的宽度占比，如parentCorner=Qt::TopLeftCorner,xp=0.2
     * 那么textitem的x定位设置为parentItem.boundingRect().topLeft().x() + parentItem.width() * xp
     * @param yp y位置相对父级item的宽度占比，如parentCorner=Qt::TopLeftCorner,yp=0.1
     * 那么textitem的y定位设置为parentItem.boundingRect().topLeft().y() + parentItem.width() * yp
     * @param parentCorner 相对定位点
     */
    void setRelativePosition(qreal xp, qreal yp, Qt::Corner parentCorner);

    /**
     * @brief 设置是否开启相对定位
     * @note 相对定位需要有父级item，否则无效
     * @param on
     */
    void setEnableRelativePosition(bool on);

    /**
     * @brief 是否开启相对定位
     * @return
     */
    bool isEnableRelativePosition() const;

    /**
     * @brief 更新相对位置
     */
    void updateRelativePosition();

protected:
    void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;
};
}
#endif  // DAGRAPHICSTEXTITEM_H
