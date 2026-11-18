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
#include <QUndoStack>
#include <QVariantMap>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

#include "Archive/DAChartItemArchive.h"
#include "Archive/DAXmlHelper.h"
#include "Archive/DAZipArchive.h"
#include "Archive/DAZipArchiveTask_ChartItem.h"
#include "Archive/DAZipArchiveTask_Xml.h"
#include "Chart/Setting/DAFigureWidgetSettingPanel.h"
#include "Common/Settings/DAPropertyPanelContainerWidget.h"
#include "Chart/DAChartItemsManager.h"
#include "DAChart3DSerialize.h"
#include "DAChart3DWidget.h"
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

/// 比例列表的失败信息文本（形如 "0.5,0.5"）
QString sizesText(const QList< qreal >& sizes)
{
    QStringList parts;
    for (qreal v : sizes) {
        parts.append(QString::number(v, 'g', 6));
    }
    return parts.join(QLatin1Char(','));
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

    // 模拟存档打开状态：opened=false 表示未 open/create 的存档（基类默认恒为 true）
    bool isOpen() const override
    {
        return opened;
    }

    QHash< QString, QByteArray > m_files;
    QSet< QString > failWritePaths;
    QSet< QString > throwWritePaths;
    bool opened { true };
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
    // transient 登记要求图元已挂在本图上：未 attach 拒绝、attach 后成功、删除后不留悬空登记
    void testTransientItemRequiresAttachment();
    // 3D 枚举属性（aspectmode/coordstyle/tickposition）越界时保留原值，不再直接 static_cast
    void testInvalid3DEnumAttributesKeepDefaults();
    // <grid> 比例个数不匹配/含非法值 → 等分轨道、不崩、撤销栈计数不变
    void testGridBadRatiosFallBackToEqualTracks();
    // enableGridLayout 复位塌陷尺寸约束后，设置面板经 figureSizeConstraintsChanged 自动刷新
    void testFigureSizeConstraintsSignalRefreshesSettingPanel();
    // 3D 子图持久 id 与 visible 的 XML 往返
    void test3DChartIdAndVisibleRoundTrip();
    // 旧工程 <chart3d> 无 id/visible：按默认加载（可见 + 生成 c3d-<n> id）
    void testLegacy3DChartWithoutIdVisible();
    // DAChartItemArchive::load：存档未打开时返回 false（不再被当成旧工程静默成功）
    void testChartItemArchiveRejectsClosedArchive();
    // 网格 figure XML 往返：<grid> 行列/比例、成员 grid-* 坐标、x/y/w/h 兼容字段
    void testFigureGridXmlRoundTrip();
    // 网格间距（wspace/hspace）写读往返；旧工程无该属性时按 0 处理
    void testFigureGridSpacingXmlRoundTrip();
    // 旧工程兼容：无 <grid> 的 figure 走自由布局，且自由布局写出不带网格字段
    void testLegacyFigureWithoutGrid();
    // 非法 grid-* 坐标回退自由布局；缺 x/y/w/h 时回退默认矩形
    void testInvalidGridMemberFallsBackToFreeLayout();
    // DAChartItemArchive 内存 archive 往返（索引 + 每图元二进制）
    void testChartItemArchiveMemoryRoundTrip();
    // DAZipArchiveTask_ChartItem 真实 zip 往返（DAAppProject 图表存档路径）
    void testZipArchiveTaskChartItemRoundTrip();
    // 快照两段式与 save() 输出字节等价，load() 读回一致（BLK-01a）
    void testChartItemSnapshotSaveEquivalence();
    // BLK-01 回归：makeSnapshots 后图元全部 delete，saveSnapshots 仍能完整恢复
    void testChartItemSnapshotSurvivesItemDeletion();
    // 工程级存档顺序回归：按 DAAppProject 的任务顺序（保存 charts.xml→chart-data；加载 chart-data→charts.xml）
    void testProjectArchiveOrderRoundTrip();
    // SF-06 回归：设过当前 3D 子图的 figure 析构不崩（析构期 3D destroyed 回调不再访问失效 PIMPL/figure）
    void testFigureDestructionWithCurrent3DChart();
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

/**
 * @brief transient 登记要求图元已挂在本图上
 *
 * 登记集合只有 detach/析构回调负责注销；未 attach 的图元不会触发它，删除后会在集合里留下
 * 悬空地址，地址被复用后新图元被误判为 transient 而静默漏存。要求 setItemTransient 前置校验。
 */
void DAChartSerializeTest::testTransientItemRequiresAttachment()
{
    DA::DAChartWidget chart;

    // ① 未 attach：登记被拒绝，集合保持为空
    QwtPlotCurve* orphan = new QwtPlotCurve(QStringLiteral("orphan"));
    chart.setItemTransient(orphan);
    QVERIFY2(!chart.isItemTransient(orphan), "an unattached item must not be registered as transient");
    QVERIFY(chart.transientItems().isEmpty());

    // ② attach 后再登记成功
    orphan->attach(&chart);
    QVERIFY(chart.itemList().contains(orphan));
    chart.setItemTransient(orphan);
    QVERIFY(chart.isItemTransient(orphan));
    QCOMPARE(chart.transientItems().size(), 1);

    // ③ 删除已登记图元：detach 回调注销登记，集合里不留悬空地址
    delete orphan;
    QVERIFY2(!chart.isItemTransient(orphan), "deleting an attached item must unregister the transient registration");
    QVERIFY(chart.transientItems().isEmpty());

    // ④ 别的绘图上的图元不能登记到本绘图（同样的悬空风险）
    DA::DAChartWidget other;
    QwtPlotCurve* onOther = other.addCurve(QVector< double >{ 0.0, 1.0 }, QVector< double >{ 3.0, 4.0 },
                                          QStringLiteral("on-other"));
    QVERIFY(onOther);
    other.setItemTransient(onOther);
    QVERIFY(other.isItemTransient(onOther));
    chart.setItemTransient(onOther);
    QVERIFY2(!chart.isItemTransient(onOther), "an item attached to another chart must not be registered here");
}

/**
 * @brief 3D 枚举属性越界时保留原值
 *
 * aspectmode/coordstyle/tickposition 由 XML 直接读成 int 后 static_cast 进枚举；手改 XML 写入越界值
 * 会让 Qwt3D 按未知枚举分支。要求读取端按有效值区间校验，越界只 qWarning 并保留当前值。
 */
void DAChartSerializeTest::testInvalid3DEnumAttributesKeepDefaults()
{
    std::unique_ptr< DA::DAChart3DWidget > chart3d(new DA::DAChart3DWidget());
    Qwt3DPlot* plot = chart3d.get();
    QVERIFY(plot);
    Qwt3DCoordinateSystem* coordSys = plot->coordinates();
    QVERIFY(coordSys);

    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("chart3d"));
    doc.appendChild(root);
    QDomElement viewEle = doc.createElement(QStringLiteral("view"));
    root.appendChild(viewEle);

    // 基线：先写入合法值，后续越界读取不得改动它们
    viewEle.setAttribute(QStringLiteral("aspectmode"), static_cast< int >(DATARATIO));
    viewEle.setAttribute(QStringLiteral("coordstyle"), static_cast< int >(FRAME));
    viewEle.setAttribute(QStringLiteral("tickposition"), static_cast< int >(TICK_TOP));
    QVERIFY(DA::DAXmlHelper::loadElement(chart3d.get(), &root));
    QCOMPARE(static_cast< int >(plot->aspectRatioMode()), static_cast< int >(DATARATIO));
    QCOMPARE(static_cast< int >(coordSys->style()), static_cast< int >(FRAME));
    QCOMPARE(static_cast< int >(coordSys->tickPosition()), static_cast< int >(TICK_TOP));

    // 越界值：保留上一组合法值（修复前会被 static_cast 成未知枚举）
    viewEle.setAttribute(QStringLiteral("aspectmode"), 99);
    viewEle.setAttribute(QStringLiteral("coordstyle"), 42);
    viewEle.setAttribute(QStringLiteral("tickposition"), 7);
    QVERIFY(DA::DAXmlHelper::loadElement(chart3d.get(), &root));
    QCOMPARE(static_cast< int >(plot->aspectRatioMode()), static_cast< int >(DATARATIO));
    QCOMPARE(static_cast< int >(coordSys->style()), static_cast< int >(FRAME));
    QCOMPARE(static_cast< int >(coordSys->tickPosition()), static_cast< int >(TICK_TOP));

    // 负值同样被拒（区间下界也要校验）
    viewEle.setAttribute(QStringLiteral("aspectmode"), -1);
    viewEle.setAttribute(QStringLiteral("coordstyle"), -3);
    viewEle.setAttribute(QStringLiteral("tickposition"), -2);
    QVERIFY(DA::DAXmlHelper::loadElement(chart3d.get(), &root));
    QCOMPARE(static_cast< int >(plot->aspectRatioMode()), static_cast< int >(DATARATIO));
    QCOMPARE(static_cast< int >(coordSys->style()), static_cast< int >(FRAME));
    QCOMPARE(static_cast< int >(coordSys->tickPosition()), static_cast< int >(TICK_TOP));
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

/**
 * @brief 网格间距（wspace/hspace）的 XML 往返
 *
 * 写入端现在真的写 QwtFigureGrid 保留的间距（与 rowSizes/colSizes 同精度），
 * 读端恢复同值；旧工程没有这两个属性时按默认 0 处理，不破坏等分轨道。
 */
void DAChartSerializeTest::testFigureGridSpacingXmlRoundTrip()
{
    const qreal wspace = 0.08;
    const qreal hspace = 0.12;

    std::unique_ptr< DA::DAFigureWidget > src(new DA::DAFigureWidget());
    src->resize(800, 600);
    src->show();
    QTest::qWaitForWindowExposed(src.get());
    QVERIFY(src->enableGridLayout(2, 2));
    QVERIFY(src->grid());
    src->grid()->setSpacing(wspace, hspace);
    QVERIFY(nearlyEqual(src->grid()->wspace(), wspace));
    QVERIFY(nearlyEqual(src->grid()->hspace(), hspace));

    DA::DAChartWidget* chart = src->getChartFactory()->createChart();
    QVERIFY(chart);
    chart->setPlotId(QStringLiteral("spacing-chart"));
    QVERIFY(src->setGridCell(chart, 0, 1));

    QDomDocument doc;
    DA::DAChartItemsManager mgr;
    const QDomElement figEle = DA::DAXmlHelper::makeElement(src.get(), QStringLiteral("figure"), &doc, &mgr);
    QVERIFY(!figEle.isNull());
    const QDomElement gridEle = figEle.firstChildElement(QStringLiteral("grid"));
    QVERIFY(!gridEle.isNull());
    // 写入端必须真的写出间距属性
    QVERIFY(gridEle.hasAttribute(QStringLiteral("wspace")));
    QVERIFY(gridEle.hasAttribute(QStringLiteral("hspace")));
    bool okW = false;
    bool okH = false;
    const qreal savedW = gridEle.attribute(QStringLiteral("wspace")).toDouble(&okW);
    const qreal savedH = gridEle.attribute(QStringLiteral("hspace")).toDouble(&okH);
    QVERIFY(okW && okH);
    QVERIFY(nearlyEqual(savedW, wspace));
    QVERIFY(nearlyEqual(savedH, hspace));

    // 读端恢复同一间距，且单元格归一化几何与源 figure 一致
    std::unique_ptr< DA::DAFigureWidget > dst(new DA::DAFigureWidget());
    dst->resize(800, 600);
    dst->show();
    QTest::qWaitForWindowExposed(dst.get());
    QVERIFY(DA::DAXmlHelper::loadElement(dst.get(), &figEle, &mgr));
    QVERIFY(dst->isGridLayoutEnabled());
    QVERIFY(dst->grid());
    QVERIFY(nearlyEqual(dst->grid()->wspace(), wspace));
    QVERIFY(nearlyEqual(dst->grid()->hspace(), hspace));
    QVERIFY(dst->findPlotById(QStringLiteral("spacing-chart")));
    const QRectF srcRect = src->grid()->normRectFor(0, 1);
    const QRectF dstRect = dst->grid()->normRectFor(0, 1);
    QVERIFY(nearlyEqual(dstRect.x(), srcRect.x()));
    QVERIFY(nearlyEqual(dstRect.y(), srcRect.y()));
    QVERIFY(nearlyEqual(dstRect.width(), srcRect.width()));
    QVERIFY(nearlyEqual(dstRect.height(), srcRect.height()));

    // 旧工程（无 wspace/hspace）：按 0 处理，等分轨道不被破坏
    QDomDocument legacyDoc;
    QDomElement legacyFigEle = legacyDoc.createElement(QStringLiteral("figure"));
    legacyFigEle.setAttribute(QStringLiteral("id"), QStringLiteral("no-spacing-figure"));
    legacyDoc.appendChild(legacyFigEle);
    QDomElement legacyGridEle = legacyDoc.createElement(QStringLiteral("grid"));
    legacyGridEle.setAttribute(QStringLiteral("rows"), 2);
    legacyGridEle.setAttribute(QStringLiteral("cols"), 2);
    legacyGridEle.setAttribute(QStringLiteral("rowSizes"), QStringLiteral("0.5,0.5"));
    legacyGridEle.setAttribute(QStringLiteral("colSizes"), QStringLiteral("0.5,0.5"));
    legacyFigEle.appendChild(legacyGridEle);
    legacyFigEle.appendChild(legacyDoc.createElement(QStringLiteral("charts")));

    std::unique_ptr< DA::DAFigureWidget > legacy(new DA::DAFigureWidget());
    legacy->resize(640, 480);
    legacy->show();
    QTest::qWaitForWindowExposed(legacy.get());
    DA::DAChartItemsManager legacyMgr;
    QVERIFY(DA::DAXmlHelper::loadElement(legacy.get(), &legacyFigEle, &legacyMgr));
    QVERIFY(legacy->grid());
    QVERIFY(nearlyEqual(legacy->grid()->wspace(), 0.0));
    QVERIFY(nearlyEqual(legacy->grid()->hspace(), 0.0));
    QVERIFY(nearlyEqual(legacy->grid()->normRectFor(0, 0).width(), 0.5));
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

/**
 * @brief <grid> 比例非法时退回等分轨道
 *
 * loadGridElement 只在 rowSizes/colSizes 个数与行列数一致时应用比例；个数不符或数值非法时保留
 * 等分轨道。此用例锁定三条边界：个数不符、含不可解析值（解析时被丢掉）、个数正确但数值非正/越界。
 * 三种情况都必须：加载成功、网格启用且为等分轨道、不压撤销命令、不崩。
 */
void DAChartSerializeTest::testGridBadRatiosFallBackToEqualTracks()
{
    struct Case
    {
        const char* name;
        QString rowSizes;
        QString colSizes;
    };
    const QList< Case > cases = {
        { "rowSizes count mismatch", QStringLiteral("0.6"), QStringLiteral("0.3,0.7") },
        { "colSizes contains unparsable value", QStringLiteral("0.6,0.4"), QStringLiteral("0.6,abc") },
        { "counts ok but ratios non-positive and > 1", QStringLiteral("1.6,-0.6"), QStringLiteral("0.5,0.5") }
    };

    for (const Case& c : cases) {
        QDomDocument doc;
        QDomElement figEle = doc.createElement(QStringLiteral("figure"));
        figEle.setAttribute(QStringLiteral("id"), QStringLiteral("bad-ratios-figure"));
        doc.appendChild(figEle);
        QDomElement gridEle = doc.createElement(QStringLiteral("grid"));
        gridEle.setAttribute(QStringLiteral("rows"), 2);
        gridEle.setAttribute(QStringLiteral("cols"), 2);
        gridEle.setAttribute(QStringLiteral("rowSizes"), c.rowSizes);
        gridEle.setAttribute(QStringLiteral("colSizes"), c.colSizes);
        figEle.appendChild(gridEle);
        figEle.appendChild(doc.createElement(QStringLiteral("charts")));

        std::unique_ptr< DA::DAFigureWidget > fig(new DA::DAFigureWidget());
        fig->resize(640, 480);
        fig->show();
        QTest::qWaitForWindowExposed(fig.get());
        QUndoStack* stack = fig->getUndoStack();
        QVERIFY(stack);
        QCOMPARE(stack->count(), 0);

        DA::DAChartItemsManager mgr;
        QVERIFY2(DA::DAXmlHelper::loadElement(fig.get(), &figEle, &mgr), c.name);
        QVERIFY2(fig->isGridLayoutEnabled(), c.name);
        QVERIFY2(realListNearlyEqual(fig->gridRowSizes(), QList< qreal >{ 0.5, 0.5 }),
                 qPrintable(QStringLiteral("%1: rows must fall back to equal tracks, got %2")
                                .arg(QString::fromUtf8(c.name), sizesText(fig->gridRowSizes()))));
        QVERIFY2(realListNearlyEqual(fig->gridColumnSizes(), QList< qreal >{ 0.5, 0.5 }),
                 qPrintable(QStringLiteral("%1: columns must fall back to equal tracks, got %2")
                                .arg(QString::fromUtf8(c.name), sizesText(fig->gridColumnSizes()))));
        // 读端不得压入撤销命令（比例恢复是程序化设置）
        QCOMPARE(stack->count(), 0);
    }
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

/**
 * @brief 未打开的存档调用 load 必须失败
 *
 * 修复前：未打开（未 open/create）的 zip 里 getAllFiles 为空、read 返回空，DAChartItemArchive::load
 * 会把「读不到」当成「旧工程没有索引」而静默返回 true —— 受影响的存档看起来加载成功但图元全丢。
 * 现在 DAAbstractArchive::isOpen() 前置拦截，DAZipArchive 返回真实打开状态。
 */
void DAChartSerializeTest::testChartItemArchiveRejectsClosedArchive()
{
    // ① 真实 zip：新建后未 open/create，load 必须返回 false
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    DA::DAZipArchive closedZip(tempDir.filePath(QStringLiteral("closed.dapro")));
    QVERIFY2(!closedZip.isOpen(), "a freshly constructed zip archive must not report itself as open");
    DA::DAChartItemsManager zipMgr;
    QString zipError;
    QVERIFY2(!DA::DAChartItemArchive::load(&closedZip, &zipMgr, QStringLiteral("chart-data"), &zipError),
             "loading chart-data from a closed zip archive must fail instead of pretending the project is old");
    QVERIFY(!zipError.isEmpty());
    QVERIFY(zipMgr.items().isEmpty());

    // ② 即使存档内容完整，未打开也不能静默成功
    DA::DAChartWidget chart;
    QVERIFY(chart.addCurve(QVector< double >{ 0.0, 1.0 }, QVector< double >{ 2.0, 3.0 }, QStringLiteral("closed-A")));
    DA::DAChartItemsManager srcMgr;
    QDomDocument doc;
    DA::DAXmlHelper::makeElement(&chart, QStringLiteral("chart"), &doc, &srcMgr);
    QCOMPARE(srcMgr.items().size(), 1);
    MemoryArchive srcMem;
    QString saveError;
    QVERIFY2(DA::DAChartItemArchive::save(&srcMem, &srcMgr, QStringLiteral("chart-data"), &saveError),
             qPrintable(saveError));

    MemoryArchive closedMem;
    closedMem.m_files = srcMem.m_files;
    closedMem.opened  = false;
    DA::DAChartItemsManager closedMgr;
    QString closedError;
    QVERIFY2(!DA::DAChartItemArchive::load(&closedMem, &closedMgr, QStringLiteral("chart-data"), &closedError),
             "a non-empty but closed archive must still be rejected");
    QVERIFY(!closedError.isEmpty());
    QVERIFY(closedMgr.items().isEmpty());

    // ③ 打开同一份内容后可正常读回（打开状态是唯一区别）
    closedMem.opened = true;
    QString reopenedError;
    QVERIFY2(DA::DAChartItemArchive::load(&closedMem, &closedMgr, QStringLiteral("chart-data"), &reopenedError),
             qPrintable(reopenedError));
    QCOMPARE(closedMgr.items().size(), 1);
    qDeleteAll(closedMgr.items());
}

/**
 * @brief enableGridLayout 复位塌陷尺寸后，设置面板经信号自动刷新
 *
 * 面板要把 qwt figure 的 min/max 尺寸展示给用户；enableGridLayout 只在塌陷时复位这些值，
 * 复位后必须让面板重新读取，否则面板仍显示「最大宽度 0」这种过期值。
 * 面板类属于 DAGui，本用例放在链接 DAGui 的测试里（DAFigureGridTest 不链接 DAGui）。
 */
void DAChartSerializeTest::testFigureSizeConstraintsSignalRefreshesSettingPanel()
{
    std::unique_ptr< DA::DAFigureWidget > fig(new DA::DAFigureWidget());
    fig->resize(800, 600);
    fig->show();
    QTest::qWaitForWindowExposed(fig.get());
    QwtFigure* qwtFig = fig->figure();
    QVERIFY(qwtFig);

    DA::DAFigureWidgetSettingPanel panel;
    panel.setTarget(fig.get());
    // 模拟设置面板写入塌陷值：最大宽度 0
    qwtFig->setMaximumWidth(0);
    panel.updateUI();
    QCOMPARE(panel.propertyPanel()->getIntValue(DA::DAFigureWidgetSettingPanel::PID_MaxWidth), 0);

    // 启用网格：复位塌陷值，面板经 figureSizeConstraintsChanged 自动刷新（不手工调 updateUI）
    QVERIFY(fig->enableGridLayout(2, 2));
    QCOMPARE(qwtFig->maximumWidth(), QWIDGETSIZE_MAX);
    QCOMPARE(panel.propertyPanel()->getIntValue(DA::DAFigureWidgetSettingPanel::PID_MaxWidth), QWIDGETSIZE_MAX);

    // 切换目标后旧 figure 的复位不再影响面板
    std::unique_ptr< DA::DAFigureWidget > other(new DA::DAFigureWidget());
    other->resize(640, 480);
    other->show();
    QTest::qWaitForWindowExposed(other.get());
    panel.setTarget(other.get());
    const int otherMaxWidth = other->figure()->maximumWidth();
    qwtFig->setMaximumWidth(0);
    QVERIFY(fig->enableGridLayout(2, 3));  // 会再次复位塌陷值并发信号
    QCOMPARE(qwtFig->maximumWidth(), QWIDGETSIZE_MAX);
    QCOMPARE(panel.propertyPanel()->getIntValue(DA::DAFigureWidgetSettingPanel::PID_MaxWidth), otherMaxWidth);
}

/**
 * @brief 3D 子图的持久 id 与 visible 存档往返
 *
 * id 是后续 chart3d-data（3D 图元持久化）与多 3D 子图区分的稳定标识；visible 用于
 * 「热图/三维瀑布同格按模式切换」这类隐藏子图，重开后不能又冒出来。
 */
void DAChartSerializeTest::test3DChartIdAndVisibleRoundTrip()
{
    std::unique_ptr< DA::DAFigureWidget > src(new DA::DAFigureWidget());
    src->resize(800, 600);
    src->show();
    QTest::qWaitForWindowExposed(src.get());
    QVERIFY(src->enableGridLayout(2, 2));

    DA::DAChart3DWidget* chart3d = src->create3DChart();
    QVERIFY(chart3d);
    QVERIFY(src->setGridCell(chart3d, 1, 1));
    const QString srcId = chart3d->getChart3DId();
    QVERIFY2(!srcId.isEmpty(), "a 3D chart added to a figure must get a persistent id");
    QVERIFY2(srcId.startsWith(QStringLiteral("c3d-")), qPrintable(srcId));
    // 与 2D 子图 id（QwtPlot::plotId，uuid）是两套命名空间，不能混用
    DA::DAChartWidget* chart2d = src->createChart();
    QVERIFY(chart2d);
    QVERIFY2(!chart2d->plotId().isEmpty() && chart2d->plotId() != chart3d->getChart3DId(),
             "the 2D plot id and the 3D chart id are different id spaces");

    // 隐藏：模拟「同格按显示模式切换」时被隐藏的三维瀑布
    chart3d->setVisible(false);

    QDomDocument doc;
    DA::DAChartItemsManager mgr;
    const QDomElement figEle = DA::DAXmlHelper::makeElement(src.get(), QStringLiteral("figure"), &doc, &mgr);
    QVERIFY(!figEle.isNull());
    QDomElement chart3dEle;
    for (QDomElement ele = figEle.firstChildElement(QStringLiteral("chart3d")); !ele.isNull();
         ele             = ele.nextSiblingElement(QStringLiteral("chart3d"))) {
        chart3dEle = ele;
    }
    QVERIFY(!chart3dEle.isNull());
    QCOMPARE(chart3dEle.attribute(QStringLiteral("id")), srcId);
    QCOMPARE(chart3dEle.attribute(QStringLiteral("visible")).toInt(), 0);

    // 载入新 figure：id/visible/grid 单元格都要恢复
    std::unique_ptr< DA::DAFigureWidget > dst(new DA::DAFigureWidget());
    dst->resize(800, 600);
    dst->show();
    QTest::qWaitForWindowExposed(dst.get());
    QVERIFY(DA::DAXmlHelper::loadElement(dst.get(), &figEle, &mgr));
    const QList< DA::DAChart3DWidget* > loaded = dst->get3DCharts();
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded.first()->getChart3DId(), srcId);
    QVERIFY2(!loaded.first()->isVisible(), "a hidden 3D chart must stay hidden after reload");
    QVERIFY(dst->grid()->gridPosition(loaded.first(), nullptr, nullptr, nullptr, nullptr));

    // 同一 figure 里第二个 3D 子图必须拿到不同的 id
    DA::DAChart3DWidget* second = dst->create3DChart();
    QVERIFY(second);
    QVERIFY(!second->getChart3DId().isEmpty());
    QVERIFY2(second->getChart3DId() != srcId, "each 3D chart must have its own persistent id");
}

/**
 * @brief 旧工程的 <chart3d> 没有 id/visible 时按默认加载
 *
 * 旧工程：3D 子图默认可见，id 在加入 figure 时生成（两个子图的 id 互不相同）。
 */
void DAChartSerializeTest::testLegacy3DChartWithoutIdVisible()
{
    QDomDocument doc;
    QDomElement figEle = doc.createElement(QStringLiteral("figure"));
    figEle.setAttribute(QStringLiteral("id"), QStringLiteral("legacy-3d-figure"));
    doc.appendChild(figEle);
    figEle.appendChild(doc.createElement(QStringLiteral("charts")));
    const auto addLegacy3D = [&](qreal x, qreal y, qreal w, qreal h) {
        QDomElement e = doc.createElement(QStringLiteral("chart3d"));
        e.setAttribute(QStringLiteral("x"), x);
        e.setAttribute(QStringLiteral("y"), y);
        e.setAttribute(QStringLiteral("w"), w);
        e.setAttribute(QStringLiteral("h"), h);
        figEle.appendChild(e);
    };
    addLegacy3D(0.05, 0.05, 0.4, 0.4);
    addLegacy3D(0.55, 0.55, 0.4, 0.4);

    std::unique_ptr< DA::DAFigureWidget > fig(new DA::DAFigureWidget());
    fig->resize(640, 480);
    fig->show();
    QTest::qWaitForWindowExposed(fig.get());
    DA::DAChartItemsManager mgr;
    QVERIFY(DA::DAXmlHelper::loadElement(fig.get(), &figEle, &mgr));

    const QList< DA::DAChart3DWidget* > charts3d = fig->get3DCharts();
    QCOMPARE(charts3d.size(), 2);
    QVERIFY2(charts3d.at(0)->isVisible() && charts3d.at(1)->isVisible(),
             "3D charts of an old project (no visible attribute) must load visible");
    QVERIFY2(!charts3d.at(0)->getChart3DId().isEmpty() && !charts3d.at(1)->getChart3DId().isEmpty(),
             "3D charts must get a persistent id even when the project has none");
    QVERIFY2(charts3d.at(0)->getChart3DId() != charts3d.at(1)->getChart3DId(),
             "the two 3D charts must not share an id");
    QVERIFY(!fig->isGridLayoutEnabled());
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

/**
 * @brief 快照两段式与 save() 输出字节等价，load() 读回一致
 *
 * 同一批图元分别走 save()（后台线程序列化）与 makeSnapshots()+saveSnapshots()
 * （GUI 线程序列化成字节、后台只写字节）：
 * - 索引 chart-items.xml 与每个 <prefix>/<key> 图元文件字节完全一致；
 * - 快照存档经 load() 读回后 keys/rtti/标题/数据量与源一致（加载端无需改动）。
 */
void DAChartSerializeTest::testChartItemSnapshotSaveEquivalence()
{
    // 源绘图：两条不同长度/数据/标题的曲线
    DA::DAChartWidget chart;
    QVERIFY(chart.addCurve(QVector< double >{ 0.0, 1.0, 2.0, 3.0 },
                           QVector< double >{ 10.0, 11.0, 12.0, 13.0 }, QStringLiteral("snap-A")));
    QVERIFY(chart.addCurve(QVector< double >{ 5.0, 6.0, 7.0 }, QVector< double >{ 1.0, 2.0, 3.0 },
                           QStringLiteral("snap-B")));

    DA::DAChartItemsManager srcMgr;
    QDomDocument doc;
    DA::DAXmlHelper::makeElement(&chart, QStringLiteral("chart"), &doc, &srcMgr);
    QCOMPARE(srcMgr.items().size(), 2);
    const QList< QString > srcKeys = srcMgr.keys();
    QCOMPARE(srcKeys.size(), 2);

    // 旧路径：save() 直接序列化图元
    MemoryArchive saveMem;
    QString saveError;
    QVERIFY2(DA::DAChartItemArchive::save(&saveMem, &srcMgr, QStringLiteral("chart-data"), &saveError),
             qPrintable(saveError));
    // 新路径：GUI 线程快照 + 后台只写字节
    const QList< DA::DAChartItemSnapshot > snapshots = DA::DAChartItemArchive::makeSnapshots(&srcMgr);
    QCOMPARE(snapshots.size(), 2);
    MemoryArchive snapMem;
    QString snapError;
    QVERIFY2(DA::DAChartItemArchive::saveSnapshots(&snapMem, snapshots, QStringLiteral("chart-data"), &snapError),
             qPrintable(snapError));

    // 索引与每图元文件字节完全一致（saveSnapshots 输出与 save() 等价）
    const QString indexRelPath = QStringLiteral("chart-data/") + DA::DAChartItemArchive::itemsIndexFileName();
    QVERIFY(saveMem.contains(indexRelPath));
    QVERIFY(snapMem.contains(indexRelPath));
    QCOMPARE(snapMem.read(indexRelPath), saveMem.read(indexRelPath));
    for (const QString& key : srcKeys) {
        const QString itemPath = QStringLiteral("chart-data/%1").arg(key);
        QVERIFY2(snapMem.contains(itemPath), qPrintable(QStringLiteral("missing snapshot item file: %1").arg(key)));
        QCOMPARE(snapMem.read(itemPath), saveMem.read(itemPath));
    }

    // 快照存档经既有 load() 读回：keys/rtti/标题/数据量与源一致
    DA::DAChartItemsManager restoredMgr;
    QString loadError;
    QVERIFY2(DA::DAChartItemArchive::load(&snapMem, &restoredMgr, QStringLiteral("chart-data"), &loadError),
             qPrintable(loadError));
    QCOMPARE(restoredMgr.items().size(), 2);
    QCOMPARE(keySet(restoredMgr.keys()), keySet(srcKeys));
    for (const QString& key : srcKeys) {
        auto* restored = dynamic_cast< QwtPlotCurve* >(restoredMgr.keyToItem(key));
        QVERIFY2(restored, qPrintable(QStringLiteral("missing snapshot-restored curve: %1").arg(key)));
        QCOMPARE(restored->rtti(), static_cast< int >(QwtPlotItem::Rtti_PlotCurve));
        const QString title = restored->title().text();
        QVERIFY(title == QStringLiteral("snap-A") || title == QStringLiteral("snap-B"));
        QCOMPARE(static_cast< int >(restored->dataSize()), title == QStringLiteral("snap-A") ? 4 : 3);
    }
    qDeleteAll(restoredMgr.items());
}

/**
 * @brief BLK-01 回归：makeSnapshots 后图元全部 delete，saveSnapshots 仍能完整恢复
 *
 * 模拟保存期间 GUI 线程（嵌套事件循环继续派发定时器）删除/重建图元：
 * 快照生成后源图元全部 delete（管理者里只剩悬空指针），后台任务只写字节——
 * 写路径不依赖任何活指针，load() 读回仍完整。
 */
void DAChartSerializeTest::testChartItemSnapshotSurvivesItemDeletion()
{
    DA::DAChartWidget chart;
    QVERIFY(chart.addCurve(QVector< double >{ 0.0, 1.0, 2.0 }, QVector< double >{ 4.0, 5.0, 6.0 },
                           QStringLiteral("uaf-A")));
    QVERIFY(chart.addCurve(QVector< double >{ 3.0, 4.0 }, QVector< double >{ 7.0, 8.0 }, QStringLiteral("uaf-B")));

    DA::DAChartItemsManager srcMgr;
    QDomDocument doc;
    DA::DAXmlHelper::makeElement(&chart, QStringLiteral("chart"), &doc, &srcMgr);
    QCOMPARE(srcMgr.items().size(), 2);
    const QList< QString > srcKeys = srcMgr.keys();
    QCOMPARE(srcKeys.size(), 2);

    // GUI 线程先生成快照（此后不再解引用任何 QwtPlotItem*）
    const QList< DA::DAChartItemSnapshot > snapshots = DA::DAChartItemArchive::makeSnapshots(&srcMgr);
    QCOMPARE(snapshots.size(), 2);
    for (const DA::DAChartItemSnapshot& snap : snapshots) {
        QVERIFY(!snap.key.isEmpty());
        QVERIFY(!snap.data.isEmpty());
        QCOMPARE(snap.rtti != 0, true);
    }

    // 模拟 GUI 线程删除全部图元（保存期间峰值 marker 重建即此场景）；
    // 管理者内部登记随之悬空，但快照已持有字节，写存档不触碰它们
    qDeleteAll(srcMgr.items());

    MemoryArchive mem;
    QString error;
    QVERIFY2(DA::DAChartItemArchive::saveSnapshots(&mem, snapshots, QStringLiteral("chart-data"), &error),
             qPrintable(error));

    // 读回：keys/rtti/标题/数据量完整恢复（证明写路径不依赖活指针）
    DA::DAChartItemsManager restoredMgr;
    QVERIFY2(DA::DAChartItemArchive::load(&mem, &restoredMgr, QStringLiteral("chart-data"), &error),
             qPrintable(error));
    QCOMPARE(restoredMgr.items().size(), 2);
    QCOMPARE(keySet(restoredMgr.keys()), keySet(srcKeys));
    for (const QString& key : srcKeys) {
        auto* restored = dynamic_cast< QwtPlotCurve* >(restoredMgr.keyToItem(key));
        QVERIFY2(restored, qPrintable(QStringLiteral("missing restored curve after item deletion: %1").arg(key)));
        const QString title = restored->title().text();
        QVERIFY(title == QStringLiteral("uaf-A") || title == QStringLiteral("uaf-B"));
        QCOMPARE(static_cast< int >(restored->dataSize()), title == QStringLiteral("uaf-A") ? 3 : 2);
    }
    qDeleteAll(restoredMgr.items());
}

/**
 * @brief 工程级存档顺序回归：按 DAAppProject 的任务顺序组装并往返
 *
 * DAAppProject 无法在单测实例化，这里按其任务顺序等价复刻（行号依据）：
 * - 保存（src/APP/DAAppProject.cpp:1493-1506 makeSaveChartTask）：先 appendXmlSaveTask
 *   写 charts.xml（:1499），再 appendChartItemSaveTask 写 chart-data（:1503）；
 * - 加载（src/APP/DAAppProject.cpp:1063-1108）：先 appendChartItemLoadTask 读 chart-data
 *   （:1063-1065，注释「ChartItemLoadTask必须在chart info 的XmlLoadTask之前」），
 *   再 appendXmlLoadTask 读 charts.xml（:1104）并按 key 把图元 attach 回子图。
 * 用真实 DAZipArchive（临时文件）+ 相同任务类，断言图元 key 能重新 attach、往返成功，
 * 锁定「保存 charts.xml→chart-data、加载 chart-data→charts.xml」的顺序。
 */
void DAChartSerializeTest::testProjectArchiveOrderRoundTrip()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString zipPath = tempDir.filePath(QStringLiteral("project-order.dapro"));

    // 源绘图：两条曲线；charts.xml 按 createChartsUIDomDocument 的结构生成
    // （root[type=chart] → project → <chart>，DAAppProject.cpp:1682-1700）
    DA::DAChartWidget chart;
    QVERIFY(chart.addCurve(QVector< double >{ 0.0, 1.0, 2.0 }, QVector< double >{ 4.0, 5.0, 6.0 },
                           QStringLiteral("order-A")));
    QVERIFY(chart.addCurve(QVector< double >{ 3.0, 4.0 }, QVector< double >{ 7.0, 8.0 }, QStringLiteral("order-B")));
    DA::DAChartItemsManager srcMgr;
    QDomDocument chartXml;
    QDomProcessingInstruction chartXmlPi =
        chartXml.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\""));
    chartXml.appendChild(chartXmlPi);
    QDomElement rootEle = chartXml.createElement(QStringLiteral("root"));
    rootEle.setAttribute(QStringLiteral("type"), QStringLiteral("chart"));
    chartXml.appendChild(rootEle);
    QDomElement projectEle = chartXml.createElement(QStringLiteral("project"));
    rootEle.appendChild(projectEle);
    QDomElement chartEle = DA::DAXmlHelper::makeElement(&chart, QStringLiteral("chart"), &chartXml, &srcMgr);
    projectEle.appendChild(chartEle);
    QCOMPARE(srcMgr.items().size(), 2);
    const QList< QString > srcKeys = srcMgr.keys();
    QCOMPARE(srcKeys.size(), 2);

    // === 保存：按 makeSaveChartTask 的顺序组装任务（charts.xml 先于 chart-data） ===
    {
        DA::DAZipArchive zip;
        QSignalSpy finished(&zip, &DA::DAAbstractArchive::taskFinished);
        // t1：charts.xml（DAAppProject.cpp:1499）
        auto xmlTask = std::make_shared< DA::DAZipArchiveTask_Xml >(QStringLiteral("charts.xml"), chartXml);
        zip.appendTask(xmlTask);
        // t2：chart-data（DAAppProject.cpp:1503）
        auto itemTask = std::make_shared< DA::DAZipArchiveTask_ChartItem >(QStringLiteral("chart-data"), srcMgr);
        zip.appendTask(itemTask);
        zip.saveAll(zipPath);
        QCOMPARE(finished.count(), 1);
        QCOMPARE(finished.takeFirst().at(0).toInt(), static_cast< int >(DA::DAAbstractArchive::SaveSuccess));
        QVERIFY2(itemTask->getLastError().isEmpty(), qPrintable(itemTask->getLastError()));
    }
    QVERIFY(QFile::exists(zipPath));

    // === 加载：按 DAAppProject.cpp:1063-1108 的顺序组装任务（chart-data 先于 charts.xml） ===
    DA::DAChartItemsManager loadedMgr;
    QDomDocument loadedChartsXml;
    {
        DA::DAZipArchive zipIn;
        QSignalSpy finishedIn(&zipIn, &DA::DAAbstractArchive::taskFinished);
        // taskChartItem 先注册（:1063-1065）：图元先读回 manager
        auto itemLoadTask = std::make_shared< DA::DAZipArchiveTask_ChartItem >(QStringLiteral("chart-data"));
        zipIn.appendTask(itemLoadTask);
        // taskCharts 后注册（:1104）：charts.xml 只记 key，应用时经 keyToItem attach
        auto xmlLoadTask = std::make_shared< DA::DAZipArchiveTask_Xml >(QStringLiteral("charts.xml"));
        zipIn.appendTask(xmlLoadTask);
        zipIn.loadAll(zipPath);
        QCOMPARE(finishedIn.count(), 1);
        QCOMPARE(finishedIn.takeFirst().at(0).toInt(), static_cast< int >(DA::DAAbstractArchive::LoadSuccess));
        QVERIFY2(itemLoadTask->getLastError().isEmpty(), qPrintable(itemLoadTask->getLastError()));
        loadedMgr     = itemLoadTask->getChartItemsManager();
        loadedChartsXml = xmlLoadTask->getDomDocument();
    }
    QVERIFY(!loadedChartsXml.isNull());
    QCOMPARE(loadedMgr.items().size(), 2);
    QCOMPARE(keySet(loadedMgr.keys()), keySet(srcKeys));

    // === 应用：charts.xml 按图元 key 把图元 attach 回新子图（loadedChartsInfo 的生产路径） ===
    DA::DAChartWidget restoredChart;
    const QDomElement loadedRoot = loadedChartsXml.documentElement();
    QVERIFY2(!loadedRoot.isNull(), "charts.xml should have a root element");
    const QDomElement loadedChartEle =
        loadedRoot.firstChildElement(QStringLiteral("project")).firstChildElement(QStringLiteral("chart"));
    QVERIFY2(!loadedChartEle.isNull(), "charts.xml should contain a <chart> element under <project>");
    QVERIFY(DA::DAXmlHelper::loadElement(&restoredChart, &loadedChartEle, &loadedMgr));
    // 两条曲线都按 key 重新 attach 到恢复的子图（曲线数为准，不排除未来默认图元）
    const QList< QwtPlotItem* > attached = restoredChart.itemList();
    int curveCount = 0;
    for (QwtPlotItem* item : attached) {
        if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
            ++curveCount;
        }
    }
    QCOMPARE(curveCount, 2);
    for (const QString& key : srcKeys) {
        auto* restored = dynamic_cast< QwtPlotCurve* >(loadedMgr.keyToItem(key));
        QVERIFY2(restored, qPrintable(QStringLiteral("missing project-order restored curve: %1").arg(key)));
        QVERIFY2(restored->plot() == &restoredChart,
                 qPrintable(QStringLiteral("restored curve %1 must be attached to the chart").arg(key)));
        const QString title = restored->title().text();
        QVERIFY(title == QStringLiteral("order-A") || title == QStringLiteral("order-B"));
        QCOMPARE(static_cast< int >(restored->dataSize()), title == QStringLiteral("order-A") ? 3 : 2);
    }
    qDeleteAll(loadedMgr.items());
}

/**
 * @brief SF-06 回归：设过当前 3D 子图的 figure 析构不崩
 *
 * 复现路径（修复前必崩）：构造 figure → create3DChart → setCurrent3DChart（SF-06 的 3D 激活
 * 路径，mCurrent3DChart 非空）→ delete figure。~DAFigureWidget 之后 ~QWidget::deleteChildren
 * 逐层释放 viewport → 内层 QwtFigure → 3D 子图，3D 子图的 destroyed 信号触发 add3DChart
 * 注册的清理 lambda：彼时 d_ptr（PIMPL 成员）已释放、内层 QwtFigure 已删除，lambda 里
 * mCurrent3DChart 命中分支再调 getCurrentChart()（内部 figure()->currentAxes()）即 UAF。
 * 修复：析构体在 PIMPL 仍存活时显式断开全部 3D 子图的 destroyed 连接，析构期不再进入 lambda。
 */
void DAChartSerializeTest::testFigureDestructionWithCurrent3DChart()
{
    std::unique_ptr< DA::DAFigureWidget > fig(new DA::DAFigureWidget());
    DA::DAChart3DWidget* chart3d = fig->create3DChart();
    QVERIFY(chart3d);
    // 走 SF-06 的 3D 激活路径（setCurrent3DChart 记录最近激活类型）
    fig->setCurrent3DChart(chart3d);
    QCOMPARE(static_cast< int >(fig->currentSubChartKind()),
             static_cast< int >(DA::DAFigureCurrentSubChart::Chart3D));
    // delete figure：析构期 3D 子图 destroyed 信号不再进入访问失效 d_ptr/内层 figure 的 lambda
    fig.reset();
    // 走到这里即证明析构路径安全（修复前在 lambda 内 UAF 崩溃）
    QVERIFY(true);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DAChartSerializeTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "main.moc"
