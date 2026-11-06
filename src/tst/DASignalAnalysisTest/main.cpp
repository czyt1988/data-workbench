#include <QtTest>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
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

QTEST_MAIN(DASignalAnalysisTest)
#include "main.moc"
