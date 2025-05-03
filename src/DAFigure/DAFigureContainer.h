#ifndef DAFIGURECONTAINER_H
#define DAFIGURECONTAINER_H
#include "DAFigureAPI.h"
#include <QWidget>
#include <QMap>
#include <QRectF>
class QResizeEvent;
class QShowEvent;
class QMoveEvent;
namespace DA
{
/**
 * @class DAFigureContainer
 * @brief 基于比例布局的窗口容器
 *
 * 该容器管理子窗口的相对位置，当容器尺寸改变时自动调整子窗口布局。
 * 支持通过百分比设置子窗口位置，并实时跟踪子窗口的位置变化。
 */
class DAFIGURE_API DAFigureContainer : public QWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAFigureContainer)
public:
	DAFigureContainer(QWidget* parent = nullptr);
	~DAFigureContainer();

	// 添加窗体
	void addWidget(QWidget* widget, const QRectF& posPercent);
	void addWidget(QWidget* widget, float xPercent, float yPercent, float wPercent, float hPercent);
	// 移除widget，但不会删除
	void removeWidget(QWidget* widget);

	// 获取所有被管理的窗口
	QList< QWidget* > getWidgetList() const;
	// 获取一个有序的窗口列表，这个顺序是在窗口显示最顶层的排位最前
	QList< QWidget* > getOrderedWidgetList() const;
	// 获取窗口的位置信息
	QRectF getWidgetPosPercent(QWidget* w) const;
	void setWidgetPosPercent(QWidget* w, const QRectF& posPercent);
	void setWidgetPosPercent(QWidget* w, float xPercent, float yPercent, float wPercent, float hPercent);

	// 判断是否存在这个widget
	bool isWidgetInContainer(const QWidget* w) const;
	// 判断是否是相对位置
	bool isWidgetRelativePos(QWidget* w) const;

	// 根据baserect，计算目标rect相对baserect的占比
	static QRect calcGeometryByPercent(const QRectF& percent, const QRect& continerRect);
	// 通过窗口Percent计算窗口尺寸
	QRect calcWidgetRectByPercent(const QRectF& percent) const;
	static QRect calcWidgetRectByPercent(DAFigureContainer* c, const QRectF& percent);

	// 根据baserect，计算目标rect相对baserect的占比
	static QRectF calcRectPosPercentByOtherRect(const QRect& baseRect, const QRect& rect);
	// 计算size对应SAFigureContainer的位置占比
	QRectF calcPercentByGeometryRect(const QRect& s) const;
	static QRectF calcPercentByGeometryRect(DAFigureContainer* c, const QRect& s);

	// 设置浮点数的精度，默认为3位
	static qreal castRealByPrecision(qreal v, int precision = 3);

	void beginResetSubWidget();
	void endResetSubWidget();
	// 获取当前位置下的窗口
	QWidget* getWidgetUnderPos(const QPoint& p, bool zorder = true);

protected:
	//
	void resizeEvent(QResizeEvent* e) override;
	//
	bool event(QEvent* e) override;
	//
	bool eventFilter(QObject* watched, QEvent* e) override;
	// 重置子窗口的Geometry
	void resetSubWidgetGeometry();

private:
	QRect calcWidgetSize(const QRectF& present);

private:
	class WidgetSizeData;
};
}  // End Of Namespace DA
#endif  // DAFIGURECONTAINER_H
