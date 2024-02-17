#ifndef DAEVENFILTERDRAGPLOTWITHGUIDE_H
#define DAEVENFILTERDRAGPLOTWITHGUIDE_H
#include <QObject>
#include <QEvent>
#include <QPointer>
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
namespace DA
{
class DAAppChartOperateWidget;
class DAFigureWidget;
/**
 * @brief 这个是针对DAAppFigureWidget的事件过滤器，可以为DAAppFigureWidget提供拖曳功能
 */
class DAEvenFilterDragPlotWithGuide : public QObject
{
    Q_OBJECT
public:
    DAEvenFilterDragPlotWithGuide(QObject* par = nullptr);
    // 设置ChartOptWidget，可以调用plotWithGuideDialog调出绘图引导对话框
    void setChartOptWidget(DAAppChartOperateWidget* c);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

protected:
    bool dragEnterEvent(QDragEnterEvent* e, DAFigureWidget* fig);
    bool dragMoveEvent(QDragMoveEvent* e, DAFigureWidget* fig);
    bool dragLeaveEvent(QDragLeaveEvent* e, DAFigureWidget* fig);
    bool dropEvent(QDropEvent* e, DAFigureWidget* fig);

private:
    QPointer< DAAppChartOperateWidget > mChartOptWidget;
};
}  // end DA

#endif  // DAEVENFILTERDRAGPLOTWITHGUIDE_H
