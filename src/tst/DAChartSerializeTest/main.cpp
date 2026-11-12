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
#include <QDomDocument>
#include <QFile>
#include <QHash>
#include <QImage>
#include <QPen>
#include <QPixmap>
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QVariantMap>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

#include "Archive/DAChartItemArchive.h"
#include "Archive/DAXmlHelper.h"
#include "Archive/DAZipArchive.h"
#include "Archive/DAZipArchiveTask_ChartItem.h"
#include "Chart/DAChartItemsManager.h"
#include "DAChart3DSerialize.h"
#include "DAChartAxisRangeBinder.h"
#include "DAChartPlotRenderer.h"
#include "DAChartSerialize.h"
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
#include "DAPlotDataTypes.h"
#include "qwt_colormap.h"
#include "qwt_colormap_preset.h"
#include "qwt_interval.h"
#include "qwt_matrix_raster_data.h"
#include "qwt_figure.h"
#include "qwt_figure_grid.h"
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

/// 归一化几何/比例比较（网格 layout 换算存在浮点误差）
bool nearlyEqual(qreal lhs, qreal rhs, qreal eps = 1e-6)
{
    return qAbs(lhs - rhs) <= eps;
}

/// 解析 XML 里的逗号分隔实数列表（rowSizes/colSizes）
QList< qreal > parseRealList(const QString& text)
{
    QList< qreal > result;
    const QStringList parts = text.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        bool ok = false;
        const qreal v = part.trimmed().toDouble(&ok);
        if (ok) {
            result.append(v);
        }
    }
    return result;
}

/// 成员节点是否带完整的网格坐标属性
bool hasGridCellAttributes(const QDomElement& ele)
{
    return ele.hasAttribute(QStringLiteral("grid-row")) && ele.hasAttribute(QStringLiteral("grid-col"))
        && ele.hasAttribute(QStringLiteral("grid-rowspan")) && ele.hasAttribute(QStringLiteral("grid-colspan"));
}

/// 在 figure 元素下按 chart id 查找 <chart> 节点
QDomElement findChartElement(const QDomElement& figureEle, const QString& chartId)
{
    const QDomElement chartsEle = figureEle.firstChildElement(QStringLiteral("charts"));
    for (QDomElement ele = chartsEle.firstChildElement(QStringLiteral("chart")); !ele.isNull();
         ele             = ele.nextSiblingElement(QStringLiteral("chart"))) {
        if (ele.attribute(QStringLiteral("id")) == chartId) {
            return ele;
        }
    }
    return QDomElement();
}

/// 网格比例/几何列表比较
bool realListNearlyEqual(const QList< qreal >& lhs, const QList< qreal >& rhs, qreal eps = 1e-6)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (int i = 0; i < lhs.size(); ++i) {
        if (!nearlyEqual(lhs.at(i), rhs.at(i), eps)) {
            return false;
        }
    }
    return true;
}

/// key 集合（DAChartItemsManager 内部 QHash 顺序不稳定，比较集合）
QSet< QString > keySet(const QList< QString >& keys)
{
    QSet< QString > result;
    for (const QString& key : keys) {
        result.insert(key);
    }
    return result;
}

/// 内存 archive：验证 DAChartItemArchive 的 save/load，不依赖磁盘/zip
class MemoryArchive : public DA::DAAbstractArchive
{
public:
    MemoryArchive() : DA::DAAbstractArchive(nullptr)
    {
    }

    bool write(const QString& relatePath, const QByteArray& byte) override
    {
        // 注入写失败/异常，用于验证 DAChartItemArchive::save 的单图元失败继续语义
        if (throwWritePaths.contains(relatePath)) {
            throw std::runtime_error("injected write failure");
        }
        if (failWritePaths.contains(relatePath)) {
            return false;
        }
        m_files.insert(relatePath, byte);
        return true;
    }

    QByteArray read(const QString& relatePath) override
    {
        return m_files.value(relatePath);
    }

    bool remove(const QString& relatePath) override
    {
        return m_files.remove(relatePath) > 0;
    }

    void saveAll(const QString& filePath) override
    {
        Q_UNUSED(filePath);
    }

    void loadAll(const QString& filePath) override
    {
        Q_UNUSED(filePath);
    }

    // 与真实存档语义一致：只要 key 存在就返回 true（0 字节文件也算存在）。
    // DAChartItemArchive::load 依赖此语义区分「索引缺失」与「索引存在但为空/损坏」。
    bool contains(const QString& relatePath) const override
    {
        return m_files.contains(relatePath);
    }

    QHash< QString, QByteArray > m_files;
    QSet< QString > failWritePaths;
    QSet< QString > throwWritePaths;
};
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
    // transient 图元 API：标记/查询/列表，removePlotItem 后清理，空指针安全
    void testTransientItemApi();
    // transient 图元不进入 chart-data：<items> 与 itemsMgr 都只记录非 transient 图元
    void testTransientItemSkippedInChartData();
    // 析构时仍挂着 transient 图元不得崩溃（itemAttached 析构期悬空回调回归）
    void testTransientChartDestruction();
    // 网格 figure XML 往返：<grid> 行列/比例、成员 grid-* 坐标、x/y/w/h 兼容字段
    void testFigureGridXmlRoundTrip();
    // 旧工程兼容：无 <grid> 的 figure 走自由布局，且自由布局写出不带网格字段
    void testLegacyFigureWithoutGrid();
    // 非法 grid-* 坐标回退自由布局；缺 x/y/w/h 时回退默认矩形
    void testInvalidGridMemberFallsBackToFreeLayout();
    // DAChartItemArchive 内存 archive 往返（索引 + 每图元二进制）
    void testChartItemArchiveMemoryRoundTrip();
    // DAZipArchiveTask_ChartItem 真实 zip 往返（DAAppProject 图表存档路径）
    void testZipArchiveTaskChartItemRoundTrip();
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

void DAChartSerializeTest::testTransientItemApi()
{
    DA::DAChartWidget chart;
    QwtPlotCurve* curveA   = chart.addCurve(QVector< double >{ 0.0, 1.0, 2.0 }, QVector< double >{ 0.0, 1.0, 4.0 },
                                          QStringLiteral("curve-a"));
    QwtPlotMarker* markerB = chart.addVerticalLine(2.0, QStringLiteral("marker-b"));
    QVERIFY(curveA);
    QVERIFY(markerB);

    // 空指针安全：标记/查询空图元都不得崩溃
    chart.setItemTransient(nullptr);
    QVERIFY(!chart.isItemTransient(nullptr));
    QVERIFY(chart.transientItems().isEmpty());

    QVERIFY(!chart.isItemTransient(curveA));
    chart.setItemTransient(curveA);
    QVERIFY(chart.isItemTransient(curveA));
    QVERIFY(!chart.isItemTransient(markerB));
    QCOMPARE(chart.transientItems().size(), 1);
    QCOMPARE(chart.transientItems().first(), curveA);

    // 取消标记
    chart.setItemTransient(curveA, false);
    QVERIFY(!chart.isItemTransient(curveA));
    QVERIFY(chart.transientItems().isEmpty());

    // removePlotItem 负责清除 transient 登记（删除后不能残留悬空指针）
    chart.setItemTransient(curveA);
    chart.removePlotItem(curveA);
    QVERIFY(chart.transientItems().isEmpty());
    QVERIFY(chart.itemList().contains(markerB));

    // 外部直接 detach（不经过 removePlotItem/removeCurve）也必须注销 transient 登记
    QwtPlotCurve* detachedCurve = chart.addCurve(QVector< double >{ 0.0, 1.0 }, QVector< double >{ 5.0, 6.0 },
                                                 QStringLiteral("detached-curve"));
    QVERIFY(detachedCurve);
    chart.setItemTransient(detachedCurve);
    QVERIFY(chart.isItemTransient(detachedCurve));
    detachedCurve->detach();
    QVERIFY2(!chart.isItemTransient(detachedCurve), "detach must clear the transient registration");
    QVERIFY(!chart.transientItems().contains(detachedCurve));
    delete detachedCurve;  // detach 后图表不再持有，测试自行释放

    // clearAllData：删除所有数据图元后不能残留 transient 登记
    QwtPlotCurve* clearedCurve = chart.addCurve(QVector< double >{ 0.0, 1.0 }, QVector< double >{ 7.0, 8.0 },
                                                QStringLiteral("cleared-curve"));
    QVERIFY(clearedCurve);
    chart.setItemTransient(clearedCurve);
    QVERIFY(chart.isItemTransient(clearedCurve));
    chart.clearAllData();
    QVERIFY(chart.transientItems().isEmpty());
}

void DAChartSerializeTest::testTransientItemSkippedInChartData()
{
    DA::DAChartWidget chart;
    QwtPlotCurve* curveA   = chart.addCurve(QVector< double >{ 0.0, 1.0 }, QVector< double >{ 2.0, 3.0 },
                                          QStringLiteral("transient-curve"));
    QwtPlotMarker* markerB = chart.addVerticalLine(1.0, QStringLiteral("persistent-marker"));
    QVERIFY(curveA);
    QVERIFY(markerB);
    chart.setItemTransient(curveA);

    QDomDocument doc;
    DA::DAChartItemsManager mgr;
    const QDomElement chartEle = DA::DAXmlHelper::makeElement(&chart, QStringLiteral("chart"), &doc, &mgr);
    QVERIFY(!chartEle.isNull());

    // <items> 只能出现持久化图元（marker），transient 曲线不记录 key
    const QDomElement itemsEle = chartEle.firstChildElement(QStringLiteral("items"));
    QVERIFY(!itemsEle.isNull());
    QCOMPARE(itemsEle.childNodes().size(), 1);
    const QDomElement itemEle = itemsEle.firstChildElement(QStringLiteral("item"));
    QVERIFY(!itemEle.isNull());
    QCOMPARE(itemEle.attribute(QStringLiteral("key")), mgr.itemToKey(markerB));

    QCOMPARE(mgr.items().size(), 1);
    QVERIFY(mgr.items().contains(markerB));
    QVERIFY(mgr.itemToKey(curveA).isEmpty());
    // 注意：本用例刻意保持 chart 析构时仍挂着 transient 曲线 + marker。
    // 它同时作为 itemAttached 析构期悬空回调崩溃的回归用例：
    // QwtPlot::~QwtPlot 会在派生 PrivateData 销毁后 emit itemAttached，若未先 disconnect 会堆破坏。
}

void DAChartSerializeTest::testTransientChartDestruction()
{
    // 回归：DAChartWidget 析构时若仍挂着图元，QwtPlot::~QwtPlot 会 emit itemAttached。
    // 生产侧必须先在 ~DAChartWidget 中断开该信号，否则槽会访问已销毁的 PrivateData（堆破坏）。
    // 本用例不做显式清理：函数结束让 chart 带着 transient 曲线 + marker 自然析构，不崩即通过。
    DA::DAChartWidget chart;
    QwtPlotCurve* curve   = chart.addCurve(QVector< double >{ 0.0, 1.0 }, QVector< double >{ 1.0, 2.0 },
                                         QStringLiteral("dtor-curve"));
    QwtPlotMarker* marker = chart.addVerticalLine(1.0, QStringLiteral("dtor-marker"));
    QVERIFY(curve);
    QVERIFY(marker);
    chart.setItemTransient(curve);
    QVERIFY(chart.isItemTransient(curve));
}

void DAChartSerializeTest::testFigureGridXmlRoundTrip()
{
    std::unique_ptr< DA::DAFigureWidget > src(new DA::DAFigureWidget());
    src->resize(800, 600);
    src->show();
    QTest::qWaitForWindowExposed(src.get());

    QVERIFY(src->enableGridLayout(2, 2));
    QVERIFY(src->setGridSizes(QList< qreal >{ 0.6, 0.4 }, QList< qreal >{ 0.25, 0.75 }));

    DA::DAChartWidget* c00 = src->getChartFactory()->createChart();
    QVERIFY(c00);
    c00->setPlotId(QStringLiteral("grid-c00"));
    QVERIFY(src->setGridCell(c00, 0, 0, 1, 2));

    DA::DAChartWidget* c10 = src->getChartFactory()->createChart();
    QVERIFY(c10);
    c10->setPlotId(QStringLiteral("grid-c10"));
    QVERIFY(src->setGridCell(c10, 1, 0));

    DA::DAChartWidget* c11 = src->getChartFactory()->createChart();
    QVERIFY(c11);
    c11->setPlotId(QStringLiteral("grid-c11"));
    QVERIFY(src->setGridCell(c11, 1, 1));

    QCOMPARE(src->getChartCount(), 3);

    // 网格 figure 同时带坐标轴联动/对齐：XML 往返不崩且成员数量不变
    QVERIFY(src->bindAxisRange(c00, QwtAxis::YLeft, c10, QwtAxis::YLeft));
    QCOMPARE(src->getBindAxisRangeInfos().size(), 1);
    const DA::DAChartAxisRangeBinder* srcBinder = src->getBindAxisRangeInfos().first();
    QVERIFY(srcBinder);
    QCOMPARE(srcBinder->getSourcePlot()->plotId(), QStringLiteral("grid-c00"));
    QCOMPARE(srcBinder->getFollowerPlot()->plotId(), QStringLiteral("grid-c10"));
    QCOMPARE(srcBinder->getSourceAxis(), static_cast< int >(QwtAxis::YLeft));
    QCOMPARE(srcBinder->getFollowerAxis(), static_cast< int >(QwtAxis::YLeft));
    src->figure()->addAxisAlignment(QList< QwtPlot* >{ c00, c10 }, QwtAxis::YLeft);
    QCOMPARE(src->figure()->axisAligmentCount(), 1);

    QDomDocument doc;
    DA::DAChartItemsManager mgr;
    const QDomElement figEle = DA::DAXmlHelper::makeElement(src.get(), QStringLiteral("figure"), &doc, &mgr);
    QVERIFY(!figEle.isNull());

    // <grid> spec：2x2 + 保存的比例
    const QDomElement gridEle = figEle.firstChildElement(QStringLiteral("grid"));
    QVERIFY(!gridEle.isNull());
    QCOMPARE(gridEle.attribute(QStringLiteral("rows")).toInt(), 2);
    QCOMPARE(gridEle.attribute(QStringLiteral("cols")).toInt(), 2);
    QVERIFY(realListNearlyEqual(parseRealList(gridEle.attribute(QStringLiteral("rowSizes"))),
                                QList< qreal >{ 0.6, 0.4 }));
    QVERIFY(realListNearlyEqual(parseRealList(gridEle.attribute(QStringLiteral("colSizes"))),
                                QList< qreal >{ 0.25, 0.75 }));

    // 每个成员同时写 grid-* 与旧的 x/y/w/h
    const auto verifyChartElement = [&](const QString& chartId, int row, int col, int rowSpan, int colSpan) {
        const QDomElement chartEle = findChartElement(figEle, chartId);
        QVERIFY2(!chartEle.isNull(), qPrintable(QStringLiteral("missing chart element: %1").arg(chartId)));
        QVERIFY(hasGridCellAttributes(chartEle));
        QCOMPARE(chartEle.attribute(QStringLiteral("grid-row")).toInt(), row);
        QCOMPARE(chartEle.attribute(QStringLiteral("grid-col")).toInt(), col);
        QCOMPARE(chartEle.attribute(QStringLiteral("grid-rowspan")).toInt(), rowSpan);
        QCOMPARE(chartEle.attribute(QStringLiteral("grid-colspan")).toInt(), colSpan);
        // 旧版本/降级路径仍需读到 x/y/w/h（网格换算值）
        QVERIFY(chartEle.hasAttribute(QStringLiteral("x")));
        QVERIFY(chartEle.hasAttribute(QStringLiteral("y")));
        QVERIFY(chartEle.hasAttribute(QStringLiteral("w")));
        QVERIFY(chartEle.hasAttribute(QStringLiteral("h")));
    };
    verifyChartElement(QStringLiteral("grid-c00"), 0, 0, 1, 2);
    verifyChartElement(QStringLiteral("grid-c10"), 1, 0, 1, 1);
    verifyChartElement(QStringLiteral("grid-c11"), 1, 1, 1, 1);

    // 载入到全新 figure
    std::unique_ptr< DA::DAFigureWidget > dst(new DA::DAFigureWidget());
    dst->resize(800, 600);
    dst->show();
    QTest::qWaitForWindowExposed(dst.get());
    QVERIFY(DA::DAXmlHelper::loadElement(dst.get(), &figEle, &mgr));

    QVERIFY(dst->isGridLayoutEnabled());
    QVERIFY(dst->grid());
    QCOMPARE(dst->grid()->rowCount(), 2);
    QCOMPARE(dst->grid()->columnCount(), 2);
    QVERIFY(realListNearlyEqual(dst->gridRowSizes(), src->gridRowSizes()));
    QVERIFY(realListNearlyEqual(dst->gridColumnSizes(), src->gridColumnSizes()));
    QCOMPARE(dst->getChartCount(), 3);
    // <axisRangeBinders>/<axisAliment> 与网格布局共存时往返不崩、chart 数不变
    QCOMPARE(dst->getBindAxisRangeInfos().size(), 1);
    const DA::DAChartAxisRangeBinder* dstBinder = dst->getBindAxisRangeInfos().first();
    QVERIFY(dstBinder);
    // 端点不能接反：source/follower 的 plotId 与保存前一致
    QCOMPARE(dstBinder->getSourcePlot()->plotId(), QStringLiteral("grid-c00"));
    QCOMPARE(dstBinder->getFollowerPlot()->plotId(), QStringLiteral("grid-c10"));
    QCOMPARE(dstBinder->getSourceAxis(), static_cast< int >(QwtAxis::YLeft));
    QCOMPARE(dstBinder->getFollowerAxis(), static_cast< int >(QwtAxis::YLeft));
    QCOMPARE(dst->figure()->axisAligmentCount(), 1);

    const auto verifyLoadedGridPosition = [&](const QString& chartId, int row, int col, int rowSpan, int colSpan) {
        DA::DAChartWidget* chart = qobject_cast< DA::DAChartWidget* >(dst->findPlotById(chartId));
        QVERIFY2(chart, qPrintable(QStringLiteral("missing loaded chart: %1").arg(chartId)));
        int r = -1;
        int c = -1;
        int rs = -1;
        int cs = -1;
        QVERIFY(dst->grid()->gridPosition(chart, &r, &c, &rs, &cs));
        QCOMPARE(r, row);
        QCOMPARE(c, col);
        QCOMPARE(rs, rowSpan);
        QCOMPARE(cs, colSpan);
    };
    verifyLoadedGridPosition(QStringLiteral("grid-c00"), 0, 0, 1, 2);
    verifyLoadedGridPosition(QStringLiteral("grid-c10"), 1, 0, 1, 1);
    verifyLoadedGridPosition(QStringLiteral("grid-c11"), 1, 1, 1, 1);
}

void DAChartSerializeTest::testLegacyFigureWithoutGrid()
{
    // ① 自由布局写端不得写入 <grid> 或 grid-* 属性（旧工程/旧版本兼容路径）
    std::unique_ptr< DA::DAFigureWidget > freeFig(new DA::DAFigureWidget());
    freeFig->resize(640, 480);
    freeFig->show();
    QTest::qWaitForWindowExposed(freeFig.get());
    DA::DAChartWidget* freeChart = freeFig->createChart(0.1f, 0.2f, 0.3f, 0.4f);
    QVERIFY(freeChart);
    freeChart->setPlotId(QStringLiteral("free-chart"));

    QDomDocument freeDoc;
    DA::DAChartItemsManager freeMgr;
    const QDomElement freeEle = DA::DAXmlHelper::makeElement(freeFig.get(), QStringLiteral("figure"), &freeDoc, &freeMgr);
    QVERIFY(!freeEle.isNull());
    QVERIFY(freeEle.firstChildElement(QStringLiteral("grid")).isNull());
    const QDomElement freeChartEle = findChartElement(freeEle, QStringLiteral("free-chart"));
    QVERIFY(!freeChartEle.isNull());
    QVERIFY(!hasGridCellAttributes(freeChartEle));
    QVERIFY(freeChartEle.hasAttribute(QStringLiteral("x")));
    QVERIFY(freeChartEle.hasAttribute(QStringLiteral("y")));
    QVERIFY(freeChartEle.hasAttribute(QStringLiteral("w")));
    QVERIFY(freeChartEle.hasAttribute(QStringLiteral("h")));

    // ② 手写旧工程 XML：只有 <charts>/<chart x y w h>，没有 <grid>/<chart3d>
    QDomDocument legacyDoc;
    QDomElement legacyFigEle = legacyDoc.createElement(QStringLiteral("figure"));
    legacyFigEle.setAttribute(QStringLiteral("id"), QStringLiteral("legacy-figure"));
    legacyDoc.appendChild(legacyFigEle);
    QDomElement legacyChartsEle = legacyDoc.createElement(QStringLiteral("charts"));
    legacyFigEle.appendChild(legacyChartsEle);
    const auto addLegacyChart = [&](const QString& id, qreal x, qreal y, qreal w, qreal h) {
        QDomElement chartEle = legacyDoc.createElement(QStringLiteral("chart"));
        chartEle.setAttribute(QStringLiteral("id"), id);
        chartEle.setAttribute(QStringLiteral("x"), x);
        chartEle.setAttribute(QStringLiteral("y"), y);
        chartEle.setAttribute(QStringLiteral("w"), w);
        chartEle.setAttribute(QStringLiteral("h"), h);
        chartEle.appendChild(legacyDoc.createElement(QStringLiteral("items")));
        legacyChartsEle.appendChild(chartEle);
    };
    addLegacyChart(QStringLiteral("legacy-1"), 0.05, 0.1, 0.4, 0.3);
    addLegacyChart(QStringLiteral("legacy-2"), 0.5, 0.55, 0.45, 0.4);

    std::unique_ptr< DA::DAFigureWidget > legacy(new DA::DAFigureWidget());
    legacy->resize(640, 480);
    legacy->show();
    QTest::qWaitForWindowExposed(legacy.get());
    DA::DAChartItemsManager legacyMgr;
    QVERIFY(DA::DAXmlHelper::loadElement(legacy.get(), &legacyFigEle, &legacyMgr));

    QVERIFY(!legacy->isGridLayoutEnabled());
    QCOMPARE(legacy->getChartCount(), 2);

    const auto verifyLegacyGeometry = [&](const QString& id, qreal x, qreal y, qreal w, qreal h) {
        QwtPlot* plot = legacy->findPlotById(id);
        QVERIFY2(plot, qPrintable(QStringLiteral("missing legacy chart: %1").arg(id)));
        const QRectF rect = legacy->axesNormRect(plot);
        QVERIFY2(nearlyEqual(rect.x(), x), qPrintable(QStringLiteral("%1 x=%2").arg(id).arg(rect.x())));
        QVERIFY2(nearlyEqual(rect.y(), y), qPrintable(QStringLiteral("%1 y=%2").arg(id).arg(rect.y())));
        QVERIFY2(nearlyEqual(rect.width(), w), qPrintable(QStringLiteral("%1 w=%2").arg(id).arg(rect.width())));
        QVERIFY2(nearlyEqual(rect.height(), h), qPrintable(QStringLiteral("%1 h=%2").arg(id).arg(rect.height())));
    };
    verifyLegacyGeometry(QStringLiteral("legacy-1"), 0.05, 0.1, 0.4, 0.3);
    verifyLegacyGeometry(QStringLiteral("legacy-2"), 0.5, 0.55, 0.45, 0.4);
}

void DAChartSerializeTest::testInvalidGridMemberFallsBackToFreeLayout()
{
    // ① 网格坐标非法（9,0 超出 2x2）：回退自由布局，位置用 x/y/w/h
    QDomDocument doc;
    QDomElement figEle = doc.createElement(QStringLiteral("figure"));
    figEle.setAttribute(QStringLiteral("id"), QStringLiteral("bad-grid-figure"));
    doc.appendChild(figEle);
    QDomElement gridEle = doc.createElement(QStringLiteral("grid"));
    gridEle.setAttribute(QStringLiteral("rows"), 2);
    gridEle.setAttribute(QStringLiteral("cols"), 2);
    gridEle.setAttribute(QStringLiteral("rowSizes"), QStringLiteral("0.5,0.5"));
    gridEle.setAttribute(QStringLiteral("colSizes"), QStringLiteral("0.5,0.5"));
    figEle.appendChild(gridEle);
    QDomElement chartsEle = doc.createElement(QStringLiteral("charts"));
    figEle.appendChild(chartsEle);
    QDomElement chartEle = doc.createElement(QStringLiteral("chart"));
    chartEle.setAttribute(QStringLiteral("id"), QStringLiteral("bad-grid-chart"));
    chartEle.setAttribute(QStringLiteral("grid-row"), 9);
    chartEle.setAttribute(QStringLiteral("grid-col"), 0);
    chartEle.setAttribute(QStringLiteral("x"), 0.1);
    chartEle.setAttribute(QStringLiteral("y"), 0.2);
    chartEle.setAttribute(QStringLiteral("w"), 0.5);
    chartEle.setAttribute(QStringLiteral("h"), 0.6);
    chartEle.appendChild(doc.createElement(QStringLiteral("items")));
    chartsEle.appendChild(chartEle);

    std::unique_ptr< DA::DAFigureWidget > fig(new DA::DAFigureWidget());
    fig->resize(640, 480);
    fig->show();
    QTest::qWaitForWindowExposed(fig.get());
    DA::DAChartItemsManager mgr;
    QVERIFY(DA::DAXmlHelper::loadElement(fig.get(), &figEle, &mgr));
    QVERIFY(fig->isGridLayoutEnabled());
    QCOMPARE(fig->getChartCount(), 1);

    auto* badChart = qobject_cast< DA::DAChartWidget* >(fig->findPlotById(QStringLiteral("bad-grid-chart")));
    QVERIFY(badChart);
    int r = 0;
    int c = 0;
    int rs = 0;
    int cs = 0;
    QVERIFY2(!fig->grid()->gridPosition(badChart, &r, &c, &rs, &cs),
             "invalid grid-* attributes must not register the chart as a grid member");
    const QRectF badRect = fig->axesNormRect(badChart);
    QVERIFY(nearlyEqual(badRect.x(), 0.1));
    QVERIFY(nearlyEqual(badRect.y(), 0.2));
    QVERIFY(nearlyEqual(badRect.width(), 0.5));
    QVERIFY(nearlyEqual(badRect.height(), 0.6));

    // ② 缺 x/y/w/h 且无合法 grid-*：回退默认矩形 (0.05,0.05,0.9,0.9)，不能生成 0 尺寸子图
    QDomDocument defaultDoc;
    QDomElement defaultFigEle = defaultDoc.createElement(QStringLiteral("figure"));
    defaultFigEle.setAttribute(QStringLiteral("id"), QStringLiteral("default-rect-figure"));
    defaultDoc.appendChild(defaultFigEle);
    QDomElement defaultGridEle = defaultDoc.createElement(QStringLiteral("grid"));
    defaultGridEle.setAttribute(QStringLiteral("rows"), 2);
    defaultGridEle.setAttribute(QStringLiteral("cols"), 2);
    defaultFigEle.appendChild(defaultGridEle);
    QDomElement defaultChartsEle = defaultDoc.createElement(QStringLiteral("charts"));
    defaultFigEle.appendChild(defaultChartsEle);
    QDomElement defaultChartEle = defaultDoc.createElement(QStringLiteral("chart"));
    defaultChartEle.setAttribute(QStringLiteral("id"), QStringLiteral("no-geometry-chart"));
    defaultChartEle.appendChild(defaultDoc.createElement(QStringLiteral("items")));
    defaultChartsEle.appendChild(defaultChartEle);

    std::unique_ptr< DA::DAFigureWidget > defaultFig(new DA::DAFigureWidget());
    defaultFig->resize(640, 480);
    defaultFig->show();
    QTest::qWaitForWindowExposed(defaultFig.get());
    DA::DAChartItemsManager defaultMgr;
    QVERIFY(DA::DAXmlHelper::loadElement(defaultFig.get(), &defaultFigEle, &defaultMgr));
    QCOMPARE(defaultFig->getChartCount(), 1);
    auto* defaultChart = qobject_cast< DA::DAChartWidget* >(defaultFig->findPlotById(QStringLiteral("no-geometry-chart")));
    QVERIFY(defaultChart);
    QVERIFY(!defaultFig->grid()->gridPosition(defaultChart, nullptr, nullptr, nullptr, nullptr));
    const QRectF defaultRect = defaultFig->axesNormRect(defaultChart);
    QVERIFY(nearlyEqual(defaultRect.x(), 0.05));
    QVERIFY(nearlyEqual(defaultRect.y(), 0.05));
    QVERIFY(nearlyEqual(defaultRect.width(), 0.9));
    QVERIFY(nearlyEqual(defaultRect.height(), 0.9));
}

void DAChartSerializeTest::testChartItemArchiveMemoryRoundTrip()
{
    // 源绘图：两条不同长度/数据/标题的曲线
    DA::DAChartWidget chart;
    QwtPlotCurve* curveA = chart.addCurve(QVector< double >{ 0.0, 1.0, 2.0, 3.0 },
                                          QVector< double >{ 10.0, 11.0, 12.0, 13.0 }, QStringLiteral("A"));
    QwtPlotCurve* curveB
        = chart.addCurve(QVector< double >{ 5.0, 6.0, 7.0 }, QVector< double >{ 1.0, 2.0, 3.0 }, QStringLiteral("B"));
    QVERIFY(curveA);
    QVERIFY(curveB);

    DA::DAChartItemsManager srcMgr;
    QDomDocument doc;
    DA::DAXmlHelper::makeElement(&chart, QStringLiteral("chart"), &doc, &srcMgr);
    QCOMPARE(srcMgr.items().size(), 2);

    MemoryArchive mem;
    QString error;
    QVERIFY2(DA::DAChartItemArchive::save(&mem, &srcMgr, QStringLiteral("chart-data"), &error), qPrintable(error));

    const QString indexRelPath = QStringLiteral("chart-data/") + DA::DAChartItemArchive::itemsIndexFileName();
    QVERIFY2(mem.contains(indexRelPath), qPrintable(QStringLiteral("missing index: %1").arg(indexRelPath)));
    const QList< QString > srcKeys = srcMgr.keys();
    QCOMPARE(srcKeys.size(), 2);
    for (const QString& key : srcKeys) {
        QVERIFY2(mem.contains(QStringLiteral("chart-data/%1").arg(key)),
                 qPrintable(QStringLiteral("missing item file for key: %1").arg(key)));
    }
    // 索引内容：<items> 下 2 个 <item key rtti/>
    QDomDocument indexDoc;
    QVERIFY(indexDoc.setContent(mem.read(indexRelPath)));
    QCOMPARE(indexDoc.documentElement().tagName(), QStringLiteral("items"));
    QCOMPARE(indexDoc.documentElement().childNodes().size(), 2);

    // ① 单图元 write 返回 false：整体 save 仍 true + errorMessage，索引/其它图元仍写入
    const QString firstItemPath = QStringLiteral("chart-data/%1").arg(srcKeys.first());
    MemoryArchive writeFailMem;
    writeFailMem.failWritePaths.insert(firstItemPath);
    QString writeFailError;
    QVERIFY2(DA::DAChartItemArchive::save(&writeFailMem, &srcMgr, QStringLiteral("chart-data"), &writeFailError),
             "a single item write failure must not abort the whole save");
    QVERIFY(!writeFailError.isEmpty());
    QVERIFY(writeFailMem.contains(indexRelPath));
    QVERIFY(!writeFailMem.contains(firstItemPath));
    for (const QString& key : srcKeys) {
        if (key == srcKeys.first()) {
            continue;
        }
        QVERIFY2(writeFailMem.contains(QStringLiteral("chart-data/%1").arg(key)),
                 qPrintable(QStringLiteral("other item file must still be written: %1").arg(key)));
    }
    // task 的 lastError 也应带上该失败信息
    MemoryArchive writeFailTaskMem;
    writeFailTaskMem.failWritePaths.insert(firstItemPath);
    DA::DAZipArchiveTask_ChartItem writeFailTask(QStringLiteral("chart-data"), srcMgr);
    QVERIFY(writeFailTask.exec(&writeFailTaskMem, DA::DAAbstractArchiveTask::WriteMode));
    QVERIFY(!writeFailTask.getLastError().isEmpty());

    // ② 单图元 write 抛 std::runtime_error：save 捕获异常后继续，整体 true 且不崩
    MemoryArchive throwWriteMem;
    throwWriteMem.throwWritePaths.insert(firstItemPath);
    QString throwWriteError;
    QVERIFY2(DA::DAChartItemArchive::save(&throwWriteMem, &srcMgr, QStringLiteral("chart-data"), &throwWriteError),
             "a throwing item write must not abort the whole save");
    QVERIFY(!throwWriteError.isEmpty());
    QVERIFY(throwWriteMem.contains(indexRelPath));
    QVERIFY(!throwWriteMem.contains(firstItemPath));

    // 读回：key 集合、图元数量、类型与数据大小一致
    DA::DAChartItemsManager dstMgr;
    QVERIFY2(DA::DAChartItemArchive::load(&mem, &dstMgr, QStringLiteral("chart-data"), &error), qPrintable(error));
    QCOMPARE(dstMgr.items().size(), 2);
    QCOMPARE(keySet(dstMgr.keys()), keySet(srcKeys));
    for (const QString& key : srcKeys) {
        auto* restored = dynamic_cast< QwtPlotCurve* >(dstMgr.keyToItem(key));
        QVERIFY2(restored, qPrintable(QStringLiteral("missing restored curve: %1").arg(key)));
        QCOMPARE(restored->rtti(), static_cast< int >(QwtPlotItem::Rtti_PlotCurve));
        const QString title = restored->title().text();
        QVERIFY(title == QStringLiteral("A") || title == QStringLiteral("B"));
        QCOMPARE(static_cast< int >(restored->dataSize()), title == QStringLiteral("A") ? 4 : 3);
    }

    // 无索引：旧工程/空绘图视为成功且不加载任何图元
    MemoryArchive emptyMem;
    DA::DAChartItemsManager emptyMgr;
    QVERIFY(DA::DAChartItemArchive::load(&emptyMem, &emptyMgr, QStringLiteral("chart-data")));
    QVERIFY(emptyMgr.items().isEmpty());

    // 索引存在但为 0 字节：不能当成「旧工程无索引」，必须返回 false
    MemoryArchive zeroIndexMem;
    zeroIndexMem.m_files.insert(indexRelPath, QByteArray());
    DA::DAChartItemsManager zeroIndexMgr;
    QString zeroIndexError;
    QVERIFY2(!DA::DAChartItemArchive::load(&zeroIndexMem, &zeroIndexMgr, QStringLiteral("chart-data"), &zeroIndexError),
             "an existing but empty chart-items.xml must be reported as an error");
    QVERIFY(!zeroIndexError.isEmpty());
    QVERIFY(zeroIndexMgr.items().isEmpty());

    // 索引存在但 XML 损坏：返回 false 且 errorMessage 非空
    MemoryArchive badIndexMem;
    badIndexMem.m_files.insert(indexRelPath, QByteArrayLiteral("not xml"));
    DA::DAChartItemsManager badIndexMgr;
    QString badIndexError;
    QVERIFY2(!DA::DAChartItemArchive::load(&badIndexMem, &badIndexMgr, QStringLiteral("chart-data"), &badIndexError),
             "a corrupt chart-items.xml must be reported as an error");
    QVERIFY(!badIndexError.isEmpty());
    QVERIFY(badIndexMgr.items().isEmpty());

    // 单个图元数据损坏：跳过坏图元，整体仍 true，并把原因带进 errorMessage
    MemoryArchive damagedItemMem;
    damagedItemMem.m_files = mem.m_files;
    damagedItemMem.m_files.insert(QStringLiteral("chart-data/%1").arg(srcKeys.first()), QByteArrayLiteral("broken-item"));
    DA::DAChartItemsManager damagedItemMgr;
    QString damagedItemError;
    QVERIFY2(DA::DAChartItemArchive::load(&damagedItemMem, &damagedItemMgr, QStringLiteral("chart-data"),
                                          &damagedItemError),
             "a single corrupt item must not abort the whole load");
    QCOMPARE(damagedItemMgr.items().size(), 1);
    QVERIFY(!damagedItemError.isEmpty());
    qDeleteAll(damagedItemMgr.items());

    qDeleteAll(dstMgr.items());
}

void DAChartSerializeTest::testZipArchiveTaskChartItemRoundTrip()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString zipPath = tempDir.filePath(QStringLiteral("chart-data.zip"));

    DA::DAChartWidget chart;
    QVERIFY(chart.addCurve(QVector< double >{ 0.0, 1.0, 2.0 }, QVector< double >{ 4.0, 5.0, 6.0 },
                           QStringLiteral("zip-A")));
    QVERIFY(chart.addCurve(QVector< double >{ 3.0, 4.0 }, QVector< double >{ 7.0, 8.0 }, QStringLiteral("zip-B")));

    DA::DAChartItemsManager srcMgr;
    QDomDocument doc;
    DA::DAXmlHelper::makeElement(&chart, QStringLiteral("chart"), &doc, &srcMgr);
    QCOMPARE(srcMgr.items().size(), 2);

    // 写：真实 zip（等价于 DAAppProject 的 chart-data 存档路径）
    {
        DA::DAZipArchive zip;
        QSignalSpy finished(&zip, &DA::DAAbstractArchive::taskFinished);
        auto saveTask = std::make_shared< DA::DAZipArchiveTask_ChartItem >(QStringLiteral("chart-data"), srcMgr);
        zip.appendTask(saveTask);
        zip.saveAll(zipPath);

        QCOMPARE(finished.count(), 1);
        QCOMPARE(finished.takeFirst().at(0).toInt(), static_cast< int >(DA::DAAbstractArchive::SaveSuccess));
        QVERIFY2(saveTask->getLastError().isEmpty(), qPrintable(saveTask->getLastError()));
    }
    QVERIFY(QFile::exists(zipPath));

    // 读：新任务/新 manager
    DA::DAZipArchive zipIn;
    auto loadTask = std::make_shared< DA::DAZipArchiveTask_ChartItem >(QStringLiteral("chart-data"));
    QSignalSpy finishedIn(&zipIn, &DA::DAAbstractArchive::taskFinished);
    zipIn.appendTask(loadTask);
    zipIn.loadAll(zipPath);

    QCOMPARE(finishedIn.count(), 1);
    QCOMPARE(finishedIn.takeFirst().at(0).toInt(), static_cast< int >(DA::DAAbstractArchive::LoadSuccess));
    QVERIFY2(loadTask->getLastError().isEmpty(), qPrintable(loadTask->getLastError()));

    const DA::DAChartItemsManager& dstMgr = loadTask->getChartItemsManager();
    QCOMPARE(dstMgr.items().size(), 2);
    QCOMPARE(keySet(dstMgr.keys()), keySet(srcMgr.keys()));
    for (const QString& key : srcMgr.keys()) {
        auto* srcCurve      = dynamic_cast< QwtPlotCurve* >(srcMgr.keyToItem(key));
        auto* restoredCurve = dynamic_cast< QwtPlotCurve* >(dstMgr.keyToItem(key));
        QVERIFY(srcCurve);
        QVERIFY2(restoredCurve, qPrintable(QStringLiteral("missing zip-restored curve: %1").arg(key)));
        QCOMPARE(restoredCurve->title().text(), srcCurve->title().text());
        QCOMPARE(static_cast< int >(restoredCurve->dataSize()), static_cast< int >(srcCurve->dataSize()));
    }

    // K：真实 zip 中索引存在但为 0 字节 → 载入失败且 lastError 非空
    const QString emptyIndexZipPath = tempDir.filePath(QStringLiteral("chart-data-empty-index.zip"));
    {
        DA::DAZipArchive emptyIndexZip;
        QVERIFY(emptyIndexZip.setBaseFilePath(emptyIndexZipPath));
        QVERIFY(emptyIndexZip.create());
        QVERIFY(emptyIndexZip.write(QStringLiteral("chart-data/") + DA::DAChartItemArchive::itemsIndexFileName(),
                                    QByteArray()));
        emptyIndexZip.close();
    }
    QVERIFY(QFile::exists(emptyIndexZipPath));

    DA::DAZipArchive emptyIndexZipIn;
    auto emptyIndexLoadTask = std::make_shared< DA::DAZipArchiveTask_ChartItem >(QStringLiteral("chart-data"));
    QSignalSpy emptyIndexFinished(&emptyIndexZipIn, &DA::DAAbstractArchive::taskFinished);
    emptyIndexZipIn.appendTask(emptyIndexLoadTask);
    emptyIndexZipIn.loadAll(emptyIndexZipPath);
    QCOMPARE(emptyIndexFinished.count(), 1);
    QCOMPARE(emptyIndexFinished.takeFirst().at(0).toInt(), static_cast< int >(DA::DAAbstractArchive::LoadFailed));
    QVERIFY2(!emptyIndexLoadTask->getLastError().isEmpty(),
             "a 0-byte chart-items.xml in a real zip must make the chart-item task fail");

    qDeleteAll(dstMgr.items());
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DAChartSerializeTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "main.moc"
