#include "DAChartSerialize.h"
#include "DAChartUtil.h"
#include <cstring>
#include <cmath>
#include <QBuffer>
#include <QDebug>
#include <QFontInfo>
#include <QMap>
// qwt
#include "qwt_plot.h"
#include "qwt_symbol.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_widget.h"
#include "qwt_text_scale_draw.h"
#include "qwt_colormap.h"
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
#include "qwt_plot_arrowmarker.h"
#include "qwt_plot_boxchart.h"
#include "qwt_plot_shapeitem.h"
// DA
#include "DADataProbeMarker.h"
#include "DAChartTextMarker.h"

namespace {
/// QwtScaleWidget 序列化块版本号
/// v1: 仅 alignment/length/labelAlignment/labelRotation 等基类几何属性
/// v2: 在 isSD 后增加 scaleDrawType(0=normal,1=QwtTextScaleDraw)，
///     若为 QwtTextScaleDraw 额外持久化 labelMap 与显式 scaleDiv 的 bounds
constexpr int gcScaleWidgetBlockVersion = 2;
}  // namespace

#ifndef INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR
#define INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(RttiValue, ClassName)                                                  \
    std::make_pair(&DAChartItemSerialize::serializeIn_T< ClassName, RttiValue >,                                       \
                   &DAChartItemSerialize::serializeOut_T< ClassName >)
#endif

#ifndef DECLARE_INITCHARTITEMSERIALIZE_FUN
#define DECLARE_INITCHARTITEMSERIALIZE_FUN(RttiValue, ClassName)                                                       \
    template QwtPlotItem* DAChartItemSerialize::serializeIn_T< ClassName, RttiValue >(const QByteArray&);              \
    template QByteArray DAChartItemSerialize::serializeOut_T< ClassName >(const QwtPlotItem*);
#endif
namespace DA
{
/**
 * @brief 规范化QFont，确保pointSizeF合法
 *
 * QFont默认构造时pointSizeF=-1，表示使用系统默认字体大小。
 * 当QFont被序列化（pointSizeF=-1）后反序列化，可能导致Qt警告：
 * "QFont::setPointSizeF: Point size <= 0 (-1.000000), must be greater than 0"
 *
 * 此函数检测非法值并修复，保持其他字体属性不变。
 *
 * @param font 需要规范化检查的QFont对象
 */
inline void normalizeQFontPointSize(QFont& font)
{
    if (font.pointSizeF() <= 0) {
        // pointSizeF <= 0 表示使用系统默认字体大小
        // 使用QFontInfo获取实际有效的字体大小
        QFontInfo fi(font);
        qreal validSize = fi.pointSizeF();
        if (validSize > 0) {
            font.setPointSizeF(validSize);
        } else {
            // 如果QFontInfo也返回无效值，使用合理的默认值
            // 12pt是常见的默认字体大小
            font.setPointSizeF(12.0);
        }
    }
}

/**
 * @brief 验证double值是否为有效的数值（非NaN/Infinity）
 *
 * 反序列化过程中，如果数据损坏或格式不兼容，可能产生NaN或Infinity值。
 * 这些值可能导致渲染异常或程序崩溃。
 *
 * @param value 需要验证的double值
 * @param defaultValue 如果值无效，使用的默认值
 * @return 如果值有效返回原值，否则返回defaultValue
 */
inline double validateDoubleValue(double value, double defaultValue = 0.0)
{
    if (!std::isfinite(value)) {
        return defaultValue;
    }
    return value;
}

/**
 * @brief 验证并确保double值在有效范围内
 *
 * @param value 需要验证的double值
 * @param minValue 允许的最小值（包含）
 * @param defaultValue 如果值无效，使用的默认值
 * @return 如果值有效且在范围内返回原值，否则返回defaultValue
 */
inline double validateDoubleRange(double value, double minValue, double defaultValue)
{
    if (!std::isfinite(value) || value < minValue) {
        return defaultValue;
    }
    return value;
}

void serialize_out_scale_widge(QDataStream& out, const QwtPlot* chart, int axis);
void serialize_in_scale_widge(QDataStream& in, QwtPlot* chart, int axis);

/**
 * @brief 构造DABadSerializeExpection，默认错误信息
 */
DABadSerializeExpection::DABadSerializeExpection()
{
    mWhy = "serialize error";
}

/**
 * @brief 构造DABadSerializeException，指定错误信息
 * @param why 错误描述字符串
 */
DABadSerializeExpection::DABadSerializeExpection(const char* why)
{
    mWhy = why;
}

/**
 * @brief 构造DABadSerializeExpection，指定错误信息
 * @param why 错误描述字符串
 */
DABadSerializeExpection::DABadSerializeExpection(const std::string& why)
{
    mWhy = why;
}

/**
 * @brief 析构DABadSerializeExpection
 */
DABadSerializeExpection::~DABadSerializeExpection()
{
}

/**
 * @brief 获取错误描述
 * @return 错误描述的C字符串
 */
const char* DABadSerializeExpection::what() const noexcept
{
    return mWhy.c_str();
}

/**
 * @brief 序列化输出坐标轴刻度部件
 * @param out 输出数据流
 * @param chart 绘图对象
 * @param axis 坐标轴ID
 */
void serialize_out_scale_widge(QDataStream& out, const QwtPlot* chart, int axis)
{
    const QwtScaleWidget* axisWid = chart->axisWidget(axis);
    bool isaxis                   = (nullptr != axisWid);
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << isaxis;
    out << DA::gc_dachart_magic_mark2;
    if (isaxis) {
        bool enable = chart->axisEnabled(axis);
        out << axisWid << enable << DA::gc_dachart_magic_mark3;
    }
}

/**
 * @brief 反序列化输入坐标轴刻度部件
 * @param in 输入数据流
 * @param chart 绘图对象
 * @param axis 坐标轴ID
 */
void serialize_in_scale_widge(QDataStream& in, QwtPlot* chart, int axis)
{
    bool isaxis;
    std::uint32_t H;
    std::uint32_t B;
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DABadSerializeExpection();
        return;
    }
    in >> isaxis;
    in >> H;
    if (DA::gc_dachart_magic_mark2 != H) {
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
        if (DA::gc_dachart_magic_mark3 != B) {
            throw DABadSerializeExpection();
            return;
        }
        chart->enableAxis(axis, enable);
        // 若安装的是 QwtTextScaleDraw，把其暂存的显式 scaleDiv 同步到 plot 并关闭自动缩放，
        // 避免 autoRefresh 用引擎重算覆盖类别刻度位置
        if (auto* td = dynamic_cast< QwtTextScaleDraw* >(chart->axisScaleDraw(axis))) {
            chart->setAxisAutoScale(axis, false);
            chart->setAxisScaleDiv(axis, td->scaleDiv());
        }
    }
}

//----------------------------------------------------
// Header
//----------------------------------------------------

/**
 * @brief 构造Header，初始化魔数、版本和RTTI
 */
DAChartItemSerialize::Header::Header() : magic(DA::gc_dachart_magic_mark4), version(1), rtti(QwtPlotItem::Rtti_PlotItem)
{
    memset(byte, 0, sizeof(byte));
}

/**
 * @brief 析构Header
 */
DAChartItemSerialize::Header::~Header()
{
}

/**
 * @brief 检查Header是否有效
 * @return 魔数匹配返回true，否则返回false
 */
bool DAChartItemSerialize::Header::isValid() const
{
    return DA::gc_dachart_magic_mark4 == magic;
}

// ADL原则，这个应放在DA命名空间下
/**
 * @brief Header的序列化输出
 * @param out 输出数据流
 * @param f Header对象
 * @return 输出数据流
 */
QDataStream& operator<<(QDataStream& out, const DA::DAChartItemSerialize::Header& f)
{
    out << f.magic << f.version << f.rtti;
    out.writeRawData(reinterpret_cast< const char* >(f.byte), sizeof(f.byte));
    return out;
}

/**
 * @brief Header的反序列化输入
 * @param in 输入数据流
 * @param f Header对象
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, DA::DAChartItemSerialize::Header& f)
{
    in >> f.magic >> f.version;
    in >> f.rtti;
    in.readRawData(reinterpret_cast< char* >(f.byte), sizeof(f.byte));
    return in;
}

//===============================================================
// name
//===============================================================
/**
 * @brief 构造DAChartItemSerialize
 */
DAChartItemSerialize::DAChartItemSerialize()
{
}

/**
 * @brief 析构DAChartItemSerialize
 */
DAChartItemSerialize::~DAChartItemSerialize()
{
}

/**
 * @brief 注册序列化函数
 * @param rtti 绘图项的RTTI值
 * @param fpIn 反序列化函数指针
 * @param fpOut 序列化函数指针
 */
void DAChartItemSerialize::registSerializeFun(int rtti,
                                              DAChartItemSerialize::FpSerializeIn fpIn,
                                              DAChartItemSerialize::FpSerializeOut fpOut)
{
    serializeFun()[ rtti ] = std::make_pair(fpIn, fpOut);
}

/**
 * @brief 判断指定RTTI是否支持序列化
 * @param rtti 绘图项的RTTI值
 * @return 支持返回true，否则返回false
 */
bool DAChartItemSerialize::isSupportSerialize(int rtti)
{
    return serializeFun().contains(rtti);
}

/**
 * @brief 获取反序列化函数
 * @param rtti 绘图项的RTTI值
 * @return 反序列化函数指针，未注册返回nullptr
 */
DAChartItemSerialize::FpSerializeIn DAChartItemSerialize::getSerializeInFun(int rtti) noexcept
{
    auto pair = serializeFun().value(rtti, std::make_pair< FpSerializeIn, FpSerializeOut >(nullptr, nullptr));
    return pair.first;
}

/**
 * @brief 获取序列化函数
 * @param rtti 绘图项的RTTI值
 * @return 序列化函数指针，未注册返回nullptr
 */
DAChartItemSerialize::FpSerializeOut DAChartItemSerialize::getSerializeOutFun(int rtti)
{
    auto pair = serializeFun().value(rtti, std::make_pair< FpSerializeIn, FpSerializeOut >(nullptr, nullptr));
    return pair.second;
}

/**
 * @brief 序列化输出绘图项
 * @param item 绘图项指针
 * @return 序列化后的字节数组
 */
QByteArray DAChartItemSerialize::serializeOut(const QwtPlotItem* item) const
{
    int rtti          = item->rtti();
    FpSerializeOut fp = getSerializeOutFun(rtti);
    if (!fp) {
        qDebug() << QString("While serializing the plot item, an unregistered RTTI value (%1) was encountered.")
                        .arg(rtti);  // cn:序列化plotitem时，遇到未注册的RTTI值(%1)
        return QByteArray();
    }
    return fp(item);
}

/**
 * @brief 反序列化输入绘图项
 * @param byte 序列化的字节数组
 * @return 反序列化后的绘图项指针，失败返回nullptr
 */
QwtPlotItem* DAChartItemSerialize::serializeIn(const QByteArray& byte) const noexcept
{
    // 使用QBuffer避免额外内存分配
    int rtti = getRtti(byte);
    if (rtti < 0) {
        return nullptr;
    }
    FpSerializeIn fp = getSerializeInFun(rtti);
    if (!fp) {
        qDebug() << QString("While deserializing the plot item, an unregistered RTTI value (%1) was encountered.")
                        .arg(rtti);  // cn:反序列化plotitem时，遇到未注册的RTTI值(%1)
        return nullptr;
    }
    QwtPlotItem* item = nullptr;
    try {
        item = fp(byte);
    } catch (const std::exception& e) {
        qDebug() << e.what();
    }
    return item;
}

/**
 * @brief 从字节数组中提取RTTI值
 * @param byte 序列化的字节数组
 * @return RTTI值，无效返回-1
 */
int DAChartItemSerialize::getRtti(const QByteArray& byte) const noexcept
{
    // 使用QBuffer避免额外内存分配
    int rtti = -1;
    try {
        QBuffer buffer;
        buffer.setData(byte);
        buffer.open(QIODevice::ReadOnly);

        QDataStream st(&buffer);
        DAChartItemSerialize::Header h;
        st.setVersion(gc_datastream_version);
        st >> h;
        if (h.isValid()) {
            rtti = h.rtti;
        }
    } catch (const std::exception& e) {
        qDebug() << e.what();
        return -1;
    }
    return rtti;
}

// === 显式实例化定义 ===
// QwtPlotCurve ---------------------------------------------------------
// DECLARE_INITCHARTITEMSERIALIZE_FUN(RttiI, T)
// 注册序列化函数

DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotCurve, QwtPlotCurve)
DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotGrid, QwtPlotGrid)
DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotLegend, QwtPlotLegendItem)
DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotMarker, QwtPlotMarker)
DECLARE_INITCHARTITEMSERIALIZE_FUN(DA::DADataProbeMarker::Rtti_DataProbeMarker, DA::DADataProbeMarker)
DECLARE_INITCHARTITEMSERIALIZE_FUN(DA::DAChartTextMarker::Rtti_TextMarker, DA::DAChartTextMarker)
DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotSpectroCurve, QwtPlotSpectroCurve)
DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotBarChart, QwtPlotBarChart)
DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotIntervalCurve, QwtPlotIntervalCurve)
DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotShape, QwtPlotShapeItem)
DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotArrowMarker, QwtPlotArrowMarker)
DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotBoxChart, QwtPlotBoxChart)
DECLARE_INITCHARTITEMSERIALIZE_FUN(QwtPlotItem::Rtti_PlotHistogram, QwtPlotHistogram)
/**
 * @brief 初始化所有支持的绘图项序列化函数映射表
 * @return 序列化函数映射表
 */
QHash< int, std::pair< DAChartItemSerialize::FpSerializeIn, DAChartItemSerialize::FpSerializeOut > > initChartItemSerialize()
{
    QHash< int, std::pair< DAChartItemSerialize::FpSerializeIn, DAChartItemSerialize::FpSerializeOut > > res;
    res[ QwtPlotItem::Rtti_PlotCurve ] = INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotCurve, QwtPlotCurve);
    res[ QwtPlotItem::Rtti_PlotGrid ] = INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotGrid, QwtPlotGrid);
    res[ QwtPlotItem::Rtti_PlotLegend ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotLegend, QwtPlotLegendItem);
    res[ QwtPlotItem::Rtti_PlotMarker ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotMarker, QwtPlotMarker);
    res[ DA::DADataProbeMarker::Rtti_DataProbeMarker ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(DA::DADataProbeMarker::Rtti_DataProbeMarker, DA::DADataProbeMarker);
    res[ DA::DAChartTextMarker::Rtti_TextMarker ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(DA::DAChartTextMarker::Rtti_TextMarker, DA::DAChartTextMarker);
    res[ QwtPlotItem::Rtti_PlotSpectroCurve ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotSpectroCurve, QwtPlotSpectroCurve);
    res[ QwtPlotItem::Rtti_PlotBarChart ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotBarChart, QwtPlotBarChart);
    res[ QwtPlotItem::Rtti_PlotIntervalCurve ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotIntervalCurve, QwtPlotIntervalCurve);
    res[ QwtPlotItem::Rtti_PlotShape ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotShape, QwtPlotShapeItem);
    res[ QwtPlotItem::Rtti_PlotArrowMarker ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotArrowMarker, QwtPlotArrowMarker);
    res[ QwtPlotItem::Rtti_PlotBoxChart ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotBoxChart, QwtPlotBoxChart);
    res[ QwtPlotItem::Rtti_PlotHistogram ] =
        INITCHARTITEMSERIALIZE_MAKE_IN_OUT_PAIR(QwtPlotItem::Rtti_PlotHistogram, QwtPlotHistogram);
    return res;
}

/**
 * @brief 获取序列化函数映射表的静态引用
 * @return 序列化函数映射表的引用
 */
QHash< int, std::pair< DAChartItemSerialize::FpSerializeIn, DAChartItemSerialize::FpSerializeOut > >&
DAChartItemSerialize::serializeFun()
{
    static QHash< int, std::pair< FpSerializeIn, FpSerializeOut > > s_serializeMap = initChartItemSerialize();
    return s_serializeMap;
}

}  // end DA

/**
 * @brief QwtText的序列化输出
 * @param out 输出数据流
 * @param t QwtText对象
 * @return 输出数据流
 */
QDataStream& operator<<(QDataStream& out, const QwtText& t)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << DA::gc_dachart_magic_mark2 << t.text() << t.font() << t.renderFlags() << t.color() << t.borderRadius()
        << t.borderPen() << t.backgroundBrush() << t.testPaintAttribute(QwtText::PaintUsingTextFont)
        << t.testPaintAttribute(QwtText::PaintUsingTextColor) << t.testPaintAttribute(QwtText::PaintBackground)
        << t.testLayoutAttribute(QwtText::MinimumLayout);
    return out;
}

/**
 * @brief QwtText的反序列化输入
 * @param in 输入数据流
 * @param t QwtText对象
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtText& t)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    std::uint32_t tmp;
    in >> tmp;
    if (DA::gc_dachart_magic_mark2 != tmp) {
        throw DA::DABadSerializeExpection();
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
    t.setBorderRadius(DA::validateDoubleRange(borderRadius, 0.0, 0.0));  // borderRadius必须>=0
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
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
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
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

/**
 * @brief QwtIntervalSymbol的序列化输出
 * @param out 输出数据流
 * @param t QwtIntervalSymbol指针
 * @return 输出数据流
 */
QDataStream& operator<<(QDataStream& out, const QwtIntervalSymbol* t)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << static_cast< int >(t->width()) << t->brush() << t->pen() << static_cast< int >(t->style());
    return out;
}

/**
 * @brief QwtIntervalSymbol的反序列化输入
 * @param in 输入数据流
 * @param t QwtIntervalSymbol指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtIntervalSymbol* t)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
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
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
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
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
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
    DA::DAChartUtil::getXYDatas(sample, item);
    out << DA::gc_dachart_magic_mark2 << sample << DA::gc_dachart_magic_mark3;
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
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
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
    if (DA::gc_dachart_magic_mark2 != tmp0) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> sample >> tmp1;
    if (DA::gc_dachart_magic_mark3 != tmp1) {
        throw DA::DABadSerializeExpection();
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << static_cast< const QwtPlotItem* >(item);
    out << item->majorPen() << item->minorPen() << item->xEnabled() << item->yEnabled() << item->xMinEnabled()
        << item->yMinEnabled();
    return out;
}

/**
 * @brief QwtPlotGrid指针的反序列化
 * @param in 输入数据流
 * @param item QwtPlotGrid指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtPlotGrid* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << static_cast< const QwtPlotItem* >(item);
    out << item->borderDistance() << item->font() << item->isScaleDivFromAxis() << item->palette() << item->position();
    return out;
}
/**
 * @brief QwtPlotScaleItem(Rtti_PlotScale)指针的反序列化
 * @param in 输入数据流
 * @param item QwtPlotScaleItem指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtPlotScaleItem* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << static_cast< const QwtPlotItem* >(item);
    out << item->alignmentInCanvas() << item->backgroundBrush() << static_cast< int >(item->backgroundMode())
        << item->borderPen() << item->borderRadius() << item->font() << item->offsetInCanvas(Qt::Horizontal)
        << item->offsetInCanvas(Qt::Vertical) << item->maxColumns() << item->margin() << item->spacing()
        << item->itemMargin() << item->itemSpacing() << item->textPen();
    return out;
}
/**
 * @brief QwtPlotLegendItem(Rtti_PlotLegend)指针的反序列化
 * @param in 输入数据流
 * @param item QwtPlotLegendItem指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtPlotLegendItem* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
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
    item->setBorderRadius(DA::validateDoubleRange(borderRadius, 0.0, 0.0));  // borderRadius必须>=0
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
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
/**
 * @brief QwtPlotMarker(Rtti_PlotMarker)指针的反序列化
 * @param in 输入数据流
 * @param item QwtPlotMarker指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtPlotMarker* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
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
 * @brief DA::DADataProbeMarker(Rtti_DataProbeMarker)指针的序列化
 *
 * 序列化顺序：先写 DADataProbeMarker 特有属性，再写基类 QwtPlotMarker。
 * 反序列化时先恢复特有属性（setLabelPosition 会调用 updateLineStyle 设置 lineStyle/labelAlignment），
 * 最后由基类 QwtPlotMarker 的反序列化覆盖 lineStyle/labelAlignment 为正确值。
 * probeType 仅写入用于未来兼容，反序列化时读取但无法设置（由工厂构造时决定）。
 */
QDataStream& operator<<(QDataStream& out, const DA::DADataProbeMarker* item)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    // DADataProbeMarker 特有属性
    out << static_cast< int >(item->probeType());
    out << item->probeName();
    out << static_cast< int >(item->labelPosition());
    out << static_cast< int >(item->labelStyle());
    out << item->probeColor();
    out << item->isLabelVisible();
    // 基类 QwtPlotMarker
    out << static_cast< const QwtPlotMarker* >(item);
    return out;
}

/**
 * @brief DA::DADataProbeMarker(Rtti_DataProbeMarker)指针的反序列化
 * @param in 输入数据流
 * @param item DADataProbeMarker指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, DA::DADataProbeMarker* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    // DADataProbeMarker 特有属性
    int probeType, labelPosition, labelStyle;
    QwtText probeName;
    QColor probeColor;
    bool labelVisible;
    in >> probeType >> probeName >> labelPosition >> labelStyle >> probeColor >> labelVisible;
    item->setProbeName(probeName);
    item->setLabelPosition(static_cast< DA::DADataProbeMarker::LabelPosition >(labelPosition));
    item->setLabelStyle(static_cast< DA::DADataProbeMarker::LabelStyle >(labelStyle));
    item->setProbeColor(probeColor);
    item->setLabelVisible(labelVisible);
    // 基类 QwtPlotMarker（最后读取，覆盖 lineStyle/labelAlignment 为正确值）
    in >> static_cast< QwtPlotMarker* >(item);
    return in;
}

/**
 * @brief DA::DAChartTextMarker(Rtti_TextMarker)指针的序列化
 *
 * DAChartTextMarker 的富文本内容保存在基类 QwtPlotMarker 的 label(QwtText)中，
 * QwtText 的序列化包含 text/font/renderFlags/color/borderRadius/borderPen/backgroundBrush
 * 及 PaintAttributes，HTML 内容可无损往返，因此此处仅转发基类即可
 */
QDataStream& operator<<(QDataStream& out, const DA::DAChartTextMarker* item)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    // 基类 QwtPlotMarker（含 label 的 QwtText 完整序列化）
    out << static_cast< const QwtPlotMarker* >(item);
    return out;
}

/**
 * @brief DA::DAChartTextMarker(Rtti_TextMarker)指针的反序列化
 * @param in 输入数据流
 * @param item DAChartTextMarker指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, DA::DAChartTextMarker* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    // 基类 QwtPlotMarker（label 的 HTML 文本经 QwtText::AutoText 自动识别为富文本）
    in >> static_cast< QwtPlotMarker* >(item);
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << static_cast< const QwtPlotItem* >(item);
    out << item->penWidth() << static_cast< int >(item->orientation())
        << item->testPaintAttribute(QwtPlotSpectroCurve::ClipPoints);
    return out;
}
/**
 * @brief QwtPlotSpectroCurve(Rtti_PlotSpectroCurve)指针的反序列化
 * @param in 输入数据流
 * @param item QwtPlotSpectroCurve指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtPlotSpectroCurve* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
    int orientation;
    double penWidth;
    bool attClipPoints;
    in >> penWidth >> orientation >> attClipPoints;
    item->setPenWidth(DA::validateDoubleRange(penWidth, 0.0, 1.0));  // penWidth必须>=0，默认1.0
    item->setOrientation(static_cast< Qt::Orientation >(orientation));
    item->setPaintAttribute(QwtPlotSpectroCurve::ClipPoints, attClipPoints);
    return in;
}

//============================================
// QwtPlotBarChart
//============================================
///
/// \brief QwtPlotBarChart指针的序列化
/// \param out
/// \param item
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtPlotBarChart* item)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << (const QwtPlotItem*)item;
    out << static_cast< int >(item->layoutPolicy()) << item->layoutHint() << item->spacing() << item->margin()
        << item->baseline() << static_cast< int >(item->legendMode()) << (int)(item->orientation());
    // save sample
    QVector< QPointF > sample;
    DA::DAChartUtil::getXYDatas(sample, item);
    out << DA::gc_dachart_magic_mark2 << sample << DA::gc_dachart_magic_mark3;
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
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
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
    if (DA::gc_dachart_magic_mark2 != tmp0) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    QVector< QPointF > sample;
    in >> sample >> tmp1;
    if (DA::gc_dachart_magic_mark3 != tmp1) {
        throw DA::DABadSerializeExpection();
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

//============================================
// QwtPlotArrowMarker
//============================================

/**
 * @brief QwtPlotArrowMarker指针的序列化
 * @param out 输出数据流
 * @param item QwtPlotArrowMarker指针
 * @return 输出数据流
 */
QDataStream& operator<<(QDataStream& out, const QwtPlotArrowMarker* item)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << (const QwtPlotItem*)item;
    out << item->startPoint() << item->endPoint() << item->length() << item->linePen()
        << static_cast< int >(item->positionMode());
    out << static_cast< int >(item->headStyle()) << item->headSize() << item->headBrush() << item->headPen();
    if (item->headStyle() == QwtPlotArrowMarker::CustomPath) {
        out << item->headCustomPath();
    }
    out << static_cast< int >(item->tailStyle()) << item->tailSize() << item->tailBrush() << item->tailPen();
    if (item->tailStyle() == QwtPlotArrowMarker::CustomPath) {
        out << item->tailCustomPath();
    }
    return out;
}

/**
 * @brief QwtPlotArrowMarker指针的反序列化
 * @param in 输入数据流
 * @param item QwtPlotArrowMarker指针
 * @return 输入数据流
 */
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtPlotArrowMarker* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
    QPointF startPoint;
    QPointF endPoint;
    double length;
    QPen linePen;
    int positionMode;
    in >> startPoint >> endPoint >> length >> linePen >> positionMode;
    item->setStartPoint(startPoint);
    item->setEndPoint(endPoint);
    item->setLength(length);
    item->setLinePen(linePen);
    item->setPositionMode(static_cast< QwtPlotArrowMarker::PositionMode >(positionMode));
    // load head
    int headStyle;
    QSizeF headSize;
    QBrush headBrush;
    QPen headPen;
    in >> headStyle >> headSize >> headBrush >> headPen;
    item->setHeadStyle(static_cast< QwtPlotArrowMarker::EndpointStyle >(headStyle));
    // 验证headSize的宽度和高度是否有效
    if (!std::isfinite(headSize.width()) || !std::isfinite(headSize.height())) {
        headSize = QSizeF(12.0, 12.0);  // 使用默认值
    }
    item->setHeadSize(headSize);
    item->setHeadBrush(headBrush);
    item->setHeadPen(headPen);
    if (item->headStyle() == QwtPlotArrowMarker::CustomPath) {
        QPainterPath path;
        in >> path;
        item->setHeadCustomPath(path);
    }
    // load tail
    int tailStyle;
    QSizeF tailSize;
    QBrush tailBrush;
    QPen tailPen;
    in >> tailStyle >> tailSize >> tailBrush >> tailPen;
    item->setTailStyle(static_cast< QwtPlotArrowMarker::EndpointStyle >(tailStyle));
    // 验证tailSize的宽度和高度是否有效
    if (!std::isfinite(tailSize.width()) || !std::isfinite(tailSize.height())) {
        tailSize = QSizeF(12.0, 12.0);  // 使用默认值
    }
    item->setTailSize(tailSize);
    item->setTailBrush(tailBrush);
    item->setTailPen(tailPen);
    if (item->tailStyle() == QwtPlotArrowMarker::CustomPath) {
        QPainterPath path;
        in >> path;
        item->setTailCustomPath(path);
    }
    return in;
}

//============================================
// QwtPlotBoxChart
//============================================

/**
 * @brief QwtPlotBoxChart指针的序列化
 * @param out 输出数据流
 * @param item QwtPlotBoxChart指针
 * @return 输出数据流
 */
QDataStream& operator<<(QDataStream& out, const QwtPlotBoxChart* item)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << (const QwtPlotItem*)item;
    out << static_cast< int >(item->boxStyle()) << static_cast< int >(item->whiskerStyle())
        << static_cast< int >(item->orientation()) << item->boxExtent() << item->minBoxWidth()
        << item->maxBoxWidth() << item->pen() << item->brush() << item->medianPen()
        << item->isMedianVisible() << item->isMeanVisible() << item->outlierJitter();
    // save samples
    QVector< QwtBoxSample > samples;
    DA::DAChartUtil::getSeriesData< QwtBoxSample >(samples, item);
    out << DA::gc_dachart_magic_mark2 << samples << DA::gc_dachart_magic_mark3;
    // save outlier symbol
    const QwtSymbol* os = item->outlierSymbol();
    bool hasOutlierSymbol = (os != nullptr);
    out << hasOutlierSymbol;
    if (hasOutlierSymbol) {
        out << os;
    }
    // save mean symbol
    const QwtSymbol* ms = item->meanSymbol();
    bool hasMeanSymbol = (ms != nullptr);
    out << hasMeanSymbol;
    if (hasMeanSymbol) {
        out << ms;
    }
    return out;
}

/**
 * @brief QwtPlotBoxChart指针的反序列化
 * @param in 输入数据流
 * @param item QwtPlotBoxChart指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtPlotBoxChart* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
    int boxStyle;
    int whiskerStyle;
    int orientation;
    double boxExtent;
    double minBoxWidth;
    double maxBoxWidth;
    QPen pen;
    QBrush brush;
    QPen medianPen;
    bool medianVisible;
    bool meanVisible;
    double outlierJitter;
    in >> boxStyle >> whiskerStyle >> orientation >> boxExtent >> minBoxWidth >> maxBoxWidth
        >> pen >> brush >> medianPen >> medianVisible >> meanVisible >> outlierJitter;
    item->setBoxStyle(static_cast< QwtPlotBoxChart::BoxStyle >(boxStyle));
    item->setWhiskerStyle(static_cast< QwtPlotBoxChart::WhiskerStyle >(whiskerStyle));
    item->setOrientation(static_cast< Qt::Orientation >(orientation));
    item->setBoxExtent(boxExtent);
    item->setMinBoxWidth(minBoxWidth);
    item->setMaxBoxWidth(maxBoxWidth);
    item->setPen(pen);
    item->setBrush(brush);
    item->setMedianPen(medianPen);
    item->setMedianVisible(medianVisible);
    item->setMeanVisible(meanVisible);
    item->setOutlierJitter(outlierJitter);
    // load samples
    std::uint32_t tmp0, tmp1;
    in >> tmp0;
    if (DA::gc_dachart_magic_mark2 != tmp0) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    QVector< QwtBoxSample > samples;
    in >> samples >> tmp1;
    if (DA::gc_dachart_magic_mark3 != tmp1) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    item->setSamples(samples);
    // load outlier symbol
    bool hasOutlierSymbol;
    in >> hasOutlierSymbol;
    if (hasOutlierSymbol) {
        QwtSymbol* sym = nullptr;
        in >> sym;
        item->setOutlierSymbol(sym);
        delete sym;
    }
    // load mean symbol
    bool hasMeanSymbol;
    in >> hasMeanSymbol;
    if (hasMeanSymbol) {
        QwtSymbol* sym = nullptr;
        in >> sym;
        item->setMeanSymbol(sym);
        delete sym;
    }
    return in;
}

//============================================
// QwtPlotShapeItem
//============================================
/**
 * @brief QwtPlotShapeItem指针的序列化
 * @param out 输出数据流
 * @param item QwtPlotShapeItem指针
 * @return 输出数据流
 */
DAFIGURE_API QDataStream& operator<<(QDataStream& out, const QwtPlotShapeItem* item)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << (const QwtPlotItem*)item;
    out << item->brush() << item->pen() << item->shape();
    out << item->testPaintAttribute(QwtPlotShapeItem::ClipPolygons) << static_cast< int >(item->legendMode())
        << item->renderTolerance();
    return out;
}

/**
 * @brief QwtPlotShapeItem指针的反序列化
 * @param in 输入数据流
 * @param item QwtPlotShapeItem指针
 * @return 输入数据流
 */
DAFIGURE_API QDataStream& operator>>(QDataStream& in, QwtPlotShapeItem* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
    QBrush brush;
    QPen pen;
    QPainterPath shape;
    bool isClipPolygons;
    int legendMode;
    double renderTolerance;
    in >> brush >> pen >> shape;
    in >> isClipPolygons >> legendMode >> renderTolerance;
    item->setPaintAttribute(QwtPlotShapeItem::ClipPolygons, isClipPolygons);
    item->setLegendMode(static_cast< QwtPlotShapeItem::LegendMode >(legendMode));
    item->setRenderTolerance(DA::validateDoubleRange(renderTolerance, 0.0, 0.0));  // renderTolerance必须>=0
    item->setBrush(brush);
    item->setPen(pen);
    item->setShape(shape);
    return in;
}

//============================================
// QwtPlotIntervalCurve
//============================================

/**
 * @brief QwtPlotIntervalCurve指针的序列化
 * @param out 输出数据流
 * @param item QwtPlotIntervalCurve指针
 * @return 输出数据流
 */
QDataStream& operator<<(QDataStream& out, const QwtPlotIntervalCurve* item)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << (const QwtPlotItem*)item;
    out << (int)item->orientation() << item->pen() << item->brush() << (int)item->style()
        << item->testPaintAttribute(QwtPlotIntervalCurve::ClipPolygons)
        << item->testPaintAttribute(QwtPlotIntervalCurve::ClipSymbol);
    // save sample
    const unsigned int ck0 = 0xabf31f;
    const unsigned int ck1 = 0x9f6fda;
    QVector< QwtIntervalSample > sample;
    DA::DAChartUtil::getSeriesData(sample, item);
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

/**
 * @brief QwtPlotIntervalCurve指针的反序列化
 * @param in 输入数据流
 * @param item QwtPlotIntervalCurve指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtPlotIntervalCurve* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
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
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> sample >> tmp1;
    if (ck1 != tmp1) {
        throw DA::DABadSerializeExpection();
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

//============================================
// QwtPlotHistogram
//============================================

QDataStream& operator<<(QDataStream& out, const QwtPlotHistogram* item)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << (const QwtPlotItem*)item;
    out << item->baseline() << item->pen() << item->brush() << static_cast< int >(item->style());
    // save sample
    QVector< QwtIntervalSample > sample;
    DA::DAChartUtil::getIntervalSampleDatas(sample, item);
    out << DA::gc_dachart_magic_mark2 << sample << DA::gc_dachart_magic_mark3;
    // Symbol
    const QwtColumnSymbol* cs = item->symbol();
    bool isColumnSymbol       = (cs != nullptr);
    out << isColumnSymbol;
    if (isColumnSymbol) {
        out << cs;
    }
    return out;
}

QDataStream& operator>>(QDataStream& in, QwtPlotHistogram* item)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> (QwtPlotItem*)item;
    double baseline;
    QPen pen;
    QBrush brush;
    int style;
    in >> baseline >> pen >> brush >> style;
    item->setBaseline(baseline);
    item->setPen(pen);
    item->setBrush(brush);
    item->setStyle(static_cast< QwtPlotHistogram::HistogramStyle >(style));
    // load sample
    std::uint32_t tmp0, tmp1;
    in >> tmp0;
    if (DA::gc_dachart_magic_mark2 != tmp0) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    QVector< QwtIntervalSample > sample;
    in >> sample >> tmp1;
    if (DA::gc_dachart_magic_mark3 != tmp1) {
        throw DA::DABadSerializeExpection();
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

///
/// \brief QwtScaleWidget指针的序列化
/// \param out
/// \param w
/// \return
///
QDataStream& operator<<(QDataStream& out, const QwtScaleWidget* w)
{
    out << gcScaleWidgetBlockVersion << DA::gc_dachart_magic_mark;
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
    out << isSD;
    if (isSD) {
        // v2 起记录 ScaleDraw 子类类型，便于反序列化时还原正确子类
        const QwtTextScaleDraw* textDraw = dynamic_cast< const QwtTextScaleDraw* >(sd);
        int scaleDrawType                = (textDraw != nullptr) ? 1 : 0;  // 0=normal, 1=QwtTextScaleDraw
        out << scaleDrawType;
        out << sd;  // 基类几何属性(alignment/length/labelAlignment/labelRotation)
        if (textDraw != nullptr) {
            // 持久化 labelMap 及显式 scaleDiv 的 bounds，主刻度即 labelMap 的 keys
            const QMap< double, QString > labelMap = textDraw->labelMap();
            out << static_cast< quint32 >(labelMap.size());
            for (auto it = labelMap.constBegin(); it != labelMap.constEnd(); ++it) {
                out << it.key() << it.value();
            }
            const QwtScaleDiv& div = sd->scaleDiv();
            out << div.lowerBound() << div.upperBound();
        }
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
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    unsigned int c0 = 0x82fa34;
    unsigned int tmp;
    in >> tmp;
    if (c0 != tmp) {
        throw DA::DABadSerializeExpection();
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
        int scaleDrawType = 0;
        if (version >= gcScaleWidgetBlockVersion) {
            in >> scaleDrawType;  // v2 起存在子类类型标记
        }
        QwtScaleDraw* sd = w->scaleDraw();
        if (scaleDrawType == 1) {
            // QwtTextScaleDraw：确保安装正确子类再读基类属性
            if (dynamic_cast< QwtTextScaleDraw* >(sd) == nullptr) {
                sd = new QwtTextScaleDraw();
                w->setScaleDraw(sd);
            }
            in >> sd;
            // 读 labelMap + 显式 scaleDiv bounds
            quint32 cnt;
            in >> cnt;
            QMap< double, QString > labelMap;
            for (quint32 i = 0; i < cnt; ++i) {
                double key;
                QString val;
                in >> key >> val;
                labelMap.insert(key, val);
            }
            double lower, upper;
            in >> lower >> upper;
            auto* textDraw = static_cast< QwtTextScaleDraw* >(sd);
            textDraw->setLabelMap(labelMap);
            // 在 draw 上暂存显式 scaleDiv（主刻度即 labelMap 的 keys），
            // serialize_in_scale_widge 会通过 chart->setAxisScaleDiv 同步到 plot
            QList< double > majorTicks = labelMap.keys();
            QwtScaleDiv div(lower, upper, QList< double >(), QList< double >(), majorTicks);
            sd->setScaleDiv(div);
        } else {
            // 普通坐标轴（v1 旧文件无 scaleDrawType，直接读基类属性）
            if (nullptr == sd) {
                w->setScaleDraw(new QwtScaleDraw());
                sd = w->scaleDraw();
            }
            in >> sd;
        }
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
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
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    unsigned int c0 = 0x92fa34;
    unsigned int tmp;
    in >> tmp;
    if (c0 != tmp) {
        throw DA::DABadSerializeExpection();
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
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
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    unsigned int c0 = 0x13fa34;
    unsigned int tmp;
    in >> tmp;
    if (c0 != tmp) {
        throw DA::DABadSerializeExpection();
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

//----------------------------------------------------
// QwtPlotCanvas
//----------------------------------------------------
/**
 * @brief QwtPlotCanvas的序列化
 * @param out
 * @param c
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtPlotCanvas* c)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    out << (const QFrame*)c;
    out << static_cast< int >(c->focusIndicator()) << c->borderRadius()
        << c->testPaintAttribute(QwtPlotCanvas::BackingStore) << c->testPaintAttribute(QwtPlotCanvas::Opaque)
        << c->testPaintAttribute(QwtPlotCanvas::HackStyledBackground)
        << c->testPaintAttribute(QwtPlotCanvas::ImmediatePaint);
    return out;
}

/**
 * @brief QwtPlotCanvas的序列化
 * @param in
 * @param c
 * @return
 */
QDataStream& operator>>(QDataStream& in, QwtPlotCanvas* c)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> (QFrame*)c;
    int focusIndicator;
    double borderRadius;
    bool isBackingStore, isOpaque, isHackStyledBackground, isImmediatePaint;
    in >> focusIndicator >> borderRadius >> isBackingStore >> isOpaque >> isHackStyledBackground >> isImmediatePaint;
    c->setFocusIndicator(static_cast< QwtPlotCanvas::FocusIndicator >(focusIndicator));
    c->setBorderRadius(DA::validateDoubleRange(borderRadius, 0.0, 0.0));  // borderRadius必须>=0
    c->setPaintAttribute(QwtPlotCanvas::BackingStore, isBackingStore);
    c->setPaintAttribute(QwtPlotCanvas::Opaque, isOpaque);
    c->setPaintAttribute(QwtPlotCanvas::HackStyledBackground, isHackStyledBackground);
    c->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, isImmediatePaint);
    return in;
}

//----------------------------------------------------
// QwtPlot
//----------------------------------------------------
/**
 * @brief QwtPlot的基本序列化支持
 * @param out
 * @param chart
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtPlot* chart)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
    // QFrame save
    out << (const QFrame*)chart;
    // QwtPlot save
    out << chart->title() << chart->footer() << chart->palette();
    // axis
    DA::serialize_out_scale_widge(out, chart, QwtPlot::xTop);
    DA::serialize_out_scale_widge(out, chart, QwtPlot::xBottom);
    DA::serialize_out_scale_widge(out, chart, QwtPlot::yLeft);
    DA::serialize_out_scale_widge(out, chart, QwtPlot::yRight);
    // QwtPlotCanvas save
    const QwtPlotCanvas* canvas = qobject_cast< const QwtPlotCanvas* >(chart->canvas());
    bool isDefaultCanvas        = (canvas != nullptr);
    out << isDefaultCanvas;
    if (isDefaultCanvas) {
        out << canvas;
    }
    return out;
}

/**
 * @brief QwtPlot的基本序列化支持
 * @param in
 * @param chart
 * @return
 */
QDataStream& operator>>(QDataStream& in, QwtPlot* chart)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
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
    DA::serialize_in_scale_widge(in, chart, QwtPlot::xTop);
    DA::serialize_in_scale_widge(in, chart, QwtPlot::xBottom);
    DA::serialize_in_scale_widge(in, chart, QwtPlot::yLeft);
    DA::serialize_in_scale_widge(in, chart, QwtPlot::yRight);
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

//----------------------------------------------------
// QwtColorMap
//----------------------------------------------------
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
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark;
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
    if (DA::gc_dachart_magic_mark != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    int format;
    in >> format;
    c->setFormat(static_cast< QwtColorMap::Format >(format));
    return in;
}

//----------------------------------------------------
// QwtLinearColorMap
//----------------------------------------------------
/**
 * @brief QwtLinearColorMap的序列化
 * @param out
 * @param c
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtLinearColorMap* c)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark2;
    out << static_cast< const QwtColorMap* >(c);
    out << static_cast< int >(c->mode());
    // color1 对应colorstop:0.0,color2 对应colorstop:1.0
    // 写入颜色停止点的数量
    const QVector< double > stopPos = c->stopPos();
    out << stopPos;
    // 写入停止点对应的颜色
    const QVector< QColor > stopColors = c->stopColors();
    out << stopColors;
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
    if (DA::gc_dachart_magic_mark2 != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> static_cast< QwtColorMap* >(c);
    int mode;
    QVector< double > stopPos;
    QVector< QColor > stopColors;
    in >> mode >> stopPos >> stopColors;
    const int size = qMin(stopPos.size(), stopColors.size());
    if (size < 2) {
        throw DA::DABadSerializeExpection("QwtLinearColorMap get color less than 2");
    }
    // 确认最开始和最后是0.0和1.0
    if (!qFuzzyCompare(stopPos.first(), 0.0)) {
        throw DA::DABadSerializeExpection("QwtLinearColorMap first stop color pos is not 0.0");
    }
    if (!qFuzzyCompare(stopPos.last(), 1.0)) {
        throw DA::DABadSerializeExpection("QwtLinearColorMap last stop color pos is not 1.0");
    }
    c->setMode(static_cast< QwtLinearColorMap::Mode >(mode));
    for (int i = 0; i < size; ++i) {
        c->addColorStop(stopPos[ i ], stopColors[ i ]);
    }

    return in;
}

//----------------------------------------------------
// QwtAlphaColorMap
//----------------------------------------------------
/**
 * @brief QwtAlphaColorMap的序列化
 * @param out
 * @param c
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtAlphaColorMap* c)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark2;
    out << static_cast< const QwtColorMap* >(c);
    out << c->alpha1() << c->alpha2() << c->color();
    return out;
}

/**
 * @brief QwtAlphaColorMap 的序列化
 * @param in
 * @param c
 * @return
 */
QDataStream& operator>>(QDataStream& in, QwtAlphaColorMap* c)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark2 != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> static_cast< QwtColorMap* >(c);
    int alpha1, alpha2;
    QColor color;
    in >> alpha1 >> alpha2 >> color;
    c->setAlphaInterval(alpha1, alpha2);
    c->setColor(color);
    return in;
}

//----------------------------------------------------
// QwtHueColorMap
//----------------------------------------------------
/**
 * @brief QwtHueColorMap的序列化
 * @param out 输出数据流
 * @param c QwtHueColorMap指针
 * @return 输出数据流
 */
QDataStream& operator<<(QDataStream& out, const QwtHueColorMap* c)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark2;
    out << static_cast< const QwtColorMap* >(c);
    out << c->hue1() << c->hue2() << c->saturation() << c->value() << c->alpha();
    return out;
}
/**
 * @brief QwtHueColorMap的反序列化
 * @param in 输入数据流
 * @param c QwtHueColorMap指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtHueColorMap* c)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark2 != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> static_cast< QwtColorMap* >(c);
    int hue1;
    int hue2;
    int saturation;
    int value;
    int alpha;
    in >> hue1 >> hue2 >> saturation >> value >> alpha;
    c->setHueInterval(hue1, hue2);
    c->setSaturation(saturation);
    c->setValue(value);
    c->setAlpha(alpha);
    return in;
}

//----------------------------------------------------
// QwtSaturationValueColorMap
//----------------------------------------------------

/**
 * @brief QwtSaturationValueColorMap的序列化
 * @param out 输出数据流
 * @param c QwtSaturationValueColorMap指针
 * @return 输出数据流
 */
QDataStream& operator<<(QDataStream& out, const QwtSaturationValueColorMap* c)
{
    out << DA::gc_dachart_version << DA::gc_dachart_magic_mark2;
    out << static_cast< const QwtColorMap* >(c);
    out << c->hue() << c->saturation1() << c->saturation2() << c->value1() << c->value2() << c->alpha();
    return out;
}
/**
 * @brief QwtSaturationValueColorMap的反序列化
 * @param in 输入数据流
 * @param c QwtSaturationValueColorMap指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtSaturationValueColorMap* c)
{
    int version;
    std::uint32_t magic;
    in >> version >> magic;
    if (DA::gc_dachart_magic_mark2 != magic) {
        throw DA::DABadSerializeExpection();
        return in;
    }
    in >> static_cast< QwtColorMap* >(c);
    int hue;
    int saturation1;
    int saturation2;
    int value1;
    int value2;
    int alpha;
    in >> hue >> saturation1 >> saturation2 >> value1 >> value2 >> alpha;
    c->setHue(hue);
    c->setSaturationInterval(saturation1, saturation2);
    c->setValueInterval(value1, value2);
    c->setAlpha(alpha);
    return in;
}
//----------------------------------------------------
// QwtIntervalSample
//----------------------------------------------------

/**
 * @brief QwtIntervalSample序列化支持
 * @param out
 * @param item
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtIntervalSample& item)
{
    out << item.value << item.interval;
    return out;
}

/**
 * @brief QwtIntervalSample序列化支持
 * @param in
 * @param item
 * @return
 */
QDataStream& operator>>(QDataStream& in, QwtIntervalSample& item)
{
    in >> item.value >> item.interval;
    return in;
}
//----------------------------------------------------
// QwtInterval
//----------------------------------------------------
/**
 * @brief QwtInterval序列化支持
 * @param out
 * @param item
 * @return
 */
QDataStream& operator<<(QDataStream& out, const QwtInterval& item)
{
    out << item.minValue() << item.maxValue() << item.borderFlags();
    return out;
}

/**
 * @brief QwtInterval序列化支持
 * @param in
 * @param item
 * @return
 */
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
// QwtBoxSample
//----------------------------------------------------

/**
 * @brief QwtBoxSample序列化支持
 * @param out 输出数据流
 * @param item QwtBoxSample引用
 * @return 输出数据流
 */
QDataStream& operator<<(QDataStream& out, const QwtBoxSample& item)
{
    out << item.position << item.whiskerLower << item.q1 << item.median << item.q3 << item.whiskerUpper
        << item.outlierCount;
    return out;
}

/**
 * @brief QwtBoxSample反序列化支持
 * @param in 输入数据流
 * @param item QwtBoxSample引用
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, QwtBoxSample& item)
{
    in >> item.position >> item.whiskerLower >> item.q1 >> item.median >> item.q3 >> item.whiskerUpper
        >> item.outlierCount;
    item.lower   = item.whiskerLower;
    item.upper   = item.whiskerUpper;
    item.center  = item.median;
    return in;
}
