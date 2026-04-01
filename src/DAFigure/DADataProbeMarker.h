#ifndef DADATAPROBEMARKER_H
#define DADATAPROBEMARKER_H
#include "DAFigureAPI.h"
#include <QColor>
#include <QList>
#include <QPointF>
#include "qwt_plot_marker.h"
#include "qwt_text.h"

class QwtPlotItem;
class QwtPlotCurve;

namespace DA
{
/**
 * \if ENGLISH
 * @brief Data probe marker class for capturing and displaying data points
 * @details This class provides vertical and horizontal probe markers that can
 *          capture data values at specific positions on the plot. Unlike
 *          QwtPlotSeriesDataPicker which follows the mouse, probes are fixed
 *          position data pickers with persistent storage.
 *
 * @note QwtPlotMarker is not a QObject subclass, so this class cannot use
 *       Qt signals/slots mechanism.
 * \endif
 *
 * \if CHINESE
 * @brief 数据探针标记类，用于捕获和显示数据点
 * @details 该类提供垂直和水平探针标记，可以捕获绘图上特定位置的数据值。
 *          与跟随鼠标的QwtPlotSeriesDataPicker不同，探针是固定位置的数据拾取器，
 *          支持持久化存储。
 *
 * @note QwtPlotMarker 不是 QObject 的子类，因此此类不能使用 Qt 信号槽机制。
 * \endif
 */
class DAFIGURE_API DADataProbeMarker : public QwtPlotMarker
{
    DA_DECLARE_PRIVATE(DADataProbeMarker)
public:
    /**
     * \if ENGLISH
     * @brief Probe type enumeration
     * \endif
     * \if CHINESE
     * @brief 探针类型枚举
     * \endif
     */
    enum ProbeType
    {
        VerticalProbe = 0,  ///< Vertical probe, picks Y values at specified X position
        HorizontalProbe     ///< Horizontal probe, picks X values at specified Y position
    };

    /**
     * \if ENGLISH
     * @brief Label position enumeration
     * \endif
     * \if CHINESE
     * @brief 标签位置枚举
     * \endif
     */
    enum LabelPosition
    {
        LabelAtTop,    ///< Label at top (for vertical probe) or left (for horizontal probe)
        LabelAtBottom  ///< Label at bottom (for vertical probe) or right (for horizontal probe)
    };

    /**
     * \if ENGLISH
     * @brief Captured data point structure
     * \endif
     * \if CHINESE
     * @brief 捕获的数据点结构
     * \endif
     */
    struct CapturedData
    {
        QwtPlotItem* item { nullptr };  ///< Corresponding curve item
        QPointF point { 0.0, 0.0 };     ///< Captured point coordinates
        size_t index { 0 };             ///< Index in the curve data
    };

public:
    // Constructor with probe type
    explicit DADataProbeMarker(ProbeType type, const QString& name = QString());
    // Constructor with QwtText title
    DADataProbeMarker(ProbeType type, const QwtText& title);
    // Destructor
    virtual ~DADataProbeMarker();

    // Get probe type
    ProbeType probeType() const;

    // Get/set probe name
    const QwtText& probeName() const;
    void setProbeName(const QwtText& name);

    // Get probe value (x for vertical, y for horizontal)
    double probeValue() const;
    void setProbeValue(double val);

    // Get position as QPointF
    QPointF probePosition() const;

    // Label position
    void setLabelPosition(LabelPosition pos);
    LabelPosition labelPosition() const;

    // Label visibility
    void setLabelVisible(bool visible);
    bool isLabelVisible() const;

    // Set probe color
    void setProbeColor(const QColor& color);
    QColor probeColor() const;

    // Data capture functions
    int captureData(bool interpolate = true);
    QList< CapturedData > capturedData() const;
    void clearCapturedData();

    // Override draw method to customize label rendering
    virtual void draw(QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const override;

    // RTTI for probe marker identification
    virtual int rtti() const override;

    // Static RTTI value for DADataProbeMarker
    static const int Rtti_DataProbeMarker = QwtPlotItem::Rtti_PlotUserItem + 100;

protected:
    // 绘制探针名称
    virtual void drawProbeName(
        QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect, const QwtText& name
    ) const;

private:
    // Internal data picking methods (referenced from QwtPlotSeriesDataPicker::pickYValue)
    int pickYValueAtPosition(double xValue, bool interpolate);
    int pickXValueAtPosition(double yValue, bool interpolate);

    void updateLabel();
    void updateLineStyle();
    QString formatAxisValue(double value, int axisId) const;
};

}  // namespace DA

#endif  // DADATAPROBEMARKER_H
