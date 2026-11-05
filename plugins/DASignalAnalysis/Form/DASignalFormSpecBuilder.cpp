#include "DASignalFormSpecBuilder.h"
#include <QJsonArray>
#include <QJsonValue>
#include <QCoreApplication>

namespace DA
{
namespace
{
QJsonObject enumOption(const QString& value, const QString& label)
{
    QJsonObject o;
    o[ "value" ] = value;
    o[ "label" ] = label;
    return o;
}

QJsonObject makeField(const QString& name,
                      const QString& type,
                      const QString& label,
                      const QVariant& defaultValue,
                      const QJsonObject& extra = QJsonObject())
{
    QJsonObject o = extra;
    o[ "name" ]    = name;
    o[ "type" ]    = type;
    o[ "label" ]   = label;
    o[ "default" ] = QJsonValue::fromVariant(defaultValue);
    return o;
}

QJsonObject makeGroup(const QString& name, const QString& label, const QJsonArray& items)
{
    QJsonObject g;
    g[ "kind" ]  = "group";
    g[ "name" ]  = name;
    g[ "label" ] = label;
    g[ "items" ] = items;
    return g;
}

QJsonObject withVisibleWhen(const QJsonObject& field, const QString& expr)
{
    QJsonObject o = field;
    o[ "visible_when" ] = expr;
    return o;
}

QJsonObject withRange(const QJsonObject& field, double min, double max, double step, int decimals)
{
    QJsonObject o      = field;
    o[ "min" ]         = min;
    o[ "max" ]         = max;
    o[ "step" ]        = step;
    o[ "decimals" ]    = decimals;
    return o;
}
}  // namespace

/**
 * @brief 构造分析参数表单 JSON（分组 + 联动规则）
 */
QJsonObject DASignalFormSpecBuilder::build(const DASignalAnalysisParams& current, double derivedResolutionHz)
{
    QJsonArray topItems;

    // ---------------- 信号预处理 ----------------
    {
        QJsonArray items;
        items.append(makeField(QStringLiteral("removeDc"), QStringLiteral("bool"), QCoreApplication::translate("DASignalFormSpecBuilder", "Remove DC"), current.removeDc));
        // cn:去直流
        QJsonObject detrend = makeField(QStringLiteral("detrend"),
                                        QStringLiteral("enum"),
                                        QCoreApplication::translate("DASignalFormSpecBuilder", "Detrend"),  // cn:去趋势
                                        current.detrend);
        detrend[ "options" ] = QJsonArray { enumOption("none", QCoreApplication::translate("DASignalFormSpecBuilder", "None")),         // cn:无
                                            enumOption("constant", QCoreApplication::translate("DASignalFormSpecBuilder", "Constant")),  // cn:常数
                                            enumOption("linear", QCoreApplication::translate("DASignalFormSpecBuilder", "Linear")) };    // cn:线性
        items.append(detrend);
        topItems.append(makeGroup(QStringLiteral("preprocess"), QCoreApplication::translate("DASignalFormSpecBuilder", "Signal Preprocessing"), items));  // cn:信号预处理
    }

    // ---------------- 频谱 ----------------
    {
        QJsonArray items;
        QJsonObject window = makeField(QStringLiteral("window"),
                                       QStringLiteral("enum"),
                                       QCoreApplication::translate("DASignalFormSpecBuilder", "Window"),  // cn:窗函数
                                       current.window);
        window[ "options" ] = QJsonArray { enumOption("rectangular", QCoreApplication::translate("DASignalFormSpecBuilder", "Rectangular")),  // cn:矩形
                                           enumOption("hann", QCoreApplication::translate("DASignalFormSpecBuilder", "Hann")),                // cn:汉宁
                                           enumOption("hamming", QCoreApplication::translate("DASignalFormSpecBuilder", "Hamming")),          // cn:汉明
                                           enumOption("blackman", QCoreApplication::translate("DASignalFormSpecBuilder", "Blackman")),        // cn:布莱克曼
                                           enumOption("flattop", QCoreApplication::translate("DASignalFormSpecBuilder", "Flat top")) };       // cn:平顶
        items.append(window);

        QJsonObject ampMode = makeField(QStringLiteral("amplitudeMode"),
                                        QStringLiteral("enum"),
                                        QCoreApplication::translate("DASignalFormSpecBuilder", "Amplitude"),  // cn:幅值口径
                                        current.amplitudeMode);
        ampMode[ "options" ] = QJsonArray { enumOption("peak", QCoreApplication::translate("DASignalFormSpecBuilder", "Peak amplitude")),  // cn:峰值幅度
                                            enumOption("rms", QCoreApplication::translate("DASignalFormSpecBuilder", "RMS")) };            // cn:均方根
        items.append(ampMode);

        QJsonObject dbRef = makeField(QStringLiteral("dbReference"),
                                      QStringLiteral("enum"),
                                      QCoreApplication::translate("DASignalFormSpecBuilder", "dB reference"),  // cn:dB 参考
                                      current.dbReference);
        dbRef[ "options" ] = QJsonArray { enumOption("fullScale", QCoreApplication::translate("DASignalFormSpecBuilder", "Full scale (dBFS)")),  // cn:满量程
                                          enumOption("one", QCoreApplication::translate("DASignalFormSpecBuilder", "Relative to 1.0")) };        // cn:相对 1.0
        items.append(dbRef);

        items.append(withRange(makeField(QStringLiteral("resolutionHz"),
                                         QStringLiteral("float"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Frequency resolution (Hz)"),  // cn:频率分辨率（赫兹）
                                         current.resolutionHz),
                               0.001,
                               1.0e6,
                               1.0,
                               4));
        items.append(withRange(makeField(QStringLiteral("zeroPadding"),
                                         QStringLiteral("int"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Zero padding points (advanced)"),  // cn:零填充点数（高级）
                                         current.zeroPadding),
                               0,
                               1 << 20,
                               1024,
                               0));
        topItems.append(makeGroup(QStringLiteral("spectrum"), QCoreApplication::translate("DASignalFormSpecBuilder", "Spectrum"), items));  // cn:频谱
    }

    // ---------------- 功率谱 ----------------
    {
        QJsonArray items;
        QJsonObject average = makeField(QStringLiteral("average"),
                                        QStringLiteral("enum"),
                                        QCoreApplication::translate("DASignalFormSpecBuilder", "Averaging"),  // cn:平均方式
                                        current.average);
        average[ "options" ] = QJsonArray { enumOption("mean", QCoreApplication::translate("DASignalFormSpecBuilder", "Mean")),        // cn:均值
                                            enumOption("median", QCoreApplication::translate("DASignalFormSpecBuilder", "Median")) };  // cn:中位数
        items.append(average);
        items.append(withRange(makeField(QStringLiteral("overlap"),
                                         QStringLiteral("float"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Overlap (%)"),  // cn:重叠率（%）
                                         current.overlap),
                               0.0,
                               95.0,
                               5.0,
                               1));
        topItems.append(makeGroup(QStringLiteral("psd"), QCoreApplication::translate("DASignalFormSpecBuilder", "Power Spectrum"), items));  // cn:功率谱
    }

    // ---------------- 短时傅里叶变换 ----------------
    {
        QJsonArray items;
        items.append(withRange(makeField(QStringLiteral("stftWindowMs"),
                                         QStringLiteral("float"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Window length (ms)"),  // cn:窗长（毫秒）
                                         current.stftWindowMs),
                               1.0,
                               10000.0,
                               10.0,
                               2));
        items.append(withRange(makeField(QStringLiteral("stftOverlap"),
                                         QStringLiteral("float"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Hop overlap (%)"),  // cn:跳步重叠率（%）
                                         current.stftOverlap),
                               0.0,
                               95.0,
                               5.0,
                               1));
        items.append(withRange(makeField(QStringLiteral("stftZeroPadding"),
                                         QStringLiteral("int"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Zero padding points (advanced)"),  // cn:零填充点数（高级）
                                         current.stftZeroPadding),
                               0,
                               1 << 20,
                               1024,
                               0));
        topItems.append(makeGroup(QStringLiteral("stft"), QCoreApplication::translate("DASignalFormSpecBuilder", "Short-Time Fourier Transform"), items));
        // cn:短时傅里叶变换
    }

    // ---------------- 峰值查找 ----------------
    {
        QJsonArray items;
        QJsonObject mode = makeField(QStringLiteral("peakMode"),
                                     QStringLiteral("enum"),
                                     QCoreApplication::translate("DASignalFormSpecBuilder", "Mode"),  // cn:模式
                                     current.peakMode);
        mode[ "options" ] = QJsonArray { enumOption("simple", QCoreApplication::translate("DASignalFormSpecBuilder", "Simple")),        // cn:简单
                                         enumOption("advanced", QCoreApplication::translate("DASignalFormSpecBuilder", "Advanced")) };  // cn:高级
        items.append(mode);

        QJsonObject topN = makeField(QStringLiteral("peakTopN"),
                                     QStringLiteral("enum"),
                                     QCoreApplication::translate("DASignalFormSpecBuilder", "Top N"),  // cn:前 N 个
                                     QString::number(current.peakTopN));
        topN[ "options" ] = QJsonArray { enumOption("10", "10"), enumOption("20", "20"), enumOption("50", "50") };
        items.append(topN);

        items.append(withRange(makeField(QStringLiteral("peakRelDb"),
                                         QStringLiteral("float"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Relative threshold (dB)"),  // cn:相对门限（dB）
                                         current.peakRelDb),
                               0.0,
                               200.0,
                               5.0,
                               1));

        // 高级模式字段：visible_when = peakMode == "advanced"
        const QString advancedExpr = QStringLiteral("peakMode == \"advanced\"");
        items.append(withVisibleWhen(
            withRange(makeField(QStringLiteral("peakProminence"),
                                QStringLiteral("float"),
                                QCoreApplication::translate("DASignalFormSpecBuilder", "Prominence"),  // cn:突出度
                                current.peakProminence),
                      0.0,
                      1.0e9,
                      0.01,
                      6),
            advancedExpr));
        // 最小间距默认 5 倍频率分辨率（D17）；用户手改后由对话框保持用户值
        double spacingDefault = current.peakMinSpacingHz;
        if (spacingDefault <= 0.0) {
            spacingDefault = (derivedResolutionHz > 0.0 ? derivedResolutionHz : current.resolutionHz) * 5.0;
        }
        items.append(withVisibleWhen(
            withRange(makeField(QStringLiteral("peakMinSpacingHz"),
                                QStringLiteral("float"),
                                QCoreApplication::translate("DASignalFormSpecBuilder", "Minimum spacing (Hz)"),  // cn:最小间距（赫兹）
                                spacingDefault),
                      0.0,
                      1.0e9,
                      1.0,
                      4),
            advancedExpr));
        items.append(withVisibleWhen(
            withRange(makeField(QStringLiteral("peakMinHeight"),
                                QStringLiteral("float"),
                                QCoreApplication::translate("DASignalFormSpecBuilder", "Minimum height"),  // cn:绝对高度门限
                                current.peakMinHeight),
                      0.0,
                      1.0e9,
                      0.01,
                      6),
            advancedExpr));
        topItems.append(makeGroup(QStringLiteral("peaks"), QCoreApplication::translate("DASignalFormSpecBuilder", "Peak Search"), items));  // cn:峰值查找
    }

    // ---------------- 标定 ----------------
    {
        QJsonArray items;
        items.append(withRange(makeField(QStringLiteral("calibration"),
                                         QStringLiteral("float"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Calibration factor"),  // cn:标定系数
                                         current.calibration),
                               -1.0e9,
                               1.0e9,
                               0.1,
                               6));
        items.append(makeField(QStringLiteral("unit"), QStringLiteral("string"), QCoreApplication::translate("DASignalFormSpecBuilder", "Amplitude unit"), current.unit));
        // cn:幅值单位
        topItems.append(makeGroup(QStringLiteral("calibration"), QCoreApplication::translate("DASignalFormSpecBuilder", "Calibration"), items));  // cn:标定
    }

    QJsonObject spec;
    spec[ "version" ] = 2;
    spec[ "title" ]   = QCoreApplication::translate("DASignalFormSpecBuilder", "Signal Analysis Settings");  // cn:信号分析设置
    spec[ "items" ]   = topItems;
    return spec;
}
}  // namespace DA
