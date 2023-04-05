#include "DAAbstractPlotEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>

namespace DA
{
DAAbstractPlotEditor::DAAbstractPlotEditor(QwtPlot* parent)
    : QObject(parent), m_isEnable(false), m_isSpacePressed(false), m_isCanUseSpaceKey(false)
{
}

DAAbstractPlotEditor::~DAAbstractPlotEditor()
{
}

const QwtPlot* DAAbstractPlotEditor::plot() const
{
    return qobject_cast< const QwtPlot* >(parent());
}

QwtPlot* DAAbstractPlotEditor::plot()
{
    return qobject_cast< QwtPlot* >(parent());
}

void DAAbstractPlotEditor::setEnabled(bool on)
{
    if (on == m_isEnable)
        return;

    QwtPlot* plot = qobject_cast< QwtPlot* >(parent());
    if (plot) {
        m_isEnable = on;

        if (plot->canvas()) {
            if (on) {
                plot->canvas()->installEventFilter(this);
            } else {
                plot->canvas()->removeEventFilter(this);
            }
        }
    }
}

bool DAAbstractPlotEditor::isEnabled() const
{
    return m_isEnable;
}
///
/// \brief 设置是否使用空格，如果这个editor使用了空格，需要设置为true，否则会被屏蔽空格的输入,默认为false
/// \param b
///
void DAAbstractPlotEditor::setUseSpaceKey(bool b)
{
    m_isCanUseSpaceKey = b;
}
///
/// \brief 设置是否可以使用空格
/// \return
///
bool DAAbstractPlotEditor::isUseSpaceKey() const
{
    return m_isCanUseSpaceKey;
}

bool DAAbstractPlotEditor::eventFilter(QObject* object, QEvent* event)
{
    QwtPlot* plot = qobject_cast< QwtPlot* >(parent());
    if (plot && (object == plot->canvas())) {
        switch (event->type()) {
            //空格按下，鼠标事件不处理
        case QEvent::MouseButtonPress: {
            const QMouseEvent* mouseEvent = dynamic_cast< QMouseEvent* >(event);
            if (mouseEvent && !m_isSpacePressed) {
                return mousePressEvent(mouseEvent);
            } else {
                return false;
            }
            break;
        }
        case QEvent::MouseMove: {
            const QMouseEvent* mouseEvent = dynamic_cast< QMouseEvent* >(event);
            if (mouseEvent && !m_isSpacePressed) {
                return mouseMovedEvent(mouseEvent);
            } else {
                return false;
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            const QMouseEvent* mouseEvent = dynamic_cast< QMouseEvent* >(event);
            if (mouseEvent && !m_isSpacePressed) {
                return mouseReleasedEvent(mouseEvent);
            } else {
                return false;
            }
            break;
        }
        case QEvent::KeyPress: {
            const QKeyEvent* keyEvent = dynamic_cast< QKeyEvent* >(event);
            if (keyEvent) {
                if (!m_isCanUseSpaceKey) {
                    if (Qt::Key_Space == keyEvent->key()) {

                        m_isSpacePressed = true;
                        return false;  //空格键屏蔽
                    } else {
                        m_isSpacePressed = false;
                        return keyPressEvent(keyEvent);
                    }
                }
                return keyPressEvent(keyEvent);
            }
            break;
        }
        case QEvent::KeyRelease: {
            const QKeyEvent* keyEvent = dynamic_cast< QKeyEvent* >(event);
            if (keyEvent) {
                if (!keyEvent->isAutoRepeat() && Qt::Key_Space == keyEvent->key()) {
                    m_isSpacePressed = false;
                }
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

bool DAAbstractPlotEditor::mousePressEvent(const QMouseEvent* e)
{
    Q_UNUSED(e);
    return false;
}

bool DAAbstractPlotEditor::mouseMovedEvent(const QMouseEvent* e)
{
    Q_UNUSED(e);
    return false;
}

bool DAAbstractPlotEditor::mouseReleasedEvent(const QMouseEvent* e)
{
    Q_UNUSED(e);
    return false;
}

bool DAAbstractPlotEditor::keyPressEvent(const QKeyEvent* e)
{
    Q_UNUSED(e);
    return false;
}

bool DAAbstractPlotEditor::keyReleaseEvent(const QKeyEvent* e)
{
    Q_UNUSED(e);
    return false;
}
}  // End Of Namespace DA
