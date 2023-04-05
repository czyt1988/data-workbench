#ifndef DACHARTSCROLLZOOMER_H
#define DACHARTSCROLLZOOMER_H
#include "DAFigureAPI.h"
#include "DAChartScrollBar.h"
#include "qwt_plot_zoomer.h"
namespace DA
{
class _DAChartScrollZoomerScrollData;
DA_IMPL_FORWARD_DECL(DAChartScrollZoomer)
/**
 * @brief The DAChartScrollZoomer class
 */
class DAFIGURE_API DAChartScrollZoomer : public QwtPlotZoomer
{
    Q_OBJECT
    DA_IMPL(DAChartScrollZoomer)
public:
    enum ScrollBarPosition
    {
        AttachedToScale,
        OppositeToScale
    };

    /**
     * @brief 坐标轴常数
     */
    enum AxisConst
    {
        AxisCnt = 4  ///< 坐标轴数量
    };

public:
    DAChartScrollZoomer(int xAxis, int yAxis, QWidget*);
    DAChartScrollZoomer(QWidget*);
    virtual ~DAChartScrollZoomer();

    DAChartScrollBar* horizontalScrollBar() const;
    DAChartScrollBar* verticalScrollBar() const;

    void setHScrollBarMode(Qt::ScrollBarPolicy);
    void setVScrollBarMode(Qt::ScrollBarPolicy);

    Qt::ScrollBarPolicy vScrollBarMode() const;
    Qt::ScrollBarPolicy hScrollBarMode() const;

    void setHScrollBarPosition(ScrollBarPosition);
    void setVScrollBarPosition(ScrollBarPosition);

    ScrollBarPosition hScrollBarPosition() const;
    ScrollBarPosition vScrollBarPosition() const;

    QWidget* cornerWidget() const;
    virtual void setCornerWidget(QWidget*);

    virtual bool eventFilter(QObject*, QEvent*);

    virtual void rescale();
    bool isEnableScrollBar() const;

public slots:
    void on_enable_scrollBar(bool enable);

protected:
    virtual DAChartScrollBar* scrollBar(Qt::Orientation);
    virtual void updateScrollBars();
    virtual void layoutScrollBars(const QRect&);

private Q_SLOTS:
    void scrollBarMoved(Qt::Orientation o, double min, double max);

private:
    bool needScrollBar(Qt::Orientation) const;
    int oppositeAxis(int) const;
};

}  // End Of Namespace DA
#endif  // SAPLOTZOOMER_H
