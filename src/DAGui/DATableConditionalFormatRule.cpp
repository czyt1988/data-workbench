#include "DATableConditionalFormatRule.h"
#include "da_qt5qt6_compat.hpp"
#include <QMetaType>
#include <QtMath>

namespace DA
{

/**
 * @brief 构造函数：默认单元格规则（大于），色阶/数据条默认色取官方色板
 */
DATableConditionalFormatRule::DATableConditionalFormatRule()
{
}

/**
 * @brief 值能否严格转数值
 *
 * invalid/null/NaN/无法整串解析（如 "123abc"、日期时间）一律 false。
 * @param v 值
 * @param out 非空时输出数值
 * @return 可转数值返回 true
 */
bool DATableConditionalFormatRule::toNumeric(const QVariant& v, double* out)
{
    if (!v.isValid() || v.isNull()) {
        return false;
    }
    bool ok      = false;
    const double d = v.toDouble(&ok);
    if (!ok || qIsNaN(d)) {
        return false;
    }
    if (out) {
        *out = d;
    }
    return true;
}

/**
 * @brief 值是否为"空"（invalid/null/NaN/空字符串）
 * @param v 值
 * @return 空返回 true
 */
bool DATableConditionalFormatRule::isCellEmpty(const QVariant& v)
{
    if (!v.isValid() || v.isNull()) {
        return true;
    }
    bool ok      = false;
    const double d = v.toDouble(&ok);
    if (ok && qIsNaN(d)) {
        return true;
    }
    if (v.userType() == QMetaType::QString) {
        return v.toString().isEmpty();
    }
    return false;
}

/**
 * @brief 条件求值（纯函数，语义钉死见头文件注释）
 *
 * 空单元格除 IsEmpty/IsNotEmpty 外一律不命中；双方都能严格转数值 → 数值比较，
 * 否则按 toString 字符串比较；Greater/Less 严格、Between 闭区间、Contains 区分大小写。
 * @param cond 操作符
 * @param cellValue 单元格原始值（Qt::EditRole）
 * @param v1 参数 1（Between 为下界）
 * @param v2 参数 2（仅 Between 使用）
 * @return 命中返回 true
 */
bool DATableConditionalFormatRule::evaluateCondition(Condition cond, const QVariant& cellValue, const QVariant& v1, const QVariant& v2)
{
    if (cond == Condition::IsEmpty) {
        return isCellEmpty(cellValue);
    }
    if (cond == Condition::IsNotEmpty) {
        return !isCellEmpty(cellValue);
    }
    if (isCellEmpty(cellValue)) {
        return false;  // 其余操作符：空值不命中
    }
    double cellNum = 0.0;
    double p1Num   = 0.0;
    const bool cellIsNum = toNumeric(cellValue, &cellNum);
    const bool p1IsNum   = toNumeric(v1, &p1Num);
    switch (cond) {
    case Condition::Greater:
        if (!v1.isValid()) {
            return false;
        }
        if (cellIsNum && p1IsNum) {
            return cellNum > p1Num;
        }
        return cellValue.toString() > v1.toString();
    case Condition::Less:
        if (!v1.isValid()) {
            return false;
        }
        if (cellIsNum && p1IsNum) {
            return cellNum < p1Num;
        }
        return cellValue.toString() < v1.toString();
    case Condition::Between: {
        if (!v1.isValid() || !v2.isValid()) {
            return false;
        }
        double p2Num       = 0.0;
        const bool p2IsNum = toNumeric(v2, &p2Num);
        if (cellIsNum && p1IsNum && p2IsNum) {
            return cellNum >= p1Num && cellNum <= p2Num;  // 闭区间
        }
        const QString cs = cellValue.toString();
        return cs >= v1.toString() && cs <= v2.toString();
    }
    case Condition::Equal:
        if (!v1.isValid()) {
            return false;
        }
        if (cellIsNum && p1IsNum) {
            return cellNum == p1Num;
        }
        return cellValue.toString() == v1.toString();
    case Condition::Contains:
        if (!v1.isValid()) {
            return false;
        }
        return cellValue.toString().contains(v1.toString(), Qt::CaseSensitive);
    default:
        break;
    }
    return false;
}

/**
 * @brief 归一化位置：hi>lo → (v-lo)/(hi-lo) 截断 [0,1]；hi==lo 退化返回 0.5
 * @param value 值
 * @param lo 下端点
 * @param hi 上端点
 * @return [0,1] 归一化位置
 */
double DATableConditionalFormatRule::normalizedPosition(double value, double lo, double hi)
{
    if (hi > lo) {
        return qBound(0.0, (value - lo) / (hi - lo), 1.0);
    }
    return 0.5;  // 退化（含 hi<lo 的异常输入）取半程，与数据条 §1.4-1 一致
}

/**
 * @brief 颜色线性插值（RGB 分量各自线性，t 截断 [0,1]）
 * @param from 起始色
 * @param to 终止色
 * @param t 插值位置
 * @return 插值色
 */
QColor DATableConditionalFormatRule::lerpColor(const QColor& from, const QColor& to, double t)
{
    const double k = qBound(0.0, t, 1.0);
    int r1 = 0, g1 = 0, b1 = 0, a1 = 0;
    int r2 = 0, g2 = 0, b2 = 0, a2 = 0;
    from.getRgb(&r1, &g1, &b1, &a1);
    to.getRgb(&r2, &g2, &b2, &a2);
    auto mix = [ k ](int a, int b) {
        return qRound(a + (b - a) * k);
    };
    return QColor(mix(r1, r2), mix(g1, g2), mix(b1, b2), mix(a1, a2));
}

/**
 * @brief 色阶取色
 *
 * 双色：按 [vmin,vmax] 归一化后 cmin→cmax 插值；
 * 三色：value<=vmed 段 cmin→cmid（区间 [vmin,vmed]），否则 cmid→cmax（区间 [vmed,vmax]）；
 * 退化区间（端点相等）由 normalizedPosition 取 0.5。前景色不因色阶变化（可读性）。
 * @param value 单元格数值
 * @param vmin 列最小值
 * @param vmax 列最大值
 * @param vmed 列中位点（三色用）
 * @param threeColor 是否三色
 * @param cmin 低端色
 * @param cmid 中点色
 * @param cmax 高端色
 * @return 背景色
 */
QColor DATableConditionalFormatRule::colorScaleColor(double value, double vmin, double vmax, double vmed, bool threeColor,
                                                     const QColor& cmin, const QColor& cmid, const QColor& cmax)
{
    if (!threeColor) {
        return lerpColor(cmin, cmax, normalizedPosition(value, vmin, vmax));
    }
    if (value <= vmed) {
        return lerpColor(cmin, cmid, normalizedPosition(value, vmin, vmed));
    }
    return lerpColor(cmid, cmax, normalizedPosition(value, vmed, vmax));
}

/**
 * @brief 数据条归一化条长比例：(v-min)/(max-min) 截断 [0,1]，max==min 退化取 0.5
 * @param value 单元格数值
 * @param vmin 列最小值
 * @param vmax 列最大值
 * @return 条长比例
 */
double DATableConditionalFormatRule::dataBarRatio(double value, double vmin, double vmax)
{
    return normalizedPosition(value, vmin, vmax);
}

/**
 * @brief 序列化为 <rule> 元素追加到 parent（短属性名，照样式段风格）
 * @param doc QDomDocument
 * @param parent 父元素（<column>）
 */
void DATableConditionalFormatRule::toXml(QDomDocument& doc, QDomElement& parent) const
{
    QDomElement re = doc.createElement(QStringLiteral("rule"));
    re.setAttribute(QStringLiteral("type"), typeToString(type));
    if (!enabled) {
        re.setAttribute(QStringLiteral("enabled"), 0);
    }
    switch (type) {
    case Type::CellRule: {
        re.setAttribute(QStringLiteral("cond"), conditionToString(condition));
        if (value1.isValid()) {
            re.setAttribute(QStringLiteral("v1"), value1.toString());
        }
        if (value2.isValid()) {
            re.setAttribute(QStringLiteral("v2"), value2.toString());
        }
        style.toXml(doc, re);  // <background>/<foreground>/<font> 子元素
        break;
    }
    case Type::ColorScale:
        re.setAttribute(QStringLiteral("three"), threeColorScale ? 1 : 0);
        re.setAttribute(QStringLiteral("cmin"), scaleMinColor.name(QColor::HexArgb));
        re.setAttribute(QStringLiteral("cmid"), scaleMidColor.name(QColor::HexArgb));
        re.setAttribute(QStringLiteral("cmax"), scaleMaxColor.name(QColor::HexArgb));
        break;
    case Type::DataBar:
        re.setAttribute(QStringLiteral("color"), barColor.name(QColor::HexArgb));
        break;
    }
    parent.appendChild(re);
}

/**
 * @brief 从 <rule> 元素反序列化（未知属性忽略，前向兼容）
 * @param e rule 元素
 * @return 类型合法返回 true；非法类型返回 false（对象保持复位后的默认值）
 */
bool DATableConditionalFormatRule::fromXml(const QDomElement& e)
{
    Type t = Type::CellRule;
    if (!typeFromString(e.attribute(QStringLiteral("type")), &t)) {
        return false;
    }
    *this   = DATableConditionalFormatRule();
    type    = t;
    enabled = e.attribute(QStringLiteral("enabled"), QStringLiteral("1")).toInt() != 0;
    switch (t) {
    case Type::CellRule: {
        Condition c = Condition::Greater;
        if (conditionFromString(e.attribute(QStringLiteral("cond"), QStringLiteral("greater")), &c)) {
            condition = c;
        }
        if (e.hasAttribute(QStringLiteral("v1"))) {
            value1 = e.attribute(QStringLiteral("v1"));
        }
        if (e.hasAttribute(QStringLiteral("v2"))) {
            value2 = e.attribute(QStringLiteral("v2"));
        }
        style.fromXml(e);
        break;
    }
    case Type::ColorScale: {
        threeColorScale = e.attribute(QStringLiteral("three"), QStringLiteral("0")).toInt() != 0;
        QColor c;
        compat::setNamedColor(c, e.attribute(QStringLiteral("cmin")));
        if (c.isValid()) {
            scaleMinColor = c;
        }
        compat::setNamedColor(c, e.attribute(QStringLiteral("cmid")));
        if (c.isValid()) {
            scaleMidColor = c;
        }
        compat::setNamedColor(c, e.attribute(QStringLiteral("cmax")));
        if (c.isValid()) {
            scaleMaxColor = c;
        }
        break;
    }
    case Type::DataBar: {
        QColor c;
        compat::setNamedColor(c, e.attribute(QStringLiteral("color")));
        if (c.isValid()) {
            barColor = c;
        }
        break;
    }
    }
    return true;
}

/**
 * @brief 类型 → XML 字符串
 * @param t 类型
 * @return 字符串
 */
QString DATableConditionalFormatRule::typeToString(Type t)
{
    switch (t) {
    case Type::CellRule:
        return QStringLiteral("cell");
    case Type::ColorScale:
        return QStringLiteral("color-scale");
    case Type::DataBar:
        return QStringLiteral("data-bar");
    }
    return QStringLiteral("cell");
}

/**
 * @brief XML 字符串 → 类型
 * @param s 字符串
 * @param t 输出类型
 * @return 合法返回 true
 */
bool DATableConditionalFormatRule::typeFromString(const QString& s, Type* t)
{
    if (s == QLatin1String("cell")) {
        *t = Type::CellRule;
    } else if (s == QLatin1String("color-scale")) {
        *t = Type::ColorScale;
    } else if (s == QLatin1String("data-bar")) {
        *t = Type::DataBar;
    } else {
        return false;
    }
    return true;
}

/**
 * @brief 条件操作符 → XML 字符串
 * @param c 操作符
 * @return 字符串
 */
QString DATableConditionalFormatRule::conditionToString(Condition c)
{
    switch (c) {
    case Condition::Greater:
        return QStringLiteral("greater");
    case Condition::Less:
        return QStringLiteral("less");
    case Condition::Between:
        return QStringLiteral("between");
    case Condition::Equal:
        return QStringLiteral("equal");
    case Condition::Contains:
        return QStringLiteral("contains");
    case Condition::IsEmpty:
        return QStringLiteral("empty");
    case Condition::IsNotEmpty:
        return QStringLiteral("notempty");
    }
    return QStringLiteral("greater");
}

/**
 * @brief XML 字符串 → 条件操作符
 * @param s 字符串
 * @param c 输出操作符
 * @return 合法返回 true
 */
bool DATableConditionalFormatRule::conditionFromString(const QString& s, Condition* c)
{
    if (s == QLatin1String("greater")) {
        *c = Condition::Greater;
    } else if (s == QLatin1String("less")) {
        *c = Condition::Less;
    } else if (s == QLatin1String("between")) {
        *c = Condition::Between;
    } else if (s == QLatin1String("equal")) {
        *c = Condition::Equal;
    } else if (s == QLatin1String("contains")) {
        *c = Condition::Contains;
    } else if (s == QLatin1String("empty")) {
        *c = Condition::IsEmpty;
    } else if (s == QLatin1String("notempty")) {
        *c = Condition::IsNotEmpty;
    } else {
        return false;
    }
    return true;
}

/**
 * @brief 相等比较（全字段；命令记录与单测 round-trip 用）
 * @param other 另一规则
 * @return 全字段相等返回 true
 */
bool DATableConditionalFormatRule::operator==(const DATableConditionalFormatRule& other) const
{
    return type == other.type
           && condition == other.condition
           && value1 == other.value1
           && value2 == other.value2
           && style.background() == other.style.background()
           && style.backgroundValid() == other.style.backgroundValid()
           && style.foreground() == other.style.foreground()
           && style.foregroundValid() == other.style.foregroundValid()
           && style.font() == other.style.font()
           && style.fontValid() == other.style.fontValid()
           && scaleMinColor == other.scaleMinColor
           && scaleMidColor == other.scaleMidColor
           && scaleMaxColor == other.scaleMaxColor
           && threeColorScale == other.threeColorScale
           && barColor == other.barColor
           && enabled == other.enabled;
}

/**
 * @brief 不等比较
 * @param other 另一规则
 * @return 任一字段不等返回 true
 */
bool DATableConditionalFormatRule::operator!=(const DATableConditionalFormatRule& other) const
{
    return !(*this == other);
}

}  // end of namespace DA
