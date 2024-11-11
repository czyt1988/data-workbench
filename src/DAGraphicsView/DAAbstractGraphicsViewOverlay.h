#ifndef DAABSTRACTGRAPHICSVIEWOVERLAY_H
#define DAABSTRACTGRAPHICSVIEWOVERLAY_H
#include "DAGraphicsViewGlobal.h"
#include "DAAbstractWidgetOverlay.h"
namespace DA
{
/**
 * @brief 提供给DAGraphicsView的Overlay
 */
class DAGRAPHICSVIEW_API DAAbstractGraphicsViewOverlay : public DAAbstractWidgetOverlay
{
public:
	explicit DAAbstractGraphicsViewOverlay(QWidget* parent);
	~DAAbstractGraphicsViewOverlay();
	QRect overlayRect() const;
};
}
#endif  // DAABSTRACTGRAPHICSVIEWOVERLAY_H
