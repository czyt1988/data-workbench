#include <QtTest>
#include <QCoreApplication>
#include "Analysis/DASignalAnalysisParams.h"
#include "Analysis/DAPeakTableModel.h"
#include "Render/DAStftRasterData.h"
#include "qwt_interval.h"

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
    QCOMPARE(model.columnCount(), int(DA::DAPeakTableModel::ColumnCount));
    // 序号列从 1 开始
    QCOMPARE(model.data(model.index(0, DA::DAPeakTableModel::ColumnIndex)).toInt(), 1);
    // 频率列
    QCOMPARE(model.data(model.index(1, DA::DAPeakTableModel::ColumnFrequency)).toString(),
             QStringLiteral("2000.00"));
    // 表头（频率列）
    QCOMPARE(model.headerData(DA::DAPeakTableModel::ColumnFrequency, Qt::Horizontal).toString(),
             QStringLiteral("Frequency (Hz)"));
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
    model.setAmplitudeMode(QStringLiteral("dbFS"), QStringLiteral("fullScale"));
    const QString top = model.data(model.index(0, DA::DAPeakTableModel::ColumnAmplitude)).toString();
    QVERIFY(top.contains(QStringLiteral("dB")));
    QVERIFY(qAbs(top.split(QLatin1Char(' ')).first().toDouble()) < 0.01);
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
