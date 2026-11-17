#include "DASignalFormSpecBuilder.h"
#include <QJsonArray>
#include <QJsonValue>
#include <QCoreApplication>

namespace DA
{
namespace
{
QJsonObject enumOption(const QVariant& value, const QString& label)
{
    QJsonObject o;
    o[ "value" ] = QJsonValue::fromVariant(value);
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
    // 常用参数（预处理 / 窗函数 / 幅值口径 / 分辨率 / STFT 窗长与跳步）已搬上丝带，
    // 本对话框只保留高级项，避免同一设置两处入口、两个真相（D8 / 铁律 10）。
    QJsonArray topItems;

    // ---------------- 频谱（高级） ----------------
    {
        QJsonArray items;
        items.append(withRange(makeField(QStringLiteral("zeroPadding"),
                                         QStringLiteral("int"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Zero padding points (advanced)"),  // cn:零填充点数（高级）
                                         current.zeroPadding),
                               0,
                               1 << 20,
                               1024,
                               0));
        topItems.append(makeGroup(QStringLiteral("spectrum"),
                                  QCoreApplication::translate("DASignalFormSpecBuilder", "Spectrum (Advanced)"),  // cn:频谱（高级）
                                  items));
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
        topItems.append(makeGroup(QStringLiteral("psd"),
                                  QCoreApplication::translate("DASignalFormSpecBuilder", "Power Spectrum"),  // cn:功率谱
                                  items));
    }

    // ---------------- 时频（高级） ----------------
    {
        QJsonArray items;
        items.append(withRange(makeField(QStringLiteral("stftZeroPadding"),
                                         QStringLiteral("int"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Zero padding points (advanced)"),  // cn:零填充点数（高级）
                                         current.stftZeroPadding),
                               0,
                               1 << 20,
                               1024,
                               0));
        topItems.append(makeGroup(QStringLiteral("stft"),
                                  QCoreApplication::translate("DASignalFormSpecBuilder", "Time-Frequency (Advanced)"),  // cn:时频（高级）
                                  items));
    }

    // ---------------- 峰值查找（高级） ----------------
    // 模式 / 标记数量 / 相对门限在丝带「峰值」panel；这里只保留「模式 = 高级」时生效的三个参数。
    {
        QJsonArray items;
        items.append(withRange(makeField(QStringLiteral("peakProminence"),
                                         QStringLiteral("float"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Prominence (advanced)"),  // cn:突出度（高级）
                                         current.peakProminence),
                               0.0,
                               1.0e9,
                               0.01,
                               6));
        // 最小间距默认 5 倍频率分辨率（D17）；参数为 -1/0（自动哨兵）时换算为派生值，
        // 避免表单 min=0 把 -1 夹成 0（评审 N-U6/N-C5）
        const double spacingDefault = signalEffectivePeakMinSpacingHz(current, derivedResolutionHz);
        items.append(withRange(makeField(QStringLiteral("peakMinSpacingHz"),
                                         QStringLiteral("float"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Minimum spacing (Hz, advanced)"),  // cn:最小间距（赫兹，高级）
                                         spacingDefault),
                               0.0,
                               1.0e9,
                               1.0,
                               4));
        items.append(withRange(makeField(QStringLiteral("peakMinHeight"),
                                         QStringLiteral("float"),
                                         QCoreApplication::translate("DASignalFormSpecBuilder", "Minimum height (advanced)"),  // cn:绝对高度门限（高级）
                                         current.peakMinHeight),
                               0.0,
                               1.0e9,
                               0.01,
                               6));
        topItems.append(makeGroup(QStringLiteral("peaks"),
                                  QCoreApplication::translate("DASignalFormSpecBuilder", "Peak Search (Advanced)"),  // cn:峰值查找（高级）
                                  items));
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
    spec[ "title" ] = QCoreApplication::translate("DASignalFormSpecBuilder", "Advanced Analysis Settings");  // cn:高级分析设置
    spec[ "items" ] = topItems;
    return spec;
}
}  // namespace DA