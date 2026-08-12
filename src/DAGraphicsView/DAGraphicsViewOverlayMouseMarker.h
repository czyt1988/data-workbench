#ifndef DAGRAPHICSVIEWOVERLAYMOUSEMARKER_H
#define DAGRAPHICSVIEWOVERLAYMOUSEMARKER_H
#include "DAGraphicsViewGlobal.h"
#include "DAAbstractGraphicsViewOverlay.h"
#include <QPen>
class QGraphicsView;
namespace DA
{

/**
 * @brief 用于显示十字线的遮罩窗口
 *
 * 这个是为了在graphicsView上显示一些内容，但又不想重绘graphicsView的paintevent函数，尽可能的减少graphicsView的绘图事件而建立的窗口。
 *
 * 这个窗口依赖@ref DA::DAAbstractWidgetOverlay
 *
 */
class DAGRAPHICSVIEW_API DAGraphicsViewOverlayMouseMarker : public DAAbstractGraphicsViewOverlay
{
public:
	/**
	 * @brief 标记样式
	 */
	enum MarkerStyle
	{
		HLine,      ///< 水平线
		VLine,      ///< 垂直线
		CrossLine,  ///< 十字线
		NoMarkerStyle
	};

public:
	explicit DAGraphicsViewOverlayMouseMarker(QGraphicsView* parent);
	~DAGraphicsViewOverlayMouseMarker();
	// 画笔
	QPen getDrawPen() const;

	// 设置画笔
	void setDrawPen(const QPen& v);

	// 获取标记样式
	MarkerStyle getMarkerStyle() const;

	// 设置标记样式
	void setMarkerStyle(MarkerStyle v);

protected:
	virtual void drawOverlay(QPainter* painter) const override;
	virtual QRegion maskHint() const override;

private:
	MarkerStyle mMarkerStyle { CrossLine };  ///< 标记样式
	QPen mDrawPen { Qt::black };             ///< 绘制画笔
};
}  // end ns da

#endif  // DAGRAPHICSVIEWOVERLAYMOUSECROSSLINE_H
