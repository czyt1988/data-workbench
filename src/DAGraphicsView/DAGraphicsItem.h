#ifndef DAGRAPHICSITEM_H
#define DAGRAPHICSITEM_H
#include <QGraphicsObject>
#include "DAUtils/DAXMLFileInterface.h"
#include "DAGraphicsViewGlobal.h"
class QDomDocument;
class QDomElement;
namespace DA
{
/**
 * @brief DAGraphicsView的基本元件
 *
 * DAGraphicsItem提供了统一的saveToXml接口
 * 加载的过程通过DAGraphicsItemFactory进行加载
 */
class DAGRAPHICSVIEW_API DAGraphicsItem : public QGraphicsObject, public DAXMLFileInterface
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAGraphicsItem)
public:
    /**
     * @brief 适用qgraphicsitem_cast
     */
    enum
    {
        Type = DA::ItemType_GraphicsItem
    };
    virtual int type() const override
    {
        return (Type);
    }

public:
    DAGraphicsItem(QGraphicsItem* parent = nullptr);
    ~DAGraphicsItem();
    // 保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement) const override;
    virtual bool loadFromXml(const QDomElement* parentElement) override;
    // 设置边框画笔，如果设置一个QPen,则不绘制边框
    void setBorderPen(const QPen& p);
    QPen getBorderPen() const;
    // 设置是否显示边框
    void setShowBorder(bool on);
    bool isShowBorder() const;
    // 背景
    void setBackgroundBrush(const QBrush& b);
    QBrush getBackgroundBrush() const;
    // 设置是否显示背景
    void setShowBackground(bool on);
    bool isShowBackground() const;
    //    //分组位置发生改变的事件
    //    virtual void groupPositionChanged();
protected:
    virtual bool sceneEvent(QEvent* event) override;
};

}

#endif  // DAGRAPHICSITEM_H
