#ifndef DAGRAPHICSTEXTITEM_H
#define DAGRAPHICSTEXTITEM_H
#include <QGraphicsTextItem>
#include "DAGraphicsResizeableItem.h"
#include <QTextCursor>
class QTextDocument;
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

	// 保存到xml中
	virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const override;
	virtual bool loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver) override;

	// 获取内部的文本item
	DAGraphicsStandardTextItem* textItem() const;
	// 设置尺寸这里的尺寸是不包括旋转和缩放的辅助控制，如果构造函数中需要设置默认大小，使用changeBodySize
	void setBodySize(const QSizeF& s) override;

	// 文本
	void setPlainText(const QString& v);
	QString getPlainText() const;

	// 文本颜色
	void setSelectTextColor(const QColor& v);
	QColor getSelectTextColor() const;

	// 字体
	void setSelectTextFont(const QFont& v);
	QFont getSelectTextFont() const;

	// 设置选中文本字体，如果没选中，将设置全部
	void setSelectTextFamily(const QString& v);
	QString getSelectTextFamily() const;

	// 设置选中文本的字体大小
	void setSelectTextPointSize(int v);
	int getSelectTextPointSize() const;

	// 文字斜体
	void setSelectTextItalic(bool on);
	bool getSelectTextItalic() const;

	// 文字粗体
	void setSelectTextBold(bool on);
	bool getSelectTextBold() const;

	// 设置编辑模式
	void setEditable(bool on = true);
	bool isEditable() const;

	// 获取doc
	QTextDocument* document() const;
	// textCursor
	QTextCursor textCursor() const;

	// 转换为富文本
	QString toHtml() const;
	void setHtml(const QString& html);

protected:
	// 绘制具体内容
	virtual void paintBody(QPainter* painter,
						   const QStyleOptionGraphicsItem* option,
						   QWidget* widget,
						   const QRectF& bodyRect) override;
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* e) override;
	//
	virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;

private:
	void init(const QString& initText);
};
}
#endif  // DAGRAPHICSTEXTITEM_H
