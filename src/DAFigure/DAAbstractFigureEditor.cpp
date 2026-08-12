#include "DAAbstractFigureEditor.h"
#include "qwt_figure.h"
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
namespace DA
{

/**
 * @brief 构造函数
 * @param parent 关联的QwtFigure
 */
DAAbstractFigureEditor::DAAbstractFigureEditor(QwtFigure* parent) : QObject(parent), mIsEnable(false)
{
}

/**
 * @brief 析构函数
 */
DAAbstractFigureEditor::~DAAbstractFigureEditor()
{
}
/**
 * @brief 设置是否启用编辑器
 * @param on 是否启用
 */
void DAAbstractFigureEditor::setEnabled(bool on)
{
	if (on == mIsEnable) {
		return;
	}

	QwtFigure* fig = figure();
	if (fig) {
		mIsEnable = on;
		if (on) {
			fig->installEventFilter(this);
		} else {
			fig->removeEventFilter(this);
		}
	}
}
/**
 * @brief 判断编辑器是否启用
 * @return 如果启用返回true，否则返回false
 */
bool DAAbstractFigureEditor::isEnabled() const
{
	return mIsEnable;
}
/**
 * @brief 获取关联的QwtFigure（const版本）
 * @return 关联的QwtFigure指针
 */
const QwtFigure* DAAbstractFigureEditor::figure() const
{
	return qobject_cast< const QwtFigure* >(parent());
}
/**
 * @brief 获取关联的QwtFigure
 * @return 关联的QwtFigure指针
 */
QwtFigure* DAAbstractFigureEditor::figure()
{
	return qobject_cast< QwtFigure* >(parent());
}
/**
 * @brief 事件过滤器
 * @param object 监听的对象
 * @param event 事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractFigureEditor::eventFilter(QObject* object, QEvent* event)
{
	QwtFigure* fig = figure();
	if (fig) {
		switch (event->type()) {
		case QEvent::MouseButtonPress: {
			const QMouseEvent* mouseEvent = static_cast< QMouseEvent* >(event);
			if (mouseEvent) {
				return mousePressEvent(mouseEvent);
			}
			break;
		}
		case QEvent::MouseMove: {
			const QMouseEvent* mouseEvent = static_cast< QMouseEvent* >(event);
			if (mouseEvent) {
				return mouseMovedEvent(mouseEvent);
			}
			break;
		}
		case QEvent::MouseButtonRelease: {
			const QMouseEvent* mouseEvent = static_cast< QMouseEvent* >(event);
			if (mouseEvent) {
				return mouseReleasedEvent(mouseEvent);
			}
			break;
		}
		case QEvent::KeyPress: {
			const QKeyEvent* keyEvent = static_cast< QKeyEvent* >(event);
			if (keyEvent) {
				return keyPressEvent(keyEvent);
			}
			break;
		}
		case QEvent::KeyRelease: {
			const QKeyEvent* keyEvent = static_cast< QKeyEvent* >(event);
			if (keyEvent) {
				return keyReleaseEvent(keyEvent);
			}
			break;
		}
		default:
			break;
		}
		return false;
	}
	return QObject::eventFilter(object, event);
}
/**
 * @brief 鼠标按下事件处理
 * @param e 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractFigureEditor::mousePressEvent(const QMouseEvent* e)
{
	Q_UNUSED(e);
	return false;
}
/**
 * @brief 鼠标移动事件处理
 * @param e 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractFigureEditor::mouseMovedEvent(const QMouseEvent* e)
{
	Q_UNUSED(e);
	return false;
}
/**
 * @brief 鼠标释放事件处理
 * @param e 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractFigureEditor::mouseReleasedEvent(const QMouseEvent* e)
{
	Q_UNUSED(e);
	return false;
}
/**
 * @brief 键盘按下事件处理
 * @param e 键盘事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractFigureEditor::keyPressEvent(const QKeyEvent* e)
{
	Q_UNUSED(e);
	return false;
}
/**
 * @brief 键盘释放事件处理
 * @param e 键盘事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAAbstractFigureEditor::keyReleaseEvent(const QKeyEvent* e)
{
	Q_UNUSED(e);
	return false;
}
}
