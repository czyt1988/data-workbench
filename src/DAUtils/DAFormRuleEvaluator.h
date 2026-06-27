#ifndef DAFORMRULEEVALUATOR_H
#define DAFORMRULEEVALUATOR_H
#include "DAUtilsAPI.h"
#include "DAFormSpec.h"
#include <QVariantMap>

/**
 * @file 此文件声明了表单字段联动规则的声明式求值器
 */
namespace DA
{
/**
 * @brief 表单字段的联动状态
 *
 * 由 DAFormRuleEvaluator 根据 visibleWhen/enabledWhen/requiredWhen 规则表达式
 * 结合当前字段值集合求值得到，描述字段在 UI 中的可见性、启用性与必填性。
 */
struct DAUTILS_API DAFormFieldState
{
    bool visible { true };    // 是否可见，缺省为 true
    bool enabled { true };    // 是否启用，缺省为 true
    bool required { false };  // 是否必填，缺省为 false
};

/**
 * @brief 表单字段联动规则的声明式求值器
 *
 * 仅支持简单比较（== / !=），不涉及 Python 或脚本。每条规则表达式形如：
 * `<field> <op> <literal>`，其中 literal 可为单/双引号字符串、布尔（true/false）或整数。
 *
 * - 表达式为空时取对应状态的缺省值（visible/enabled 为 true，required 为 false）。
 * - 表达式非法时不抛出异常，按缺省值处理。
 * - 字段在值集合中缺失时，对 == 视为不匹配，对 != 视为匹配。
 */
class DAUTILS_API DAFormRuleEvaluator
{
public:
    // 根据字段定义与当前值集合求值字段的联动状态
    static DAFormFieldState evaluate(const DAFormFieldDef& field, const QVariantMap& values);
};
}  // namespace DA
#endif  // DAFORMRULEEVALUATOR_H
