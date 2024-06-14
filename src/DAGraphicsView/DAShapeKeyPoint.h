#ifndef DASHAPEKEYPOINT_H
#define DASHAPEKEYPOINT_H
#include <QRect>
#include "DAGraphicsViewGlobal.h"
namespace DA
{
/**
 * @brief 定义一个形状的9个关键位置点
 * @details
 * 	 * ```
 * 1---2---3
 * |       |
 * 4---5---6
 * |       |
 * 7---8---9
 * ```
 */
class DAGRAPHICSVIEW_API DAShapeKeyPoint
{
public:
	enum KeyPoint
	{
		TopLeft = 0,
		TopCenter,
		TopRight,
		CenterLeft,
		Center,
		CenterRight,
		BottomLeft,
		BottomCenter,
		BottomRight,
		None = 128
	};
	DAShapeKeyPoint(KeyPoint kp = KeyPoint::Center);
	~DAShapeKeyPoint();
	// 是否有效
	bool isValid() const;
	// 关键点在矩形的绝对
	QPoint rectKeyPoint(const QRect& r) const;
	QPointF rectKeyPoint(const QRectF& r) const;
	static QPoint rectKeyPoint(const QRect& r, const DAShapeKeyPoint& kp);
	static QPointF rectKeyPoint(const QRectF& r, const DAShapeKeyPoint& kp);
	// 值
	KeyPoint value() const;
	// 等号操作符
	DAShapeKeyPoint& operator=(KeyPoint kp);
	bool operator==(KeyPoint kp) const;

private:
	KeyPoint mKeyPoint { KeyPoint::None };
};
// ShapeKeyPoint 的枚举转换
DAGRAPHICSVIEW_API QString enumToString(DAShapeKeyPoint e);
// ShapeKeyPoint 的枚举转换
DAGRAPHICSVIEW_API DAShapeKeyPoint stringToEnum(const QString& s, DAShapeKeyPoint defaultEnum = DAShapeKeyPoint::Center);
}  // end DA
#endif  // DASHAPEKEYPOINT_H
