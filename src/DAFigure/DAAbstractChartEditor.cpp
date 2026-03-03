#include "DAAbstractChartEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>

namespace DA
{
DAAbstractChartEditor::DAAbstractChartEditor(QwtPlot* parent) : QObject(parent), m_isEnable(false)
{
}

DAAbstractChartEditor::~DAAbstractChartEditor()
{
}

const QwtPlot* DAAbstractChartEditor::plot() const
{
	return qobject_cast< const QwtPlot* >(parent());
}

QwtPlot* DAAbstractChartEditor::plot()
{
	return qobject_cast< QwtPlot* >(parent());
}

void DAAbstractChartEditor::setEnabled(bool on)
{
	if (on == m_isEnable)
		return;

	QwtPlot* p = plot();
	if (p) {
		m_isEnable = on;

		if (p->canvas()) {
			if (on) {
				p->canvas()->installEventFilter(this);
			} else {
				p->canvas()->removeEventFilter(this);
			}
		}
	}
}

bool DAAbstractChartEditor::isEnabled() const
{
	return m_isEnable;
}

/**
 * @brief 取消编辑
 *
 * 默认按Esc取消,按Esc后的动作可以重载此函数
 * @return 返回true说明取消成功，返回false说明取消失败，取消成功不会继续响应，
 * 并发送@ref editorFinished(true)信号
 *
 * @note 默认取消成功后，会调用etEnable(false)，editor不在监听plot
 */
bool DAAbstractChartEditor::cancel()
{
	return true;
}

void DAAbstractChartEditor::setBlockKeys(const QList< int >& keys)
{
	m_blockKeys = keys;
}

const QList< int >& DAAbstractChartEditor::getBlockKeys() const
{
	return m_blockKeys;
}

bool DAAbstractChartEditor::eventFilter(QObject* object, QEvent* event)
{
	QwtPlot* plot = qobject_cast< QwtPlot* >(parent());
	if (plot && (object == plot->canvas())) {
		switch (event->type()) {
			// 空格按下，鼠标事件不处理
		case QEvent::MouseButtonPress: {
			const QMouseEvent* mouseEvent = dynamic_cast< QMouseEvent* >(event);
			if (mouseEvent) {
				return mousePressEvent(mouseEvent);
			} else {
				return false;
			}
			break;
		}
		case QEvent::MouseMove: {
			const QMouseEvent* mouseEvent = dynamic_cast< QMouseEvent* >(event);
			if (mouseEvent) {
				return mouseMoveEvent(mouseEvent);
			} else {
				return false;
			}
			break;
		}
		case QEvent::MouseButtonRelease: {
			const QMouseEvent* mouseEvent = dynamic_cast< QMouseEvent* >(event);
			if (mouseEvent) {
				return mouseReleaseEvent(mouseEvent);
			} else {
				return false;
			}
			break;
		}
		case QEvent::KeyPress: {
			const QKeyEvent* keyEvent = dynamic_cast< QKeyEvent* >(event);
			if (keyEvent) {
				if (m_blockKeys.size() > 0 && m_blockKeys.contains(keyEvent->key())) {
					return QObject::eventFilter(object, event);
				}
				if (Qt::Key_Escape == keyEvent->key()) {
					if (cancel()) {
						setEnabled(false);
						Q_EMIT editorFinished(true);
					}
				}
				return keyPressEvent(keyEvent);
			}
			break;
		}
		case QEvent::KeyRelease: {
			const QKeyEvent* keyEvent = dynamic_cast< QKeyEvent* >(event);
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

bool DAAbstractChartEditor::mousePressEvent(const QMouseEvent* e)
{
	Q_UNUSED(e);
	return false;
}

bool DAAbstractChartEditor::mouseMoveEvent(const QMouseEvent* e)
{
	Q_UNUSED(e);
	return false;
}

bool DAAbstractChartEditor::mouseReleaseEvent(const QMouseEvent* e)
{
	Q_UNUSED(e);
	return false;
}

bool DAAbstractChartEditor::keyPressEvent(const QKeyEvent* e)
{
	Q_UNUSED(e);
	return false;
}

bool DAAbstractChartEditor::keyReleaseEvent(const QKeyEvent* e)
{
	Q_UNUSED(e);
	return false;
}
}  // End Of Namespace DA
