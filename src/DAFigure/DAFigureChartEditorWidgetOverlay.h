#ifndef DAFIGURECHARTEDITORWIDGETOVERLAY_H
#define DAFIGURECHARTEDITORWIDGETOVERLAY_H
#include "DAFigureWidgetOverlay.h"
#include "DAFigureAPI.h"
#include "DAAbstractChartEditor.h"
#include <functional>
#include <QPointer>
namespace DA
{
/**
 * @brief 它第一次点击所在的绘图会设置给对应的charteditor
 */
class DAFIGURE_API DAFigureChartEditorWidgetOverlay : public DAFigureWidgetOverlay
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigureChartEditorWidgetOverlay)
public:
    using FpChartEditorFactory = std::function< DAAbstractChartEditor*(QwtPlot*) >;

public:
    explicit DAFigureChartEditorWidgetOverlay(QwtFigure* fig, FpChartEditorFactory funFactory = nullptr);
    virtual ~DAFigureChartEditorWidgetOverlay();
    // 设置图形编辑器工厂
    void setChartEditorFactory(FpChartEditorFactory funFactory);
    FpChartEditorFactory getChartEditorFactory() const;
private Q_SLOTS:
    void onActiveWidgetChanged(QWidget* oldActive, QWidget* newActive);
    void onEditorFinished(bool isCancel);

protected:
    void mouseMoveEvent(QMouseEvent* me) override;
    void mouseReleaseEvent(QMouseEvent* me) override;
    void mousePressEvent(QMouseEvent* me) override;
};

}

#endif  // DAFIGURECHARTEDITORWIDGETOVERLAY_H
