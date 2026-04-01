#include "DADataProbeMarker.h"
#include <QPainter>
#include <QPen>
#include <QtMath>
#include <algorithm>
#include <limits>
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_item.h"
#include "qwt_scale_map.h"
#include "qwt_scale_draw.h"
#include "qwt_painter.h"
#include "qwt_utils.h"

namespace DA
{
class DADataProbeMarker::PrivateData
{
    DA_DECLARE_PUBLIC(DADataProbeMarker)
public:
    PrivateData(DADataProbeMarker* p);
    ~PrivateData();

    DADataProbeMarker::ProbeType probeType { DADataProbeMarker::VerticalProbe };
    QwtText probeName;
    DADataProbeMarker::LabelPosition labelPosition { DADataProbeMarker::LabelAtTop };
    QColor probeColor { Qt::red };
    bool labelVisible { true };
    QList< DADataProbeMarker::CapturedData > capturedData;
};

DADataProbeMarker::PrivateData::PrivateData(DADataProbeMarker* p) : q_ptr(p)
{
}

DADataProbeMarker::PrivateData::~PrivateData()
{
}

/**
 * \if ENGLISH
 * @brief Constructor with probe type
 * @param type Probe type (vertical or horizontal)
 * @param name Optional probe name
 * \endif
 *
 * \if CHINESE
 * @brief 构造函数，指定探针类型
 * @param type 探针类型（垂直或水平）
 * @param name 探针名称（可选）
 * \endif
 */
DADataProbeMarker::DADataProbeMarker(ProbeType type, const QString& name) : QwtPlotMarker(name), DA_PIMPL_CONSTRUCT
{
    d_ptr->probeType = type;
    d_ptr->probeName = name;
    updateLineStyle();
    setLinePen(QPen(d_ptr->probeColor, 1, Qt::DashLine));
}

/**
 * \if ENGLISH
 * @brief Constructor with QwtText title
 * @param type Probe type (vertical or horizontal)
 * @param title Probe title as QwtText
 * \endif
 *
 * \if CHINESE
 * @brief 构造函数，使用 QwtText 作为标题
 * @param type 探针类型（垂直或水平）
 * @param title 探针标题
 * \endif
 */
DADataProbeMarker::DADataProbeMarker(ProbeType type, const QwtText& title) : QwtPlotMarker(title), DA_PIMPL_CONSTRUCT
{
    d_ptr->probeType = type;
    d_ptr->probeName = title.text();
    updateLineStyle();
    setLinePen(QPen(d_ptr->probeColor, 1, Qt::DashLine));
}

/**
 * \if ENGLISH
 * @brief Destructor
 * \endif
 *
 * \if CHINESE
 * @brief 析构函数
 * \endif
 */
DADataProbeMarker::~DADataProbeMarker()
{
}

/**
 * \if ENGLISH
 * @brief Get the probe type
 * @return Probe type enumeration value
 * \endif
 *
 * \if CHINESE
 * @brief 获取探针类型
 * @return 探针类型枚举值
 * \endif
 */
DADataProbeMarker::ProbeType DADataProbeMarker::probeType() const
{
    return d_ptr->probeType;
}

/**
 * \if ENGLISH
 * @brief Get the probe name
 * @return Probe name string
 * \endif
 *
 * \if CHINESE
 * @brief 获取探针名称
 * @return 探针名称字符串
 * \endif
 */
const QwtText& DADataProbeMarker::probeName() const
{
    return d_ptr->probeName;
}

/**
 * \if ENGLISH
 * @brief Set the probe name
 * @param name New probe name
 * \endif
 *
 * \if CHINESE
 * @brief 设置探针名称
 * @param name 新的探针名称
 * \endif
 */
void DADataProbeMarker::setProbeName(const QwtText& name)
{
    if (d_ptr->probeName != name) {
        d_ptr->probeName = name;
        updateLabel();
    }
}

/**
 * \if ENGLISH
 * @brief Get the probe value
 * @return X value for vertical probe, Y value for horizontal probe
 * \endif
 *
 * \if CHINESE
 * @brief 获取探针值
 * @return 垂直探针返回X值，水平探针返回Y值
 * \endif
 */
double DADataProbeMarker::probeValue() const
{
    if (d_ptr->probeType == VerticalProbe) {
        return xValue();
    } else {
        return yValue();
    }
}

/**
 * \if ENGLISH
 * @brief Set the probe value
 * @param val X value for vertical probe, Y value for horizontal probe
 * \endif
 *
 * \if CHINESE
 * @brief 设置探针值
 * @param val 垂直探针设置X值，水平探针设置Y值
 * \endif
 */
void DADataProbeMarker::setProbeValue(double val)
{
    if (d_ptr->probeType == VerticalProbe) {
        setValue(val, yValue());
    } else {
        setValue(xValue(), val);
    }
    updateLabel();
}

/**
 * \if ENGLISH
 * @brief Get the probe position as QPointF
 * @return Probe position in plot coordinates
 * \endif
 *
 * \if CHINESE
 * @brief 获取探针位置
 * @return 探针在绘图坐标系中的位置
 * \endif
 */
QPointF DADataProbeMarker::probePosition() const
{
    return value();
}

/**
 * \if ENGLISH
 * @brief Set the label position
 * @param pos Label position (top/bottom for vertical, left/right for horizontal)
 * \endif
 *
 * \if CHINESE
 * @brief 设置标签位置
 * @param pos 标签位置（垂直探针为顶部/底部，水平探针为左侧/右侧）
 * \endif
 */
void DADataProbeMarker::setLabelPosition(LabelPosition pos)
{
    d_ptr->labelPosition = pos;
    updateLineStyle();
}

/**
 * \if ENGLISH
 * @brief Get the label position
 * @return Current label position
 * \endif
 *
 * \if CHINESE
 * @brief 获取标签位置
 * @return 当前标签位置
 * \endif
 */
DADataProbeMarker::LabelPosition DADataProbeMarker::labelPosition() const
{
    return d_ptr->labelPosition;
}

/**
 * \if ENGLISH
 * @brief Set label visibility
 * @param visible True to show label, false to hide
 * \endif
 *
 * \if CHINESE
 * @brief 设置标签可见性
 * @param visible true显示标签，false隐藏标签
 * \endif
 */
void DADataProbeMarker::setLabelVisible(bool visible)
{
    d_ptr->labelVisible = visible;
}

/**
 * \if ENGLISH
 * @brief Check if label is visible
 * @return True if label is visible
 * \endif
 *
 * \if CHINESE
 * @brief 检查标签是否可见
 * @return 如果标签可见返回true
 * \endif
 */
bool DADataProbeMarker::isLabelVisible() const
{
    return d_ptr->labelVisible;
}

/**
 * \if ENGLISH
 * @brief Set the probe color
 * @param color New probe color
 * \endif
 *
 * \if CHINESE
 * @brief 设置探针颜色
 * @param color 新的探针颜色
 * \endif
 */
void DADataProbeMarker::setProbeColor(const QColor& color)
{
    d_ptr->probeColor = color;
    setLinePen(QPen(color, 1, Qt::DashLine));
}

/**
 * \if ENGLISH
 * @brief Get the probe color
 * @return Current probe color
 * \endif
 *
 * \if CHINESE
 * @brief 获取探针颜色
 * @return 当前探针颜色
 * \endif
 */
QColor DADataProbeMarker::probeColor() const
{
    return d_ptr->probeColor;
}

/**
 * \if ENGLISH
 * @brief Capture data at current probe position
 * @param interpolate Whether to use linear interpolation
 * @return Number of captured data points
 * \endif
 *
 * \if CHINESE
 * @brief 在当前探针位置捕获数据
 * @param interpolate 是否使用线性插值
 * @return 捕获的数据点数量
 * \endif
 */
int DADataProbeMarker::captureData(bool interpolate)
{
    if (!plot()) {
        return 0;
    }

    d_ptr->capturedData.clear();

    int count = 0;
    if (d_ptr->probeType == VerticalProbe) {
        count = pickYValueAtPosition(xValue(), interpolate);
    } else {
        count = pickXValueAtPosition(yValue(), interpolate);
    }

    return count;
}

/**
 * \if ENGLISH
 * @brief Get captured data points
 * @return List of captured data points
 * \endif
 *
 * \if CHINESE
 * @brief 获取捕获的数据点
 * @return 捕获的数据点列表
 * \endif
 */
QList< DADataProbeMarker::CapturedData > DADataProbeMarker::capturedData() const
{
    return d_ptr->capturedData;
}

/**
 * \if ENGLISH
 * @brief Clear captured data
 * \endif
 *
 * \if CHINESE
 * @brief 清除捕获的数据
 * \endif
 */
void DADataProbeMarker::clearCapturedData()
{
    d_ptr->capturedData.clear();
}

/**
 * \if ENGLISH
 * @brief Draw the probe marker
 * @param painter Painter object
 * @param xMap X-axis scale map
 * @param yMap Y-axis scale map
 * @param canvasRect Canvas rectangle
 * \endif
 *
 * \if CHINESE
 * @brief 绘制探针标记
 * @param painter 绘图对象
 * @param xMap X轴比例映射
 * @param yMap Y轴比例映射
 * @param canvasRect 画布矩形
 * \endif
 */
void DADataProbeMarker::draw(QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const
{
    QwtPlotMarker::draw(painter, xMap, yMap, canvasRect);
    if (!d_ptr->labelVisible) {
        return;
    }
    drawProbeName(painter, xMap, yMap, canvasRect, d_ptr->probeName);
}

/**
 * @brief 绘制探针名称
 *
 * 绘制探针名称
 * @param painter 绘图对象
 * @param xMap X轴比例映射
 * @param yMap Y轴比例映射
 * @param canvasRect 画布矩形
 * @param name 探针名称
 */
void DA::DADataProbeMarker::drawProbeName(
    QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect, const QwtText& name
) const
{
    DA_DC(d);
    QPointF pos       = value();
    double x          = xMap.transform(pos.x());
    double y          = yMap.transform(pos.y());
    QSizeF textSize   = name.textSize();
    int offsetX       = 0;
    int offsetY       = 0;
    QSizeF borderSize = textSize + QSizeF(2 * offsetX, 2 * offsetY);
    QPointF labelPos;
    if (d->probeType == VerticalProbe) {
        if (d->labelPosition == LabelAtTop) {
            labelPos = QPointF(x - borderSize.width() / 2, canvasRect.top());
        } else {
            labelPos = QPointF(x - borderSize.width() / 2, canvasRect.bottom() - borderSize.height());
        }
    } else {
        if (d->labelPosition == LabelAtTop) {
            labelPos = QPointF(canvasRect.left(), y - borderSize.height() / 2);
        } else {
            labelPos = QPointF(canvasRect.right() - borderSize.width(), y - borderSize.height() / 2);
        }
    }

    labelPos.setX(qBound(canvasRect.left(), labelPos.x(), canvasRect.right() - borderSize.width()));
    labelPos.setY(qBound(canvasRect.top(), labelPos.y(), canvasRect.bottom() - borderSize.height()));
    QRectF drawTextRect(labelPos.x() + offsetX, labelPos.y() + offsetY, textSize.width(), textSize.height());
    name.draw(painter, drawTextRect);
}
/**
 * \if ENGLISH
 * @brief Get the RTTI value for this item
 * @return RTTI value (Rtti_DataProbeMarker)
 * \endif
 *
 * \if CHINESE
 * @brief 获取此项目的RTTI值
 * @return RTTI值 (Rtti_DataProbeMarker)
 * \endif
 */
int DADataProbeMarker::rtti() const
{
    return Rtti_DataProbeMarker;
}


/**
 * \if ENGLISH
 * @brief Pick Y values at specified X position
 * @param xValue X coordinate value
 * @param interpolate Whether to use interpolation
 * @return Number of captured data points
 * \endif
 *
 * \if CHINESE
 * @brief 在指定X位置拾取Y值
 * @param xValue X坐标值
 * @param interpolate 是否使用插值
 * @return 捕获的数据点数量
 * \endif
 */
int DADataProbeMarker::pickYValueAtPosition(double xValue, bool interpolate)
{
    QwtPlot* currentPlot = plot();
    if (!currentPlot) {
        return 0;
    }

    const QList< QwtPlot* > plotList = currentPlot->plotList();

    for (QwtPlot* oneplot : plotList) {
        const QwtPlotItemList& items = oneplot->itemList();
        for (QwtPlotItem* item : items) {
            if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
                QwtPlotCurve* curve = static_cast< QwtPlotCurve* >(item);
                if (!curve->isVisible() || curve->dataSize() == 0) {
                    continue;
                }

                const size_t curveSize = curve->dataSize();
                const QRectF br        = curve->boundingRect();

                if (xValue < br.left() || xValue > br.right()) {
                    continue;
                }

                size_t index =
                    qwtUpperSampleIndex< QPointF >(*curve->data(), xValue, [](const double x, const QPointF& pos) -> bool {
                        return (x < pos.x());
                    });

                if (index == curveSize) {
                    continue;
                }

                CapturedData cd;
                cd.item  = curve;
                cd.index = index;

                if (interpolate && curveSize > 2 && index > 0) {
                    const QPointF& p2 = curve->sample(index);
                    const QPointF& p1 = curve->sample(index - 1);
                    if (qFuzzyCompare(p1.x(), p2.x())) {
                        cd.point = p2;
                    } else {
                        double t = (xValue - p1.x()) / (p2.x() - p1.x());
                        cd.point.setX(xValue);
                        cd.point.setY(p1.y() + t * (p2.y() - p1.y()));
                    }
                } else {
                    cd.point = curve->sample(index);
                }

                d_ptr->capturedData.append(cd);
            }
        }
    }

    return d_ptr->capturedData.size();
}

/**
 * \if ENGLISH
 * @brief Pick X values at specified Y position
 * @param yValue Y coordinate value
 * @param interpolate Whether to use interpolation
 * @return Number of captured data points
 * \endif
 *
 * \if CHINESE
 * @brief 在指定Y位置拾取X值
 * @param yValue Y坐标值
 * @param interpolate 是否使用插值
 * @return 捕获的数据点数量
 * \endif
 */
int DADataProbeMarker::pickXValueAtPosition(double yValue, bool interpolate)
{
    QwtPlot* currentPlot = plot();
    if (!currentPlot) {
        return 0;
    }

    const QList< QwtPlot* > plotList = currentPlot->plotList();

    for (QwtPlot* oneplot : plotList) {
        const QwtPlotItemList& items = oneplot->itemList();
        for (QwtPlotItem* item : items) {
            if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
                QwtPlotCurve* curve = static_cast< QwtPlotCurve* >(item);
                if (!curve->isVisible() || curve->dataSize() == 0) {
                    continue;
                }

                const size_t curveSize = curve->dataSize();

                for (size_t i = 1; i < curveSize; ++i) {
                    const QPointF& p1 = curve->sample(i - 1);
                    const QPointF& p2 = curve->sample(i);

                    bool crosses = (p1.y() <= yValue && yValue <= p2.y()) || (p2.y() <= yValue && yValue <= p1.y());

                    if (crosses) {
                        CapturedData cd;
                        cd.item  = curve;
                        cd.index = i;

                        if (interpolate && !qFuzzyCompare(p1.y(), p2.y())) {
                            double t = (yValue - p1.y()) / (p2.y() - p1.y());
                            cd.point.setX(p1.x() + t * (p2.x() - p1.x()));
                            cd.point.setY(yValue);
                        } else {
                            cd.point = (qAbs(p1.y() - yValue) < qAbs(p2.y() - yValue)) ? p1 : p2;
                        }

                        d_ptr->capturedData.append(cd);
                    }
                }
            }
        }
    }

    return d_ptr->capturedData.size();
}

/**
 * \if ENGLISH
 * @brief Update the label text
 * \endif
 *
 * \if CHINESE
 * @brief 更新标签文本
 * \endif
 */
void DADataProbeMarker::updateLabel()
{
    setLabel(d_ptr->probeName);
}

/**
 * \if ENGLISH
 * @brief Update the line style based on probe type
 * \endif
 *
 * \if CHINESE
 * @brief 根据探针类型更新线条样式
 * \endif
 */
void DADataProbeMarker::updateLineStyle()
{
    if (d_ptr->probeType == VerticalProbe) {
        setLineStyle(QwtPlotMarker::VLine);
        if (d_ptr->labelPosition == LabelAtTop) {
            setLabelAlignment(Qt::AlignTop | Qt::AlignHCenter);
        } else {
            setLabelAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        }
    } else {
        setLineStyle(QwtPlotMarker::HLine);
        if (d_ptr->labelPosition == LabelAtTop) {
            setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        } else {
            setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
    }
}

/**
 * \if ENGLISH
 * @brief Format axis value using axis scale draw
 * @param value Value to format
 * @param axisId Axis ID
 * @return Formatted string
 * \endif
 *
 * \if CHINESE
 * @brief 使用坐标轴刻度绘制器格式化值
 * @param value 要格式化的值
 * @param axisId 坐标轴ID
 * @return 格式化后的字符串
 * \endif
 */
QString DADataProbeMarker::formatAxisValue(double value, int axisId) const
{
    QwtPlot* p = plot();
    if (!p) {
        return QString::number(value, 'f', 3);
    }

    const QwtScaleDraw* scaleDraw = p->axisScaleDraw(axisId);
    if (scaleDraw) {
        QwtText text = scaleDraw->label(value);
        return text.text();
    }

    return QString::number(value, 'f', 3);
}

}  // namespace DA
