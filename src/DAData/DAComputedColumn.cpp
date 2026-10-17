#include "DAComputedColumn.h"

namespace DA
{

/**
 * @brief pandas eval 引擎标识（磁盘契约 engine 字段的 v1 取值）
 * @return 常量字符串 "pandas.eval"
 */
const QString& DAComputedColumn::pandasEvalEngine()
{
    static const QString s = QStringLiteral("pandas.eval");
    return s;
}

/**
 * @brief 默认构造（空定义，isValid() 为 false）
 */
DAComputedColumn::DAComputedColumn() : engine(pandasEvalEngine())
{
}

/**
 * @brief 构造
 * @param target 目标列名
 * @param expression 表达式体（RHS）
 */
DAComputedColumn::DAComputedColumn(const QString& target, const QString& expression)
    : targetColumnName(target), expr(expression), engine(pandasEvalEngine())
{
}

/**
 * @brief 定义是否可用（目标列名与表达式都非空）
 * @return 可用返回 true
 */
bool DAComputedColumn::isValid() const
{
    return !targetColumnName.isEmpty() && !expr.isEmpty();
}

/**
 * @brief 表达式是否引用了目标列（自覆盖，R7 禁止）
 *
 * 依据已解析的依赖列名列表判断——dependsOn 由 da_expr_depends 提取（正确处理反引号
 * 与空格列名），比在原始表达式文本上做子串匹配更可靠（避免 "AA" 命中 "A" 的误判）。
 * @return 自覆盖返回 true
 */
bool DAComputedColumn::isSelfReferencing() const
{
    return !targetColumnName.isEmpty() && dependsOn.contains(targetColumnName);
}

/**
 * @brief 相等比较（全字段）
 */
bool DAComputedColumn::operator==(const DAComputedColumn& other) const
{
    return targetColumnName == other.targetColumnName && expr == other.expr && engine == other.engine
           && version == other.version && dependsOn == other.dependsOn && enabled == other.enabled;
}

/**
 * @brief 不等比较
 */
bool DAComputedColumn::operator!=(const DAComputedColumn& other) const
{
    return !(*this == other);
}

}  // end of namespace DA
