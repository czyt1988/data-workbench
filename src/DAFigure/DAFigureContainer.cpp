#include "DAFigureContainer.h"
#include <math.h>
#include <QResizeEvent>
#include <QDebug>
#include <QChildEvent>
#include <QSet>

/**
 * @def 定义精度
 */
#define DAFIGURECONTAINER_PRECISION 5

namespace DA
{
//===============================================================
// DAFigureContainer::WidgetSizeData
//===============================================================
/**
 * @brief 用来保存尺寸数据
 */
class DAFigureContainer::WidgetSizeData
{
public:
	WidgetSizeData();
	WidgetSizeData(const QRectF& s, bool isrel);
	bool isValid() const;
	QRectF sizeData;               ///< 尺寸数据，有可能是绝对尺寸位置，有可能是相对尺寸位置
	bool isRelativePos { false };  ///< 相对位置标记，此参数为true，说明sizeData保存的是相对位置
};

DAFigureContainer::WidgetSizeData::WidgetSizeData()
{
}

DAFigureContainer::WidgetSizeData::WidgetSizeData(const QRectF& s, bool isrel) : sizeData(s), isRelativePos(isrel)
{
}

bool DAFigureContainer::WidgetSizeData::isValid() const
{
	return sizeData.isValid();
}

//===============================================================
// DAFigureContainer::PrivateData
//===============================================================

class DAFigureContainer::PrivateData
{
	DA_DECLARE_PUBLIC(DAFigureContainer)
public:
	// 使用using替代部分冗长类型
	using WidgetMap = QMap< QWidget*, DAFigureContainer::WidgetSizeData >;
	// TODO ,这里需要定义一个类，这个类包含QRectF，和是否是相对位置或绝对位置
	WidgetMap managedWidgets;
	bool isUpdatingLayout { false };
	explicit PrivateData(DAFigureContainer* p) : q_ptr(p)
	{
	}
};
//===============================================================
// DAFigureContainer
//===============================================================
DAFigureContainer::DAFigureContainer(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
}

DAFigureContainer::~DAFigureContainer()
{
    // qDebug() <<"SAFigureContainer destroy";
}

/**
 * @brief 添加窗体
 * @param widget
 * @param posPercent
 */
void DAFigureContainer::addWidget(QWidget* widget, const QRectF& posPercent)
{
	if (widget->parentWidget() != this) {
		widget->setParent(this);
	}
	QRect widgetSize = calcWidgetSize(posPercent);
	widget->setGeometry(widgetSize);
	DAFigureContainer::WidgetSizeData sd(posPercent, true);
	d_ptr->managedWidgets.insert(widget, sd);
	widget->installEventFilter(this);
	// qDebug() << "DAFigureContainer::addWidget,Widget setGeometry=" << widgetSize << ",DAFigureContainer rect=" << rect();
}

void DAFigureContainer::addWidget(QWidget* widget, float xPercent, float yPercent, float wPercent, float hPercent)
{
	addWidget(widget,
			  QRectF(castRealByPrecision(xPercent, DAFIGURECONTAINER_PRECISION),
					 castRealByPrecision(yPercent, DAFIGURECONTAINER_PRECISION),
					 castRealByPrecision(wPercent, DAFIGURECONTAINER_PRECISION),
					 castRealByPrecision(hPercent, DAFIGURECONTAINER_PRECISION)));
}

/**
 * @brief 移除窗口
 * @param widget 窗口不会删除
 */
void DAFigureContainer::removeWidget(QWidget* widget)
{
	d_ptr->managedWidgets.remove(widget);
	if (widget->parent() == this) {
		widget->setParent(nullptr);
	}
}

/**
 * @brief 获取所有的子widget
 * @return
 */
QList< QWidget* > DAFigureContainer::getWidgetList() const
{
    return d_ptr->managedWidgets.keys();
}

/**
 * @brief 获取一个有序的窗口列表，这个顺序是在窗口显示最顶层的排位最前
 * @note 此函数效率不高
 * @return
 */
QList< QWidget* > DAFigureContainer::getOrderedWidgetList() const
{
	QList< QWidget* > ws = getWidgetList();
	QObjectList objl     = children();
	QList< QWidget* > res;
	for (QObject* obj : qAsConst(objl)) {
		if (ws.contains((QWidget*)obj)) {
			res.append((QWidget*)obj);
		}
	}
	return res;
}

/**
 * @brief 获取当前窗口的占比值
 * @param w
 * @return 如果窗体没有管理，返回一个QRectF(),否则，无论窗口是相对位置管理还是绝对位置管理都返回一个相对位置占比
 */
QRectF DAFigureContainer::getWidgetPosPercent(QWidget* w) const
{
	WidgetSizeData sd = d_ptr->managedWidgets.value(w, WidgetSizeData());
	if (!sd.isValid()) {
		return QRectF();
	}
	if (sd.isRelativePos) {
		return sd.sizeData;
	}
	// 说明不是相对位置，是绝对位置
	return calcPercentByGeometryRect(w->geometry());
}

/**
 * @brief 设置窗口的百分比
 * @param w
 * @param posPercent
 */
void DAFigureContainer::setWidgetPosPercent(QWidget* w, const QRectF& posPercent)
{
	if (w->parentWidget() != this) {
		return;
	}
	QRect g = calcWidgetRectByPercent(posPercent);
	w->setGeometry(g);
	d_ptr->managedWidgets[ w ] = WidgetSizeData(posPercent, true);
}

/**
 * @brief 设置窗口的百分比
 * @param w
 * @param xPercent
 * @param yPercent
 * @param wPercent
 * @param hPercent
 */
void DAFigureContainer::setWidgetPosPercent(QWidget* w, float xPercent, float yPercent, float wPercent, float hPercent)
{
	setWidgetPosPercent(w,
						QRectF(castRealByPrecision(xPercent, DAFIGURECONTAINER_PRECISION),
							   castRealByPrecision(yPercent, DAFIGURECONTAINER_PRECISION),
							   castRealByPrecision(wPercent, DAFIGURECONTAINER_PRECISION),
							   castRealByPrecision(hPercent, DAFIGURECONTAINER_PRECISION)));
}

/**
 * @brief 窗口是否在容器窗口中
 * @param w
 * @return
 */
bool DAFigureContainer::isWidgetInContainer(const QWidget* w) const
{
    return (w->parentWidget() == this);
}

/**
 * @brief 通过percent计算在continerRect下的子窗体的Geometry
 * @param percent
 * @param continerRect
 * @return
 */
QRect DAFigureContainer::calcGeometryByPercent(const QRectF& percent, const QRect& continerRect)
{
	QRect newrect;
	newrect.setRect(continerRect.width() * percent.left(),
					continerRect.height() * percent.top(),
					continerRect.width() * percent.width(),
					continerRect.height() * percent.height());
	return newrect;
}

/**
 * @brief 通过窗口Percent计算窗口尺寸
 * @param percent
 * @return
 */
QRect DAFigureContainer::calcWidgetRectByPercent(const QRectF& percent) const
{
    return calcGeometryByPercent(percent, rect());
}

/**
 * @brief 通过窗口Percent计算窗口尺寸
 * @param c
 * @param percent
 * @return
 */
QRect DAFigureContainer::calcWidgetRectByPercent(DAFigureContainer* c, const QRectF& percent)
{
    return c->calcWidgetRectByPercent(percent);
}

/**
 * @brief 根据baserect，计算目标rect相对baserect的占比
 * @param baseRect 基础rect，一般就是DAFigureContainer的尺寸
 * @param rect 目标rect，就是子窗口的尺寸
 * @return QRectF.x x点占比，QRectF.y y点占比，QRectF.width width占比，QRectF.height height占比
 */
QRectF DAFigureContainer::calcRectPosPercentByOtherRect(const QRect& baseRect, const QRect& rect)
{
	QRectF present;
	if (0 == baseRect.width()) {
		present.setX(0);
		present.setWidth(0);
	} else {
		present.setX(castRealByPrecision((qreal)rect.x() / baseRect.width(), DAFIGURECONTAINER_PRECISION));
		present.setWidth(castRealByPrecision((qreal)rect.width() / baseRect.width(), DAFIGURECONTAINER_PRECISION));
	}
	if (0 == baseRect.height()) {
		present.setY(0);
		present.setHeight(0);
	} else {
		present.setY(castRealByPrecision((qreal)rect.y() / baseRect.height(), DAFIGURECONTAINER_PRECISION));
		present.setHeight(castRealByPrecision((qreal)rect.height() / baseRect.height(), DAFIGURECONTAINER_PRECISION));
	}
	return (present);
}

/**
 * @brief 计算size对应SAFigureContainer的位置占比
 * @param size
 * @return 返回rect对应SAFigureContainer的位置占比
 */
QRectF DAFigureContainer::calcPercentByGeometryRect(const QRect& s) const
{
    return calcRectPosPercentByOtherRect(rect(), s);
}

/**
 * @brief 计算size对应SAFigureContainer的位置占比
 * @param c
 * @param size
 * @return 返回rect对应SAFigureContainer的位置占比
 */
QRectF DAFigureContainer::calcPercentByGeometryRect(DAFigureContainer* c, const QRect& s)
{
    return c->calcPercentByGeometryRect(s);
}

/**
 * @brief 限制浮点数的位数，避免出现过多小数
 * @param v 需要限制的浮点数
 * @param precision 精度，默认为3位
 * @return 限制后的浮点数值
 */
qreal DAFigureContainer::castRealByPrecision(qreal v, int precision)
{
	qreal p = pow(10, precision);

	return (round(v * p) / p);
}
/**
 * @brief 设置允许在窗口改变的时候重新修改widgetPosPersent,此函数需要和 @sa endResetSubWidget 配套
 */
void DAFigureContainer::beginResetSubWidget()
{
    d_ptr->isUpdatingLayout = true;
}

/**
 * @brief 设置不允许在窗口改变的时候重新修改widgetPosPersent,此函数需要和 @sa beginResetSubWidget 配套
 */
void DAFigureContainer::endResetSubWidget()
{
    d_ptr->isUpdatingLayout = false;
}

/**
 * @brief 获取当前位置下的窗口
 * @param p 相对DAFigureContainer的位置
 * @param zorder 考虑z-order,考虑z会保证优先返回最上层的窗口
 * @return 如果没有返回nullptr
 */
QWidget* DAFigureContainer::getWidgetUnderPos(const QPoint& p, bool zorder)
{
	if (zorder) {
		QList< QWidget* > zw = getOrderedWidgetList();
		for (QWidget* w : qAsConst(zw)) {
			if (w->geometry().contains(p)) {
				return w;
			}
		}
	} else {
		for (auto i = d_ptr->managedWidgets.begin(); i != d_ptr->managedWidgets.end(); ++i) {
			if (i.key()->geometry().contains(p)) {
				return i.key();
			}
		}
	}
	return nullptr;
}
/**
 * @brief DAFigureContainer::event
 * @param e
 * @return
 */
bool DAFigureContainer::event(QEvent* e)
{
	if (e) {
		//        qDebug() << "DAFigureContainer::event,type=" << e->type() << ",rect=" << rect();
		if (QEvent::ChildRemoved == e->type()) {
			// 窗口删除，把记录也得删除
			QChildEvent* ce = static_cast< QChildEvent* >(e);
			QObject* obj    = ce->child();
			if (obj && obj->isWidgetType()) {
				QWidget* w = qobject_cast< QWidget* >(obj);
				d_ptr->managedWidgets.remove(w);
			}
		}
	}
	return (QWidget::event(e));
}

/**
 * @brief DAFigureContainer::resizeEvent
 *
 * @note 窗口首次生成的时候会给予一个默认尺寸640x480，但窗口在没有show之前，resizeEvent是不会触发的
 * @param event
 */
void DAFigureContainer::resizeEvent(QResizeEvent* e)
{
	//    qDebug() << "DAFigureContainer::resizeEvent rect=" << rect();
	resetSubWidgetGeometry();
	QWidget::resizeEvent(e);
}

bool DAFigureContainer::eventFilter(QObject* watched, QEvent* e)
{
	if (nullptr == e) {
		return (QWidget::eventFilter(watched, e));
	}
	if (d_ptr->isUpdatingLayout) {
		if (QEvent::Resize == e->type()) {
			if (watched && watched->isWidgetType()) {
				QWidget* w = qobject_cast< QWidget* >(watched);
				if (w && isWidgetInContainer(w)) {
					QResizeEvent* re = static_cast< QResizeEvent* >(e);
					auto& data       = d_ptr->managedWidgets[ w ];
					if (data.isRelativePos) {
						data.sizeData.setWidth(
							castRealByPrecision(re->size().width() / (double)width(), DAFIGURECONTAINER_PRECISION));
						data.sizeData.setHeight(
							castRealByPrecision(re->size().height() / (double)height(), DAFIGURECONTAINER_PRECISION));
					} else {
						data.sizeData = QRectF(w->geometry());
					}

					// qDebug() <<" == " <<watched << " set new:" <<  e->size() << " old" <<e->oldSize() << "present:"<<data;
				}
			}
		} else if (QEvent::Move == e->type()) {
			if (watched && watched->isWidgetType()) {
				QWidget* w = qobject_cast< QWidget* >(watched);
				if (w && isWidgetInContainer(w)) {
					QMoveEvent* me = static_cast< QMoveEvent* >(e);
					auto& data     = d_ptr->managedWidgets[ w ];
					if (data.isRelativePos) {
						data.sizeData.setX(castRealByPrecision(me->pos().x() / (double)width(), DAFIGURECONTAINER_PRECISION));
						data.sizeData.setY(
							castRealByPrecision(me->pos().y() / (double)height(), DAFIGURECONTAINER_PRECISION));
					} else {
						data.sizeData.setTopLeft(me->pos());
					}
				}
			}
		}
	}
	return (QWidget::eventFilter(watched, e));
}

/**
 * @brief 根据设定的占比，和当前窗口的尺寸，重置子窗口的Geometry
 */
void DAFigureContainer::resetSubWidgetGeometry()
{
	QRect subWidgetSize;
	for (auto i = d_ptr->managedWidgets.begin(); i != (d_ptr->managedWidgets.end()); ++i) {
		if (i.value().isRelativePos) {
			subWidgetSize = calcWidgetRectByPercent(i.value().sizeData);
			QWidget* w    = i.key();
			w->setGeometry(subWidgetSize);
		} else {
			// 不是相对位置，是绝对位置
			QWidget* w = i.key();
			w->setGeometry(i.value().sizeData.toRect());
		}
	}
}

QRect DAFigureContainer::calcWidgetSize(const QRectF& present)
{
	return calcWidgetRectByPercent(this, present);
}

}  // End Of Namespace DA
