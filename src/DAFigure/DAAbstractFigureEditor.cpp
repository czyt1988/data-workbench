#include "DAAbstractFigureEditor.h"
#include "qwt_figure.h"
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
namespace DA
{
DAAbstractFigureEditor::DAAbstractFigureEditor(QwtFigure* parent) : QObject(parent), m_isEnable(false)
{
}

DAAbstractFigureEditor::~DAAbstractFigureEditor()
{
}
void DAAbstractFigureEditor::setEnabled(bool on)
{
	if (on == m_isEnable) {
		return;
	}

	QwtFigure* fig = figure();
	if (fig) {
		m_isEnable = on;
		if (on) {
			fig->installEventFilter(this);
		} else {
			fig->removeEventFilter(this);
		}
	}
}
bool DAAbstractFigureEditor::isEnabled() const
{
	return m_isEnable;
}
const QwtFigure* DAAbstractFigureEditor::figure() const
{
	return qobject_cast< const QwtFigure* >(parent());
}
QwtFigure* DAAbstractFigureEditor::figure()
{
	return qobject_cast< QwtFigure* >(parent());
}
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
bool DAAbstractFigureEditor::mousePressEvent(const QMouseEvent* e)
{
	Q_UNUSED(e);
	return false;
}
bool DAAbstractFigureEditor::mouseMovedEvent(const QMouseEvent* e)
{
	Q_UNUSED(e);
	return false;
}
bool DAAbstractFigureEditor::mouseReleasedEvent(const QMouseEvent* e)
{
	Q_UNUSED(e);
	return false;
}
bool DAAbstractFigureEditor::keyPressEvent(const QKeyEvent* e)
{
	Q_UNUSED(e);
	return false;
}
bool DAAbstractFigureEditor::keyReleaseEvent(const QKeyEvent* e)
{
	Q_UNUSED(e);
	return false;
}
}