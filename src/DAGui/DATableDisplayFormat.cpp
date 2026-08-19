#include "DATableDisplayFormat.h"
#include <QDateTime>
#include <QMetaType>

namespace DA
{

/**
 * @brief 构造一个指定类别的格式
 * @param c 类别
 */
DATableDisplayFormat::DATableDisplayFormat(Category c) : mCategory(c), mValid(true)
{
}

DATableDisplayFormat::Category DATableDisplayFormat::category() const
{
    return mCategory;
}

void DATableDisplayFormat::setCategory(Category c)
{
    mCategory = c;
}

int DATableDisplayFormat::precision() const
{
    return mPrecision;
}

void DATableDisplayFormat::setPrecision(int p)
{
    mPrecision = p;
}

QString DATableDisplayFormat::pattern() const
{
    return mPattern;
}

void DATableDisplayFormat::setPattern(const QString& p)
{
    mPattern = p;
}

DATableDisplayFormat::EpochUnit DATableDisplayFormat::epochUnit() const
{
    return mEpochUnit;
}

void DATableDisplayFormat::setEpochUnit(EpochUnit u)
{
    mEpochUnit = u;
}

bool DATableDisplayFormat::isValid() const
{
    return mValid;
}

void DATableDisplayFormat::setValid(bool v)
{
    mValid = v;
}

/**
 * @brief 把原始 QVariant 格式化为显示文本
 *
 * - 无效或 General/Text：返回 raw.toString()（General 等价于 Qt 默认显示）
 * - QDateTime + DateTime：按 pattern 格式化
 * - QDateTime + DatetimeAsNumber：epoch 秒/毫秒
 * - 数值 + Number/Scientific/Percent：定点/科学/百分比
 * - 类别与值类型不匹配时回退 raw.toString()
 * @param raw 原始值（来自 DAPyDataFrame::iat 经 pybind caster 转换）
 * @return 显示文本
 */
QString DATableDisplayFormat::formatValue(const QVariant& raw) const
{
    if (!mValid || mCategory == General || mCategory == Text) {
        return raw.toString();
    }
    // 日期时间类
    if (raw.userType() == QMetaType::QDateTime) {
        QDateTime dt = raw.toDateTime();
        if (!dt.isValid()) {
            return raw.toString();
        }
        switch (mCategory) {
        case DateTime: {
            QString p = mPattern.isEmpty() ? defaultDateTimePattern() : mPattern;
            return dt.toString(p);
        }
        case DatetimeAsNumber:
            return (mEpochUnit == EpochSeconds) ? QString::number(dt.toSecsSinceEpoch())
                                                : QString::number(dt.toMSecsSinceEpoch());
        default:
            break;  // 数值类别不适用于日期，回退
        }
        return raw.toString();
    }
    // 数值类（double/int，统一按 double 处理）
    bool ok = false;
    double v = raw.toDouble(&ok);
    if (!ok) {
        return raw.toString();
    }
    switch (mCategory) {
    case Number:
        return QString::number(v, 'f', mPrecision);
    case Scientific:
        return QString::number(v, 'e', mPrecision);
    case Percent:
        return QString::number(v * 100.0, 'f', mPrecision) + QChar('%');
    default:
        break;
    }
    return raw.toString();
}

/**
 * @brief 常用日期时间 pattern 预设
 * @return 预设列表（英文 pattern 串）
 */
QStringList DATableDisplayFormat::dateTimePresets()
{
    return QStringList() << QStringLiteral("yyyy-MM-dd")
                         << QStringLiteral("yyyy-MM-dd HH:mm:ss")
                         << QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")
                         << QStringLiteral("yyyy/MM/dd")
                         << QStringLiteral("HH:mm:ss")
                         << QStringLiteral("yyyy-MM-ddTHH:mm:ss");
}

QString DATableDisplayFormat::defaultDateTimePattern()
{
    return QStringLiteral("yyyy-MM-dd HH:mm:ss");
}

QString DATableDisplayFormat::categoryToString(Category c)
{
    switch (c) {
    case General:
        return QStringLiteral("General");
    case Number:
        return QStringLiteral("Number");
    case Scientific:
        return QStringLiteral("Scientific");
    case Percent:
        return QStringLiteral("Percent");
    case DateTime:
        return QStringLiteral("DateTime");
    case DatetimeAsNumber:
        return QStringLiteral("DatetimeAsNumber");
    case Text:
        return QStringLiteral("Text");
    }
    return QStringLiteral("General");
}

DATableDisplayFormat::Category DATableDisplayFormat::stringToCategory(const QString& s, bool* ok)
{
    if (ok) *ok = true;
    if (s == QLatin1String("General")) return General;
    if (s == QLatin1String("Number")) return Number;
    if (s == QLatin1String("Scientific")) return Scientific;
    if (s == QLatin1String("Percent")) return Percent;
    if (s == QLatin1String("DateTime")) return DateTime;
    if (s == QLatin1String("DatetimeAsNumber")) return DatetimeAsNumber;
    if (s == QLatin1String("Text")) return Text;
    if (ok) *ok = false;
    return General;
}

QString DATableDisplayFormat::epochUnitToString(EpochUnit u)
{
    return (u == EpochSeconds) ? QStringLiteral("seconds") : QStringLiteral("milliseconds");
}

DATableDisplayFormat::EpochUnit DATableDisplayFormat::stringToEpochUnit(const QString& s, bool* ok)
{
    if (ok) *ok = true;
    if (s == QLatin1String("seconds")) return EpochSeconds;
    if (s == QLatin1String("milliseconds")) return EpochMilliseconds;
    if (ok) *ok = false;
    return EpochMilliseconds;
}

bool DATableDisplayFormat::operator==(const DATableDisplayFormat& other) const
{
    if (mValid != other.mValid) return false;
    if (!mValid) return true;  // 两者都无效
    return mCategory == other.mCategory && mPrecision == other.mPrecision && mPattern == other.mPattern
           && mEpochUnit == other.mEpochUnit;
}

bool DATableDisplayFormat::operator!=(const DATableDisplayFormat& other) const
{
    return !(*this == other);
}

/**
 * @brief 序列化：把属性写到元素 e 上
 *
 * manager 会包成 <col-format index="N">，本函数写 category/precision/pattern/epochunit 属性。
 * @param doc QDomDocument（未直接使用，保留以与项目其他 toXml 签名一致）
 * @param e 目标元素
 */
void DATableDisplayFormat::toXml(QDomDocument& doc, QDomElement& e) const
{
    Q_UNUSED(doc);
    e.setAttribute(QStringLiteral("category"), categoryToString(mCategory));
    e.setAttribute(QStringLiteral("precision"), mPrecision);
    e.setAttribute(QStringLiteral("pattern"), mPattern);
    e.setAttribute(QStringLiteral("epochunit"), epochUnitToString(mEpochUnit));
}

/**
 * @brief 反序列化：从元素读取属性
 *
 * 缺失属性使用合理默认值。解析后置 valid=true。
 * @param e 源元素
 * @return 始终返回 true
 */
bool DATableDisplayFormat::fromXml(const QDomElement& e)
{
    bool ok = false;
    mCategory   = stringToCategory(e.attribute(QStringLiteral("category"), "General"), &ok);
    mPrecision  = e.attribute(QStringLiteral("precision"), "2").toInt();
    mPattern    = e.attribute(QStringLiteral("pattern"));
    mEpochUnit  = stringToEpochUnit(e.attribute(QStringLiteral("epochunit"), "milliseconds"), &ok);
    mValid      = true;
    return true;
}

}  // end of namespace DA
