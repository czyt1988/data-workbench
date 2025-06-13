#include "DAChartSerialize.h"
#include "DAChartUtil.h"
#include <cstring>
#include <QBuffer>
// qwt
#include "qwt_plot.h"
#include "qwt_symbol.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_widget.h"
#include "qwt_color_map.h"
#include "qwt_column_symbol.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_interval_symbol.h"
#include "qwt_plot_item.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_scaleitem.h"
#include "qwt_plot_legenditem.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_plot_graphicitem.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_plot_textlabel.h"
#include "qwt_plot_zoneitem.h"
#include "qwt_plot_vectorfield.h"
// DAChart
#include "DAChartPlotItemFactory.h"
///< 版本标示，每个序列化都应该带有版本信息，用于对下兼容
const int gc_dachart_version                     = 1;
const std::uint32_t gc_dachart_magic_mark        = 0x5A6B4CF1;
const std::uint32_t gc_dachart_magic_mark2       = 0xAA123456;
const std::uint32_t gc_dachart_magic_mark3       = 0x12345678;
const std::uint32_t gc_dachart_magic_mark4       = 0xAAB23498;
const QDataStream::Version gc_datastream_version = QDataStream::Qt_5_12;
namespace DA
{
void serialize_out_scale_widge(QDataStream& out, const QwtPlot* chart, int axis);
void serialize_in_scale_widge(QDataStream& in, QwtPlot* chart, int axis);

DABadSerializeExpection::DABadSerializeExpection()
{
    mWhy = QObject::tr("serialize error");
}

DABadSerializeExpection::DABadSerializeExpection(const char* why)
{
    mWhy = why;
}

DABadSerializeExpection::DABadSerializeExpection(const QString& why)
{
    mWhy = why;
}

DABadSerializeExpection::~DABadSerializeExpection()
{
}

const char* DABadSerializeExpection::what() const noexcept
{
    return mWhy.toStdString().c_str();
}

void serialize_out_scale_widge(QDataStream& out, const QwtPlot* chart, int axis)
{
    const QwtScaleWidget* axisWid = chart->axisWidget(axis);
    bool isaxis                   = (nullptr != axisWid);
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << isaxis;
    out << gc_dachart_magic_mark2;
    if (isaxis) {
        bool enable = chart->axisEnabled(axis);
        out << axisWid << enable << gc_dachart_magic_mark3;
    }
}

void serialize_in_scale_widge(QDataStream& in, QwtPlot* chart, int axis)
{
    bool isaxis;
    std::uint32_t H;
    std::uint32_t B;
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return;
    }
    in >> isaxis;
    in >> H;
    if (gc_dachart_magic_mark2 != H) {
        throw DABadSerializeExpection();
        return;
    }
    if (isaxis) {
        QwtScaleWidget* axisWid = chart->axisWidget(axis);
        if (nullptr == axisWid) {
            chart->enableAxis(axis);
            axisWid = chart->axisWidget(axis);
        }
        if (nullptr == axisWid) {
            QwtScaleWidget w;
            QwtScaleWidget* pw = &w;
            in >> pw;
            return;
        }
        bool enable;
        in >> axisWid >> enable;
        in >> B;
        if (gc_dachart_magic_mark3 != B) {
            throw DABadSerializeExpection();
            return;
        }
        chart->enableAxis(axis, enable);
    }
}

QDataStream& operator<<(QDataStream& out, const DAChartItemSerialize::Header& f)
{
    out << f.magic << f.version << f.rtti << f.size;
    out.writeRawData(reinterpret_cast< const char* >(f.byte), sizeof(f.byte));
}

QDataStream& operator>>(QDataStream& in, DAChartItemSerialize::Header& f)
{
    in >> f.magic >> f.version;
    in >> f.rtti >> f.size;
    in.readRawData(reinterpret_cast< char* >(f.byte), sizeof(f.byte));
}

QDataStream& operator<<(QDataStream& out, const QwtText& t)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << gc_dachart_magic_mark2 << t.text() << t.font() << t.renderFlags() << t.color() << t.borderRadius()
        << t.borderPen() << t.backgroundBrush() << t.testPaintAttribute(QwtText::PaintUsingTextFont)
        << t.testPaintAttribute(QwtText::PaintUsingTextColor) << t.testPaintAttribute(QwtText::PaintBackground)
        << t.testLayoutAttribute(QwtText::MinimumLayout);
    return out;
}

QDataStream& operator>>(QDataStream& in, QwtText& t)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    std::uint32_t tmp;
    in >> tmp;
    if (gc_dachart_magic_mark2 != tmp) {
        throw DABadSerializeExpection();
        return in;
    }
    QString str;
    QFont font;
    int renderFlags;
    QColor clr;
    double borderRadius;
    QPen borderPen;
    QBrush backgroundBrush;
    bool isPaintUsingTextFont, isPaintUsingTextColor, isPaintBackground;
    bool isMinimumLayout;
    in >> str >> font >> renderFlags >> clr >> borderRadius >> borderPen >> backgroundBrush >> isPaintUsingTextFont
        >> isPaintUsingTextColor >> isPaintBackground >> isMinimumLayout;
    t.setText(str);
    t.setFont(font);
    t.setRenderFlags(renderFlags);
    t.setColor(clr);
    t.setBorderRadius(borderRadius);
    t.setBorderPen(borderPen);
    t.setBackgroundBrush(backgroundBrush);
    t.setPaintAttribute(QwtText::PaintUsingTextFont, isPaintUsingTextFont);
    t.setPaintAttribute(QwtText::PaintUsingTextColor, isPaintUsingTextColor);
    t.setPaintAttribute(QwtText::PaintBackground, isPaintBackground);
    t.setLayoutAttribute(QwtText::MinimumLayout, isMinimumLayout);
    return in;
}
///
/// \brief QwtSymbol 指针的序列化
/// \param out
/// \param t
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtSymbol* t)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << static_cast< int >(t->cachePolicy()) << t->size() << t->pinPoint() << t->isPinPointEnabled() << t->brush()
        << t->pen() << static_cast< int >(t->style()) << t->path() << t->pixmap();
    return out;
}
///
/// \brief QwtSymbol 指针的序列化
/// \param in
/// \param t
/// \return
///
QDataStream& operator>>(QDataStream& in, QwtSymbol* t)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    int cachePolicy;
    QSize size;
    QPointF pinPoint;
    bool isPinPointEnabled;
    QBrush brush;
    QPen pen;
    int style;
    QPainterPath path;
    QPixmap pixmap;
    in >> cachePolicy >> size >> pinPoint >> isPinPointEnabled >> brush >> pen >> style >> path >> pixmap;
    t->setCachePolicy(static_cast< QwtSymbol::CachePolicy >(cachePolicy));
    t->setSize(size);
    t->setPinPoint(pinPoint);
    t->setPinPointEnabled(isPinPointEnabled);
    t->setStyle(static_cast< QwtSymbol::Style >(style));
    if (!path.isEmpty()) {
        t->setPath(path);
    }
    t->setBrush(brush);
    t->setPen(pen);
    if (!pixmap.isNull()) {
        t->setPixmap(pixmap);
    }
    return in;
}

QDataStream& operator<<(QDataStream& out, const QwtIntervalSymbol* t)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << static_cast< int >(t->width()) << t->brush() << t->pen() << static_cast< int >(t->style());
    return out;
}

QDataStream& operator>>(QDataStream& in, QwtIntervalSymbol* t)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    int width;
    QBrush brush;
    QPen pen;
    int style;
    in >> width >> brush >> pen >> style;
    t->setWidth(width);
    t->setBrush(brush);
    t->setPen(pen);
    t->setStyle(static_cast< QwtIntervalSymbol::Style >(style));
    return in;
}

///
/// \brief QwtColumnSymbol的序列化
/// \param out
/// \param t
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtColumnSymbol* t)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << static_cast< int >(t->frameStyle()) << t->pen() << t->brush() << static_cast< int >(t->style());
    return out;
}
///
/// \brief QwtColumnSymbol的序列化
/// \param in
/// \param t
/// \return
///
QDataStream& operator>>(QDataStream& in, QwtColumnSymbol* t)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    int frameStyle;
    QPen pen;
    QBrush brush;
    int style;
    in >> frameStyle >> pen >> brush >> style;
    t->setFrameStyle(static_cast< QwtColumnSymbol::FrameStyle >(frameStyle));
    t->setPen(pen);
    t->setBrush(brush);
    t->setStyle(static_cast< QwtColumnSymbol::Style >(style));
    return in;
}

/**
 * @brief QwtPlotItem序列化
 *
 * 这是一个接口类的序列化，正常用户不应该使用到此函数
 * @param out
 * @param item
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtPlotItem* item)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << item->rtti();  // 写入rtti是为了可以识别是什么类型的item
    out << item->title() << item->z() << item->isVisible() << item->xAxis() << item->yAxis() << item->legendIconSize()
        << item->testItemAttribute(QwtPlotItem::Legend) << item->testItemAttribute(QwtPlotItem::AutoScale)
        << item->testItemAttribute(QwtPlotItem::Margins) << item->testItemInterest(QwtPlotItem::ScaleInterest)
        << item->testItemInterest(QwtPlotItem::LegendInterest) << item->testRenderHint(QwtPlotItem::RenderAntialiased)
        << item->renderThreadCount();
    return out;
}

/**
 * @brief QwtPlotItem序列化
 *
 *  这是一个接口类的序列化，正常用户不应该使用到此函数
 * @param in
 * @param item
 * @return
 */
QDataStream& operator>>(QDataStream& in, QwtPlotItem* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    int rtti;
    in >> rtti;
    QwtText title;
    double z;
    bool isVisible;
    int xaxis, yaxis;
    QSize legendIconSize;
    bool isLegend, isAutoScale, isMargins;
    bool isScaleInterest, isLegendInterest;
    bool isRenderAntialiased;
    uint renderThreadCount;
    in >> title >> z >> isVisible >> xaxis >> yaxis >> legendIconSize >> isLegend >> isAutoScale >> isMargins
        >> isScaleInterest >> isLegendInterest >> isRenderAntialiased >> renderThreadCount;
    item->setTitle(title);
    item->setZ(z);
    item->setVisible(isVisible);
    item->setAxes(xaxis, yaxis);
    item->setLegendIconSize(legendIconSize);
    item->setItemAttribute(QwtPlotItem::Legend, isLegend);
    item->setItemAttribute(QwtPlotItem::AutoScale, isAutoScale);
    item->setItemAttribute(QwtPlotItem::Margins, isMargins);
    item->setItemInterest(QwtPlotItem::ScaleInterest, isScaleInterest);
    item->setItemInterest(QwtPlotItem::LegendInterest, isLegendInterest);
    item->setRenderHint(QwtPlotItem::RenderAntialiased, isRenderAntialiased);
    item->setRenderThreadCount(renderThreadCount);
    return in;
}

///
/// \brief QwtPlotCurve指针的序列化
/// \param out
/// \param item
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtPlotCurve* item)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << (const QwtPlotItem*)item;
    out << item->baseline() << item->brush() << item->pen() << static_cast< int >(item->style())
        << item->testPaintAttribute(QwtPlotCurve::ClipPolygons) << item->testPaintAttribute(QwtPlotCurve::FilterPoints)
        << item->testPaintAttribute(QwtPlotCurve::MinimizeMemory) << item->testPaintAttribute(QwtPlotCurve::ImageBuffer)
        << item->testLegendAttribute(QwtPlotCurve::LegendNoAttribute)
        << item->testLegendAttribute(QwtPlotCurve::LegendShowLine)
        << item->testLegendAttribute(QwtPlotCurve::LegendShowSymbol)
        << item->testLegendAttribute(QwtPlotCurve::LegendShowBrush) << item->testCurveAttribute(QwtPlotCurve::Inverted)
        << item->testCurveAttribute(QwtPlotCurve::Fitted) << (int)(item->orientation());
    // save sample
    QVector< QPointF > sample;
    DAChartUtil::getXYDatas(sample, item);
    out << gc_dachart_magic_mark2 << sample << gc_dachart_magic_mark3;
    // QwtSymbol的序列化
    const QwtSymbol* symbol = item->symbol();
    bool isHaveSymbol       = (symbol != nullptr);
    out << isHaveSymbol;
    if (isHaveSymbol) {
        out << symbol;
    }
    return out;
}
///
/// \brief QwtPlotCurve指针的序列化
/// \param in
/// \param item
/// \return
///
QDataStream& operator>>(QDataStream& in, QwtPlotCurve* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    double baseline;
    QBrush brush;
    QPen pen;
    int style;
    bool isClipPolygons;
    bool isFilterPoints;
    bool isMinimizeMemory;
    bool isImageBuffer;
    bool isLegendNoAttribute;
    bool isLegendShowLine;
    bool isLegendShowSymbol;
    bool isLegendShowBrush;
    bool isInverted;
    bool isFitted;
    int orientation;
    in >> (QwtPlotItem*)item;
    in >> baseline >> brush >> pen >> style >> isClipPolygons >> isFilterPoints >> isMinimizeMemory >> isImageBuffer
        >> isLegendNoAttribute >> isLegendShowLine >> isLegendShowSymbol >> isLegendShowBrush >> isInverted >> isFitted
        >> orientation;
    item->setBaseline(baseline);
    item->setBrush(brush);
    item->setPen(pen);
    item->setStyle(static_cast< QwtPlotCurve::CurveStyle >(style));
    item->setPaintAttribute(QwtPlotCurve::ClipPolygons, isClipPolygons);
    item->setPaintAttribute(QwtPlotCurve::FilterPoints, isFilterPoints);
    item->setPaintAttribute(QwtPlotCurve::MinimizeMemory, isMinimizeMemory);
    item->setPaintAttribute(QwtPlotCurve::ImageBuffer, isImageBuffer);
    item->setLegendAttribute(QwtPlotCurve::LegendNoAttribute, isLegendNoAttribute);
    item->setLegendAttribute(QwtPlotCurve::LegendShowLine, isLegendShowLine);
    item->setLegendAttribute(QwtPlotCurve::LegendShowSymbol, isLegendShowSymbol);
    item->setLegendAttribute(QwtPlotCurve::LegendShowBrush, isLegendShowBrush);
    item->setCurveAttribute(QwtPlotCurve::Inverted, isInverted);
    item->setCurveAttribute(QwtPlotCurve::Fitted, isFitted);
    item->setOrientation((Qt::Orientation)orientation);
    // load sample
    std::uint32_t tmp0, tmp1;
    QVector< QPointF > sample;
    in >> tmp0;
    if (gc_dachart_magic_mark2 != tmp0) {
        throw DABadSerializeExpection();
        return in;
    }
    in >> sample >> tmp1;
    if (gc_dachart_magic_mark3 != tmp1) {
        throw DABadSerializeExpection();
        return in;
    }
    item->setSamples(sample);
    // QwtSymbol的序列化
    bool isHaveSymbol;
    in >> isHaveSymbol;
    if (isHaveSymbol) {
        QwtSymbol* symbol = new QwtSymbol();
        in >> symbol;
        item->setSymbol(symbol);
    }
    return in;
}

/**
 * @brief QwtPlotGrid指针的序列化
 * @param out
 * @param item
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtPlotGrid* item)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << static_cast< const QwtPlotItem* >(item);
    out << item->majorPen() << item->minorPen() << item->xEnabled() << item->yEnabled() << item->xMinEnabled()
        << item->yMinEnabled();
    return out;
}

QDataStream& operator>>(QDataStream& in, QwtPlotGrid* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    QPen majorPen, minorPen;
    bool xEnabled, yEnabled, xMinEnabled, yMinEnabled;
    in >> majorPen >> minorPen >> xEnabled >> yEnabled >> xMinEnabled >> yMinEnabled;
    item->setMajorPen(majorPen);
    item->setMinorPen(minorPen);
    item->enableX(xEnabled);
    item->enableY(yEnabled);
    item->enableXMin(xMinEnabled);
    item->enableYMin(yMinEnabled);
    return in;
}

/**
 * @brief  QwtPlotScaleItem(Rtti_PlotScale)指针的序列化
 * @param out
 * @param item
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtPlotScaleItem* item)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << static_cast< const QwtPlotItem* >(item);
    out << item->borderDistance() << item->font() << item->isScaleDivFromAxis() << item->palette() << item->position();
    return out;
}
QDataStream& operator>>(QDataStream& in, QwtPlotScaleItem* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    int borderDistance;
    QFont font;
    bool isScaleDivFromAxis;
    QPalette palette;
    double position;
    in >> borderDistance >> font >> isScaleDivFromAxis >> palette >> position;
    item->setBorderDistance(borderDistance);
    item->setFont(font);
    item->setScaleDivFromAxis(isScaleDivFromAxis);
    item->setPalette(palette);
    item->setPosition(position);
    return in;
}

/**
 * @brief QwtPlotLegendItem(Rtti_PlotLegend)指针的序列化
 * @param out
 * @param item
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtPlotLegendItem* item)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << static_cast< const QwtPlotItem* >(item);
    out << item->alignmentInCanvas() << item->backgroundBrush() << static_cast< int >(item->backgroundMode())
        << item->borderPen() << item->borderRadius() << item->font() << item->offsetInCanvas(Qt::Horizontal)
        << item->offsetInCanvas(Qt::Vertical) << item->maxColumns() << item->margin() << item->spacing()
        << item->itemMargin() << item->itemSpacing() << item->textPen();
    return out;
}
QDataStream& operator>>(QDataStream& in, QwtPlotLegendItem* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    Qt::Alignment alignmentInCanvas;
    QBrush backgroundBrush;
    int backgroundMode;
    QPen borderPen, textPen;
    double borderRadius;
    QFont font;
    uint maxColumns;
    int offsetInCanvasHorizontal, offsetInCanvasVertical, margin, spacing, itemMargin, itemSpacing;
    in >> alignmentInCanvas >> backgroundBrush >> backgroundMode >> borderPen >> borderRadius >> font
        >> offsetInCanvasHorizontal >> offsetInCanvasVertical >> maxColumns >> margin >> spacing >> itemMargin
        >> itemSpacing >> textPen;
    item->setAlignmentInCanvas(alignmentInCanvas);
    item->setBackgroundBrush(backgroundBrush);
    item->setBackgroundMode(static_cast< QwtPlotLegendItem::BackgroundMode >(backgroundMode));
    item->setBorderPen(borderPen);
    item->setBorderRadius(borderRadius);
    item->setFont(font);
    item->setOffsetInCanvas(Qt::Horizontal, offsetInCanvasHorizontal);
    item->setOffsetInCanvas(Qt::Vertical, offsetInCanvasVertical);
    item->setMaxColumns(maxColumns);
    item->setMargin(margin);
    item->setSpacing(spacing);
    item->setItemMargin(itemMargin);
    item->setItemSpacing(itemSpacing);
    item->setTextPen(textPen);
    return in;
}

/**
 * @brief QwtPlotMarker(Rtti_PlotMarker)指针的序列化
 * @param out
 * @param item
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtPlotMarker* item)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << static_cast< const QwtPlotItem* >(item);
    out << item->xValue() << item->yValue() << static_cast< int >(item->lineStyle()) << item->linePen() << item->label()
        << static_cast< int >(item->labelAlignment()) << static_cast< int >(item->labelOrientation()) << item->spacing();
    const QwtSymbol* symbol = item->symbol();
    bool isHaveSymbol       = (symbol != nullptr);
    out << isHaveSymbol;
    if (isHaveSymbol) {
        out << symbol;
    }
    return out;
}
QDataStream& operator>>(QDataStream& in, QwtPlotMarker* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    double xValue, yValue;
    int lineStyle, labelAlignment, labelOrientation, spacing;
    QPen linePen;
    QwtText label;
    in >> xValue >> yValue >> lineStyle >> linePen >> label >> labelAlignment >> labelOrientation >> spacing;
    item->setXValue(xValue);
    item->setYValue(yValue);
    item->setLineStyle(static_cast< QwtPlotMarker::LineStyle >(lineStyle));
    item->setLinePen(linePen);
    item->setLabel(label);
    item->setLabelAlignment(static_cast< Qt::Alignment >(labelAlignment));
    item->setLabelOrientation(static_cast< Qt::Orientation >(labelOrientation));
    item->setSpacing(spacing);
    // QwtSymbol的序列化
    bool isHaveSymbol;
    in >> isHaveSymbol;
    if (isHaveSymbol) {
        QwtSymbol* symbol = new QwtSymbol();
        in >> symbol;
        item->setSymbol(symbol);
    }
    return in;
}

/**
 * @brief QwtPlotSpectroCurve(Rtti_PlotSpectroCurve)指针的序列化
 * @param out
 * @param item
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtPlotSpectroCurve* item)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << static_cast< const QwtPlotItem* >(item);
    out << item->penWidth() << static_cast< int >(item->orientation())
        << item->testPaintAttribute(QwtPlotSpectroCurve::ClipPoints);
    return out;
}
QDataStream& operator>>(QDataStream& in, QwtPlotSpectroCurve* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    int orientation;
    double penWidth;
    bool attClipPoints;
    in >> penWidth >> orientation >> attClipPoints;
    item->setPenWidth(penWidth);
    item->setOrientation(static_cast< Qt::Orientation >(orientation));
    item->setPaintAttribute(QwtPlotSpectroCurve::ClipPoints, attClipPoints);
    return in;
}
///
/// \brief QwtPlotBarChart指针的序列化
/// \param out
/// \param item
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtPlotBarChart* item)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << (const QwtPlotItem*)item;
    out << static_cast< int >(item->layoutPolicy()) << item->layoutHint() << item->spacing() << item->margin()
        << item->baseline() << static_cast< int >(item->legendMode()) << (int)(item->orientation());
    // save sample
    QVector< QPointF > sample;
    DAChartUtil::getXYDatas(sample, item);
    out << gc_dachart_magic_mark2 << sample << gc_dachart_magic_mark3;
    // Symbol
    const QwtColumnSymbol* cs = item->symbol();
    bool isColumnSymbol       = (cs != nullptr);
    out << isColumnSymbol;
    if (isColumnSymbol) {
        out << cs;
    }
    return out;
}
///
/// \brief QwtPlotBarChart指针的序列化
/// \param in
/// \param item
/// \return
///
QDataStream& operator>>(QDataStream& in, QwtPlotBarChart* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
    int layoutPolicy;
    double layoutHint;
    int spacing;
    int margin;
    double baseline;
    int legendMode;
    int orientation;
    in >> layoutPolicy >> layoutHint >> spacing >> margin >> baseline >> legendMode >> orientation;
    item->setLayoutPolicy(static_cast< QwtPlotBarChart::LayoutPolicy >(layoutPolicy));
    item->setLayoutHint(layoutHint);
    item->setSpacing(spacing);
    item->setMargin(margin);
    item->setBaseline(baseline);
    item->setLegendMode(static_cast< QwtPlotBarChart::LegendMode >(legendMode));
    item->setOrientation((Qt::Orientation)orientation);
    // load sample
    std::uint32_t tmp0, tmp1;
    in >> tmp0;
    if (gc_dachart_magic_mark2 != tmp0) {
        throw DABadSerializeExpection();
        return in;
    }
    QVector< QPointF > sample;
    in >> sample >> tmp1;
    if (gc_dachart_magic_mark3 != tmp1) {
        throw DABadSerializeExpection();
        return in;
    }
    item->setSamples(sample);
    // Symbol
    bool isColumnSymbol;
    in >> isColumnSymbol;
    if (isColumnSymbol) {
        QwtColumnSymbol* cs = new QwtColumnSymbol;
        in >> cs;
        item->setSymbol(cs);
    }
    return in;
}

QDataStream& operator<<(QDataStream& out, const QwtPlotIntervalCurve* item)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << (const QwtPlotItem*)item;
    out << (int)item->orientation() << item->pen() << item->brush() << (int)item->style()
        << item->testPaintAttribute(QwtPlotIntervalCurve::ClipPolygons)
        << item->testPaintAttribute(QwtPlotIntervalCurve::ClipSymbol);
    // save sample
    const unsigned int ck0 = 0xabf31f;
    const unsigned int ck1 = 0x9f6fda;
    QVector< QwtIntervalSample > sample;
    DAChartUtil::getSeriesData(sample, item);
    out << ck0 << sample << ck1;
    // QwtSymbol的序列化
    const QwtIntervalSymbol* symbol = item->symbol();
    bool isHaveSymbol               = (symbol != nullptr);
    out << isHaveSymbol;
    if (isHaveSymbol) {
        out << symbol;
    }
    return out;
}

QDataStream& operator>>(QDataStream& in, QwtPlotIntervalCurve* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
    int orientation;
    QBrush brush;
    QPen pen;
    int style;
    bool isClipPolygons;
    bool isClipSymbol;
    in >> orientation >> pen >> brush >> style >> isClipPolygons >> isClipSymbol;
    item->setPen(pen);
    item->setBrush(brush);
    item->setStyle(static_cast< QwtPlotIntervalCurve::CurveStyle >(style));
    item->setPaintAttribute(QwtPlotIntervalCurve::ClipPolygons, isClipPolygons);
    item->setPaintAttribute(QwtPlotIntervalCurve::ClipSymbol, isClipSymbol);
    // load sample
    const unsigned int ck0 = 0xabf31f;
    const unsigned int ck1 = 0x9f6fda;
    unsigned int tmp0, tmp1;
    QVector< QwtIntervalSample > sample;
    in >> tmp0;
    if (ck0 != tmp0) {
        throw DABadSerializeExpection();
        return in;
    }
    in >> sample >> tmp1;
    if (ck1 != tmp1) {
        throw DABadSerializeExpection();
        return in;
    }
    item->setSamples(sample);
    // QwtSymbol的序列化
    bool isHaveSymbol;
    in >> isHaveSymbol;
    if (isHaveSymbol) {
        QwtIntervalSymbol* symbol = new QwtIntervalSymbol();
        in >> symbol;
        item->setSymbol(symbol);
    }
    return in;
}

///
/// \brief QwtScaleWidget指针的序列化
/// \param out
/// \param w
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtScaleWidget* w)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    unsigned int c0 = 0x82fa34;
    out << c0;
    int minBorderDistStart, minBorderDistEnd;
    w->getMinBorderDist(minBorderDistStart, minBorderDistEnd);
    out << w->title() << w->testLayoutFlag(QwtScaleWidget::TitleInverted) << w->startBorderDist() << w->endBorderDist()
        << minBorderDistStart << minBorderDistEnd << w->margin() << w->spacing() << w->isColorBarEnabled()
        << w->colorBarWidth() << static_cast< int >(w->alignment()) << w->colorBarInterval();
    // QwtScaleDraw
    const QwtScaleDraw* sd = w->scaleDraw();
    bool isSD              = (sd != nullptr);
    if (isSD) {
        out << isSD << sd;
    }
    return out;
}

///
/// \brief QwtScaleWidget指针的序列化
/// \param in
/// \param w
/// \return
///
QDataStream& operator>>(QDataStream& in, QwtScaleWidget* w)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    unsigned int c0 = 0x82fa34;
    unsigned int tmp;
    in >> tmp;
    if (c0 != tmp) {
        throw DABadSerializeExpection();
        return in;
    }
    QwtText title;
    bool isTitleInverted;
    int startBorderDist, endBorderDist;
    int minBorderDistStart, minBorderDistEnd;
    int margin, spacing;
    bool isColorBarEnabled;
    int colorBarWidth;
    int alignment;
    QwtInterval colorBarInterval;
    in >> title >> isTitleInverted >> startBorderDist >> endBorderDist >> minBorderDistStart >> minBorderDistEnd
        >> margin >> spacing >> isColorBarEnabled >> colorBarWidth >> alignment >> colorBarInterval;
    w->setTitle(title);
    w->setLayoutFlag(QwtScaleWidget::TitleInverted, isTitleInverted);
    w->setBorderDist(startBorderDist, endBorderDist);
    w->setMinBorderDist(minBorderDistStart, minBorderDistEnd);
    w->setMargin(margin);
    w->setSpacing(spacing);
    w->setColorBarEnabled(isColorBarEnabled);
    w->setColorBarWidth(colorBarWidth);
    w->setAlignment(static_cast< QwtScaleDraw::Alignment >(alignment));
    // QwtScaleDraw
    bool isSD;
    in >> isSD;
    if (isSD) {
        QwtScaleDraw* sd = w->scaleDraw();
        if (nullptr == sd) {
            w->setScaleDraw(new QwtScaleDraw());
            sd = w->scaleDraw();
        }
        in >> sd;
    }
    return in;
}
///
/// \brief  QwtScaleDraw指针的序列化
/// \param out
/// \param w
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtScaleDraw* w)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    unsigned int c0 = 0x92fa34;
    out << c0;
    out << static_cast< int >(w->alignment()) << w->length() << static_cast< int >(w->labelAlignment())
        << w->labelRotation();
    return out;
}
///
/// \brief  QwtScaleDraw指针的序列化
/// \param in
/// \param w
/// \return
///
QDataStream& operator>>(QDataStream& in, QwtScaleDraw* w)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    unsigned int c0 = 0x92fa34;
    unsigned int tmp;
    in >> tmp;
    if (c0 != tmp) {
        throw DABadSerializeExpection();
        return in;
    }
    int alignment;
    double length;
    int labelAlignment;
    double labelRotation;
    in >> alignment >> length >> labelAlignment >> labelRotation;
    w->setAlignment(static_cast< QwtScaleDraw::Alignment >(alignment));
    w->setLength(length);
    w->setLabelAlignment(static_cast< Qt::Alignment >(labelAlignment));
    w->setLabelRotation(labelRotation);

    return in;
}
///
/// \brief QFrame的序列化
/// \param out
/// \param f
/// \return
///
QDataStream& operator<<(QDataStream& out, const QFrame* f)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    unsigned int c0 = 0x13fa34;
    out << c0;
    out << static_cast< int >(f->frameShadow()) << static_cast< int >(f->frameShape()) << f->frameStyle()
        << f->lineWidth() << f->midLineWidth() << f->palette() << f->styleSheet();
    return out;
}
///
/// \brief QFrame的序列化
/// \param in
/// \param f
/// \return
///
QDataStream& operator>>(QDataStream& in, QFrame* f)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    unsigned int c0 = 0x13fa34;
    unsigned int tmp;
    in >> tmp;
    if (c0 != tmp) {
        throw DABadSerializeExpection();
        return in;
    }
    int frameShadow;
    int frameShape;
    int frameStyle;
    int lineWidth;
    int midLineWidth;
    QPalette pl;
    QString styleSheet;
    in >> frameShadow >> frameShape >> frameStyle >> lineWidth >> midLineWidth >> pl >> styleSheet;
    f->setFrameShadow(static_cast< QFrame::Shadow >(frameShadow));
    f->setFrameShape(static_cast< QFrame::Shape >(frameShadow));
    f->setFrameStyle(frameStyle);
    f->setLineWidth(lineWidth);
    f->setMidLineWidth(midLineWidth);
    f->setPalette(pl);
    if (!styleSheet.isEmpty()) {
        f->setStyleSheet(styleSheet);
    }
    return in;
}

///
/// \brief QwtPlotCanvas的序列化
/// \param out
/// \param c
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtPlotCanvas* c)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << (const QFrame*)c;
    out << static_cast< int >(c->focusIndicator()) << c->borderRadius()
        << c->testPaintAttribute(QwtPlotCanvas::BackingStore) << c->testPaintAttribute(QwtPlotCanvas::Opaque)
        << c->testPaintAttribute(QwtPlotCanvas::HackStyledBackground)
        << c->testPaintAttribute(QwtPlotCanvas::ImmediatePaint);
    return out;
}
///
/// \brief QwtPlotCanvas的序列化
/// \param in
/// \param c
/// \return
///
QDataStream& operator>>(QDataStream& in, QwtPlotCanvas* c)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    in >> (QFrame*)c;
    int focusIndicator;
    double borderRadius;
    bool isBackingStore, isOpaque, isHackStyledBackground, isImmediatePaint;
    in >> focusIndicator >> borderRadius >> isBackingStore >> isOpaque >> isHackStyledBackground >> isImmediatePaint;
    c->setFocusIndicator(static_cast< QwtPlotCanvas::FocusIndicator >(focusIndicator));
    c->setBorderRadius(borderRadius);
    c->setPaintAttribute(QwtPlotCanvas::BackingStore, isBackingStore);
    c->setPaintAttribute(QwtPlotCanvas::Opaque, isOpaque);
    c->setPaintAttribute(QwtPlotCanvas::HackStyledBackground, isHackStyledBackground);
    c->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, isImmediatePaint);
    return in;
}

///
/// \brief SAChart2D的序列化
/// \param out
/// \param chart
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtPlot* chart)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    // QFrame save
    out << (const QFrame*)chart;
    // QwtPlot save
    out << chart->title() << chart->footer() << chart->palette();
    // axis
    serialize_out_scale_widge(out, chart, QwtPlot::xTop);
    serialize_out_scale_widge(out, chart, QwtPlot::xBottom);
    serialize_out_scale_widge(out, chart, QwtPlot::yLeft);
    serialize_out_scale_widge(out, chart, QwtPlot::yRight);
    // QwtPlotCanvas save
    const QwtPlotCanvas* canvas = qobject_cast< const QwtPlotCanvas* >(chart->canvas());
    bool isDefaultCanvas        = (canvas != nullptr);
    out << isDefaultCanvas;
    if (isDefaultCanvas) {
        out << canvas;
    }
    return out;
}
///
/// \brief SAChart2D的序列化
/// \param in
/// \param chart
/// \return
///
QDataStream& operator>>(QDataStream& in, QwtPlot* chart)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    // QFrame load
    in >> (QFrame*)chart;
    // QwtPlot load
    QwtText title;
    QwtText footer;
    QPalette pl;
    in >> title >> footer >> pl;
    chart->setTitle(title);
    chart->setFooter(footer);
    chart->setPalette(pl);
    // axis
    serialize_in_scale_widge(in, chart, QwtPlot::xTop);
    serialize_in_scale_widge(in, chart, QwtPlot::xBottom);
    serialize_in_scale_widge(in, chart, QwtPlot::yLeft);
    serialize_in_scale_widge(in, chart, QwtPlot::yRight);
    // QwtPlotCanvas load
    bool isDefaultCanvas;
    in >> isDefaultCanvas;
    if (isDefaultCanvas) {
        QwtPlotCanvas* canvas = qobject_cast< QwtPlotCanvas* >(chart->canvas());
        if (nullptr == canvas) {
            canvas = new QwtPlotCanvas(chart);
            chart->setCanvas(canvas);
        }
        in >> canvas;
    }
    return in;
}

/**
 * @brief QwtColorMap的序列化
 *
 * 这是一个接口类的序列化，正常用户不应该使用到此函数
 * @param out
 * @param c
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtColorMap* c)
{
    out << gc_dachart_version << gc_dachart_magic_mark;
    out << static_cast< int >(c->format());
    return out;
}

/**
 * @brief QwtColorMap的序列化
 *
 * 这是一个接口类的序列化，正常用户不应该使用到此函数
 * @param in
 * @param c
 * @return
 */
QDataStream& operator>>(QDataStream& in, QwtColorMap* c)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    int format;
    in >> format;
    c->setFormat(static_cast< QwtColorMap::Format >(format));
    return in;
}

/**
 * @brief QwtLinearColorMap的序列化
 * @param out
 * @param c
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtLinearColorMap* c)
{
    out << gc_dachart_version << gc_dachart_magic_mark2;
    out << static_cast< const QwtColorMap* >(c);
    out << static_cast< int >(c->mode());
    // color1 对应colorstop:0.0,color2 对应colorstop:1.0
    // 写入颜色停止点的数量
    const QVector< double > colorStops = c->colorStops();
    out << colorStops;
    // 写入停止点对应的颜色
    for (const double& v : colorStops) { }
    return out;
}
/**
 * @brief QwtLinearColorMap的序列化
 * @param in
 * @param c
 * @return
 */
QDataStream& operator>>(QDataStream& in, QwtLinearColorMap* c)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (gc_dachart_magic_mark2 != magic) {
        throw DABadSerializeExpection();
        return in;
    }
    int mode;
    QColor color1, color2;
    in >> static_cast< QwtColorMap* >(c) >> mode >> color1 >> color2;
    c->setMode(static_cast< QwtLinearColorMap::Mode >(mode));
    c->setColorInterval(color1, color2);
    return in;
}

///
/// \brief QwtIntervalSample序列化支持
/// \param out
/// \param item
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtIntervalSample& item)
{
    out << item.value << item.interval;
    return out;
}
///
/// \brief QwtIntervalSample序列化支持
/// \param out
/// \param item
/// \return
///
QDataStream& operator>>(QDataStream& in, QwtIntervalSample& item)
{
    in >> item.value >> item.interval;
    return in;
}
///
/// \brief QwtInterval序列化支持
/// \param out
/// \param item
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtInterval& item)
{
    out << item.minValue() << item.maxValue() << item.borderFlags();
    return out;
}
///
/// \brief QwtInterval序列化支持
/// \param out
/// \param item
/// \return
///
QDataStream& operator>>(QDataStream& in, QwtInterval& item)
{
    int flag;
    double min, max;
    in >> min >> max >> flag;
    item.setMinValue(min);
    item.setMaxValue(max);
    item.setBorderFlags(static_cast< QwtInterval::BorderFlags >(flag));
    return in;
}
//----------------------------------------------------
// Header
//----------------------------------------------------

DAChartItemSerialize::Header::Header() : magic(gc_dachart_magic_mark4), version(1), rtti(QwtPlotItem::Rtti_PlotItem)
{
    memset(byte, 0, sizeof(byte));
}

bool DAChartItemSerialize::Header::isValid() const
{
    return gc_dachart_magic_mark4 == magic;
}

DAChartItemSerialize::DAChartItemSerialize()
{
}

DAChartItemSerialize::~DAChartItemSerialize()
{
}

void DAChartItemSerialize::registSerializeFun(int rtti,
                                              DAChartItemSerialize::FpSerializeIn fpIn,
                                              DAChartItemSerialize::FpSerializeOut fpOut)
{
    serializeFun()[ rtti ] = std::make_pair(fpIn, fpOut);
}

bool DAChartItemSerialize::isSupportSerialize(int rtti)
{
    return serializeFun().contains(rtti);
}

DAChartItemSerialize::FpSerializeIn DAChartItemSerialize::getSerializeInFun(int rtti)
{
    auto pair = serializeFun().value(rtti, std::make_pair< FpSerializeIn, FpSerializeOut >(nullptr, nullptr));
    return pair.first;
}

DAChartItemSerialize::FpSerializeOut DAChartItemSerialize::getSerializeOutFun(int rtti)
{
    auto pair = serializeFun().value(rtti, std::make_pair< FpSerializeIn, FpSerializeOut >(nullptr, nullptr));
    return pair.second;
}

QByteArray DAChartItemSerialize::serializeOut(const QwtPlotItem* item)
{
    int rtti          = item->rtti();
    FpSerializeOut fp = getSerializeOutFun(rtti);
    if (!fp) {
        return QByteArray();
    }
    DAChartItemSerialize::Header h;
    h.rtti = item->rtti();
    QByteArray byte;
    QDataStream st(&byte, QIODevice::WriteOnly);
    st.setVersion(gc_datastream_version);  // 以5.12为准，因为最低编译要求为qt5.12
    st << h;
    st << fp(item);
    return byte;
}

QwtPlotItem* DAChartItemSerialize::serializeIn(const QByteArray& byte)
{
    // 使用QBuffer避免额外内存分配
    QBuffer buffer;
    buffer.setData(byte);
    buffer.open(QIODevice::ReadOnly);

    QDataStream st(&buffer);
    DAChartItemSerialize::Header h;
    st.setVersion(gc_datastream_version);
    st >> h;

    if (!h.isValid()) {
        return nullptr;
    }

    FpSerializeIn fp = getSerializeInFun(h.rtti);
    if (!fp) {
        return nullptr;
    }

    // 直接获取剩余数据的引用（零拷贝）
    qint64 remainingSize = buffer.bytesAvailable();
    const char* rawData  = buffer.data().constData() + buffer.pos();

    // 创建视图（不复制数据）
    QByteArray remainingData = QByteArray::fromRawData(rawData, remainingSize);

    return fp(remainingData);
}
#define DACHARTITEMSERIALIZE_MAKE_SERIALIZE_PAIR(RTTI_Value, PLotItemClass)                                            \
    { [](const QByteArray& byte) -> QwtPlotItem* {                                                                     \
         QDataStream st(byte);                                                                                         \
         QwtPlotItem* item = DAChartPlotItemFactory::createItem(RTTI_Value);                                           \
         st >> static_cast< PLotItemClass* >(item);                                                                    \
         return item;                                                                                                  \
     },                                                                                                                \
      [](const QwtPlotItem* item) -> QByteArray {                                                                      \
          QByteArray byte;                                                                                             \
          QDataStream st(&byte, QIODevice::WriteOnly);                                                                 \
          st << static_cast< const PLotItemClass* >(item);                                                             \
          return byte;                                                                                                 \
      } }
QHash< int, std::pair< DAChartItemSerialize::FpSerializeIn, DAChartItemSerialize::FpSerializeOut > > initChartItemSerialize()
{
    QHash< int, std::pair< DAChartItemSerialize::FpSerializeIn, DAChartItemSerialize::FpSerializeOut > > res;
    res[ QwtPlotItem::Rtti_PlotCurve ] = DACHARTITEMSERIALIZE_MAKE_SERIALIZE_PAIR(QwtPlotItem::Rtti_PlotCurve, QwtPlotCurve);
    res[ QwtPlotItem::Rtti_PlotBarChart ] = DACHARTITEMSERIALIZE_MAKE_SERIALIZE_PAIR(QwtPlotItem::Rtti_PlotBarChart,
                                                                                     QwtPlotBarChart);
    res[ QwtPlotItem::Rtti_PlotIntervalCurve ] = DACHARTITEMSERIALIZE_MAKE_SERIALIZE_PAIR(QwtPlotItem::Rtti_PlotIntervalCurve,
                                                                                          QwtPlotIntervalCurve);

    /**
        DAChartItemSerialize::FpSerializeIn in = [](int rtti, const QByteArray& byte) -> QwtPlotItem* {
            QwtPlotItem* item = DAChartPlotItemFactory::createItem(rtti);
            QDataStream st(byte);
            st >> item;
            switch (rtti) {
            case QwtPlotItem::Rtti_PlotCurve: {
                QwtPlotCurve* plotItem = static_cast< QwtPlotCurve* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotGrid: {
                QwtPlotGrid* plotItem = static_cast< QwtPlotGrid* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotScale: {
                QwtPlotScaleItem* plotItem = static_cast< QwtPlotScaleItem* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotLegend: {
                QwtPlotLegendItem* plotItem = static_cast< QwtPlotLegendItem* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotMarker: {
                QwtPlotMarker* plotItem = static_cast< QwtPlotMarker* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotSpectroCurve: {
                QwtPlotSpectroCurve* plotItem = static_cast< QwtPlotSpectroCurve* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotIntervalCurve: {
                QwtPlotIntervalCurve* plotItem = static_cast< QwtPlotIntervalCurve* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotHistogram: {
                QwtPlotHistogram* plotItem = static_cast< QwtPlotHistogram* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotSpectrogram: {
                QwtPlotSpectrogram* plotItem = static_cast< QwtPlotSpectrogram* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotGraphic: {
                QwtPlotGraphicItem* plotItem = static_cast< QwtPlotGraphicItem* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotTradingCurve: {
                QwtPlotTradingCurve* plotItem = static_cast< QwtPlotTradingCurve* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotBarChart: {
                QwtPlotBarChart* plotItem = static_cast< QwtPlotBarChart* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotMultiBarChart: {
                QwtPlotMultiBarChart* plotItem = static_cast< QwtPlotMultiBarChart* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotShape: {
                QwtPlotShapeItem* plotItem = static_cast< QwtPlotShapeItem* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotTextLabel: {
                QwtPlotTextLabel* plotItem = static_cast< QwtPlotTextLabel* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotZone: {
                QwtPlotZoneItem* plotItem = static_cast< QwtPlotZoneItem* >(item);
            } break;
            case QwtPlotItem::Rtti_PlotVectorField: {
                QwtPlotVectorField* plotItem = static_cast< QwtPlotVectorField* >(item);
            } break;
            default:
                break;
            }

            return item;
        };
        **/
    return res;
}

QHash< int, std::pair< DAChartItemSerialize::FpSerializeIn, DAChartItemSerialize::FpSerializeOut > >& DAChartItemSerialize::serializeFun()
{
    static QHash< int, std::pair< FpSerializeIn, FpSerializeOut > > s_serializeMap;
    return s_serializeMap;
}

}  // end DA
