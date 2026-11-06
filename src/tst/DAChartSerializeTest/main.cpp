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

#include "DAChartPlotRenderer.h"
#include "DAChartSerialize.h"
#include "DAChartWidget.h"
#include "DAPlotDataTypes.h"
#include "qwt_colormap.h"
#include "qwt_interval.h"
#include "qwt_plot_item.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_plot_zoneitem.h"
#include "qwt_raster_data.h"
#include "qwt_scale_widget.h"
#include "qwt_text_scale_draw.h"

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
}  // namespace

class DAChartSerializeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    // 未给 zmin/zmax 时必须按有效数据推导 Z 区间（否则渲染空图）
    void testSpectrogramZIntervalFallsBackToDataRange();
    // style 的 vmin/vmax 优先于数据范围，真正压动态范围
    void testSpectrogramStyleVminVmaxOverride();
    // 常量矩阵也要有有效 Z 区间（不能是零宽/无效区间）
    void testSpectrogramConstantMatrixIntervalValid();
    // 渲染结果非空：修复前 renderImage 因 Z 区间无效直接返回空图
    void testSpectrogramRendersNonEmptyImage();
    // 同一数据、不同 vmin/vmax 的渲染像素必须不同（证明 vmin/vmax 生效）
    void testSpectrogramVminVmaxChangesPixels();
    // v3 色标条块往返：启用状态/宽度/区间/色带停止点
    void testScaleWidgetRoundTripV3();
    // v2 旧块：scaleDrawType 从 v2 起存在，必须能读回 QwtTextScaleDraw 与 labelMap
    void testScaleWidgetReadsV2Stream();
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

void DAChartSerializeTest::testScaleWidgetReadsV2Stream()
{
    // 先按 v3 写一遍，再把块版本改回 2：模拟存量 v2 工程（v2 有 scaleDrawType，但没有色带段）
    QwtScaleWidget src;
    auto* textDraw = new QwtTextScaleDraw();
    QMap< double, QString > labelMap;
    labelMap.insert(0.0, QStringLiteral("zero"));
    labelMap.insert(1.0, QStringLiteral("one"));
    textDraw->setLabelMap(labelMap);
    src.setScaleDraw(textDraw);

    QByteArray bytes;
    {
        QDataStream out(&bytes, QIODevice::WriteOnly);
        out.setVersion(DA::gc_datastream_version);
        out << static_cast< const QwtScaleWidget* >(&src);
    }
    QVERIFY(bytes.size() > 4);
    // QDataStream 默认大端，块版本是写在前 4 个字节的 int
    bytes[ 0 ] = 0x00;
    bytes[ 1 ] = 0x00;
    bytes[ 2 ] = 0x00;
    bytes[ 3 ] = 0x02;

    QwtScaleWidget dst;
    try {
        QDataStream in(&bytes, QIODevice::ReadOnly);
        in.setVersion(DA::gc_datastream_version);
        in >> static_cast< QwtScaleWidget* >(&dst);
    } catch (const std::exception& e) {
        QFAIL(qPrintable(QStringLiteral("v2 stream read threw: %1").arg(QString::fromUtf8(e.what()))));
    }
    auto* readDraw = dynamic_cast< QwtTextScaleDraw* >(dst.scaleDraw());
    QVERIFY2(readDraw, "v2 block must still restore QwtTextScaleDraw (scaleDrawType gate must be >= 2)");
    QCOMPARE(readDraw->labelMap().value(0.0), QStringLiteral("zero"));
    QCOMPARE(readDraw->labelMap().value(1.0), QStringLiteral("one"));
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
