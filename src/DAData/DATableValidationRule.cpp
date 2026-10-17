#include "DATableValidationRule.h"

namespace DA
{

/**
 * @brief 默认构造（非空规则，Error 级别）
 */
DATableValidationRule::DATableValidationRule()
{
}

/**
 * @brief 定义是否完整可用（列名非空 + 各类型必要参数齐备）
 * @return 可用返回 true
 */
bool DATableValidationRule::isValid() const
{
    if (columnName.isEmpty()) {
        return false;
    }
    switch (type) {
    case Type::NumericRange:
        // 至少要设一端；两端都设时下界 <= 上界
        if (!hasNumericMin && !hasNumericMax) {
            return false;
        }
        if (hasNumericMin && hasNumericMax && numericMin > numericMax) {
            return false;
        }
        return true;
    case Type::DateRange:
        if (!dateMin.isValid() && !dateMax.isValid()) {
            return false;
        }
        if (dateMin.isValid() && dateMax.isValid() && dateMin > dateMax) {
            return false;
        }
        return true;
    case Type::EnumSet:
        return !enumValues.isEmpty();
    case Type::Regex:
        return !regexPattern.isEmpty();
    case Type::NotEmpty:
    case Type::Unique:
        return true;
    }
    return false;
}

/**
 * @brief 参数摘要（报告/列表显示用，纯符号与值，不含需翻译的文字——显示名由 GUI 侧组合）
 *
 * NumericRange → "[min, max]"（缺省端留空）；DateRange → "[yyyy-MM-dd, yyyy-MM-dd]"；
 * EnumSet → "{a, b, c}"；Regex → 正则式本身；NotEmpty/Unique → 空串。
 * @return 摘要文本
 */
QString DATableValidationRule::paramSummary() const
{
    switch (type) {
    case Type::NumericRange: {
        const QString lo = hasNumericMin ? QString::number(numericMin) : QString();
        const QString hi = hasNumericMax ? QString::number(numericMax) : QString();
        return QStringLiteral("[%1, %2]").arg(lo, hi);
    }
    case Type::DateRange: {
        const QString lo = dateMin.isValid() ? dateMin.toString(QStringLiteral("yyyy-MM-dd")) : QString();
        const QString hi = dateMax.isValid() ? dateMax.toString(QStringLiteral("yyyy-MM-dd")) : QString();
        return QStringLiteral("[%1, %2]").arg(lo, hi);
    }
    case Type::EnumSet:
        return QStringLiteral("{%1}").arg(enumValues.join(QStringLiteral(", ")));
    case Type::Regex:
        return regexPattern;
    case Type::NotEmpty:
    case Type::Unique:
    default:
        return QString();
    }
}

/**
 * @brief 规则类型 → 稳定英文标识（XML 序列化，勿翻译）
 */
QString DATableValidationRule::typeToString(Type t)
{
    switch (t) {
    case Type::NumericRange:
        return QStringLiteral("numeric-range");
    case Type::DateRange:
        return QStringLiteral("date-range");
    case Type::EnumSet:
        return QStringLiteral("enum");
    case Type::NotEmpty:
        return QStringLiteral("not-empty");
    case Type::Unique:
        return QStringLiteral("unique");
    case Type::Regex:
        return QStringLiteral("regex");
    }
    return QStringLiteral("not-empty");
}

/**
 * @brief 稳定英文标识 → 规则类型
 * @return 解析成功返回 true
 */
bool DATableValidationRule::typeFromString(const QString& s, Type* t)
{
    if (!t) {
        return false;
    }
    if (s == QLatin1String("numeric-range")) {
        *t = Type::NumericRange;
    } else if (s == QLatin1String("date-range")) {
        *t = Type::DateRange;
    } else if (s == QLatin1String("enum")) {
        *t = Type::EnumSet;
    } else if (s == QLatin1String("not-empty")) {
        *t = Type::NotEmpty;
    } else if (s == QLatin1String("unique")) {
        *t = Type::Unique;
    } else if (s == QLatin1String("regex")) {
        *t = Type::Regex;
    } else {
        return false;
    }
    return true;
}

/**
 * @brief 违规级别 → 稳定英文标识（XML 序列化，勿翻译）
 */
QString DATableValidationRule::levelToString(Level l)
{
    return l == Level::Warning ? QStringLiteral("warning") : QStringLiteral("error");
}

/**
 * @brief 稳定英文标识 → 违规级别
 * @return 解析成功返回 true
 */
bool DATableValidationRule::levelFromString(const QString& s, Level* l)
{
    if (!l) {
        return false;
    }
    if (s == QLatin1String("warning")) {
        *l = Level::Warning;
    } else if (s == QLatin1String("error")) {
        *l = Level::Error;
    } else {
        return false;
    }
    return true;
}

/**
 * @brief 相等比较（全字段）
 */
bool DATableValidationRule::operator==(const DATableValidationRule& other) const
{
    return columnName == other.columnName && columnOccurrence == other.columnOccurrence && type == other.type
           && level == other.level && enabled == other.enabled && label == other.label
           && numericMin == other.numericMin && numericMax == other.numericMax
           && hasNumericMin == other.hasNumericMin && hasNumericMax == other.hasNumericMax
           && dateMin == other.dateMin && dateMax == other.dateMax && enumValues == other.enumValues
           && enumAllowOther == other.enumAllowOther && regexPattern == other.regexPattern;
}

/**
 * @brief 不等比较
 */
bool DATableValidationRule::operator!=(const DATableValidationRule& other) const
{
    return !(*this == other);
}

}  // end of namespace DA
