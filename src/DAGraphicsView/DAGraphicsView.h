#ifndef DAGRAPHICSVIEW_H
#define DAGRAPHICSVIEW_H
#include <QGraphicsView>
#include "DAGraphicsViewGlobal.h"
#include "DAGraphicsItem.h"
class QWheelEvent;
namespace DA
{
/**
 * @brief 支持缩放和拖动的GraphicsView
 *
 * 快捷键说明：
 * Ctrl++ 放大
 * Ctrl+- 缩小
 *
 */
class DAGRAPHICSVIEW_API DAGraphicsView : public QGraphicsView
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAGraphicsView)
public:
	/**
	 * @brief 缩放设置
	 */
	enum ZoomFlag
	{
		ZoomNotUseWheel     = 0x0001,  ///< 不进行滚轮缩放
		ZoomUseWheel        = 0x0002,  ///< 直接用wheel就可以
		ZoomUseWheelAndCtrl = 0x0004   ///< 需要使用ctrl键
	};
	Q_DECLARE_FLAGS(ZoomFlags, ZoomFlag)
	Q_FLAG(ZoomFlag)

	/**
	 * @brief 表征拖动状态
	 */
	enum PadFlag
	{
		PadDiable              = 0x0001,  ///< 不允许拖动
		PadByWheelMiddleButton = 0x0002   ///< 通过中键移动
	};
	Q_DECLARE_FLAGS(PadFlags, PadFlag)
	Q_FLAG(PadFlag)
public:
	DAGraphicsView(QWidget* parent = 0);
	DAGraphicsView(QGraphicsScene* scene, QWidget* parent = 0);
	~DAGraphicsView();
	// 设置缩放的因子
	void setScaleRange(qreal min, qreal max);
	// 获取缩放因子
	qreal getScaleMaxFactor() const;
	qreal getScaleMinFactor() const;

	// 设置是否可以滚轮缩放
	bool isEnaleWheelZoom() const;
	void setEnaleWheelZoom(bool enaleWheelZoom = true, ZoomFlags zf = ZoomUseWheelAndCtrl);

	// 获取鼠标对应的scence的位置坐标
	QPointF getMouseScenePos() const;

	// 设置缩放flag
	void setZoomFrags(ZoomFlags zf);
	ZoomFlags getZoomFlags() const;

	// 判断是否在拖动
	bool isPadding() const;
	// 设置拖动属性
	void setPaddingFrags(PadFlags pf);
	PadFlags getPaddingFrags() const;
	// 选中的item
	QList< DAGraphicsItem* > selectedDAItems() const;

public slots:
	// 放大
	void zoomIn();
	// 缩小
	void zoomOut();
	// 设置最适合视图尺寸
	void setWholeView();
	// 选中所有可选的item
	void selectAll();
    // 取消所有选中
    void clearSelection();

protected:
	// void paintEvent(QPaintEvent * event);
	void wheelEvent(QWheelEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

protected:
	void wheelZoom(QWheelEvent* event);
	// 开始拖动
	void startPad(QMouseEvent* event);
	// 结束拖动
	void endPad();

private:
	void init();

private:
};
}
#endif  // GGRAPHICSVIEW_H
