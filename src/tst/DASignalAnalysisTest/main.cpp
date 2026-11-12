#include <QtTest>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QOpenGLContext>
#include <QSignalSpy>
#include <QMetaType>
#include <QStringList>
#include <QSurfaceFormat>
#include <QVariantMap>
#include <cmath>
#include <memory>
#include "Analysis/DASignalAmplitude.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DASignalMemoryEstimate.h"
#include "Analysis/DAPeakTableModel.h"
#include "Form/DASignalFormSpecBuilder.h"
#include "Page/DAPeakCurvePlotSamples.h"
#include "Page/DASignalAnalysisView.h"
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
    // 表单口径：Top N 选项值是整数、最小间距默认 5 倍实际分辨率（评审 U5/U7）
    void formSpecTopNAndSpacing();
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
    // T7：视图 figure 为 2×2 网格，波形跨两列，热图与三维瀑布同格
    void signalViewFigureGrid();
    // T7：分析结果图元标 transient，峰值标记保持非 transient
    void signalViewTransientItems();
    // T7：瀑布模式切换热图/三维瀑布显隐与当前子图判定
    void signalViewWaterfallMode();
    // T8：点击 2D 子图画布即切换当前子图（设置面板 target 跟随）
    void signalViewClickSwitchesCurrentChart();
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
 * @brief 表单口径：Top N 选项值是整数、最小间距默认 5 倍实际分辨率
 */
void DASignalAnalysisTest::formSpecTopNAndSpacing()
{
    const DA::DASignalAnalysisParams params = DA::DASignalAnalysisParams::defaults();
    const QJsonObject spec                  = DA::DASignalFormSpecBuilder::build(params, 10.0);

    const QJsonObject topN = findFormField(spec, QStringLiteral("peakTopN"));
    QVERIFY2(!topN.isEmpty(), "peakTopN field not found");
    // 默认值/选项值都必须是数值：字符串会让 QJsonValue::toInt(default) 静默退回默认值（评审 U5）
    QCOMPARE(topN.value(QStringLiteral("default")).toInt(), 10);
    const QJsonArray options = topN.value(QStringLiteral("options")).toArray();
    QCOMPARE(options.size(), 3);
    QVector< int > values;
    for (const QJsonValue& ov : options) {
        const QJsonObject opt = ov.toObject();
        QVERIFY2(opt.value(QStringLiteral("value")).isDouble(), "Top N option value must be an integer");
        values.append(opt.value(QStringLiteral("value")).toInt());
    }
    QCOMPARE(values, QVector< int >({ 10, 20, 50 }));

    // 最小间距默认 = 5 × 实际分辨率（10 赫兹 → 50 赫兹，评审 U7）
    const QJsonObject spacing = findFormField(spec, QStringLiteral("peakMinSpacingHz"));
    QVERIFY2(!spacing.isEmpty(), "peakMinSpacingHz field not found");
    QCOMPARE(spacing.value(QStringLiteral("default")).toDouble(), 50.0);

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

    // figure id 由信号派生
    QCOMPARE(view.figureId(), QStringLiteral("sig-unit"));
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

QTEST_MAIN(DASignalAnalysisTest)
#include "main.moc"
