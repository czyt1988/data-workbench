// DAChartSerializeTest - DAFigure 图表层宿主回归测试
//
// 覆盖评审报告里的三类问题：
// 1. B3：QwtMatrixRasterData 不推导 Z 轴区间，谱图 Z 区间无效时 renderImage 返回空图
//         （统计热图/二维核密度图整片空白），且 style 里的 vmin/vmax 从未被读取；
// 2. B4/H1：色标条块版本闸门写错（scaleDrawType 从 v2 起存在）与读出区间未回填；
// 3. H6：区间高亮（QwtPlotZoneItem）序列化漏了 orientation。

#include <QtTest/QtTest>
#include <QApplication>
#include <QBrush>
#include <QByteArray>
#include <QColor>
#include <QDataStream>
#include <QImage>
#include <QPen>
#include <QPixmap>
#include <QSet>
#include <QVariantMap>
#include <cstring>
#include <limits>
#include <memory>

#include "DAChart3DSerialize.h"
#include "DAChartPlotRenderer.h"
#include "DAChartSerialize.h"
#include "DAChartWidget.h"
#include "DAPlotDataTypes.h"
#include "qwt_colormap.h"
#include "qwt_colormap_preset.h"
#include "qwt_interval.h"
#include "qwt_matrix_raster_data.h"
#include "qwt_plot_item.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_plot_zoneitem.h"
#include "qwt_raster_data.h"
#include "qwt_scale_widget.h"
#include "qwt_text.h"
#include "qwt_text_scale_draw.h"
#include "qwt3d_waterfall.h"

namespace
{
/// 构造 4 列 × 3 行的谱图数据（行主序，值 0..11）
DA::DASpectrogramData makeSpectrogramData()
{
    DA::DASpectrogramData data;
    data.nrows = 3;
    data.ncols = 4;
    data.xmin  = 0.0;
    data.xmax  = 3.0;
    data.ymin  = 0.0;
    data.ymax  = 2.0;
    data.values.reserve(data.nrows * data.ncols);
    for (int i = 0; i < data.nrows * data.ncols; ++i) {
        data.values.append(static_cast< double >(i));
    }
    return data;
}

/// 统计画布中心区域的颜色数（排除坐标轴/边距；全空白时只有背景一种颜色）
int countCanvasColors(DA::DAChartWidget& chart)
{
    chart.replot();
    const QPixmap pm = chart.grab();
    if (pm.isNull()) {
        return 0;
    }
    const QImage img = pm.toImage();
    if (img.isNull() || !chart.canvas()) {
        return 0;
    }
    const QRect canvasRect = chart.canvas()->geometry();
    const QRect sample     = canvasRect.adjusted(8, 8, -8, -8).intersected(QRect(QPoint(0, 0), img.size()));
    if (sample.isEmpty()) {
        return 0;
    }
    QSet< QRgb > colors;
    for (int y = sample.top(); y <= sample.bottom(); ++y) {
        for (int x = sample.left(); x <= sample.right(); ++x) {
            colors.insert(img.pixel(x, y));
        }
    }
    return colors.size();
}

/// 以单元格中心坐标采样栅格数据（行主序：index = row * ncols + col）
double rasterValueAtCell(const QwtRasterData* rd, int nrows, int ncols, int row, int col)
{
    const QwtInterval ix = rd->interval(Qt::XAxis);
    const QwtInterval iy = rd->interval(Qt::YAxis);
    const double x       = ix.minValue() + (col + 0.5) * ix.width() / ncols;
    const double y       = iy.minValue() + (row + 0.5) * iy.width() / nrows;
    return rd->value(x, y);
}
}  // namespace

class DAChartSerializeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    // 未给 style vmin/vmax 时必须按有效数据推导 Z 区间（否则渲染空图）
    void testSpectrogramZIntervalFallsBackToDataRange();
    // style 的 vmin/vmax 优先于数据范围，真正压动态范围
    void testSpectrogramStyleVminVmaxOverride();
    // 常量矩阵兜底 Z 区间为 [v-0.5, v+0.5]（与数据量纲一致，不能是固定 0..1）
    void testSpectrogramConstantMatrixIntervalValid();
    // 渲染结果非空：修复前 renderImage 因 Z 区间无效直接返回空图
    void testSpectrogramRendersNonEmptyImage();
    // 同一数据、不同 vmin/vmax 的渲染像素必须不同（证明 vmin/vmax 生效）
    void testSpectrogramVminVmaxChangesPixels();
    // H2：缺测单元必须以 NaN 填充（不是 0），渲染时该单元不着色
    void testSpectrogramNaNFill();
    // H7：谱图图元二进制往返（生产路径 DAChartItemSerialize：栅格数据 + Z 区间 + 色带）
    void testSpectrogramItemBinaryRoundTrip();
    // H7：三维瀑布图元二进制往返（生产路径 DAChart3DItemSerialize）
    void testWaterfall3DItemBinaryRoundTrip();
    // v3 色标条块往返：启用状态/宽度/区间/色带停止点
    void testScaleWidgetRoundTripV3();
    // N-B4T：真实 v2 字节流（按 v2 布局手写、无色带段），读后必须正好消费完
    void testScaleWidgetReadsRealV2Stream();
    // N-B4T：v1 旧块（无 scaleDrawType、无色带段）兼容
    void testScaleWidgetReadsV1Stream();
    // 区间高亮方向往返（水平区间重开后不能变垂直）
    void testZoneOrientationRoundTrip();
};

void DAChartSerializeTest::testSpectrogramZIntervalFallsBackToDataRange()
{
    DA::DAChartWidget chart;
    DAChartPlotRenderer renderer(&chart);
    QwtPlotItem* item = renderer.renderSpectrogram(makeSpectrogramData(), QVariantMap());
    QVERIFY(item);
    auto* spectro = static_cast< QwtPlotSpectrogram* >(item);
    QVERIFY(spectro->data());
    const QwtInterval z = spectro->data()->interval(Qt::ZAxis);
    QVERIFY2(z.isValid(), "Z interval must always be valid (otherwise renderImage returns an empty image)");
    QCOMPARE(z.minValue(), 0.0);
    QCOMPARE(z.maxValue(), 11.0);
}

void DAChartSerializeTest::testSpectrogramStyleVminVmaxOverride()
{
    DA::DAChartWidget chart;
    DAChartPlotRenderer renderer(&chart);
    QVariantMap style;
    style[ QStringLiteral("vmin") ] = 2.0;
    style[ QStringLiteral("vmax") ] = 5.0;
    QwtPlotItem* item = renderer.renderSpectrogram(makeSpectrogramData(), style);
    QVERIFY(item);
    const QwtInterval z = static_cast< QwtPlotSpectrogram* >(item)->data()->interval(Qt::ZAxis);
    QVERIFY(z.isValid());
    QCOMPARE(z.minValue(), 2.0);
    QCOMPARE(z.maxValue(), 5.0);
}

void DAChartSerializeTest::testSpectrogramConstantMatrixIntervalValid()
{
    DA::DASpectrogramData data = makeSpectrogramData();
    data.values.fill(3.0);
    DA::DAChartWidget chart;
    DAChartPlotRenderer renderer(&chart);
    QwtPlotItem* item = renderer.renderSpectrogram(data, QVariantMap());
    QVERIFY(item);
    const QwtInterval z = static_cast< QwtPlotSpectrogram* >(item)->data()->interval(Qt::ZAxis);
    QVERIFY(z.isValid());
    QVERIFY(z.maxValue() > z.minValue());
    // 常量矩阵 v=3：兜底区间必须是 [v-0.5, v+0.5]，而不是与量纲脱节的 0..1
    QCOMPARE(z.minValue(), 2.5);
    QCOMPARE(z.maxValue(), 3.5);
}

void DAChartSerializeTest::testSpectrogramRendersNonEmptyImage()
{
    DA::DAChartWidget chart;
    chart.resize(480, 360);
    chart.show();
    QTest::qWait(20);
    DAChartPlotRenderer renderer(&chart);
    QVERIFY(renderer.renderSpectrogram(makeSpectrogramData(), QVariantMap()));
    chart.replot();
    const int colors = countCanvasColors(chart);
    QVERIFY2(colors > 10, qPrintable(QStringLiteral("canvas colors = %1 (blank spectrogram?)").arg(colors)));
}

void DAChartSerializeTest::testSpectrogramVminVmaxChangesPixels()
{
    const auto renderPixels = [](double vmin, double vmax) {
        DA::DAChartWidget chart;
        chart.resize(320, 240);
        chart.show();
        QTest::qWait(10);
        DAChartPlotRenderer renderer(&chart);
        QVariantMap style;
        style[ QStringLiteral("vmin") ] = vmin;
        style[ QStringLiteral("vmax") ] = vmax;
        renderer.renderSpectrogram(makeSpectrogramData(), style);
        chart.replot();
        return chart.grab().toImage();
    };
    const QImage narrow = renderPixels(0.0, 1.0);
    const QImage wide   = renderPixels(0.0, 11.0);
    QVERIFY(!narrow.isNull() && !wide.isNull());
    QVERIFY(narrow.size() == wide.size());
    QVERIFY2(narrow != wide, "vmin/vmax must change the rendered colors");
}

void DAChartSerializeTest::testSpectrogramNaNFill()
{
    // 3x4 矩阵只给 10 个值：最后两个单元必须补 NaN，而不是 0（H2）
    DA::DASpectrogramData data = makeSpectrogramData();
    data.values.resize(10);
    DA::DAChartWidget chart;
    chart.resize(320, 240);
    chart.show();
    QTest::qWait(10);
    DAChartPlotRenderer renderer(&chart);
    QwtPlotItem* item = renderer.renderSpectrogram(data, QVariantMap());
    QVERIFY(item);
    const QwtRasterData* rd = static_cast< QwtPlotSpectrogram* >(item)->data();
    QVERIFY(rd);
    // 已提供单元保留原值（0..9）
    QCOMPARE(rasterValueAtCell(rd, data.nrows, data.ncols, 0, 0), 0.0);
    QCOMPARE(rasterValueAtCell(rd, data.nrows, data.ncols, 2, 1), 9.0);
    // 缺测单元（index 10/11）为 NaN，渲染时不着色
    QVERIFY2(std::isnan(rasterValueAtCell(rd, data.nrows, data.ncols, 2, 2)),
             "missing cell (index 10) must be NaN, not 0");
    QVERIFY2(std::isnan(rasterValueAtCell(rd, data.nrows, data.ncols, 2, 3)),
             "missing cell (index 11) must be NaN, not 0");
    // 有效单元仍可见（不是整片空白）
    chart.replot();
    const int colors = countCanvasColors(chart);
    QVERIFY2(colors > 10, qPrintable(QStringLiteral("canvas colors = %1").arg(colors)));
}

void DAChartSerializeTest::testSpectrogramItemBinaryRoundTrip()
{
    // 生产路径（工程 zip 的 chart-data）序列化：spetrogram item 走 DAChartItemSerialize
    DA::DAChartWidget chart;
    DAChartPlotRenderer renderer(&chart);
    QVariantMap style;
    style[ QStringLiteral("vmin") ] = 1.5;
    style[ QStringLiteral("vmax") ] = 9.5;
    style[ QStringLiteral("cmap") ] = QStringLiteral("viridis");
    style[ QStringLiteral("title") ] = QStringLiteral("spec-roundtrip");
    QwtPlotItem* item             = renderer.renderSpectrogram(makeSpectrogramData(), style);
    QVERIFY(item);

    DA::DAChartItemSerialize serializer;
    const QByteArray bytes = serializer.serializeOut(item);
    QVERIFY(!bytes.isEmpty());
    QCOMPARE(serializer.getRtti(bytes), static_cast< int >(QwtPlotItem::Rtti_PlotSpectrogram));

    std::unique_ptr< QwtPlotItem > restored(serializer.serializeIn(bytes));
    QVERIFY2(restored, "spectrogram item must be deserializable through the production path");
    auto* spectro = dynamic_cast< QwtPlotSpectrogram* >(restored.get());
    QVERIFY(spectro);
    QVERIFY(spectro->data());
    QCOMPARE(spectro->title().text(), QStringLiteral("spec-roundtrip"));
    // X/Y 区间
    const QwtInterval ix = spectro->data()->interval(Qt::XAxis);
    const QwtInterval iy = spectro->data()->interval(Qt::YAxis);
    QCOMPARE(ix.minValue(), 0.0);
    QCOMPARE(ix.maxValue(), 3.0);
    QCOMPARE(iy.minValue(), 0.0);
    QCOMPARE(iy.maxValue(), 2.0);
    // Z 区间（style vmin/vmax）
    const QwtInterval z = spectro->data()->interval(Qt::ZAxis);
    QCOMPARE(z.minValue(), 1.5);
    QCOMPARE(z.maxValue(), 9.5);
    // 栅格矩阵值本身
    QCOMPARE(rasterValueAtCell(spectro->data(), 3, 4, 0, 0), 0.0);
    QCOMPARE(rasterValueAtCell(spectro->data(), 3, 4, 2, 3), 11.0);
    // 色带
    auto* cmap = dynamic_cast< const QwtLinearColorMap* >(spectro->colorMap());
    QVERIFY2(cmap, "spectrogram color map must survive the binary round trip");
    QVERIFY(cmap->stopColors().size() >= 3);
}

void DAChartSerializeTest::testWaterfall3DItemBinaryRoundTrip()
{
    Qwt3DWaterfall src;
    src.setTitle(QStringLiteral("wf-roundtrip"));
    src.setTraceCapacity(5);
    src.setPlotStyle(Qwt3DWaterfall::RibbonMesh);
    src.setColorMode(Qwt3DWaterfall::ByAmplitude);
    src.setDepthOrder(Qwt3DWaterfall::NewestBehind);
    src.setBaseline(-1.25);
    src.setBinValues(QVector< double >({ 0.0, 0.5, 1.0 }));

    DA::DAChart3DItemSerialize serializer;
    const QByteArray bytes = serializer.serializeOut(&src);
    QVERIFY(!bytes.isEmpty());
    QCOMPARE(serializer.getRtti(bytes), src.rtti());

    std::unique_ptr< Qwt3DPlotItem > restored(serializer.serializeIn(bytes));
    QVERIFY2(restored, "waterfall item must be deserializable through the production path");
    auto* wf = dynamic_cast< Qwt3DWaterfall* >(restored.get());
    QVERIFY(wf);
    QCOMPARE(wf->rtti(), src.rtti());
    QCOMPARE(wf->title(), QStringLiteral("wf-roundtrip"));
    QCOMPARE(wf->traceCapacity(), 5);
    QCOMPARE(static_cast< int >(wf->plotStyle()), static_cast< int >(Qwt3DWaterfall::RibbonMesh));
    QCOMPARE(static_cast< int >(wf->colorMode()), static_cast< int >(Qwt3DWaterfall::ByAmplitude));
    QCOMPARE(static_cast< int >(wf->depthOrder()), static_cast< int >(Qwt3DWaterfall::NewestBehind));
    QCOMPARE(wf->baseline(), -1.25);
    QCOMPARE(wf->binValues(), QVector< double >({ 0.0, 0.5, 1.0 }));
}

void DAChartSerializeTest::testScaleWidgetRoundTripV3()
{
    QwtScaleWidget src;
    src.setColorBarEnabled(true);
    src.setColorBarWidth(22);
    QwtLinearColorMap* cmap = new QwtLinearColorMap(Qt::blue, Qt::red);
    cmap->addColorStop(0.5, Qt::green);
    src.setColorMap(QwtInterval(1.25, 8.75), cmap);
    auto* textDraw = new QwtTextScaleDraw();
    QMap< double, QString > labelMap;
    labelMap.insert(1.25, QStringLiteral("low"));
    labelMap.insert(8.75, QStringLiteral("high"));
    textDraw->setLabelMap(labelMap);
    src.setScaleDraw(textDraw);

    QByteArray bytes;
    {
        QDataStream out(&bytes, QIODevice::WriteOnly);
        out.setVersion(DA::gc_datastream_version);
        out << static_cast< const QwtScaleWidget* >(&src);
    }

    QwtScaleWidget dst;
    {
        QDataStream in(&bytes, QIODevice::ReadOnly);
        in.setVersion(DA::gc_datastream_version);
        in >> static_cast< QwtScaleWidget* >(&dst);
    }
    QVERIFY(dst.isColorBarEnabled());
    QCOMPARE(dst.colorBarWidth(), 22);
    QVERIFY(dst.colorBarInterval().isValid());
    QCOMPARE(dst.colorBarInterval().minValue(), 1.25);
    QCOMPARE(dst.colorBarInterval().maxValue(), 8.75);
    auto* readCmap = dynamic_cast< const QwtLinearColorMap* >(dst.colorMap());
    QVERIFY2(readCmap, "color map must survive the round trip");
    QVERIFY(readCmap->stopColors().size() >= 3);
    auto* readDraw = dynamic_cast< QwtTextScaleDraw* >(dst.scaleDraw());
    QVERIFY2(readDraw, "QwtTextScaleDraw subtype must survive the round trip");
    QCOMPARE(readDraw->labelMap().value(1.25), QStringLiteral("low"));
    QCOMPARE(readDraw->labelMap().value(8.75), QStringLiteral("high"));
}

void DAChartSerializeTest::testScaleWidgetReadsRealV2Stream()
{
    // 按 v2 布局手写真实字节流：块版本=2（有 scaleDrawType，**没有** v3 的色带段）。
    // 只改前 4 字节版本号的旧用例测不出"色带段闸门写错"：v3 色带字节仍留在流里。
    QByteArray bytes;
    {
        QDataStream out(&bytes, QIODevice::WriteOnly);
        out.setVersion(DA::gc_datastream_version);
        out << 2 << DA::gc_dachart_magic_mark;
        out << static_cast< unsigned int >(0x82fa34);
        QwtText title;
        title.setText(QStringLiteral("v2-title"));
        out << title << false << 5 << 7 << 0 << 0 << 4 << 3 << true << 18
            << static_cast< int >(QwtScaleDraw::LeftScale) << QwtInterval(2.0, 9.0);
        // QwtScaleDraw：v2 起写子类类型标记
        out << true << 1;
        QwtTextScaleDraw srcDraw;
        QMap< double, QString > labelMap;
        labelMap.insert(0.0, QStringLiteral("zero"));
        labelMap.insert(1.0, QStringLiteral("one"));
        srcDraw.setLabelMap(labelMap);
        out << static_cast< const QwtScaleDraw* >(&srcDraw);
        // QwtTextScaleDraw 附加字段：labelMap + 显式 scaleDiv bounds
        out << static_cast< quint32 >(2) << 0.0 << QStringLiteral("zero") << 1.0 << QStringLiteral("one") << 0.0 << 1.0;
        // v2 到此结束：没有色带段
    }

    QwtScaleWidget dst;
    {
        QDataStream in(&bytes, QIODevice::ReadOnly);
        in.setVersion(DA::gc_datastream_version);
        in >> static_cast< QwtScaleWidget* >(&dst);
        // 若色带段闸门被错写成"无条件读"，这里会读越界 → 状态不再是 Ok
        QVERIFY2(in.status() == QDataStream::Ok, "v2 stream must not be read past the end (color-map gate)");
        QVERIFY2(in.atEnd(), "v2 stream must be consumed exactly (unexpected trailing bytes)");
    }
    QCOMPARE(dst.isColorBarEnabled(), true);
    QCOMPARE(dst.colorBarWidth(), 18);
    auto* readDraw = dynamic_cast< QwtTextScaleDraw* >(dst.scaleDraw());
    QVERIFY2(readDraw, "v2 block must restore QwtTextScaleDraw (scaleDrawType gate must be >= 2)");
    QCOMPARE(readDraw->labelMap().value(0.0), QStringLiteral("zero"));
    QCOMPARE(readDraw->labelMap().value(1.0), QStringLiteral("one"));
    // 旧 v2 文件本来就没有色带段，区间无法回填 → 保持不绘制（与旧行为一致，不视为回归）
    QVERIFY(!dst.colorBarInterval().isValid());
}

void DAChartSerializeTest::testScaleWidgetReadsV1Stream()
{
    // v1 布局：无 scaleDrawType、无色带段；scaleDraw 直接写基类属性
    QByteArray bytes;
    {
        QDataStream out(&bytes, QIODevice::WriteOnly);
        out.setVersion(DA::gc_datastream_version);
        out << 1 << DA::gc_dachart_magic_mark;
        out << static_cast< unsigned int >(0x82fa34);
        QwtText title;
        title.setText(QStringLiteral("v1-title"));
        out << title << false << 2 << 4 << 0 << 0 << 6 << 2 << false << 12
            << static_cast< int >(QwtScaleDraw::BottomScale) << QwtInterval();
        out << true;  // isSD
        QwtScaleDraw plainDraw;
        plainDraw.setLabelRotation(30.0);
        out << static_cast< const QwtScaleDraw* >(&plainDraw);
        // v1 到此结束
    }

    QwtScaleWidget dst;
    {
        QDataStream in(&bytes, QIODevice::ReadOnly);
        in.setVersion(DA::gc_datastream_version);
        in >> static_cast< QwtScaleWidget* >(&dst);
        QVERIFY2(in.status() == QDataStream::Ok, "v1 stream must not be read past the end");
        QVERIFY2(in.atEnd(), "v1 stream must be consumed exactly (unexpected trailing bytes)");
    }
    QCOMPARE(dst.isColorBarEnabled(), false);
    QCOMPARE(dst.colorBarWidth(), 12);
    QVERIFY2(dst.scaleDraw() != nullptr, "v1 block must keep/restore a QwtScaleDraw");
    QVERIFY2(dynamic_cast< QwtTextScaleDraw* >(dst.scaleDraw()) == nullptr,
             "v1 block has no scaleDrawType, must not turn into QwtTextScaleDraw");
    QCOMPARE(dst.scaleDraw()->labelRotation(), 30.0);
    QVERIFY(!dst.colorBarInterval().isValid());
}

void DAChartSerializeTest::testZoneOrientationRoundTrip()
{
    QwtPlotZoneItem src;
    src.setOrientation(Qt::Horizontal);
    src.setInterval(QwtInterval(2.0, 5.0));
    src.setPen(QPen(QColor(82, 128, 193), 1.5));
    src.setBrush(QBrush(QColor(189, 205, 232, 60)));

    QByteArray bytes;
    {
        QDataStream out(&bytes, QIODevice::WriteOnly);
        out.setVersion(DA::gc_datastream_version);
        out << static_cast< const QwtPlotZoneItem* >(&src);
    }
    QwtPlotZoneItem dst;
    {
        QDataStream in(&bytes, QIODevice::ReadOnly);
        in.setVersion(DA::gc_datastream_version);
        in >> static_cast< QwtPlotZoneItem* >(&dst);
    }
    QCOMPARE(dst.orientation(), Qt::Horizontal);
    QCOMPARE(dst.interval().minValue(), 2.0);
    QCOMPARE(dst.interval().maxValue(), 5.0);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DAChartSerializeTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "main.moc"
