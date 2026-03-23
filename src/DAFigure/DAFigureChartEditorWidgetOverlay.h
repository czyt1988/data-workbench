#ifndef DAFIGURECHARTEDITORWIDGETOVERLAY_H
#define DAFIGURECHARTEDITORWIDGETOVERLAY_H
#include "DAFigureWidgetOverlay.h"
#include "DAFigureAPI.h"
#include <functional>
#include <QPointer>
class QwtFigure;
class QMouseEvent;
class QKeyEvent;
class QPaintEvent;
class QPainter;
namespace DA
{
class DAAbstractChartEditor;
/**
 * @brief 它第一次点击所在的绘图会设置给对应的charteditor
 *
 * 它是charteditor的代理，用于在figure中识别哪个chart需要安装editor
 *
 * @note 在chart editor编辑完成，会触发@ref DAFigureChartEditorWidgetOverlay::finished 信号
 */
class DAFIGURE_API DAFigureChartEditorWidgetOverlay : public DAFigureWidgetOverlay
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigureChartEditorWidgetOverlay)
public:
    // 图表编辑器工厂
    using FpChartEditorFactory = std::function< DA::DAAbstractChartEditor*(QwtPlot*) >;
    // 激活绘图canvas的绘制函数(painter,pos:鼠标位置, normGeometry:绘图canvas相对figure的矩形区域)
    using FpActiveChartCanvasPainter = std::function< void(QPainter*, const QPoint&, const QRectF&) >;

public:
    explicit DAFigureChartEditorWidgetOverlay(QwtFigure* fig, FpChartEditorFactory funFactory = nullptr);
    virtual ~DAFigureChartEditorWidgetOverlay();
    // 设置图形编辑器工厂
    void setChartEditorFactory(FpChartEditorFactory funFactory);
    FpChartEditorFactory getChartEditorFactory() const;
    // 设置自动启动图表编辑器
    void setAutoStart(bool autoStart);
    bool isAutoStart() const;
    // 是否激活编辑器
    bool isChartEditorActive() const;
    // 设置激活绘图canvas的绘制函数
    void setActiveChartCanvasPainter(FpActiveChartCanvasPainter painterFp);
    FpActiveChartCanvasPainter getActiveChartCanvasPainter() const;
    // 将源Widget的rect映射到目标Widget的坐标系
    static QRect mapRectTo(const QWidget* sourceWidget, const QWidget* targetWidget, const QRect& sourceRect);
    // 获取图表编辑器
    DAAbstractChartEditor* getChartEditor() const;
private Q_SLOTS:
    void onActiveWidgetChanged(QWidget* oldActive, QWidget* newActive);
    void onEditorFinished(bool isCancel);
    void onEditorBegin();

private:
    void createChartEditor(QwtPlot* plot);

protected:
    void mouseMoveEvent(QMouseEvent* me) override;
    void mouseReleaseEvent(QMouseEvent* me) override;
    void mousePressEvent(QMouseEvent* me) override;
    void keyPressEvent(QKeyEvent* ke) override;
    void paintEvent(QPaintEvent* pe) override;
};

}

#endif  // DAFIGURECHARTEDITORWIDGETOVERLAY_H
