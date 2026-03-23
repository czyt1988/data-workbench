#ifndef DAABSTRACTFIGUREEDITOR_H
#define DAABSTRACTFIGUREEDITOR_H
#include "DAFigureAPI.h"
#include <QObject>
class QwtFigure;
class QEvent;
class QMouseEvent;
class QKeyEvent;
namespace DA
{
class DAFIGURE_API DAAbstractFigureEditor : public QObject
{
	Q_OBJECT
public:
	enum RTTI
	{
		Rtti_ChartEditor = 0x1000,
	};

public:
	explicit DAAbstractFigureEditor(QwtFigure* parent);
	virtual ~DAAbstractFigureEditor();
	virtual int rtti() const = 0;
	virtual void setEnabled(bool on);
	bool isEnabled() const;
	const QwtFigure* figure() const;
	QwtFigure* figure();

protected:
	virtual bool eventFilter(QObject* object, QEvent* event);
	virtual bool mousePressEvent(const QMouseEvent* e);
	virtual bool mouseMovedEvent(const QMouseEvent* e);
	virtual bool mouseReleasedEvent(const QMouseEvent* e);
	virtual bool keyPressEvent(const QKeyEvent* e);
	virtual bool keyReleaseEvent(const QKeyEvent* e);

private:
	bool m_isEnable { false };
};
}  // end namespace DA
#endif