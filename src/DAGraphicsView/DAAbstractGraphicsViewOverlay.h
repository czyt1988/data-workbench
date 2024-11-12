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
	/**
	 * @brief 是否激活
	 * @return
	 */
	bool isActive() const;

	/**
	 * @brief 激活
	 * @param v
	 */
	void setActive(bool v);

	/**
	 * @brief 对父窗口进行过滤
	 *
	 * overlay已经默认捕获父窗口，不需要手动再次安装
	 */
	virtual bool eventFilter(QObject* obj, QEvent* event);

    /**
     * @brief 获取view
     *  
     * @return 如果没有返回nullptr
     */
    QGraphicsView* view() const;

    /**
     * @brief 判断是否有效，如果view没有场景，就属于无效
     */
    bool isValid() const;
protected:
	/**
	 * @brief view的鼠标移动事件
	 *
	 * 类似widget的鼠标事件，但由于overlay是透明窗口，本身是没有鼠标事件，这里是捕获了view的鼠标事件，
	 * 方便基于宿主的状态进行特殊的显示
	 *
	 * @note 注意，在继承此虚函数，要保证父类的虚函数运行，否则@ref DAAbstractGraphicsViewOverlay::getMousePos 将不起作用
	 */
	virtual void viewMouseMove(const QPoint& viewPos,const QPointF& secnePos);

	/**
	 * @brief view的鼠标点击事件
	 *
	 * 类似widget的鼠标事件，但由于overlay是透明窗口，本身是没有鼠标事件，这里是捕获了view的鼠标事件，
	 * 方便基于宿主的状态进行特殊的显示
	 *
	 * @note 注意，在继承此虚函数，要保证父类的虚函数运行，否则@ref DAAbstractGraphicsViewOverlay::getMousePos 将不起作用
	 */
	virtual void viewMousePress(const QPoint& viewPos,const QPointF& secnePos);

	/**
	 * @brief view的鼠标释放事件
	 *
	 * 类似widget的鼠标事件，但由于overlay是透明窗口，本身是没有鼠标事件，这里是捕获了view的鼠标事件，
	 * 方便基于宿主的状态进行特殊的显示
	 *
	 * @note 注意，在继承此虚函数，要保证父类的虚函数运行，否则@ref DAAbstractGraphicsViewOverlay::getMousePos 将不起作用
	 */
	virtual void viewMouseRelease(const QPoint& viewPos,const QPointF& secnePos);
private:
    bool tryInstall();
private:
	bool mIsActive { true };
	QPoint mMousePos;
    QWidget* mViewPort{nullptr};
    QPointer<QGraphicsScene> mScene{ nullptr };
    bool mIsInstalled{ false };
};
}
#endif  // DAABSTRACTGRAPHICSVIEWOVERLAY_H
