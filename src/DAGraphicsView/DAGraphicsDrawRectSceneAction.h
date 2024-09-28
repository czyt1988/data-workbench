#ifndef DAGRAPHICSDRAWRECTSCENEACTION_H
#define DAGRAPHICSDRAWRECTSCENEACTION_H
#include "DAGraphicsViewGlobal.h"
#include "DAAbstractGraphicsSceneAction.h"
namespace DA
{
class DAGraphicsRubberBandItem;
/**
 * @brief 绘制矩形action
 */
class DAGRAPHICSVIEW_API DAGraphicsDrawRectSceneAction : public DAAbstractGraphicsSceneAction
{
public:
	DAGraphicsDrawRectSceneAction(DAGraphicsScene* sc);
	virtual ~DAGraphicsDrawRectSceneAction();

protected:
	// 开始激活，这是使用setAction后调用的函数
	virtual void beginActive();
	// 鼠标点击事件,返回true，代表action劫持了此事件，不会在scene中继续传递事件,默认返回false
	virtual bool mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent);
    // 鼠标释放
    virtual bool mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent);
	// 鼠标移动事件,返回true，代表action劫持了此事件，不会在scene中继续传递事件,默认返回false
	virtual bool mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent);

private:
	QPointF mStartPoint;
    bool mIsStarted { false };
	std::unique_ptr< DAGraphicsRubberBandItem > mRubberBand;
};
}  // end namespace DA
#endif  // DAGRAPHICSDRAWRECTSCENEACTION_H
