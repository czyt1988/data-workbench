#ifndef DAFIGUREWIDGETOVERLAY_H
#define DAFIGUREWIDGETOVERLAY_H
#include "qwt_widget_overlay.h"
#include "DAFigureWidget.h"
#include "DAFigureAPI.h"
namespace DA
{

/**
 * @brief DAFigureWidget编辑遮罩
 */
class DAFIGURE_API DAFigureWidgetOverlay : public QwtWidgetOverlay
{
    Q_OBJECT
public:
    DAFigureWidgetOverlay(DAFigureWidget* fig);
    DAFigureWidget* figure() const;

private:
    DAFigureWidget* mFigure { nullptr };
};
}  // end DA namespace
#endif  // SAFIGUREWINDOWOVERLAY_H
