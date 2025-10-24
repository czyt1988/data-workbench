#ifndef DAFIGUREWIDGETOVERLAY_H
#define DAFIGUREWIDGETOVERLAY_H
#include "qwt_figure_widget_overlay.h"
#include "DAFigureWidget.h"
#include "DAFigureAPI.h"
namespace DA
{

/**
 * @brief DAFigureWidget编辑遮罩
 */
class DAFIGURE_API DAFigureWidgetOverlay : public QwtFigureWidgetOverlay
{
    Q_OBJECT
public:
    DAFigureWidgetOverlay(QwtFigure* fig);
    ~DAFigureWidgetOverlay();
};
}  // end DA namespace
#endif  // SAFIGUREWINDOWOVERLAY_H
