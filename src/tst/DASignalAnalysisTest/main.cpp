#include <QtTest>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSignalSpy>
#include <cmath>
#include "Analysis/DASignalAmplitude.h"
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DAPeakTableModel.h"
#include "Form/DASignalFormSpecBuilder.h"
#include "Render/DAStftRasterData.h"
#include "qwt_interval.h"

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

QTEST_MAIN(DASignalAnalysisTest)
#include "main.moc"
