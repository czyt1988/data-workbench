#include "DAFormRuleEvaluator.h"

namespace DA
{
namespace
{
/**
 * @brief 求值单条规则表达式
 *
 * 表达式形如 `<field> <op> <literal>`，op 为 == 或 !=，literal 为引号字符串、布尔或整数。
 * - 表达式为空时返回 defaultValue。
 * - 表达式非法时同样返回 defaultValue（不抛异常）。
 * - 字段在 values 中缺失时，== 视为不匹配，!= 视为匹配。
 * @param expression 规则表达式
 * @param values 当前字段值集合
 * @param defaultValue 表达式为空或非法时的回退值
 * @return 规则求值结果
 */
bool evaluateRule(const QString& expression, const QVariantMap& values, bool defaultValue)
{
    QString expr = expression.trimmed();
    if (expr.isEmpty()) {
        return defaultValue;
    }

    // 查找首个 == 或 != 出现位置，取更早者作为运算符
    int eqPos  = expr.indexOf("==");
    int neqPos = expr.indexOf("!=");
    int opPos       = -1;
    bool isNotEqual = false;
    if (eqPos >= 0 && neqPos >= 0) {
        if (eqPos <= neqPos) {
            opPos = eqPos;
        } else {
            opPos       = neqPos;
            isNotEqual  = true;
        }
    } else if (eqPos >= 0) {
        opPos = eqPos;
    } else if (neqPos >= 0) {
        opPos      = neqPos;
        isNotEqual = true;
    } else {
        // 未找到运算符，视为非法
        return defaultValue;
    }

    QString lhs = expr.left(opPos).trimmed();
    QString rhs = expr.mid(opPos + 2).trimmed();
    if (lhs.isEmpty() || rhs.isEmpty()) {
        return defaultValue;
    }

    bool fieldPresent = values.contains(lhs);
    QVariant fieldValue = values.value(lhs);

    // 字符串字面量：单引号或双引号包裹
    if (rhs.size() >= 2) {
        QChar first = rhs.at(0);
        if ((first == QLatin1Char('\'') || first == QLatin1Char('"')) && rhs.at(rhs.size() - 1) == first) {
            QString literal = rhs.mid(1, rhs.size() - 2);
            bool match = fieldPresent && (fieldValue.toString() == literal);
            return isNotEqual ? !match : match;
        }
    }

    // 布尔字面量
    if (rhs == QStringLiteral("true") || rhs == QStringLiteral("false")) {
        bool literalBool = (rhs == QStringLiteral("true"));
        bool match = fieldPresent && (fieldValue.toBool() == literalBool);
        return isNotEqual ? !match : match;
    }

    // 整数字面量
    bool ok = false;
    int literalInt = rhs.toInt(&ok);
    if (ok) {
        bool match = fieldPresent && (fieldValue.toInt() == literalInt);
        return isNotEqual ? !match : match;
    }

    // 无法识别的字面量，视为非法
    return defaultValue;
}
}  // namespace

/**
 * @brief 根据字段定义与当前值集合求值字段的联动状态
 *
 * 分别对 visibleWhen/enabledWhen/requiredWhen 求值：
 * - visible 来自 visibleWhen，缺省/非法为 true。
 * - enabled 来自 enabledWhen，缺省/非法为 true。
 * - required 来自 requiredWhen，缺省/非法为 false。
 * @param field 字段定义
 * @param values 当前字段值集合
 * @return 字段的联动状态
 */
DAFormFieldState DAFormRuleEvaluator::evaluate(const DAFormFieldDef& field, const QVariantMap& values)
{
    DAFormFieldState state;
    state.visible  = evaluateRule(field.visibleWhen, values, true);
    state.enabled  = evaluateRule(field.enabledWhen, values, true);
    state.required = evaluateRule(field.requiredWhen, values, false);
    return state;
}

}  // namespace DA
