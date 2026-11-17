#include <QtTest>
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QOpenGLContext>
#include <QSignalSpy>
#include <QMetaType>
#include <QStringList>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QStatusBar>
#include <QToolButton>
#include <QVariantMap>
#include <QDomDocument>
#include <QDomElement>
#include <QHash>
#include <QSet>
#include <QUndoCommand>
#include <QUndoGroup>
#include <QUndoStack>
#include <cmath>
#include <memory>
#include "Analysis/DASignalAmplitude.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASignalMemoryEstimate.h"
#include "Analysis/DAPeakTableModel.h"
#include "Archive/DAChartItemArchive.h"
#include "Archive/DAAbstractArchive.h"
#include "Archive/DASignalArchiveTask.h"
#include "Archive/DASignalFigureArchive.h"
#include "Chart/DAChartItemsManager.h"
#include "Common/DAStatusBarWidget.h"
#include "Dialogs/DAPeakMarkerDisplayDialog.h"
#include "Form/DASignalFormSpecBuilder.h"
#include "Page/DAPeakCurvePlotSamples.h"
#include "Page/DAPeakMarkerLabel.h"
#include "Page/DAPeakMarkerSamples.h"
#include "Page/DASignalAnalysisView.h"
#include "Page/DASignalChartExport.h"
#include "Render/DAStftRasterData.h"
#include "DAChart3DWidget.h"
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
#include "qwt_figure_grid.h"
#include "qwt_interval.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_symbol.h"

namespace
{
/**
 * @brief 测试用信号来源：不读真实文件，按请求长度返回 0 值样本
 */
class FakeSignalSource : public DA::DASignalSource
{
public:
    FakeSignalSource(int channels, qint64 sampleCount, double sampleRate)
        : m_channels(channels), m_sampleCount(sampleCount), m_sampleRate(sampleRate)
    {
    }
    SourceType type() const override
    {
        return FileSource;
    }
    bool readRange(int channel, qint64 start, qint64 count, QVector< float >& out, QString* error) const override
    {
        Q_UNUSED(channel)
        Q_UNUSED(error)
        if (start < 0 || start >= m_sampleCount) {
            out.clear();
            return true;
        }
        out.resize(int(qBound< qint64 >(0, count, m_sampleCount - start)));
        out.fill(0.0f);
        return true;
    }
    int channelCount() const override
    {
        return m_channels;
    }
    QStringList channelNames() const override
    {
        QStringList names;
        for (int i = 0; i < m_channels; ++i) {
            names.append(QStringLiteral("CH%1").arg(i + 1));
        }
        return names;
    }
    qint64 sampleCount() const override
    {
        return m_sampleCount;
    }
    double sampleRate() const override
    {
        return m_sampleRate;
    }
    QString playbackFilePath() const override
    {
        return QString();
    }
    QString sourceDescription() const override
    {
        return QStringLiteral("fake-source");
    }
    void setSampleRate(double fs) override
    {
        m_sampleRate = fs;
    }

private:
    int m_channels;
    qint64 m_sampleCount;
    double m_sampleRate;
};

/// 构造测试信号（默认单通道 1000 点 @1000 Hz）
DA::DASignalPtr makeTestSignal(int channels = 1, qint64 sampleCount = 1000, double sampleRate = 1000.0)
{
    auto source = std::make_shared< FakeSignalSource >(channels, sampleCount, sampleRate);
    return std::make_shared< DA::DASignal >(QStringLiteral("unit"), source);
}

/// 无 OpenGL（离屏/无 GL）时跳过依赖 3D 子图的用例
bool openGLAvailable()
{
    if (QGuiApplication::platformName().contains(QStringLiteral("offscreen"), Qt::CaseInsensitive)) {
        return false;
    }
    if (QSurfaceFormat::defaultFormat().majorVersion() < 2) {
        return false;
    }
    QOpenGLContext ctx;
    return ctx.create();
}
}  // namespace

namespace
{
/// 在表单规格里按字段名查找字段对象（遍历 group 的 items）
QJsonObject findFormField(const QJsonObject& spec, const QString& fieldName)
{
    const QJsonArray groups = spec.value(QStringLiteral("items")).toArray();
    for (const QJsonValue& gv : groups) {
        const QJsonObject group = gv.toObject();
        if (group.value(QStringLiteral("kind")).toString() != QLatin1String("group")) {
            continue;
        }
        const QJsonArray fields = group.value(QStringLiteral("items")).toArray();
        for (const QJsonValue& fv : fields) {
            const QJsonObject field = fv.toObject();
            if (field.value(QStringLiteral("name")).toString() == fieldName) {
                return field;
            }
        }
    }
    return QJsonObject();
}

/// 内存 archive：验证存档任务的读写/旧工程分支，不依赖磁盘/zip
class MemoryArchive : public DA::DAAbstractArchive
{
public:
    MemoryArchive() : DA::DAAbstractArchive(nullptr)
    {
    }

    bool write(const QString& relatePath, const QByteArray& byte) override
    {
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

    // 与真实存档语义一致：只要 key 存在就返回 true（0 字节文件也算存在）
    bool contains(const QString& relatePath) const override
    {
        return m_files.contains(relatePath);
    }

    QHash< QString, QByteArray > m_files;
};

/// 网格比例逐项比较（存档用 'g',12 格式写入，往返误差可忽略）
void compareSizeList(const QList< qreal >& actual, const QList< qreal >& expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (int i = 0; i < expected.size(); ++i) {
        QVERIFY2(qAbs(actual.at(i) - expected.at(i)) < 1e-6,
                 qPrintable(QStringLiteral("size[%1]: actual=%2 expected=%3")
                                .arg(i)
                                .arg(actual.at(i))
                                .arg(expected.at(i))));
    }
}

/// 子图的非 transient 图元（存档应保存的图元集合）
QList< QwtPlotItem* > nonTransientItems(DA::DAChartWidget* chart)
{
    QList< QwtPlotItem* > result;
    if (!chart) {
        return result;
    }
    const QList< QwtPlotItem* > items = chart->itemList();
    for (QwtPlotItem* item : items) {
        if (!chart->isItemTransient(item)) {
            result.append(item);
        }
    }
    return result;
}
}  // namespace

/**
 * @brief DASignalAnalysis 插件单元测试（方案 §13.2）
 *
 * 覆盖：参数结构体 ↔ JSON 映射、峰值表模型（列数/数值/表头）、
 * DAStftRasterData 的行列与区间换算。
 */
class DASignalAnalysisTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void paramsJsonRoundTrip();
    void paramsDefaults();
    void peakModelColumnsAndData();
    void peakModelAmplitudeMode();
    void stftRasterGrid();
    // 表单口径：峰值查找常见项（模式/TopN/相对门限）已迁到丝带，对话框只保留高级项
    void formSpecAdvancedPeakParams();
    // 老表单字符串值兼容（peakTopN 曾是 "10"/"20"/"50"）
    void paramsPeakTopNStringCompat();
    // 频率分辨率 ↔ STFT 窗长联动换算（方案 §6.3 / 评审 C4）
    void stftResolutionLinkage();
    // N-C4：默认参数自洽（stftWindowMs ↔ resolutionHz 互为倒数）
    void paramsDefaultsSelfConsistent();
    // N-C5/N-U6：最小间距「-1 = 自动」哨兵与表单派生默认值
    void peakSpacingAutoSentinel();
    // N-C2：幅值口径换算（峰值/均方根/dB，dBFS 参考无效时回退 1.0）
    void amplitudeDisplayModes();
    // U11/N-U4：峰值表时间列显隐（列数/表头映射，数据不丢）
    void peakModelTimeColumnToggle();
    // D2/测试缺口：参数 → Python 编排字典的键集合/原生类型契约（paramsToDict 下沉后可纯逻辑测试）
    void paramsPythonDictContract();
    // D5：内存估算公式契约（流式任务不随信号总长增长；整段取数任务按 float32 线性增长）
    void memoryEstimateContract();
    // 峰值曲线渲染：相邻帧跳变阈值（8 × Δf）与未知分辨率时的兜底
    void peakCurveJumpThreshold();
    // 峰值曲线渲染：连续段才连线，跳变/缺峰处插入 NaN 断开（其余帧仍以点保留）
    void peakCurvePlotSamples();
    void peakMarkerSnapping();
    // 峰值标记文本模板：占位符替换、紧凑数字格式、空模板与未知占位符
    void peakMarkerLabelText();
    // 峰值标记显示对话框：模板回显、实时预览（含 dB 口径）
    void peakMarkerDisplayDialog();
    // 状态栏进度条取消按钮：绑定动作、随进度条显隐、固定尺寸不改变状态栏高度
    void statusBarProgressCancelButton();
    // T7：视图 figure 为 2×2 网格，波形跨两列，热图与三维瀑布同格
    void signalViewFigureGrid();
    // T7：分析结果图元标 transient，峰值标记保持非 transient
    void signalViewTransientItems();
    // T7：瀑布模式切换热图/三维瀑布显隐与当前子图判定
    void signalViewWaterfallMode();
    // T8：点击 2D 子图画布即切换当前子图（设置面板 target 跟随）
    void signalViewClickSwitchesCurrentChart();
    // T9：figure XML 保存（角色/峰值/网格比例/非 transient 图元集合）
    void signalFigureSaveXml();
    // T9：存档任务往返（signals.json + figures.xml + chart-data）与 figure 恢复
    void signalArchiveRoundTrip();
    // T9：旧工程无 figures.xml/chart-data 时加载成功且无图元
    void signalArchiveLegacyWithoutFigures();
    // T9：损坏 figures.xml 恢复失败且视图保持默认布局
    void signalFigureRestoreCorruptXml();
    // T9：图表导出工具（整页/当前子图 PNG/SVG/剪贴板）
    void signalChartExportToFile();
    // BLK-02：同名信号不串档——signal-id 存档键 + 恢复端元素/图元 key 防争用
    void signalArchiveDuplicateNames();
    // BLK-01：快照两段式——写路径与活图元指针解耦（delete 全部图元后仍可写/读回）
    void signalArchiveSnapshotDecoupledFromLiveItems();
    // BLK-03a：外部删除缓存图元（图元树路径）后回填/重译不崩溃且按需重建
    void signalViewExternalItemRemoval();
    // SF-11：figure 撤销栈入组等价接线——activeStack 跟随 setActive、栈销毁自动摘除
    void undoStackGroupWiring();
    // SF-13：峰值「存档优先」只在加载后到第一次重算前——幅值/参数变化即重建
    void signalPeakMarkersRestoredUntilRecompute();
};

/**
 * @brief 参数 JSON 往返：所有字段都能保存并读回
 */
void DASignalAnalysisTest::paramsJsonRoundTrip()
{
    DA::DASignalAnalysisParams p;
    p.removeDc          = true;
    p.detrend           = QStringLiteral("linear");
    p.window            = QStringLiteral("blackman");
    p.amplitudeMode     = QStringLiteral("rms");
    p.dbReference       = QStringLiteral("one");
    p.resolutionHz      = 2.5;
    p.zeroPadding       = 4096;
    p.average           = QStringLiteral("median");
    p.overlap           = 75.0;
    p.stftWindowMs      = 20.0;
    p.stftOverlap       = 25.0;
    p.stftZeroPadding   = 512;
    p.peakMode          = QStringLiteral("advanced");
    p.peakTopN          = 50;
    p.peakRelDb         = 30.0;
    p.peakProminence    = 0.05;
    p.peakMinSpacingHz  = 12.5;
    p.peakMinHeight     = 0.001;
    p.calibration       = 2.0;
    p.unit              = QStringLiteral("Pa");

    const QJsonObject json = p.toJson();
    const DA::DASignalAnalysisParams q = DA::DASignalAnalysisParams::fromJson(json);

    QCOMPARE(q.removeDc, p.removeDc);
    QCOMPARE(q.detrend, p.detrend);
    QCOMPARE(q.window, p.window);
    QCOMPARE(q.amplitudeMode, p.amplitudeMode);
    QCOMPARE(q.dbReference, p.dbReference);
    QCOMPARE(q.resolutionHz, p.resolutionHz);
    QCOMPARE(q.zeroPadding, p.zeroPadding);
    QCOMPARE(q.average, p.average);
    QCOMPARE(q.overlap, p.overlap);
    QCOMPARE(q.stftWindowMs, p.stftWindowMs);
    QCOMPARE(q.stftOverlap, p.stftOverlap);
    QCOMPARE(q.stftZeroPadding, p.stftZeroPadding);
    QCOMPARE(q.peakMode, p.peakMode);
    QCOMPARE(q.peakTopN, p.peakTopN);
    QCOMPARE(q.peakRelDb, p.peakRelDb);
    QCOMPARE(q.peakProminence, p.peakProminence);
    QCOMPARE(q.peakMinSpacingHz, p.peakMinSpacingHz);
    QCOMPARE(q.peakMinHeight, p.peakMinHeight);
    QCOMPARE(q.calibration, p.calibration);
    QCOMPARE(q.unit, p.unit);
}

/**
 * @brief 默认参数与「恢复默认」口径
 */
void DASignalAnalysisTest::paramsDefaults()
{
    const DA::DASignalAnalysisParams d = DA::DASignalAnalysisParams::defaults();
    QCOMPARE(d.window, QStringLiteral("hann"));
    QCOMPARE(d.amplitudeMode, QStringLiteral("peak"));
    QCOMPARE(d.peakTopN, 10);
    QCOMPARE(d.peakRelDb, 40.0);
    QCOMPARE(d.calibration, 1.0);
    QVERIFY(d.unit.isEmpty());
    // 空 JSON 反序列化得到默认值
    const DA::DASignalAnalysisParams e = DA::DASignalAnalysisParams::fromJson(QJsonObject());
    QCOMPARE(e.window, d.window);
    QCOMPARE(e.peakTopN, d.peakTopN);
}

/**
 * @brief 峰值表模型：列数与数值
 */
void DASignalAnalysisTest::peakModelColumnsAndData()
{
    DA::DAPeakResult peaks;
    peaks.frequencies = { 1000.0, 2000.0, 3000.0 };
    peaks.magnitudes  = { 0.5, 0.3, 0.1 };
    peaks.relDb       = { 0.0, -4.44, -13.98 };
    peaks.share       = { 80.0, 15.0, 5.0 };
    peaks.resolutionHz = 1.0;

    DA::DAPeakTableModel model;
    model.setPeaks(peaks);
    QCOMPARE(model.rowCount(), 3);
    // 默认（整段频谱）不显示时间列：总列数 = ColumnCount - 1
    QCOMPARE(model.columnCount(), int(DA::DAPeakTableModel::ColumnCount) - 1);
    // 序号列从 1 开始
    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    // 频率列（隐藏时间列时视图列左移一列）
    const int freqViewColumn = int(DA::DAPeakTableModel::ColumnFrequency) - 1;
    QCOMPARE(model.data(model.index(1, freqViewColumn)).toString(), QStringLiteral("2000.00"));
    // 表头（频率列）
    QCOMPARE(model.headerData(freqViewColumn, Qt::Horizontal).toString(), QStringLiteral("Frequency (Hz)"));
    // STFT/跟随滑块模式：多一列帧时间（评审 U11）
    model.setShowTimeColumn(true);
    QCOMPARE(model.columnCount(), int(DA::DAPeakTableModel::ColumnCount));
    QCOMPARE(model.headerData(DA::DAPeakTableModel::ColumnTime, Qt::Horizontal).toString(),
             QStringLiteral("Time (s)"));
    QCOMPARE(model.data(model.index(0, DA::DAPeakTableModel::ColumnTime)).toString(),
             QStringLiteral("0.000"));
    model.setShowTimeColumn(false);
    QCOMPARE(model.columnCount(), int(DA::DAPeakTableModel::ColumnCount) - 1);
    // 清空
    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

/**
 * @brief 峰值表幅值列随显示口径切换（dBFS 时主峰为 0 dB）
 */
void DASignalAnalysisTest::peakModelAmplitudeMode()
{
    DA::DAPeakResult peaks;
    peaks.frequencies = { 1000.0, 2000.0 };
    peaks.magnitudes  = { 1.0, 0.5 };
    peaks.relDb       = { 0.0, -6.02 };
    peaks.share       = { 80.0, 20.0 };

    DA::DAPeakTableModel model;
    model.setPeaks(peaks);
    model.setSignalPeak(1.0);  // dBFS 参考 = 信号峰值（标定后），与频谱图同一参考
    model.setAmplitudeMode(QStringLiteral("dbFS"), QStringLiteral("fullScale"));
    const int ampViewColumn = int(DA::DAPeakTableModel::ColumnAmplitude) - 1;
    const QString top        = model.data(model.index(0, ampViewColumn)).toString();
    QVERIFY(top.contains(QStringLiteral("dB")));
    QVERIFY(qAbs(top.split(QLatin1Char(' ')).first().toDouble()) < 0.01);
    // 均方根口径：0.5 峰值幅值 → 0.353553（直流/零频不换算）
    model.setAmplitudeMode(QStringLiteral("rms"), QStringLiteral("fullScale"));
    QVERIFY(qAbs(model.data(model.index(1, ampViewColumn)).toString().toDouble() - 0.3535533906) < 1e-5);
}

/**
 * @brief 表单口径：峰值查找常见项已迁到丝带，对话框只保留高级项
 *
 * 模式 / 标记数量 / 相对门限在丝带「峰值」panel（单一入口），分析设置对话框不能再出现
 * 同名字段（否则两处状态会分叉、点确定还会覆盖丝带设置）；高级三项仍在对话框里。
 */
void DASignalAnalysisTest::formSpecAdvancedPeakParams()
{
    const DA::DASignalAnalysisParams params = DA::DASignalAnalysisParams::defaults();
    const QJsonObject spec                  = DA::DASignalFormSpecBuilder::build(params, 10.0);

    // 常见峰值查找项：不在对话框表单里（迁到 ribbon 峰值 panel）
    QVERIFY2(findFormField(spec, QStringLiteral("peakMode")).isEmpty(), "peakMode must not be in the analysis form");
    QVERIFY2(findFormField(spec, QStringLiteral("peakTopN")).isEmpty(), "peakTopN must not be in the analysis form");
    QVERIFY2(findFormField(spec, QStringLiteral("peakRelDb")).isEmpty(),
             "peakRelDb must not be in the analysis form");

    // 高级项仍在：最小间距默认 = 5 × 实际分辨率（10 赫兹 → 50 赫兹，评审 U7）
    const QJsonObject spacing = findFormField(spec, QStringLiteral("peakMinSpacingHz"));
    QVERIFY2(!spacing.isEmpty(), "peakMinSpacingHz field not found");
    QCOMPARE(spacing.value(QStringLiteral("default")).toDouble(), 50.0);
    QVERIFY2(!findFormField(spec, QStringLiteral("peakProminence")).isEmpty(),
             "peakProminence field not found");
    QVERIFY2(!findFormField(spec, QStringLiteral("peakMinHeight")).isEmpty(), "peakMinHeight field not found");

    // 幅值口径支持四种显示状态（评审 U8）
    const QJsonObject ampMode = findFormField(spec, QStringLiteral("amplitudeMode"));
    QVERIFY2(!ampMode.isEmpty(), "amplitudeMode field not found");
    QStringList ampValues;
    for (const QJsonValue& ov : ampMode.value(QStringLiteral("options")).toArray()) {
        ampValues.append(ov.toObject().value(QStringLiteral("value")).toString());
    }
    QCOMPARE(ampValues, QStringList({ QStringLiteral("peak"), QStringLiteral("rms"),
                                      QStringLiteral("dbFS"), QStringLiteral("dbRef1") }));
}

/**
 * @brief 老工程/老表单里的 peakTopN 字符串值仍能读回
 */
void DASignalAnalysisTest::paramsPeakTopNStringCompat()
{
    QJsonObject obj;
    obj[ QStringLiteral("peakTopN") ] = QStringLiteral("20");
    QCOMPARE(DA::DASignalAnalysisParams::fromJson(obj).peakTopN, 20);
    obj[ QStringLiteral("peakTopN") ] = 50;
    QCOMPARE(DA::DASignalAnalysisParams::fromJson(obj).peakTopN, 50);
}

/**
 * @brief 频率分辨率 ↔ STFT 窗长联动换算：Δf = 1 / 窗长(秒)
 *
 * 改分辨率必须同时改变 STFT 窗长（热图随分辨率变化，评审 C4），
 * 两者互逆且非法输入返回 0（不在对话框里写入无效值）。
 */
void DASignalAnalysisTest::stftResolutionLinkage()
{
    // 10 赫兹分辨率 → 100 毫秒窗长
    QCOMPARE(DA::signalStftWindowMsFromResolution(10.0), 100.0);
    // 100 毫秒窗长 → 10 赫兹分辨率（互逆）
    QCOMPARE(DA::signalResolutionFromStftWindowMs(100.0), 10.0);
    // 20 赫兹 → 50 毫秒；50 毫秒 → 20 赫兹
    QCOMPARE(DA::signalStftWindowMsFromResolution(20.0), 50.0);
    QCOMPARE(DA::signalResolutionFromStftWindowMs(50.0), 20.0);
    // 非法输入返回 0，调用方据此跳过写入
    QCOMPARE(DA::signalStftWindowMsFromResolution(0.0), 0.0);
    QCOMPARE(DA::signalStftWindowMsFromResolution(-1.0), 0.0);
    QCOMPARE(DA::signalResolutionFromStftWindowMs(0.0), 0.0);
    QCOMPARE(DA::signalResolutionFromStftWindowMs(-5.0), 0.0);
}

/**
 * @brief STFT 栅格：行列数与坐标区间换算
 */
void DASignalAnalysisTest::stftRasterGrid()
{
    DA::DAStftRasterData raster;
    const int rows = 4;
    const int cols = 5;
    QVector< double > values(rows * cols);
    for (int i = 0; i < values.size(); ++i) {
        values[ i ] = double(i);
    }
    raster.setGrid(values, rows, cols, 0.5, 1.0, 0.0, 2000.0);
    QCOMPARE(raster.rowCount(), rows);
    QCOMPARE(raster.columnCount(), cols);
    const QwtInterval xi = raster.interval(Qt::XAxis);
    const QwtInterval yi = raster.interval(Qt::YAxis);
    QCOMPARE(xi.minValue(), 0.5);
    QCOMPARE(xi.maxValue(), 1.0);
    QCOMPARE(yi.minValue(), 0.0);
    QCOMPARE(yi.maxValue(), 2000.0);
    // 显式色标范围生效
    raster.setValueRange(-80.0, 0.0);
    const QwtInterval zi = raster.interval(Qt::ZAxis);
    QCOMPARE(zi.minValue(), -80.0);
    QCOMPARE(zi.maxValue(), 0.0);
    // 清零范围回退到数据自动范围（数据 0..19）
    raster.setValueRange(-1.0, -2.0);
    const QwtInterval zAuto = raster.interval(Qt::ZAxis);
    QCOMPARE(zAuto.minValue(), 0.0);
    QCOMPARE(zAuto.maxValue(), 19.0);
}

/**
 * @brief N-C4：默认参数自洽 —— 10 赫兹分辨率 ↔ 100 毫秒 STFT 窗长
 *
 * 两处默认值曾独立定义（resolutionHz=10 与 stftWindowMs=50 ↔ 20 赫兹），
 * 打开对话框不做联动就自相矛盾；默认值与空 JSON 反序列化都必须自洽。
 */
void DASignalAnalysisTest::paramsDefaultsSelfConsistent()
{
    const DA::DASignalAnalysisParams d = DA::DASignalAnalysisParams::defaults();
    QCOMPARE(d.resolutionHz, 10.0);
    QCOMPARE(d.stftWindowMs, 100.0);
    // Δf = 1/窗长：两者必须互为倒数（评审 N-C4）
    QVERIFY2(qAbs(DA::signalResolutionFromStftWindowMs(d.stftWindowMs) - d.resolutionHz) < 1e-9,
             "default stftWindowMs is not consistent with resolutionHz");
    QCOMPARE(DA::signalStftWindowMsFromResolution(d.resolutionHz), 100.0);

    // 空 JSON（老工程/缺键）反序列化同样得到自洽默认值
    const DA::DASignalAnalysisParams e = DA::DASignalAnalysisParams::fromJson(QJsonObject());
    QCOMPARE(e.stftWindowMs, 100.0);
    QVERIFY2(qAbs(DA::signalResolutionFromStftWindowMs(e.stftWindowMs) - e.resolutionHz) < 1e-9,
             "empty-JSON defaults are not self-consistent");
}

/**
 * @brief N-C5/N-U6：最小间距「-1 = 自动」哨兵与表单派生默认值
 *
 * 参数用 -1 表示「未手改 = 跟随分辨率」，表单字段 min=0 不能直接写 -1，
 * 回显/默认值必须换算为 5×实际分辨率（否则显示 0，评审 N-U6）；
 * 手改后固定用户值，JSON 往返保持 -1 与显式值两种语义（评审 N-C5）。
 */
void DASignalAnalysisTest::peakSpacingAutoSentinel()
{
    const DA::DASignalAnalysisParams defaults = DA::DASignalAnalysisParams::defaults();
    // 1) 默认参数是「自动」哨兵，不是显式 0/50
    QCOMPARE(defaults.peakMinSpacingHz, -1.0);
    // 2) 自动口径 = 5 倍实际分辨率（实际分辨率 10 赫兹 → 50 赫兹）
    QCOMPARE(DA::signalEffectivePeakMinSpacingHz(defaults, 10.0), 50.0);
    // 实际分辨率随信号变化时，自动值继续跟随（20 赫兹 → 100 赫兹）
    QCOMPARE(DA::signalEffectivePeakMinSpacingHz(defaults, 20.0), 100.0);
    // 实际分辨率无效（如采样率未知）时退回参数里的 resolutionHz，避免回显 0
    QCOMPARE(DA::signalEffectivePeakMinSpacingHz(defaults, 0.0), 50.0);
    // 非法分辨率 → 自动间距 0（调用方据此跳过写入）
    QCOMPARE(DA::signalAutoPeakMinSpacingHz(0.0), 0.0);
    QCOMPARE(DA::signalAutoPeakMinSpacingHz(-5.0), 0.0);

    // 3) 用户手改 33 后不再跟随分辨率（跨对话框保留显式值，评审 N-C5）
    DA::DASignalAnalysisParams manual = defaults;
    manual.peakMinSpacingHz            = 33.0;
    QCOMPARE(DA::signalEffectivePeakMinSpacingHz(manual, 10.0), 33.0);
    QCOMPARE(DA::signalEffectivePeakMinSpacingHz(manual, 20.0), 33.0);

    // 4) 表单默认值：自动参数 → 5×实际分辨率（高级模式不再显示 0，评审 U7/N-U6）
    const QJsonObject autoSpec   = DA::DASignalFormSpecBuilder::build(defaults, 10.0);
    const QJsonObject autoField  = findFormField(autoSpec, QStringLiteral("peakMinSpacingHz"));
    QVERIFY2(!autoField.isEmpty(), "peakMinSpacingHz field not found");
    QCOMPARE(autoField.value(QStringLiteral("default")).toDouble(), 50.0);
    // 手改参数 → 表单默认值保持用户值
    const QJsonObject manualSpec  = DA::DASignalFormSpecBuilder::build(manual, 10.0);
    const QJsonObject manualField = findFormField(manualSpec, QStringLiteral("peakMinSpacingHz"));
    QVERIFY2(!manualField.isEmpty(), "peakMinSpacingHz field not found (manual)");
    QCOMPARE(manualField.value(QStringLiteral("default")).toDouble(), 33.0);

    // 5) JSON 往返分别保持「自动哨兵」与「显式值」，两者不互相污染
    QCOMPARE(DA::DASignalAnalysisParams::fromJson(defaults.toJson()).peakMinSpacingHz, -1.0);
    QCOMPARE(DA::DASignalAnalysisParams::fromJson(manual.toJson()).peakMinSpacingHz, 33.0);
}

/**
 * @brief N-C2：幅值口径换算（唯一真相 DASignalAmplitude::toDisplay）
 *
 * 覆盖峰值/RMS（直流不除 √2）/dBFS/dbRef1，以及 dBFS 参考无效（<=0）时
 * 回退 1.0 而不是 cEps（否则会出现 ≈ +600 dB 的荒谬结果）。
 */
void DASignalAnalysisTest::amplitudeDisplayModes()
{
    using DA::DASignalAmplitude::toDisplay;

    // peak：乘标定（calibration=2、峰值 0.5 → 1.0）
    QCOMPARE(toDisplay(0.5, false, QStringLiteral("peak"), QStringLiteral("fullScale"), 2.0, 1.0), 1.0);

    // rms：非直流 = 峰值/√2；直流（零频）不除 √2
    QVERIFY(qAbs(toDisplay(0.5, false, QStringLiteral("rms"), QStringLiteral("fullScale"), 1.0, 1.0) - 0.3535533906) < 1e-9);
    QCOMPARE(toDisplay(0.5, true, QStringLiteral("rms"), QStringLiteral("fullScale"), 1.0, 1.0), 0.5);

    // dbFS：20·log10(0.5 / 1.0) = -6.0206
    const double dbFs = toDisplay(0.5, false, QStringLiteral("dbFS"), QStringLiteral("fullScale"), 1.0, 1.0);
    QVERIFY2(qAbs(dbFs - (-6.020599913279624)) < 1e-9, "dbFS conversion of 0.5 re 1.0 must be -6.0206 dB");

    // dBFS 参考无效（0 或负值）→ 回退 1.0 参考，结果仍有限且为 -6.0206（不是 +600 dB）
    const double invalidRef = toDisplay(0.5, false, QStringLiteral("dbFS"), QStringLiteral("fullScale"), 1.0, 0.0);
    const double negativeRef = toDisplay(0.5, false, QStringLiteral("dbFS"), QStringLiteral("fullScale"), 1.0, -3.0);
    QVERIFY2(std::isfinite(invalidRef), "dbFS with invalid reference must stay finite");
    QVERIFY2(std::isfinite(negativeRef), "dbFS with negative reference must stay finite");
    QVERIFY(qAbs(invalidRef - dbFs) < 1e-9);
    QVERIFY(qAbs(negativeRef - dbFs) < 1e-9);
    QVERIFY2(invalidRef < 0.0 && invalidRef > -7.0, "invalid dBFS reference must not blow up to +600 dB");

    // dbRef1 与 dbFS + dbReference=one 等价（都不依赖信号峰值参考）
    QCOMPARE(toDisplay(0.5, false, QStringLiteral("dbRef1"), QStringLiteral("fullScale"), 1.0, 0.0),
             toDisplay(0.5, false, QStringLiteral("dbFS"), QStringLiteral("one"), 1.0, 123.0));

    // 口径判定与坐标轴文字（界面/导出共用）
    QVERIFY(DA::DASignalAmplitude::isDbMode(QStringLiteral("dbFS")));
    QVERIFY(DA::DASignalAmplitude::isDbMode(QStringLiteral("dbRef1")));
    QVERIFY(!DA::DASignalAmplitude::isDbMode(QStringLiteral("peak")));
    QVERIFY(DA::DASignalAmplitude::isRmsMode(QStringLiteral("rms")));
    QCOMPARE(DA::DASignalAmplitude::axisLabelKey(QStringLiteral("dbFS")), QStringLiteral("dB (FS)"));
}

/**
 * @brief U11/N-U4：峰值表时间列显隐（列数/表头映射，数据不丢）
 *
 * STFT/跟随滑块模式显示时间列，切回整段频谱隐藏时间列；隐藏时非序号列整体左移一列，
 * 数值与表头都跟随映射（评审 §七-6 / U11）。
 */
void DASignalAnalysisTest::peakModelTimeColumnToggle()
{
    DA::DAPeakResult peaks;
    peaks.frequencies = { 1000.0, 2000.0 };
    peaks.magnitudes  = { 0.5, 0.25 };
    peaks.relDb       = { 0.0, -6.02 };
    peaks.share       = { 80.0, 20.0 };
    peaks.frameTime   = 1.234;
    peaks.hasFrameTime = true;

    DA::DAPeakTableModel model;
    model.setPeaks(peaks);
    QSignalSpy headerSpy(&model, &DA::DAPeakTableModel::headerTextChanged);

    // 显示时间列
    model.setShowTimeColumn(true);
    QCOMPARE(headerSpy.count(), 1);
    QCOMPARE(model.columnCount(), int(DA::DAPeakTableModel::ColumnCount));
    QCOMPARE(model.headerData(DA::DAPeakTableModel::ColumnTime, Qt::Horizontal).toString(), QStringLiteral("Time (s)"));
    QCOMPARE(model.data(model.index(0, DA::DAPeakTableModel::ColumnTime)).toString(), QStringLiteral("1.234"));
    QCOMPARE(model.data(model.index(0, DA::DAPeakTableModel::ColumnFrequency)).toString(), QStringLiteral("1000.00"));

    // 重复设置在相同状态时不重复发信号（幂等）
    model.setShowTimeColumn(true);
    QCOMPARE(headerSpy.count(), 1);

    // 隐藏时间列：列数 -1，非序号列左移一列，数值/表头不变
    model.setShowTimeColumn(false);
    QCOMPARE(headerSpy.count(), 2);
    QCOMPARE(model.columnCount(), int(DA::DAPeakTableModel::ColumnCount) - 1);
    const int freqViewColumn = int(DA::DAPeakTableModel::ColumnFrequency) - 1;
    QCOMPARE(model.headerData(freqViewColumn, Qt::Horizontal).toString(), QStringLiteral("Frequency (Hz)"));
    QCOMPARE(model.data(model.index(0, freqViewColumn)).toString(), QStringLiteral("1000.00"));
    QCOMPARE(model.data(model.index(1, freqViewColumn)).toString(), QStringLiteral("2000.00"));

    // 再次显示：行数/频率数据不丢
    model.setShowTimeColumn(true);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(1, DA::DAPeakTableModel::ColumnFrequency)).toString(), QStringLiteral("2000.00"));
}

/**
 * @brief 参数 → Python 编排参数字典契约（D2 / 测试缺口补强）
 *
 * DASignalAnalysisWorker::paramsToDict 原先在 .cpp 匿名命名空间里写死一遍键名，无法单测；
 * 键名映射下沉到 DASignalAnalysisParams::toPythonDict() 后，这里锁定跨语言契约：
 * - 键集合精确等于 Python 侧 spectrum.py / stft.py / peaks.py 读取的键 + 4 个仅显示层键；
 * - 原生类型不被 JSON 往返改成 double（peakTopN/zeroPadding 仍是 int 而非 double）；
 * - 默认值与 DASignalAnalysisParams::defaults() 一致。
 */
void DASignalAnalysisTest::paramsPythonDictContract()
{
    DA::DASignalAnalysisParams p;
    p.removeDc         = true;
    p.detrend          = QStringLiteral("linear");
    p.window           = QStringLiteral("blackman");
    p.amplitudeMode    = QStringLiteral("rms");
    p.dbReference      = QStringLiteral("one");
    p.resolutionHz     = 2.5;
    p.zeroPadding      = 4096;
    p.average          = QStringLiteral("median");
    p.overlap          = 75.0;
    p.stftWindowMs     = 20.0;
    p.stftOverlap      = 25.0;
    p.stftZeroPadding  = 512;
    p.peakMode         = QStringLiteral("advanced");
    p.peakTopN         = 50;
    p.peakRelDb        = 30.0;
    p.peakProminence   = 0.05;
    p.peakMinSpacingHz = 12.5;
    p.peakMinHeight    = 0.001;
    p.calibration      = 2.0;
    p.unit             = QStringLiteral("Pa");

    const QVariantMap d = p.toPythonDict();
    // 20 个键：16 个被 Python 编排读取 + amplitudeMode/dbReference/calibration/unit 仅显示层使用
    const QStringList expectedKeys = {
        QStringLiteral("removeDc"),         QStringLiteral("detrend"),          QStringLiteral("window"),
        QStringLiteral("amplitudeMode"),    QStringLiteral("dbReference"),      QStringLiteral("resolutionHz"),
        QStringLiteral("zeroPadding"),      QStringLiteral("average"),          QStringLiteral("overlap"),
        QStringLiteral("stftWindowMs"),     QStringLiteral("stftOverlap"),      QStringLiteral("stftZeroPadding"),
        QStringLiteral("peakMode"),         QStringLiteral("peakTopN"),         QStringLiteral("peakRelDb"),
        QStringLiteral("peakProminence"),   QStringLiteral("peakMinSpacingHz"), QStringLiteral("peakMinHeight"),
        QStringLiteral("calibration"),      QStringLiteral("unit")
    };
    QCOMPARE(d.size(), expectedKeys.size());
    for (const QString& key : expectedKeys) {
        QVERIFY2(d.contains(key), qPrintable(QStringLiteral("missing python param key: %1").arg(key)));
    }

    // 原生类型：布尔/整数/浮点/字符串，不经 JSON 往返（否则 int 会被转成 double）
    QCOMPARE(d.value(QStringLiteral("removeDc")).userType(), int(QMetaType::Bool));
    QCOMPARE(d.value(QStringLiteral("zeroPadding")).userType(), int(QMetaType::Int));
    QCOMPARE(d.value(QStringLiteral("peakTopN")).userType(), int(QMetaType::Int));
    QCOMPARE(d.value(QStringLiteral("stftZeroPadding")).userType(), int(QMetaType::Int));
    QCOMPARE(d.value(QStringLiteral("resolutionHz")).userType(), int(QMetaType::Double));
    QCOMPARE(d.value(QStringLiteral("window")).userType(), int(QMetaType::QString));

    // 取值（改哪个字段，Python 就应收到哪个值）
    QCOMPARE(d.value(QStringLiteral("removeDc")).toBool(), true);
    QCOMPARE(d.value(QStringLiteral("detrend")).toString(), QStringLiteral("linear"));
    QCOMPARE(d.value(QStringLiteral("resolutionHz")).toDouble(), 2.5);
    QCOMPARE(d.value(QStringLiteral("zeroPadding")).toInt(), 4096);
    QCOMPARE(d.value(QStringLiteral("peakTopN")).toInt(), 50);
    QCOMPARE(d.value(QStringLiteral("peakMinSpacingHz")).toDouble(), 12.5);
    QCOMPARE(d.value(QStringLiteral("unit")).toString(), QStringLiteral("Pa"));

    // 默认值口径与 defaults() 一致
    const QVariantMap def = DA::DASignalAnalysisParams::defaults().toPythonDict();
    QCOMPARE(def.value(QStringLiteral("window")).toString(), QStringLiteral("hann"));
    QCOMPARE(def.value(QStringLiteral("resolutionHz")).toDouble(), 10.0);
    QCOMPARE(def.value(QStringLiteral("stftWindowMs")).toDouble(), 100.0);
    QCOMPARE(def.value(QStringLiteral("peakTopN")).toInt(), 10);
    QCOMPARE(def.value(QStringLiteral("peakMinSpacingHz")).toDouble(), -1.0);
    QVERIFY(def.value(QStringLiteral("unit")).toString().isEmpty());
}

/**
 * @brief D5：内存估算公式契约
 *
 * - 频谱/热图在 D1 取数架构后为流式，峰值内存只与段长/栅格相关，不随 sampleCount 增长；
 * - 峰值表/峰值曲线仍整段取数，按 float32 × 并发副本线性增长；
 * - 包络只按传入的区段长度估算；
 * - 段长（分辨率越细、窗长越长）增大时估算单调上升。
 */
void DASignalAnalysisTest::memoryEstimateContract()
{
    DA::DASignalMemoryInputs in;
    in.sampleRate         = 48000.0;
    in.resolutionHz       = 10.0;
    in.stftWindowMs       = 100.0;
    in.stftOverlapPercent = 50.0;
    in.peakTopN           = 10;
    in.gridColumns        = 400;
    in.gridRows           = 200;
    in.sampleCount        = qint64(3600) * 48000;  // 1 小时 / 48 kHz

    const qint64 spec1h = DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::Spectrum, in);
    const qint64 heat1h = DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::Heatmap, in);
    in.sampleCount      = in.sampleCount * 10;  // 10 小时
    const qint64 spec10h = DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::Spectrum, in);
    const qint64 heat10h = DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::Heatmap, in);
    QCOMPARE(spec1h, spec10h);  // 流式：与信号总长无关
    QCOMPARE(heat1h, heat10h);

    // 峰值表/峰值曲线：含整段 float32 副本，随采样点数线性增长
    in.sampleCount = qint64(3600) * 48000;
    const qint64 peak1h  = DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::PeakTable, in);
    const qint64 curve1h = DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::PeakCurves, in);
    in.sampleCount       = in.sampleCount * 2;
    QVERIFY(DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::PeakTable, in) > peak1h);
    QVERIFY(DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::PeakCurves, in) > curve1h);

    // 并发副本系数：2 份 > 1 份（频谱+峰值+曲线同批提交的保守放大）
    DA::DASignalMemoryInputs one = in;
    one.concurrentCopies         = 1;
    QVERIFY(DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::PeakTable, in)
            > DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::PeakTable, one));

    // 段长越大（分辨率更细 / STFT 窗更长）→ 估算单调上升
    DA::DASignalMemoryInputs fine = in;
    fine.resolutionHz             = 1.0;  // Δf 更细 → nperseg 更大
    QVERIFY(DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::Spectrum, fine) > spec1h);
    DA::DASignalMemoryInputs longWin = in;
    longWin.stftWindowMs             = 1000.0;
    QVERIFY(DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::Heatmap, longWin) > heat1h);

    // 包络只按区段估算：短区段远小于整段
    DA::DASignalMemoryInputs env     = in;
    env.sampleCount                  = qint64(3600) * 48000;
    env.rangeSamples                 = 48000;  // 1 秒
    env.gridColumns                  = 2000;
    const qint64 envShort            = DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::Envelope, env);
    env.rangeSamples                 = env.sampleCount;
    QVERIFY(DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::Envelope, env) > envShort * 10);

    // 流式频谱 1 h/48 kHz @10 Hz 的估算应远低于 1.5 GB 预算（与实际实测 187 MB 同量级）
    DA::DASignalMemoryInputs real = in;
    real.sampleCount              = qint64(3600) * 48000;
    real.resolutionHz             = 10.0;
    QVERIFY(DA::dasignalEstimateAnalysisBytes(DA::DASignalAnalysisStage::Spectrum, real)
            < DA::kDASignalAnalysisBudgetBytes / 4);
}

/**
 * @brief 峰值曲线渲染采样：跳变阈值 = 8 × 频率分辨率，分辨率未知时返回 0（不断开）
 */
void DASignalAnalysisTest::peakCurveJumpThreshold()
{
    DA::DAPeakCurveResult curves;
    curves.topN         = 1;
    curves.nFrames      = 1;
    curves.times        = QVector< double >{ 0.0 };
    curves.freqByRank   = QVector< double >{ 100.0 };
    curves.nperseg      = 5000;
    curves.sampleRateHz = 50000.0;  // Δf = 10 Hz → 阈值 80 Hz
    QCOMPARE(DA::daPeakCurveJumpThresholdHz(curves), 80.0);

    // 采样率未知（旧数据/未回填）：返回 0，保持「整条折线」的旧行为
    curves.sampleRateHz = 0.0;
    QCOMPARE(DA::daPeakCurveJumpThresholdHz(curves), 0.0);

    // 段长非法：同样返回 0
    curves.sampleRateHz = 50000.0;
    curves.nperseg      = 0;
    QCOMPARE(DA::daPeakCurveJumpThresholdHz(curves), 0.0);
}

/**
 * @brief 峰值曲线渲染采样：连续段才连线，跳变/缺峰处插入 NaN 断开
 *
 * 场景：100 → 150（连续）→ 900（跳变）→ 缺帧 → 910 → 920（连续），阈值 80 Hz。
 * 点保留全部 5 个有效帧；折线被跳变与缺帧断成 3 段（2 个 NaN 分隔）。
 */
void DASignalAnalysisTest::peakCurvePlotSamples()
{
    DA::DAPeakCurveResult curves;
    curves.topN         = 1;
    curves.nFrames      = 6;
    curves.times        = QVector< double >{ 0.0, 1.0, 2.0, 3.0, 4.0, 5.0 };
    curves.freqByRank   = QVector< double >{ 100.0, 150.0, 900.0, std::nan(""), 910.0, 920.0 };
    curves.nperseg      = 5000;
    curves.sampleRateHz = 50000.0;

    const double threshold = DA::daPeakCurveJumpThresholdHz(curves);
    QCOMPARE(threshold, 80.0);
    const DA::DAPeakCurvePlotSamples samples = DA::daPeakCurvePlotSamples(curves, 0, threshold);

    // 点：缺峰帧不画（其余 5 帧都在）
    QCOMPARE(samples.dotXs.size(), 5);
    QCOMPARE(samples.dotYs.size(), 5);
    QCOMPARE(samples.dotXs.at(0), 0.0);
    QCOMPARE(samples.dotYs.at(0), 100.0);
    QCOMPARE(samples.dotXs.at(3), 4.0);  // 缺帧之后的第一点
    QCOMPARE(samples.dotYs.at(4), 920.0);

    // 折线：100→150 连续；跳变/缺帧处各插入一个 NaN
    QCOMPARE(samples.lineXs.size(), 7);
    QCOMPARE(samples.lineYs.size(), 7);
    QCOMPARE(samples.lineYs.at(0), 100.0);
    QCOMPARE(samples.lineYs.at(1), 150.0);
    QVERIFY(std::isnan(samples.lineYs.at(2)));  // 150 → 900 跳变断开
    QCOMPARE(samples.lineYs.at(3), 900.0);
    QVERIFY(std::isnan(samples.lineYs.at(4)));  // 缺帧断开
    QCOMPARE(samples.lineYs.at(5), 910.0);
    QCOMPARE(samples.lineYs.at(6), 920.0);

    // 时间原点平移到 100 s：点与折线的 x 同步平移
    const DA::DAPeakCurvePlotSamples shifted = DA::daPeakCurvePlotSamples(curves, 0, threshold, 100.0);
    QCOMPARE(shifted.dotXs.at(0), 100.0);
    QCOMPARE(shifted.lineXs.at(0), 100.0);

    // 阈值 <= 0：不做断开（旧行为），只在缺帧处断开
    const DA::DAPeakCurvePlotSamples noBreak = DA::daPeakCurvePlotSamples(curves, 0, 0.0);
    QCOMPARE(noBreak.lineYs.size(), 6);  // 5 个有效点 + 缺帧处 1 个 NaN
    QCOMPARE(noBreak.lineYs.at(2), 900.0);
    QVERIFY(std::isnan(noBreak.lineYs.at(3)));

    // rank 越界 / 空结果：返回空采样
    QVERIFY(DA::daPeakCurvePlotSamples(curves, 1, threshold).dotXs.isEmpty());
    QVERIFY(DA::daPeakCurvePlotSamples(DA::DAPeakCurveResult(), 0, threshold).lineXs.isEmpty());
}

/**
 * @brief 峰值标记吸附：标记落在当前已绘制频谱曲线的最近局部极大上
 *
 * 峰值表频率与频谱曲线按离散频点绘制的位置可能差一两个 bin，直接按峰值频率放标记会
 * 落在曲线旁边（用户报告的「标记与真实峰值偏离」）。snapPeakMarkersToCurve 的口径：
 * - 峰频正好是曲线局部极大样本：保持不动；
 * - 峰频偏一个 bin / 落在上升沿 / 落在谷底：吸附到邻域内曲线数值最大的样本；
 * - 峰频超出曲线范围 / NaN / 空曲线 / 长度不一致 / 频率非严格递增：跳过（输出可少于输入）。
 *
 * 合成曲线：峰在 20 Hz（5.0）与 60 Hz（7.0），其余为 0；邻域半径 2 个样本。
 */
void DASignalAnalysisTest::peakMarkerSnapping()
{
    QVector< double > freqs;
    for (int i = 0; i <= 8; ++i) {
        freqs.append(10.0 * i);
    }
    const QVector< double > values{ 0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 7.0, 0.0, 0.0 };

    // 正好命中峰顶：保持 (20, 5.0)
    QVector< QPointF > snapped = DA::snapPeakMarkersToCurve(freqs, values, QVector< double >{ 20.0 });
    QCOMPARE(snapped.size(), 1);
    QCOMPARE(snapped.first().x(), 20.0);
    QCOMPARE(snapped.first().y(), 5.0);

    // 偏一个 bin（19 Hz 最近样本为 20 Hz）：吸附到曲线峰顶
    snapped = DA::snapPeakMarkersToCurve(freqs, values, QVector< double >{ 19.0 });
    QCOMPARE(snapped.size(), 1);
    QCOMPARE(snapped.first().x(), 20.0);
    QCOMPARE(snapped.first().y(), 5.0);

    // 落在上升沿（5 Hz 最近样本为 0 Hz）：吸附到 20 Hz 峰顶，而不是留在原频率
    snapped = DA::snapPeakMarkersToCurve(freqs, values, QVector< double >{ 5.0 });
    QCOMPARE(snapped.size(), 1);
    QCOMPARE(snapped.first().x(), 20.0);

    // 正好落在谷底样本（30 Hz 不是局部极大）：吸附到邻域内最高的 20 Hz 峰顶
    snapped = DA::snapPeakMarkersToCurve(freqs, values, QVector< double >{ 30.0 });
    QCOMPARE(snapped.size(), 1);
    QCOMPARE(snapped.first().x(), 20.0);

    // 超出曲线范围 / NaN：跳过；输出保持输入中有效峰的顺序
    snapped = DA::snapPeakMarkersToCurve(freqs, values, QVector< double >{ -5.0, 90.0, 20.0, std::nan(""), 60.0 });
    QCOMPARE(snapped.size(), 2);
    QCOMPARE(snapped.at(0).x(), 20.0);
    QCOMPARE(snapped.at(1).x(), 60.0);
    QCOMPARE(snapped.at(1).y(), 7.0);

    // 空输入 / 长度不一致 / 频率非严格递增：安全返回空
    QVERIFY(DA::snapPeakMarkersToCurve(QVector< double >(), QVector< double >(), QVector< double >{ 10.0 }).isEmpty());
    QVERIFY(DA::snapPeakMarkersToCurve(freqs, QVector< double >{ 1.0 }, QVector< double >{ 10.0 }).isEmpty());
    QVERIFY(DA::snapPeakMarkersToCurve(QVector< double >{ 0.0, 20.0, 10.0 },
                                       QVector< double >{ 0.0, 1.0, 2.0 },
                                       QVector< double >{ 10.0 })
                .isEmpty());
    QVERIFY(DA::snapPeakMarkersToCurve(freqs, values, QVector< double >()).isEmpty());
}

/**
 * @brief 峰值标记文本模板：占位符替换、紧凑数字、空模板与未知占位符
 *
 * 标记文本是纯显示层字符串（与峰值数据/吸附无关），这里锁定渲染口径：
 * 默认模板渲染成「1-32.5 Hz」这类短文本，dB 模式幅值自动带单位，空模板表示不显示文本。
 */
void DASignalAnalysisTest::peakMarkerLabelText()
{
    // 默认模板：序号-频率（用户示例：1-32.5 Hz）
    QCOMPARE(DA::daDefaultPeakMarkerText(), QStringLiteral("{index}-{fre} Hz"));
    QCOMPARE(DA::daFormatPeakMarkerText(DA::daDefaultPeakMarkerText(), 1, 32.5, 0.85, false),
             QStringLiteral("1-32.5 Hz"));

    // 三个占位符都能替换；按名次/频率/幅值顺序无关
    QCOMPARE(DA::daFormatPeakMarkerText(QStringLiteral("{index} {fre}Hz {amp}"), 2, 100.0, 0.5, false),
             QStringLiteral("2 100Hz 0.5"));
    // dB 模式幅值带单位，保留两位小数
    QCOMPARE(DA::daFormatPeakMarkerText(QStringLiteral("{index}-{amp}"), 3, 10.0, -12.345, true),
             QStringLiteral("3--12.35 dB"));
    // 空模板：不显示文本；未知占位符原样保留（用户可写单位/备注）
    QVERIFY(DA::daFormatPeakMarkerText(QString(), 1, 10.0, 1.0, false).isEmpty());
    QCOMPARE(DA::daFormatPeakMarkerText(QStringLiteral("{peak}"), 1, 10.0, 1.0, false),
             QStringLiteral("{peak}"));

    // 紧凑数字：尾零去掉、极端值用科学计数法
    QCOMPARE(DA::daFormatCompactNumber(32.50, 2), QStringLiteral("32.5"));
    QCOMPARE(DA::daFormatCompactNumber(100.0, 2), QStringLiteral("100"));
    QCOMPARE(DA::daFormatCompactNumber(0.0, 2), QStringLiteral("0"));
    QCOMPARE(DA::daFormatCompactNumber(1.0e-9, 2), QStringLiteral("1e-09"));
    QCOMPARE(DA::daFormatCompactNumber(2.5e7, 2), QStringLiteral("2.5e+07"));
}

/**
 * @brief 峰值标记显示对话框：模板回显、实时预览（含 dB 口径）
 */
void DASignalAnalysisTest::peakMarkerDisplayDialog()
{
    DA::DAPeakMarkerDisplayDialog dlg(QStringLiteral("{index}-{fre} Hz"), false);
    auto* edit    = dlg.findChild< QLineEdit* >(QStringLiteral("da-peak-marker-edit"));
    auto* preview = dlg.findChild< QLabel* >(QStringLiteral("da-peak-marker-preview"));
    QVERIFY(edit != nullptr);
    QVERIFY(preview != nullptr);
    QCOMPARE(edit->text(), QStringLiteral("{index}-{fre} Hz"));
    QCOMPARE(dlg.peakMarkerText(), QStringLiteral("{index}-{fre} Hz"));
    // 预览用示例值（序号 1 / 频率 32.5 赫兹）渲染，用户示例即「1-32.5 Hz」
    QVERIFY2(preview->text().contains(QStringLiteral("1-32.5 Hz")), qPrintable(preview->text()));
    // 编辑框改动实时反映到预览与结果
    edit->setText(QStringLiteral("{amp}"));
    QVERIFY2(preview->text().contains(QStringLiteral("0.85")), qPrintable(preview->text()));
    QCOMPARE(dlg.peakMarkerText(), QStringLiteral("{amp}"));
    // 空模板：预览不再显示幅值示例（表示不显示文本）
    edit->setText(QString());
    QVERIFY(!preview->text().contains(QStringLiteral("0.85")));
    QCOMPARE(dlg.peakMarkerText(), QString());

    // dB 口径：预览幅值带单位
    DA::DAPeakMarkerDisplayDialog dbDlg(QStringLiteral("{index} {amp}"), true);
    auto* dbPreview = dbDlg.findChild< QLabel* >(QStringLiteral("da-peak-marker-preview"));
    QVERIFY(dbPreview != nullptr);
    QVERIFY2(dbPreview->text().contains(QStringLiteral("1 -12.3 dB")), qPrintable(dbPreview->text()));
}

/**
 * @brief 状态栏进度条取消按钮：绑定动作、随进度条显隐、固定小尺寸（不改变状态栏高度）
 *
 * 宿主状态栏的进度条是全局的：信号分析把「停止计算」动作绑到它右侧，按钮必须与
 * 进度条同显隐、点击能触发动作，且按钮/进度条尺寸固定，不能让状态栏因显隐而变高。
 */
void DASignalAnalysisTest::statusBarProgressCancelButton()
{
    QAction stopAction(QStringLiteral("stop"));
    stopAction.setObjectName(QStringLiteral("da-test-stop-action"));
    // 放进真实的 QStatusBar：断言的是宿主状态栏整体高度不被进度控件改变
    QStatusBar host;
    host.resize(800, 24);
    host.show();  // isProgressBarVisible() 用 isVisible()，父窗口必须处于显示状态
    auto* bar = new DA::DAStatusBarWidget(&host);
    host.addWidget(bar, 1);
    QCoreApplication::processEvents();
    bar->setProgressCancelAction(&stopAction);
    QCOMPARE(bar->progressCancelAction(), &stopAction);

    auto* button = bar->findChild< QToolButton* >(QStringLiteral("da-statusbar-progress-cancel"));
    QVERIFY2(button != nullptr, "the cancel button must be created for the bound action");
    QCOMPARE(button->size(), QSize(18, 18));
    QVERIFY2(button->isHidden(), "the cancel button must stay hidden until the progress bar shows");

    const int idleHeight = host.sizeHint().height();
    // 别的调用方显示进度条：不得带出本插件绑定的停止按钮
    bar->showProgressBar();
    bar->setProgressText(QStringLiteral("Computing..."));
    bar->setProgress(42);
    QVERIFY2(button->isHidden(), "a foreign progress bar must not reveal this cancel button");
    QVERIFY(bar->isProgressBarVisible());
    QCOMPARE(host.sizeHint().height(), idleHeight);
    bar->hideProgressBar();
    QVERIFY(button->isHidden());

    // 本插件请求显示进度：进度条 + 文本 + 停止按钮同时出现，状态栏高度不变
    bar->setProgressCancelVisible(true);
    bar->showProgressBar();
    bar->setProgressText(QStringLiteral("Computing..."));
    bar->setProgress(42);
    QVERIFY(!button->isHidden());
    QVERIFY(bar->isProgressBarVisible());
    QCOMPARE(host.sizeHint().height(), idleHeight);

    QSignalSpy triggered(&stopAction, &QAction::triggered);
    button->click();
    QCOMPARE(triggered.count(), 1);

    bar->setProgressCancelVisible(false);
    bar->hideProgressBar();
    QVERIFY(button->isHidden());
    QVERIFY(!bar->isProgressBarVisible());
    QCOMPARE(host.sizeHint().height(), idleHeight);

    // 解绑：动作归还调用方，按钮销毁（deleteLater 排进事件循环）
    bar->setProgressCancelAction(nullptr);
    QCOMPARE(bar->progressCancelAction(), nullptr);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(bar->findChild< QToolButton* >(QStringLiteral("da-statusbar-progress-cancel")) == nullptr);
}

/**
 * @brief T7：信号视图使用 DAFigureWidget 2×2 网格，子图网格坐标正确
 */
void DASignalAnalysisTest::signalViewFigureGrid()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the signal view grid test");
    }
    DA::DASignalAnalysisView view(makeTestSignal());
    DA::DAFigureWidget* fig = view.figure();
    QVERIFY(fig);
    QVERIFY(fig->isGridLayoutEnabled());
    QwtFigureGrid* grid = fig->grid();
    QVERIFY(grid);
    QCOMPARE(grid->rowCount(), 2);
    QCOMPARE(grid->columnCount(), 2);

    auto checkCell = [grid](QWidget* w, int expectRow, int expectCol, int expectRowSpan, int expectColSpan) {
        QVERIFY(w);
        int row = -1, col = -1, rowSpan = 0, colSpan = 0;
        const bool ok = grid->gridPosition(w, &row, &col, &rowSpan, &colSpan);
        QVERIFY(ok);
        QCOMPARE(row, expectRow);
        QCOMPARE(col, expectCol);
        QCOMPARE(rowSpan, expectRowSpan);
        QCOMPARE(colSpan, expectColSpan);
    };
    // 波形跨两列，频谱/热图各占一格；热图与三维瀑布同格（显隐切换）
    checkCell(view.waveChart(), 0, 0, 1, 2);
    checkCell(view.spectrumChart(), 1, 0, 1, 1);
    checkCell(view.heatmapChart(), 1, 1, 1, 1);
    checkCell(view.waterfall3DChart(), 1, 1, 1, 1);

    // figure id 由信号唯一 id 派生（BLK-02：同名信号不再共用同一 figure id）
    QVERIFY(view.signal()->signalId() > 0);
    QCOMPARE(view.figureId(), QStringLiteral("sig-%1").arg(view.signal()->signalId()));
    // 当前 2D 子图可显式设置
    view.setCurrentChart(view.waveChart());
    QCOMPARE(view.currentChart(), view.waveChart());
    QVERIFY(!view.is3DChartActive());
}

/**
 * @brief T7：分析结果图元 transient（序列化跳过），峰值标记保持非 transient
 */
void DASignalAnalysisTest::signalViewTransientItems()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the signal view item test");
    }
    DA::DASignalAnalysisView view(makeTestSignal());

    QVector< double > xs { 0.0, 1.0, 2.0 };
    QVector< double > mins { -1.0, -2.0, -1.5 };
    QVector< double > maxs { 1.0, 2.0, 1.5 };
    view.setEnvelope(xs, mins, maxs);

    DA::DASpectrumResult spectrum;
    spectrum.freqs       = { 0.0, 10.0, 20.0 };
    spectrum.amplitudes  = { 1.0, 2.0, 3.0 };
    spectrum.resolutionHz = 10.0;
    view.setSpectrum(spectrum);

    DA::DAStftResult stft;
    stft.rows          = 2;
    stft.columns       = 3;
    stft.times         = { 0.0, 0.5, 1.0 };
    stft.freqs         = { 0.0, 10.0 };
    stft.values        = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    stft.windowSeconds = 0.1;
    view.setStft(stft);

    DA::DAPeakResult peaks;
    peaks.frequencies = { 10.0 };
    peaks.magnitudes  = { 2.0 };
    view.setPeaks(peaks);

    // 波形：包络曲线 / 播放头 / STFT 窗口矩形均为 transient
    QwtPlotIntervalCurve* envelope = nullptr;
    QwtPlotMarker* playhead        = nullptr;
    QwtPlotShapeItem* stftWindow   = nullptr;
    const QList< QwtPlotItem* > waveItems = view.waveChart()->itemList();
    for (QwtPlotItem* item : waveItems) {
        if (auto* c = dynamic_cast< QwtPlotIntervalCurve* >(item)) {
            envelope = c;
            QVERIFY(view.waveChart()->isItemTransient(item));
        } else if (auto* m = dynamic_cast< QwtPlotMarker* >(item)) {
            playhead = m;
            QVERIFY(view.waveChart()->isItemTransient(item));
        } else if (auto* s = dynamic_cast< QwtPlotShapeItem* >(item)) {
            stftWindow = s;
            QVERIFY(view.waveChart()->isItemTransient(item));
        }
    }
    QVERIFY(envelope);
    QVERIFY(playhead);
    QVERIFY(stftWindow);

    // 频谱：曲线 transient；峰值标记非 transient（T9 写入 chart-data）
    int spectrumCurves = 0;
    int peakMarkers    = 0;
    const QList< QwtPlotItem* > spectrumItems = view.spectrumChart()->itemList();
    for (QwtPlotItem* item : spectrumItems) {
        if (dynamic_cast< QwtPlotCurve* >(item) || dynamic_cast< QwtPlotIntervalCurve* >(item)) {
            ++spectrumCurves;
            QVERIFY(view.spectrumChart()->isItemTransient(item));
        } else if (dynamic_cast< QwtPlotMarker* >(item)) {
            ++peakMarkers;
            QVERIFY(!view.spectrumChart()->isItemTransient(item));
        }
    }
    QCOMPARE(spectrumCurves, 1);  // 未开启峰值标记时只有幅值谱曲线
    QCOMPARE(peakMarkers, 0);

    view.setPeakMarkersVisible(true);
    peakMarkers = 0;
    const QList< QwtPlotItem* > spectrumItemsWithPeaks = view.spectrumChart()->itemList();
    for (QwtPlotItem* item : spectrumItemsWithPeaks) {
        if (dynamic_cast< QwtPlotMarker* >(item)) {
            ++peakMarkers;
            QVERIFY(!view.spectrumChart()->isItemTransient(item));
        }
    }
    QCOMPARE(peakMarkers, 1);

    // 峰值标记形状与文本：倒三角（Path + pinPoint，尖端对准峰值点）+ 默认模板「1-… Hz」
    QCOMPARE(view.peakMarkers().size(), 1);
    QwtPlotMarker* peakMarker = view.peakMarkers().first();
    QVERIFY(peakMarker != nullptr);
    const QwtSymbol* peakSymbol = peakMarker->symbol();
    QVERIFY(peakSymbol != nullptr);
    QCOMPARE(peakSymbol->style(), QwtSymbol::Path);
    QVERIFY2(peakSymbol->isPinPointEnabled(), "peak marker symbol must anchor its tip with pinPoint");
    QVERIFY2(peakSymbol->path().elementCount() >= 3, "peak marker symbol path is not a triangle");
    QVERIFY2(!peakMarker->label().isEmpty(), "peak marker label must show the default template text");

    // 模板可配：空模板隐藏文本，自定义模板按占位符渲染
    // 注意标记 x 是吸附到已绘制曲线后的频率（本例峰值表给 10 Hz，曲线局部极大在 20 Hz），
    // 文本里的频率必须与标记实际位置一致，不能按峰值表频率断言
    const QString markerFrequencyText = DA::daFormatCompactNumber(peakMarker->xValue(), 2);
    view.setPeakMarkerText(QStringLiteral("{index}@{fre}Hz"));
    QCOMPARE(view.peakMarkers().first()->label().text(),
             QStringLiteral("1@%1Hz").arg(markerFrequencyText));
    view.setPeakMarkerText(QString());
    QVERIFY(view.peakMarkers().first()->label().isEmpty());
    view.setPeakMarkerText(DA::daDefaultPeakMarkerText());

    // 热图：时频栅格 transient
    bool hasSpectrogram = false;
    const QList< QwtPlotItem* > heatmapItems = view.heatmapChart()->itemList();
    for (QwtPlotItem* item : heatmapItems) {
        if (dynamic_cast< QwtPlotSpectrogram* >(item)) {
            hasSpectrogram = true;
            QVERIFY(view.heatmapChart()->isItemTransient(item));
        }
    }
    QVERIFY(hasSpectrogram);
}

/**
 * @brief T7：瀑布模式切换热图/三维瀑布显隐与当前子图判定
 */
void DASignalAnalysisTest::signalViewWaterfallMode()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the signal view waterfall test");
    }
    DA::DASignalAnalysisView view(makeTestSignal());
    DA::DAChartWidget* heatmap = view.heatmapChart();
    DA::DAChart3DWidget* waterfall = view.waterfall3DChart();
    QVERIFY(heatmap);
    QVERIFY(waterfall);

    // 默认二维
    view.setWaterfallMode(0);
    QVERIFY(heatmap->isVisibleTo(&view));
    QVERIFY(!waterfall->isVisibleTo(&view));
    QVERIFY(!view.is3DChartActive());

    // 三维：热图隐藏、瀑布可见、当前子图为 3D
    view.setWaterfallMode(1);
    QVERIFY(!heatmap->isVisibleTo(&view));
    QVERIFY(waterfall->isVisibleTo(&view));
    QVERIFY(view.is3DChartActive());
    QCOMPARE(view.current3DChart(), waterfall);

    // 隐藏：两者都不可见
    view.setWaterfallMode(2);
    QVERIFY(!heatmap->isVisibleTo(&view));
    QVERIFY(!waterfall->isVisibleTo(&view));
    QVERIFY(!view.is3DChartActive());
}

/**
 * @brief T8：点击 2D 子图画布即切换当前子图（设置面板 target 跟随）
 *
 * qwt 只在事件传播到 QwtPlot 时才切换当前坐标轴；波形画布上的 QwtPicker 会消费点击，
 * 因此视图在画布层显式同步。这里直接向画布发鼠标事件验证三条路径。
 */
void DASignalAnalysisTest::signalViewClickSwitchesCurrentChart()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the signal view click test");
    }
    DA::DASignalAnalysisView view(makeTestSignal());
    auto clickCanvas = [](QWidget* canvas) {
        QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    };

    view.setCurrentChart(view.heatmapChart());
    QCOMPARE(view.currentChart(), view.heatmapChart());
    clickCanvas(view.waveChart()->canvas());
    QCOMPARE(view.currentChart(), view.waveChart());
    clickCanvas(view.spectrumChart()->canvas());
    QCOMPARE(view.currentChart(), view.spectrumChart());
    clickCanvas(view.heatmapChart()->canvas());
    QCOMPARE(view.currentChart(), view.heatmapChart());
    QVERIFY(!view.is3DChartActive());
}

/**
 * @brief T9：figure XML 保存——角色/峰值/网格比例/非 transient 图元集合
 *
 * 校验：
 * - 根节点 signal-figures/version、figure 的 signal-name；
 * - <grid rows/cols/rowSizes/colSizes>；
 * - chart 的 role（wave/spectrum/heatmap）与 chart3d 的 role=waterfall；
 * - <items> 只含非 transient 图元（包络/频谱曲线/热图不出现），峰值标记 item 打 peak="1"；
 * - 图元管理者记录了所有非 transient 图元的 key。
 */
void DASignalAnalysisTest::signalFigureSaveXml()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the signal figure archive test");
    }
    DA::DASignalAnalysisView view(makeTestSignal());
    // 分析结果图元（transient）：包络 + 频谱曲线 + 热图
    view.setEnvelope({ 0.0, 1.0, 2.0 }, { -1.0, -2.0, -1.5 }, { 1.0, 2.0, 1.5 });
    DA::DASpectrumResult spectrum;
    spectrum.freqs        = { 0.0, 10.0, 20.0 };
    spectrum.amplitudes   = { 1.0, 2.0, 3.0 };
    spectrum.resolutionHz = 10.0;
    view.setSpectrum(spectrum);
    DA::DAStftResult stft;
    stft.rows          = 2;
    stft.columns       = 3;
    stft.times         = { 0.0, 0.5, 1.0 };
    stft.freqs         = { 0.0, 10.0 };
    stft.values        = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    stft.windowSeconds = 0.1;
    view.setStft(stft);
    // 峰值标记（非 transient，可见）
    DA::DAPeakResult peaks;
    peaks.frequencies = { 10.0 };
    peaks.magnitudes  = { 2.0 };
    view.setPeaks(peaks);
    view.setPeakMarkersVisible(true);
    QCOMPARE(view.peakMarkers().size(), 1);

    // 用户图元：波形 chart 上加一个非 transient 标记
    auto* userMarker = new QwtPlotMarker();
    userMarker->setXValue(1.0);
    userMarker->setYValue(0.5);
    userMarker->attach(view.waveChart());

    // 改网格比例
    const QList< qreal > gridRows { 0.62, 0.38 };
    const QList< qreal > gridCols { 0.45, 0.55 };
    view.setGridSizes(gridRows, gridCols);

    DA::DAChartItemsManager itemsMgr;
    const QByteArray xml = DA::DASignalFigureArchive::saveFigures({ &view }, &itemsMgr);
    QVERIFY(!xml.isEmpty());

    QDomDocument doc;
    QString parseError;
    QVERIFY2(doc.setContent(xml, &parseError), qPrintable(parseError));
    const QDomElement root = doc.documentElement();
    QCOMPARE(root.tagName(), QStringLiteral("signal-figures"));
    QCOMPARE(root.attribute(QStringLiteral("version")), QStringLiteral("1"));

    const QDomElement figureEle = root.firstChildElement(QStringLiteral("figure"));
    QVERIFY(!figureEle.isNull());
    QCOMPARE(figureEle.attribute(QStringLiteral("signal-name")), QStringLiteral("unit"));
    // BLK-02：figure id 由信号唯一 id 派生；signal-id 是恢复端优先匹配的存档键
    QCOMPARE(figureEle.attribute(QStringLiteral("id")), QStringLiteral("sig-%1").arg(view.signal()->signalId()));
    QCOMPARE(figureEle.attribute(QStringLiteral("signal-id")), QString::number(view.signal()->signalId()));

    // <grid>：2×2 且行列比例与设置一致
    const QDomElement gridEle = figureEle.firstChildElement(QStringLiteral("grid"));
    QVERIFY(!gridEle.isNull());
    QCOMPARE(gridEle.attribute(QStringLiteral("rows")).toInt(), 2);
    QCOMPARE(gridEle.attribute(QStringLiteral("cols")).toInt(), 2);
    QStringList rowParts = gridEle.attribute(QStringLiteral("rowSizes")).split(QLatin1Char(','));
    QStringList colParts = gridEle.attribute(QStringLiteral("colSizes")).split(QLatin1Char(','));
    QCOMPARE(rowParts.size(), 2);
    QCOMPARE(colParts.size(), 2);
    QVERIFY(qAbs(rowParts.at(0).toDouble() - gridRows.at(0)) < 1e-6);
    QVERIFY(qAbs(colParts.at(1).toDouble() - gridCols.at(1)) < 1e-6);

    // <charts>：每个子图都有 role；<items> 只含非 transient 图元
    const QDomElement chartsEle = figureEle.firstChildElement(QStringLiteral("charts"));
    QVERIFY(!chartsEle.isNull());
    QStringList roles;
    int chartCount = 0;
    for (QDomElement chartEle = chartsEle.firstChildElement(QStringLiteral("chart")); !chartEle.isNull();
         chartEle             = chartEle.nextSiblingElement(QStringLiteral("chart"))) {
        ++chartCount;
        const QString role = chartEle.attribute(QStringLiteral("role"));
        if (!role.isEmpty()) {
            roles.append(role);
        }
        const QDomElement itemsEle = chartEle.firstChildElement(QStringLiteral("items"));
        QVERIFY(!itemsEle.isNull());
        if (role == QLatin1String("wave")) {
            // 波形：只有用户图元（包络曲线/播放头/STFT 矩形都是 transient）
            QCOMPARE(itemsEle.childNodes().size(), 1);
            QCOMPARE(itemsEle.firstChildElement(QStringLiteral("item")).attribute(QStringLiteral("peak")),
                     QString());
        } else if (role == QLatin1String("spectrum")) {
            // 频谱：只有峰值标记（幅值谱曲线是 transient），且打 peak="1"
            QCOMPARE(itemsEle.childNodes().size(), 1);
            QCOMPARE(itemsEle.firstChildElement(QStringLiteral("item")).attribute(QStringLiteral("peak")),
                     QStringLiteral("1"));
        } else if (role == QLatin1String("heatmap")) {
            // 热图：时频栅格是 transient，不写任何 item
            QCOMPARE(itemsEle.childNodes().size(), 0);
        }
    }
    QCOMPARE(chartCount, 3);
    QStringList expectedRoles;
    expectedRoles << QStringLiteral("heatmap") << QStringLiteral("spectrum") << QStringLiteral("wave");
    roles.sort();
    QCOMPARE(roles, expectedRoles);

    // chart3d：瀑布角色
    const QDomElement chart3dEle = figureEle.firstChildElement(QStringLiteral("chart3d"));
    QVERIFY(!chart3dEle.isNull());
    QCOMPARE(chart3dEle.attribute(QStringLiteral("role")), QStringLiteral("waterfall"));

    // 图元管理者：峰值标记 + 用户图元
    QCOMPARE(itemsMgr.keys().size(), 2);

    // 清理手动添加的用户图元（图元管理者不持有所有权）
    userMarker->detach();
    delete userMarker;
}

/**
 * @brief T9：存档任务往返——signals.json + figures.xml + chart-data 与 figure 恢复
 *
 * 校验：三个条目的读写、字节一致、图元管理者有图元；新视图恢复网格比例、峰值标记
 * （视图接管且再次回填频谱不丢）、用户图元、figure id；transient 图元不从存档恢复。
 */
void DASignalAnalysisTest::signalArchiveRoundTrip()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the signal archive round trip test");
    }
    // ---------- 1. 源视图：包络/频谱/热图/峰值(可见)/用户图元，改网格比例 ----------
    DA::DASignalAnalysisView sourceView(makeTestSignal());
    sourceView.setEnvelope({ 0.0, 1.0, 2.0 }, { -1.0, -2.0, -1.5 }, { 1.0, 2.0, 1.5 });
    DA::DASpectrumResult spectrum;
    spectrum.freqs        = { 0.0, 10.0, 20.0 };
    spectrum.amplitudes   = { 1.0, 2.0, 3.0 };
    spectrum.resolutionHz = 10.0;
    sourceView.setSpectrum(spectrum);
    DA::DAStftResult stft;
    stft.rows          = 2;
    stft.columns       = 3;
    stft.times         = { 0.0, 0.5, 1.0 };
    stft.freqs         = { 0.0, 10.0 };
    stft.values        = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    stft.windowSeconds = 0.1;
    sourceView.setStft(stft);
    DA::DAPeakResult peaks;
    peaks.frequencies = { 10.0 };
    peaks.magnitudes  = { 2.0 };
    sourceView.setPeaks(peaks);
    sourceView.setPeakMarkersVisible(true);
    QCOMPARE(sourceView.peakMarkers().size(), 1);

    auto* userMarker = new QwtPlotMarker();
    userMarker->setXValue(1.0);
    userMarker->setYValue(0.5);
    userMarker->attach(sourceView.waveChart());

    const QList< qreal > gridRows { 0.35, 0.65 };
    const QList< qreal > gridCols { 0.7, 0.3 };
    sourceView.setGridSizes(gridRows, gridCols);
    sourceView.figure()->setFigureId(QStringLiteral("sig-archived"));

    // ---------- 2. 保存：figure XML + chart-data 经存档任务写内存 archive ----------
    // BLK-01：GUI 线程先把图元序列化成字节快照，后台任务只写字节（与活指针解耦）
    const QByteArray signalsJson = QByteArrayLiteral("{\"signals\":[]}");
    DA::DAChartItemsManager saveItems;
    const QByteArray figuresXml = DA::DASignalFigureArchive::saveFigures({ &sourceView }, &saveItems);
    QVERIFY(!figuresXml.isEmpty());
    QVERIFY(!saveItems.isEmpty());
    const QList< DA::DAChartItemSnapshot > snapshots = DA::DAChartItemArchive::makeSnapshots(&saveItems);
    QVERIFY(!snapshots.isEmpty());

    MemoryArchive mem;
    DA::DASignalArchiveTask writeTask(signalsJson, figuresXml, snapshots);
    QVERIFY(writeTask.exec(&mem, DA::DAAbstractArchiveTask::WriteMode));
    QVERIFY(mem.contains(DA::DASignalArchiveTask::archivePath()));
    QVERIFY(mem.contains(DA::DASignalArchiveTask::figuresArchivePath()));
    QVERIFY(mem.contains(DA::DASignalArchiveTask::chartDataPrefix() + QStringLiteral("/chart-items.xml")));

    // 保存后源视图的用户图元可删除（图元管理者不持有所有权；写路径只依赖快照字节）
    userMarker->detach();
    delete userMarker;

    // ---------- 3. 读回 ----------
    DA::DASignalArchiveTask readTask;
    QVERIFY(readTask.exec(&mem, DA::DAAbstractArchiveTask::ReadMode));
    QCOMPARE(readTask.loadedData(), signalsJson);
    QVERIFY(readTask.figuresEntryFound());
    QCOMPARE(readTask.loadedFiguresXml(), figuresXml);
    QVERIFY(!readTask.chartItemsManager().isEmpty());
    QCOMPARE(readTask.chartItemsManager().keys().size(), saveItems.keys().size());

    // ---------- 4. 新视图恢复（模拟 restoreState：回填持久化 signalId 后建视图） ----------
    QDomDocument doc;
    QString parseError;
    QVERIFY2(doc.setContent(readTask.loadedFiguresXml(), &parseError), qPrintable(parseError));
    DA::DASignalPtr restoredSignal = makeTestSignal();
    restoredSignal->restoreSignalId(sourceView.signal()->signalId());
    const QDomElement figureEle =
        DA::DASignalFigureArchive::findFigureElement(doc, restoredSignal->signalId(), QStringLiteral("unit"));
    QVERIFY(!figureEle.isNull());

    DA::DASignalAnalysisView restoredView(restoredSignal);
    QVERIFY(DA::DASignalFigureArchive::restoreFigure(&restoredView, figureEle, &readTask.chartItemsManager()));

    // 网格比例恢复
    compareSizeList(restoredView.gridRowSizes(), gridRows);
    compareSizeList(restoredView.gridColumnSizes(), gridCols);
    // figure id 往返
    QCOMPARE(restoredView.figureId(), QStringLiteral("sig-archived"));
    // 峰值标记恢复并被视图接管
    QCOMPARE(restoredView.peakMarkers().size(), 1);
    QwtPlotMarker* restoredPeak = restoredView.peakMarkers().first();
    QVERIFY(restoredPeak);
    QVERIFY(restoredPeak->isVisible());
    QVERIFY(restoredView.spectrumChart()->itemList().contains(restoredPeak));
    // 用户图元恢复到对应子图（波形），transient 分析结果图元未被恢复
    const QList< QwtPlotItem* > waveItems = nonTransientItems(restoredView.waveChart());
    QCOMPARE(waveItems.size(), 1);
    QVERIFY(dynamic_cast< QwtPlotMarker* >(waveItems.first()));
    bool hasEnvelope = false;
    const QList< QwtPlotItem* > waveAllItems = restoredView.waveChart()->itemList();
    for (QwtPlotItem* item : waveAllItems) {
        if (dynamic_cast< QwtPlotIntervalCurve* >(item)) {
            hasEnvelope = true;
        }
    }
    QVERIFY(!hasEnvelope);
    QCOMPARE(nonTransientItems(restoredView.spectrumChart()).size(), 1);
    bool hasSpectrumCurve = false;
    const QList< QwtPlotItem* > spectrumAllItems = restoredView.spectrumChart()->itemList();
    for (QwtPlotItem* item : spectrumAllItems) {
        if (dynamic_cast< QwtPlotCurve* >(item)) {
            hasSpectrumCurve = true;
        }
    }
    QVERIFY(!hasSpectrumCurve);
    QCOMPARE(nonTransientItems(restoredView.heatmapChart()).size(), 0);

    // 恢复后再次回填频谱：存档峰值标记不被删除/重建
    restoredView.setSpectrum(spectrum);
    QCOMPARE(restoredView.peakMarkers().size(), 1);
    QCOMPARE(restoredView.peakMarkers().first(), restoredPeak);
    QVERIFY(restoredView.spectrumChart()->itemList().contains(restoredPeak));
}

/**
 * @brief T9：旧工程没有 figures.xml/chart-data 时加载成功且无图元
 *
 * 只有 signals.json 的旧存档：figuresEntryFound()==false、loadedFiguresXml() 为空、
 * 图元管理者为空，exec 返回 true（界面按默认布局重建）。
 */
void DASignalAnalysisTest::signalArchiveLegacyWithoutFigures()
{
    const QByteArray signalsJson = QByteArrayLiteral("{\"signals\":[{\"name\":\"legacy\"}]}");
    MemoryArchive mem;
    QVERIFY(mem.write(DA::DASignalArchiveTask::archivePath(), signalsJson));

    DA::DASignalArchiveTask task;
    QVERIFY(task.exec(&mem, DA::DAAbstractArchiveTask::ReadMode));
    QCOMPARE(task.loadedData(), signalsJson);
    QVERIFY(task.archiveEntryFound());
    QVERIFY(!task.figuresEntryFound());
    QVERIFY(task.loadedFiguresXml().isEmpty());
    QVERIFY(task.chartItemsManager().isEmpty());
}

/**
 * @brief T9：损坏 figures.xml 恢复失败且视图保持默认布局
 *
 * 结构不完整（无 <chart>）的 figure 元素直接返回 false，不得崩、不得改动网格比例。
 */
void DASignalAnalysisTest::signalFigureRestoreCorruptXml()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the corrupt figure archive test");
    }
    DA::DASignalAnalysisView view(makeTestSignal());
    QVERIFY(view.figure());
    QVERIFY(view.figure()->grid());
    const qint64 signalId = view.signal()->signalId();
    const QList< qreal > defaultRows = view.gridRowSizes();
    const QList< qreal > defaultCols = view.gridColumnSizes();

    QDomDocument doc;
    QString parseError;
    QVERIFY2(doc.setContent(
                 QByteArrayLiteral(
                     "<signal-figures version=\"1\"><figure signal-name=\"unit\"><charts/></figure></signal-figures>"),
                 &parseError),
             qPrintable(parseError));
    const QDomElement figureEle =
        DA::DASignalFigureArchive::findFigureElement(doc, signalId, QStringLiteral("unit"));
    QVERIFY(!figureEle.isNull());
    QVERIFY(!DA::DASignalFigureArchive::restoreFigure(&view, figureEle, nullptr));

    // 视图仍是默认 2×2 网格与默认比例
    QCOMPARE(view.figure()->grid()->rowCount(), 2);
    QCOMPARE(view.figure()->grid()->columnCount(), 2);
    compareSizeList(view.gridRowSizes(), defaultRows);
    compareSizeList(view.gridColumnSizes(), defaultCols);

    // 空元素/非法元素同样安全失败
    QVERIFY(!DA::DASignalFigureArchive::restoreFigure(&view, QDomElement(), nullptr));
    QDomDocument notFigureDoc;
    QVERIFY(notFigureDoc.setContent(QByteArrayLiteral("<signal-figures version=\"1\"><charts/></signal-figures>")));
    QDomElement rootAsFigure = notFigureDoc.documentElement();
    QVERIFY(!DA::DASignalFigureArchive::restoreFigure(&view, rootAsFigure, nullptr));
}

/**
 * @brief T9：图表导出工具（整页 PNG / 当前子图 PNG/SVG / 复制剪贴板）
 *
 * 只在有 OpenGL 的环境跑（视图含 3D 子控件）；导出依赖控件已布局出非零尺寸，
 * 测试里不 show()/不进事件循环（会卡在 GL 上下文初始化），而是直接设尺寸并同步
 * 激活 QwtFigure 布局。临时文件写到 QTemporaryDir，不污染工作目录。
 */
void DASignalAnalysisTest::signalChartExportToFile()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the signal chart export test");
    }
    DA::DASignalAnalysisView view(makeTestSignal());
    // 不 show() 也不进事件循环：视图含 GL 子控件，测试进程里 show/qWait 会卡在 GL 上下文初始化。
    // 直接设尺寸并同步激活 QwtFigure 的布局（QLayout::activate 会立刻给子图分配几何），
    // 导出只依赖控件有非零尺寸。
    view.resize(900, 640);
    if (view.figure()) {
        view.figure()->resize(900, 640);
        if (view.figure()->layout()) {
            view.figure()->layout()->activate();
        }
    }
    // 兜底：布局未生效时直接给 2D 子图一个尺寸（导出要求 chart->size() 非空）
    const QList< DA::DAChartWidget* > charts = { view.waveChart(), view.spectrumChart(), view.heatmapChart() };
    for (DA::DAChartWidget* chart : charts) {
        if (chart && chart->size().isEmpty()) {
            chart->resize(400, 240);
        }
    }

    // 造分析结果，让子图有可见内容（导出不应只抓空画布）
    QVector< double > xs { 0.0, 1.0, 2.0 };
    QVector< double > mins { -1.0, -2.0, -1.5 };
    QVector< double > maxs { 1.0, 2.0, 1.5 };
    view.setEnvelope(xs, mins, maxs);
    DA::DASpectrumResult spectrum;
    spectrum.freqs       = { 0.0, 10.0, 20.0 };
    spectrum.amplitudes  = { 1.0, 2.0, 3.0 };
    spectrum.resolutionHz = 10.0;
    view.setSpectrum(spectrum);
    view.setCurrentChart(view.waveChart());

    // 整页抓图 + 保存 PNG
    QVERIFY(!DA::DASignalChartExport::grabWholePage(&view).isNull());
    QVERIFY(DA::DASignalChartExport::grabWholePage(nullptr).isNull());
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString wholePagePath = tempDir.filePath(QStringLiteral("page.png"));
    QVERIFY(DA::DASignalChartExport::saveWholePage(&view, wholePagePath));
    QVERIFY(QFileInfo(wholePagePath).size() > 0);
    QVERIFY(!DA::DASignalChartExport::saveWholePage(&view, QString()));

    // 当前子图：PNG + SVG 都要产出非空文件
    const QString pngPath = tempDir.filePath(QStringLiteral("chart.png"));
    QVERIFY(DA::DASignalChartExport::saveCurrentSubChart(&view, pngPath, DA::DAFigureExportFormat::Png));
    QVERIFY(QFileInfo(pngPath).size() > 0);
    const QString svgPath = tempDir.filePath(QStringLiteral("chart.svg"));
    QVERIFY(DA::DASignalChartExport::saveCurrentSubChart(&view, svgPath, DA::DAFigureExportFormat::Svg));
    QFile svgFile(svgPath);
    QVERIFY(svgFile.open(QIODevice::ReadOnly));
    const QByteArray svgHead = svgFile.read(256);
    QVERIFY(svgHead.contains("<svg"));
    QVERIFY(!DA::DASignalChartExport::saveCurrentSubChart(&view, QString(), DA::DAFigureExportFormat::Png));

    // 剪贴板：整页与当前子图都应写入（该断言只验证工具类抓图成功并调用 setPixmap；
    // 不回读系统剪贴板——无人交互的测试会话里 OpenClipboard 可能被其它进程占用而偶发失败）
    QVERIFY(DA::DASignalChartExport::copyWholePageToClipboard(&view));
    QVERIFY(DA::DASignalChartExport::copyCurrentSubChartToClipboard(&view));
    QVERIFY(!DA::DASignalChartExport::copyWholePageToClipboard(nullptr));
    QVERIFY(!DA::DASignalChartExport::copyCurrentSubChartToClipboard(nullptr));
}

/**
 * @brief BLK-02：同名信号不串档——signal-id 存档键 + 恢复端元素/图元 key 防争用
 *
 * 场景：两个同名信号（重复导入同一文件）各自有可见峰值标记与不同网格比例。
 * 校验：
 * - saveFigures 产出两个 signal-name 相同、signal-id 不同的 <figure> 元素；
 * - 恢复端 takeFigureElement 逐个消费候选元素，两个视图拿到**不同**元素；
 * - 共享 consumedItemKeys 恢复后，两个视图的峰值标记指针互不共享、各归各的频谱图；
 * - 旧格式（无 signal-id 属性）按 signal-name 回退仍可命中，消费语义把同名元素按顺序错开。
 */
void DASignalAnalysisTest::signalArchiveDuplicateNames()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the duplicate signal names archive test");
    }
    // ---------- 1. 两个同名信号（makeTestSignal 的名字都是 "unit"） ----------
    DA::DASignalPtr sig1 = makeTestSignal();
    DA::DASignalPtr sig2 = makeTestSignal();
    QVERIFY(sig1->signalId() != sig2->signalId());
    DA::DASignalAnalysisView view1(sig1);
    DA::DASignalAnalysisView view2(sig2);
    DA::DAPeakResult peaks;
    peaks.frequencies = { 10.0 };
    peaks.magnitudes  = { 2.0 };
    view1.setPeaks(peaks);
    view1.setPeakMarkersVisible(true);
    view2.setPeaks(peaks);
    view2.setPeakMarkersVisible(true);
    QCOMPARE(view1.peakMarkers().size(), 1);
    QCOMPARE(view2.peakMarkers().size(), 1);
    // 不同网格比例，用于证明恢复端各自拿到自己的元素
    const QList< qreal > rows1 { 0.7, 0.3 }, cols1 { 0.6, 0.4 };
    const QList< qreal > rows2 { 0.35, 0.65 }, cols2 { 0.3, 0.7 };
    view1.setGridSizes(rows1, cols1);
    view2.setGridSizes(rows2, cols2);

    // ---------- 2. 保存：两个元素 signal-name 相同、signal-id 不同 ----------
    DA::DAChartItemsManager saveItems;
    const QByteArray figuresXml = DA::DASignalFigureArchive::saveFigures({ &view1, &view2 }, &saveItems);
    QVERIFY(!figuresXml.isEmpty());
    QCOMPARE(saveItems.keys().size(), 2);
    const QList< DA::DAChartItemSnapshot > snapshots = DA::DAChartItemArchive::makeSnapshots(&saveItems);
    QCOMPARE(snapshots.size(), 2);

    MemoryArchive mem;
    DA::DASignalArchiveTask writeTask(QByteArrayLiteral("{\"signals\":[]}"), figuresXml, snapshots);
    QVERIFY(writeTask.exec(&mem, DA::DAAbstractArchiveTask::WriteMode));
    DA::DASignalArchiveTask readTask;
    QVERIFY(readTask.exec(&mem, DA::DAAbstractArchiveTask::ReadMode));

    QDomDocument doc;
    QString parseError;
    QVERIFY2(doc.setContent(readTask.loadedFiguresXml(), &parseError), qPrintable(parseError));
    const QList< QDomElement > savedFigures = DA::DASignalFigureArchive::figureElements(doc);
    QCOMPARE(savedFigures.size(), 2);
    QStringList savedIds;
    for (const QDomElement& figureEle : savedFigures) {
        QCOMPARE(figureEle.attribute(QStringLiteral("signal-name")), QStringLiteral("unit"));
        savedIds.append(figureEle.attribute(QStringLiteral("signal-id")));
    }
    QCOMPARE(savedIds, QStringList({ QString::number(sig1->signalId()), QString::number(sig2->signalId()) }));
    QVERIFY(savedIds.at(0) != savedIds.at(1));

    // ---------- 3. 恢复：候选元素逐个消费，两个视图拿到不同元素 ----------
    DA::DASignalPtr restoredSig1 = makeTestSignal();
    DA::DASignalPtr restoredSig2 = makeTestSignal();
    // 模拟 restoreState：回填持久化 signalId 后建视图（figure id 与存档键一致）
    restoredSig1->restoreSignalId(sig1->signalId());
    restoredSig2->restoreSignalId(sig2->signalId());
    DA::DASignalAnalysisView restored1(restoredSig1);
    DA::DASignalAnalysisView restored2(restoredSig2);

    QList< QDomElement > candidates = DA::DASignalFigureArchive::figureElements(doc);
    const QDomElement ele1 =
        DA::DASignalFigureArchive::takeFigureElement(candidates, restoredSig1->signalId(), QStringLiteral("unit"));
    const QDomElement ele2 =
        DA::DASignalFigureArchive::takeFigureElement(candidates, restoredSig2->signalId(), QStringLiteral("unit"));
    QVERIFY(!ele1.isNull());
    QVERIFY(!ele2.isNull());
    QVERIFY(ele1 != ele2);  // 消费不重复：两个视图绝不命中同一个元素
    QCOMPARE(candidates.size(), 0);

    QSet< QString > consumedKeys;
    QVERIFY(DA::DASignalFigureArchive::restoreFigure(&restored1, ele1, &readTask.chartItemsManager(), &consumedKeys));
    QVERIFY(DA::DASignalFigureArchive::restoreFigure(&restored2, ele2, &readTask.chartItemsManager(), &consumedKeys));

    // 各自的网格比例恢复正确（证明拿到的是自己的元素，不是对方的）
    compareSizeList(restored1.gridRowSizes(), rows1);
    compareSizeList(restored1.gridColumnSizes(), cols1);
    compareSizeList(restored2.gridRowSizes(), rows2);
    compareSizeList(restored2.gridColumnSizes(), cols2);
    // 峰值标记互不共享：各归各的频谱图，绝不出现同一 QwtPlotMarker 挂两个 chart
    QCOMPARE(restored1.peakMarkers().size(), 1);
    QCOMPARE(restored2.peakMarkers().size(), 1);
    QwtPlotMarker* marker1 = restored1.peakMarkers().first();
    QwtPlotMarker* marker2 = restored2.peakMarkers().first();
    QVERIFY(marker1 != marker2);
    QVERIFY(restored1.spectrumChart()->itemList().contains(marker1));
    QVERIFY(!restored1.spectrumChart()->itemList().contains(marker2));
    QVERIFY(restored2.spectrumChart()->itemList().contains(marker2));
    QVERIFY(!restored2.spectrumChart()->itemList().contains(marker1));

    // ---------- 4. 旧格式回退：无 signal-id 属性按 signal-name 命中；同名元素消费式错开 ----------
    QDomDocument legacyDoc;
    QVERIFY(legacyDoc.setContent(
        QByteArrayLiteral("<signal-figures version=\"1\">"
                          "<figure signal-name=\"unit\" id=\"sig-unit\"/>"
                          "</signal-figures>")));
    const QDomElement legacyEle =
        DA::DASignalFigureArchive::findFigureElement(legacyDoc, 12345, QStringLiteral("unit"));
    QVERIFY(!legacyEle.isNull());  // 旧档元素无 signal-id：按名字回退命中

    QDomDocument dupDoc;
    QVERIFY(dupDoc.setContent(
        QByteArrayLiteral("<signal-figures version=\"1\">"
                          "<figure signal-name=\"X\"/>"
                          "<figure signal-name=\"X\"/>"
                          "</signal-figures>")));
    QList< QDomElement > dupCandidates = DA::DASignalFigureArchive::figureElements(dupDoc);
    QCOMPARE(dupCandidates.size(), 2);
    const QDomElement dupA = DA::DASignalFigureArchive::takeFigureElement(dupCandidates, 1, QStringLiteral("X"));
    const QDomElement dupB = DA::DASignalFigureArchive::takeFigureElement(dupCandidates, 2, QStringLiteral("X"));
    QVERIFY(!dupA.isNull());
    QVERIFY(!dupB.isNull());
    QVERIFY(dupA != dupB);  // 旧档两个同名元素：消费语义按顺序错开，不再两个视图共用第一个
    QVERIFY(DA::DASignalFigureArchive::takeFigureElement(dupCandidates, 3, QStringLiteral("X")).isNull());
}

/**
 * @brief BLK-01：快照两段式——写路径与活图元指针解耦
 *
 * 场景：GUI 线程 makeSnapshots 预序列化后，**删除全部图元**（模拟保存期间 GUI 线程
 * 删除/重建峰值标记），后台任务 saveSnapshots 仍能写出完整 chart-data，load 后
 * 峰值标记与用户图元完整恢复——证明后台写路径不依赖任何存活的 QwtPlotItem*。
 */
void DASignalAnalysisTest::signalArchiveSnapshotDecoupledFromLiveItems()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the snapshot decoupling test");
    }
    DA::DASignalAnalysisView view(makeTestSignal());
    DA::DAPeakResult peaks;
    peaks.frequencies = { 10.0, 20.0 };
    peaks.magnitudes  = { 2.0, 1.0 };
    view.setPeaks(peaks);
    view.setPeakMarkersVisible(true);
    QCOMPARE(view.peakMarkers().size(), 2);
    auto* userMarker = new QwtPlotMarker();
    userMarker->setXValue(1.0);
    userMarker->setYValue(0.5);
    userMarker->attach(view.waveChart());

    // GUI 线程：登记 + 预序列化快照
    DA::DAChartItemsManager saveItems;
    const QByteArray figuresXml = DA::DASignalFigureArchive::saveFigures({ &view }, &saveItems);
    QVERIFY(!figuresXml.isEmpty());
    QCOMPARE(saveItems.keys().size(), 3);  // 2 个峰值标记 + 1 个用户图元
    const QList< DA::DAChartItemSnapshot > snapshots = DA::DAChartItemArchive::makeSnapshots(&saveItems);
    QCOMPARE(snapshots.size(), 3);
    for (const DA::DAChartItemSnapshot& snapshot : snapshots) {
        QVERIFY(!snapshot.key.isEmpty());
        QVERIFY(!snapshot.data.isEmpty());
    }

    // 模拟 GUI 线程在存档期间删除全部图元（后台任务只持有快照字节，不得触碰它们）
    QList< QwtPlotItem* > doomed;
    for (const QString& key : saveItems.keys()) {
        QwtPlotItem* item = saveItems.keyToItem(key);
        if (item) {
            doomed.append(item);
        }
    }
    QCOMPARE(doomed.size(), 3);
    for (QwtPlotItem* item : doomed) {
        item->detach();
        delete item;
    }

    // 后台任务只写快照字节：图元已全部销毁仍能写出并读回完整 chart-data
    MemoryArchive mem;
    DA::DASignalArchiveTask writeTask(QByteArrayLiteral("{\"signals\":[]}"), figuresXml, snapshots);
    QVERIFY(writeTask.exec(&mem, DA::DAAbstractArchiveTask::WriteMode));
    DA::DASignalArchiveTask readTask;
    QVERIFY(readTask.exec(&mem, DA::DAAbstractArchiveTask::ReadMode));
    QVERIFY(!readTask.chartItemsManager().isEmpty());
    QCOMPARE(readTask.chartItemsManager().keys().size(), 3);

    // 恢复到新视图：峰值标记与用户图元完整回来（写路径与活指针解耦的端到端证明）
    QDomDocument doc;
    QString parseError;
    QVERIFY2(doc.setContent(readTask.loadedFiguresXml(), &parseError), qPrintable(parseError));
    DA::DASignalPtr restoredSignal = makeTestSignal();
    restoredSignal->restoreSignalId(view.signal()->signalId());
    const QDomElement figureEle =
        DA::DASignalFigureArchive::findFigureElement(doc, restoredSignal->signalId(), QStringLiteral("unit"));
    QVERIFY(!figureEle.isNull());
    DA::DASignalAnalysisView restoredView(restoredSignal);
    QVERIFY(DA::DASignalFigureArchive::restoreFigure(&restoredView, figureEle, &readTask.chartItemsManager()));
    QCOMPARE(restoredView.peakMarkers().size(), 2);
    QCOMPARE(nonTransientItems(restoredView.waveChart()).size(), 1);
}

/**
 * @brief BLK-03a：外部删除缓存图元（图元树路径）后回填/重译不崩溃且按需重建
 *
 * 模拟共享图元树对 transient 图元执行 removePlotItem（detach+delete）：
 * - 删包络曲线后再 setEnvelope：包络重建（itemList 重新出现 QwtPlotIntervalCurve）；
 * - 删播放头后 setCurrentTime：播放头重建；
 * - 删谱图图元后 setDbFloor（applyHeatmapColors 路径）不崩溃，再 setStft 重建谱图；
 * - LanguageChange（retranslateUi 路径）对失效缓存不崩溃。
 */
void DASignalAnalysisTest::signalViewExternalItemRemoval()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the external item removal test");
    }
    DA::DASignalAnalysisView view(makeTestSignal());
    const QVector< double > xs { 0.0, 1.0, 2.0 };
    const QVector< double > mins { -1.0, -2.0, -1.5 };
    const QVector< double > maxs { 1.0, 2.0, 1.5 };
    view.setEnvelope(xs, mins, maxs);
    // 找到包络曲线与播放头（波形图上的 transient 图元）
    QwtPlotIntervalCurve* envelope = nullptr;
    QwtPlotMarker* playhead        = nullptr;
    const QList< QwtPlotItem* > waveItems = view.waveChart()->itemList();
    for (QwtPlotItem* item : waveItems) {
        if (auto* curve = dynamic_cast< QwtPlotIntervalCurve* >(item)) {
            envelope = curve;
        } else if (auto* marker = dynamic_cast< QwtPlotMarker* >(item)) {
            playhead = marker;
        }
    }
    QVERIFY(envelope);
    QVERIFY(playhead);

    // 删包络曲线（模拟图元树 Delete）：缓存指针悬空，再回填必须重建而不是崩溃
    view.waveChart()->removePlotItem(envelope);
    view.setEnvelope(xs, mins, maxs);
    bool envelopeRebuilt = false;
    const QList< QwtPlotItem* > waveAfterEnvelope = view.waveChart()->itemList();
    for (QwtPlotItem* item : waveAfterEnvelope) {
        if (dynamic_cast< QwtPlotIntervalCurve* >(item)) {
            envelopeRebuilt = true;
        }
    }
    QVERIFY2(envelopeRebuilt, "the envelope curve must be rebuilt after external removal");

    // 删播放头：下一次时间更新（updatePlayhead）按需重建
    view.waveChart()->removePlotItem(playhead);
    view.setCurrentTime(0.1);
    bool playheadRebuilt = false;
    const QList< QwtPlotItem* > waveAfterPlayhead = view.waveChart()->itemList();
    for (QwtPlotItem* item : waveAfterPlayhead) {
        if (dynamic_cast< QwtPlotMarker* >(item)) {
            playheadRebuilt = true;
        }
    }
    QVERIFY2(playheadRebuilt, "the playhead must be rebuilt after external removal");

    // 删谱图图元：applyHeatmapColors（setDbFloor 路径）对失效缓存不崩溃；
    // 再回填 STFT 结果时谱图重建
    DA::DAStftResult stft;
    stft.rows          = 2;
    stft.columns       = 3;
    stft.times         = { 0.0, 0.5, 1.0 };
    stft.freqs         = { 0.0, 10.0 };
    stft.values        = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    stft.windowSeconds = 0.1;
    view.setStft(stft);
    QwtPlotSpectrogram* spectrogram = nullptr;
    const QList< QwtPlotItem* > heatItems = view.heatmapChart()->itemList();
    for (QwtPlotItem* item : heatItems) {
        if (auto* raster = dynamic_cast< QwtPlotSpectrogram* >(item)) {
            spectrogram = raster;
        }
    }
    QVERIFY(spectrogram);
    view.heatmapChart()->removePlotItem(spectrogram);
    view.setDbFloor(-60.0);   // applyHeatmapColors：失效缓存早退，不崩溃
    view.setColormapName(QStringLiteral("inferno"));  // 同一路径再走一遍
    view.setStft(stft);       // rebuildHeatmap：谱图重建
    bool spectrogramRebuilt = false;
    const QList< QwtPlotItem* > heatAfter = view.heatmapChart()->itemList();
    for (QwtPlotItem* item : heatAfter) {
        if (dynamic_cast< QwtPlotSpectrogram* >(item)) {
            spectrogramRebuilt = true;
        }
    }
    QVERIFY2(spectrogramRebuilt, "the spectrogram must be rebuilt after external removal");

    // retranslateUi（LanguageChange 路径）对失效/重建后的缓存不崩溃
    QEvent langEvent(QEvent::LanguageChange);
    QApplication::sendEvent(&view, &langEvent);
}

/**
 * @brief SF-11：figure 撤销栈入组等价接线
 *
 * DASignalAnalysisUI::connectSignalView 对每个信号 figure 的 getUndoStack() 调
 * DACommandInterface::addStack（实现即 QUndoGroup::addStack），切信号时 setActive。
 * 测试环境无法实例化完整 UI，这里直接验证组语义等价：
 * - activeStack 跟随 setActive 切换；
 * - 组的 canUndo/canRedo 跟随活动栈；
 * - 栈随 figure 销毁被 QUndoGroup 自动摘除（活动栈销毁后组无活动栈）。
 */
void DASignalAnalysisTest::undoStackGroupWiring()
{
    DA::DAFigureWidget fig1;
    DA::DAFigureWidget fig2;
    QUndoStack* stack1 = fig1.getUndoStack();
    QUndoStack* stack2 = fig2.getUndoStack();
    QVERIFY(stack1);
    QVERIFY(stack2);
    QVERIFY(stack1 != stack2);

    QUndoGroup group;
    group.addStack(stack1);
    group.addStack(stack2);
    QCOMPARE(group.stacks().size(), 2);
    QVERIFY(group.activeStack() == nullptr);  // 未激活任何栈

    stack1->setActive();
    QCOMPARE(group.activeStack(), stack1);
    stack2->setActive();
    QCOMPARE(group.activeStack(), stack2);  // 切换信号标签页：activeStack 跟随

    // 活动栈压入命令 → 组的 undo/redo 可用性跟随
    stack2->push(new QUndoCommand());
    QVERIFY(group.canUndo());
    stack1->setActive();
    QVERIFY(!group.canUndo());  // stack1 为空栈
    stack2->setActive();
    QVERIFY(group.canUndo());

    // 栈随 figure 销毁自动摘除（QUndoGroup 连接 destroyed；figure 值持有栈）
    DA::DAFigureWidget* heapFig = new DA::DAFigureWidget();
    QUndoStack* heapStack       = heapFig->getUndoStack();
    group.addStack(heapStack);
    heapStack->setActive();
    QCOMPARE(group.activeStack(), heapStack);
    delete heapFig;
    QCOMPARE(group.stacks().size(), 2);
    QVERIFY(group.activeStack() == nullptr);  // 活动栈销毁后组无活动栈（动作置灰）
    // 剩余栈仍可再次激活（切回另一个信号标签页）
    stack1->setActive();
    QCOMPARE(group.activeStack(), stack1);
}

/**
 * @brief SF-13：峰值「存档优先」只在加载后到第一次重算前
 *
 * 时序镜像真实恢复流：connectSignalView 先应用显隐开关（此时无峰值数据、无标记），
 * restoreFigure 再接管存档标记；**setPeaks 收到第一次重算结果即结束存档优先窗口**并
 * 按当前峰值重建（v1.8：此前要等 setParams/setAmplitudeDisplay/切通道，峰值窗口打开或
 * 峰值标记打开触发重算后旧标记仍停在存档位置，表现为「标记与曲线峰值偏离」）；
 * setAmplitudeDisplay/setParams 也会复位并按当前 mPeaks 重建：
 * - 幅值口径切换：新对象、新位置（dBFS 下 2.0 → 20·log10(2.0) ≈ 6.0206）；
 * - 参数变化：同样复位存档优先并重建。
 */
void DASignalAnalysisTest::signalPeakMarkersRestoredUntilRecompute()
{
    if (!openGLAvailable()) {
        QSKIP("OpenGL is not available: skip the peak markers restored test");
    }
    DA::DASignalAnalysisView view(makeTestSignal());
    DA::DASpectrumResult spectrum;
    spectrum.freqs        = { 0.0, 10.0, 20.0 };
    // 10 Hz 必须是曲线局部极大：rebuildPeakMarkers 会把标记吸附到已绘制曲线的峰顶
    spectrum.amplitudes   = { 1.0, 2.0, 1.0 };
    spectrum.resolutionHz = 10.0;
    view.setSpectrum(spectrum);
    // 恢复流程先应用显隐开关：此时 mPeaks 无效，不产生标记（connectSignalView 时序）
    view.setPeakMarkersVisible(true);
    QCOMPARE(view.peakMarkers().size(), 0);

    // 模拟存档接管（restoreFigure 的第 4 步）：手工构造标记并交给视图
    auto* restoredMarker = new QwtPlotMarker();
    restoredMarker->setXValue(10.0);
    restoredMarker->setYValue(2.0);
    restoredMarker->attach(view.spectrumChart());
    view.adoptRestoredPeakMarkers({ restoredMarker });
    QCOMPARE(view.peakMarkers().size(), 1);

    // setPeaks 收到新的计算结果即结束「存档优先」窗口：旧存档标记被删除，按新峰值重建。
    // 这是 v1.8 口径修正：此前只在 setParams/setAmplitudeDisplay/切通道复位该标志，
    // 峰值窗口打开/峰值标记打开触发重算后旧标记仍停在那里，表现为「标记与曲线峰值偏离」。
    DA::DAPeakResult peaks;
    peaks.frequencies = { 10.0 };
    peaks.magnitudes  = { 2.0 };
    view.setPeaks(peaks);
    QCOMPARE(view.peakMarkers().size(), 1);
    // 注意不要用已 delete 的 restoredMarker 做 itemList().contains()：堆地址复用会让断言偶发通过/失败。
    // 用「图上只有 1 个标记，且登记的那个标记仍挂在图上」判断重建结果。
    QVERIFY2(view.spectrumChart()->itemList().contains(view.peakMarkers().first()),
             "the recomputed peak marker must be attached to the spectrum chart");
    int markerItems = 0;
    const QList< QwtPlotItem* > spectrumItems = view.spectrumChart()->itemList();
    for (QwtPlotItem* item : spectrumItems) {
        if (item && item->rtti() == QwtPlotItem::Rtti_PlotMarker) {
            ++markerItems;
        }
    }
    QCOMPARE(markerItems, 1);
    // 重建后的标记吸附到当前已绘制频谱曲线的局部极大（10 Hz 处幅值 2.0）
    QCOMPARE(view.peakMarkers().first()->xValue(), 10.0);
    QCOMPARE(view.peakMarkers().first()->yValue(), 2.0);

    // 之后再回填频谱只按当前峰值重建标记，不会退回存档状态
    view.setSpectrum(spectrum);
    QCOMPARE(view.peakMarkers().size(), 1);
    QCOMPARE(view.peakMarkers().first()->xValue(), 10.0);

    // 幅值口径切换：按当前 mPeaks 以新口径重建（SF-13）。
    // 断言用「重建后标记的幅值口径」而非指针不等：delete+new 在堆地址复用时可能拿到
    // 相同地址，指针不等不可靠（实测可复现）
    view.setAmplitudeDisplay(QStringLiteral("dbFS"), QStringLiteral("fullScale"));
    QCOMPARE(view.peakMarkers().size(), 1);
    QwtPlotMarker* rebuilt = view.peakMarkers().first();
    // dBFS 参考 = signalPeak()（无包络时 0）→ 回退 1.0：20·log10(2.0/1.0)
    QVERIFY2(qAbs(rebuilt->yValue() - 6.020599913279624) < 1e-6,
             "the rebuilt marker must use the new amplitude conversion");
    QCOMPARE(rebuilt->xValue(), 10.0);

    // 参数变化同样复位存档优先并重建（新视图重复接管流程，镜像真实时序）。
    // 用 dbFS 参数让「重建后的口径」可断言：peak 口径下重建值与存档值相同（2.0），
    // 只有指针能区分——而指针不等因堆地址复用不可靠
    DA::DASignalAnalysisView view2(makeTestSignal());
    view2.setSpectrum(spectrum);
    view2.setPeakMarkersVisible(true);
    auto* restoredAgain = new QwtPlotMarker();
    restoredAgain->setXValue(10.0);
    restoredAgain->setYValue(2.0);
    restoredAgain->attach(view2.spectrumChart());
    view2.adoptRestoredPeakMarkers({ restoredAgain });
    QCOMPARE(view2.peakMarkers().size(), 1);
    view2.setPeaks(peaks);
    DA::DASignalAnalysisParams dbfsParams = DA::DASignalAnalysisParams::defaults();
    dbfsParams.amplitudeMode = QStringLiteral("dbFS");
    dbfsParams.dbReference   = QStringLiteral("fullScale");
    view2.setParams(dbfsParams);
    QCOMPARE(view2.peakMarkers().size(), 1);
    QVERIFY2(qAbs(view2.peakMarkers().first()->yValue() - 6.020599913279624) < 1e-6,
             "setParams must reset the archive priority and rebuild the markers with the new amplitude mode");
    QCOMPARE(view2.peakMarkers().first()->xValue(), 10.0);
}

QTEST_MAIN(DASignalAnalysisTest)
#include "main.moc"