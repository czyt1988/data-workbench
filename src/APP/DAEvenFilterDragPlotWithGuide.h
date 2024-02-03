#ifndef DAEVENFILTERDRAGPLOTWITHGUIDE_H
#define DAEVENFILTERDRAGPLOTWITHGUIDE_H
#include <QObject>
#include <QEvent>
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
namespace DA
{
class DAFigureWidget;
/**
 * @brief The DAEvenFilterDragPlotWithGuide class
 */
class DAEvenFilterDragPlotWithGuide : public QObject
{
    Q_OBJECT
public:
    DAEvenFilterDragPlotWithGuide(QObject* par = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

protected:
    bool dragEnterEvent(QDragEnterEvent* e, DAFigureWidget* fig);
    bool dragMoveEvent(QDragMoveEvent* e, DAFigureWidget* fig);
    bool dragLeaveEvent(QDragLeaveEvent* e, DAFigureWidget* fig);
    bool dropEvent(QDropEvent* e, DAFigureWidget* fig);
};
}  // end DA

#endif  // DAEVENFILTERDRAGPLOTWITHGUIDE_H
