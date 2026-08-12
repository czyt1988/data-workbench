#ifndef DAABSTRACTGRAPHICSVIEWOVERLAY_H
#define DAABSTRACTGRAPHICSVIEWOVERLAY_H
#include "DAGraphicsViewGlobal.h"
#include "DAAbstractWidgetOverlay.h"
#include <QPointer>
class QMouseEvent;
class QGraphicsView;
class QGraphicsScene;
namespace DA
{
/**
 * @brief 提供给DAGraphicsView的Overlay
 *
 * @note 由于Overlay是对鼠标隐藏的，因此不能直接使用mouseEvent，直接捕获parent的event
 */
class DAGRAPHICSVIEW_API DAAbstractGraphicsViewOverlay : public DAAbstractWidgetOverlay
{
public:
	explicit DAAbstractGraphicsViewOverlay(QGraphicsView* parent);
	~DAAbstractGraphicsViewOverlay();
	QRect overlayRect() const;
	//
	QPoint getMousePos() const;
	// 是否激活
	bool isActive() const;

	// 激活
	void setActive(bool v);

	// 对父窗口进行过滤
	virtual bool eventFilter(QObject* obj, QEvent* event) override;

	// 获取view
	QGraphicsView* view() const;

	// 判断是否有效，如果view没有场景，就属于无效
	bool isValid() const;

protected:
	// view的鼠标移动事件
	virtual void viewMouseMove(const QPoint& viewPos);

	// view的鼠标点击事件
	virtual void viewMousePress(const QPoint& viewPos);

	// view的鼠标释放事件
	virtual void viewMouseRelease(const QPoint& viewPos);

private:
	bool tryInstall();

private:
	bool mIsActive { true };
	QPoint mMousePos;
	QWidget* mViewPort { nullptr };
	bool mIsInstalled { false };
};
}
#endif  // DAABSTRACTGRAPHICSVIEWOVERLAY_H
