#ifndef DASTANDARDNODEINPUTOUTPUTGRAPHICSITEM_H
#define DASTANDARDNODEINPUTOUTPUTGRAPHICSITEM_H
#include "DAAbstractNodeGraphicsItem.h"
#include <functional>
#include <QPixmap>
namespace DA
{
class DAWORKFLOW_API DAStandardNodeInputOutputGraphicsItem : public DAAbstractNodeGraphicsItem
{
public:
	DAStandardNodeInputOutputGraphicsItem(DA::DAAbstractNode* n, QGraphicsItem* p = nullptr);
	~DAStandardNodeInputOutputGraphicsItem();
	// 计算尺寸，根据输入输出进行尺寸计算
	QSize calcSize() const;
	// 绘制具体内容
	virtual void paintBody(QPainter* painter,
						   const QStyleOptionGraphicsItem* option,
						   QWidget* widget,
						   const QRectF& bodyRect) override;
	// 获取连接点尺寸
	QSize getLinkpointSize() const;
	// 节点名字改变的回调
	virtual void nodeDisplayNameChanged(const QString& name);
	// 允许一个节点多次链接
	void setEnableMultLink(bool on);
	bool isEnableMultLink() const;
	// 设置在中间显示icon，icon以node的icon来显示
	void setEnableShowIcon(bool on);
	bool isEnableShowIcon() const;
	// 设置icon的尺寸
	void setIconSize(const QSize& s);
	QSize getIconSize() const;

protected:
	// 绘制显示名字
	void paintDisplayName(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QString& name);
	void paintIcon(QPainter* painter,
				   const QStyleOptionGraphicsItem* option,
				   QWidget* widget,
				   const QRectF& bodyRect,
				   const QPixmap& pixmap);
	// 绘制图标
	QPixmap generateIcon();

private:
	QString mDisplayName;
	bool mEnableShowIcon { true };
	QSize mIconSize { 20, 20 };
	QPixmap mIconPixmap;
};
}
#endif  // DASTANDARDNODEINPUTOUTPUTGRAPHICSITEM_H
