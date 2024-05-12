#ifndef DASTANDARDNODETEXTGRAPHICSITEM_H
#define DASTANDARDNODETEXTGRAPHICSITEM_H
#include "DAWorkFlowGlobal.h"
#include "DAAbstractNodeGraphicsItem.h"
namespace DA
{
class DAGraphicsStandardTextItem;
/**
 * @brief 标准的可resize的GraphicsTextItem,可以作为需要输入文本内容的节点显示
 *
 */
class DAWORKFLOW_API DAStandardNodeTextGraphicsItem : public DAAbstractNodeGraphicsItem
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAStandardNodeTextGraphicsItem)
public:
	/**
	 * @brief 适用qgraphicsitem_cast
	 */
	enum
	{
		Type = DA::ItemType_GraphicsStandardTextItem
	};
	int type() const override
	{
		return (Type);
	}

public:
	DAStandardNodeTextGraphicsItem(DAAbstractNode* n, QGraphicsItem* p = nullptr);
	~DAStandardNodeTextGraphicsItem();
	// 设置编辑模式
	// 设置编辑模式
	void setEditable(bool on = true);
	bool isEditable() const;
	// 自动调整大小
	void setAutoAdjustSize(bool on);
	bool isAutoAdjustSize() const;
	// 文本
	void setText(const QString& v);
	QString getText() const;
	// 字体
	void setFont(const QFont& v);
	QFont getFont() const;
	// 设置尺寸这里的尺寸是不包括旋转和缩放的辅助控制，如果构造函数中需要设置默认大小，使用changeBodySize
	void setBodySize(const QSizeF& s) override;
	// 获取内部的文本item
	DAGraphicsStandardTextItem* textItem() const;
};
}  // end DA
#endif  // DASTANDARDNODETEXTGRAPHICSITEM_H
