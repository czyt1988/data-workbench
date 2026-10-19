#ifndef DATABLEVALIDATIONRULE_H
#define DATABLEVALIDATIONRULE_H
#include "DADataAPI.h"
#include <QString>
#include <QStringList>
#include <QDateTime>

namespace DA
{
/**
 * @brief 数据校验规则（纯数据类，无 GUI 依赖，005 T1.25）
 *
 * 列级校验规则定义：数值区间 / 日期区间 / 枚举集合 / 非空 / 唯一 / 正则，每条带
 * 「错误/警告」级别与启用开关。定义类放 src/DAData 是为将来被非 GUI 消费留位
 * （T4.9 校验规则引擎工作流节点直接复用，设计稿 §4.9 落位 / D3 语义类归属）；
 * v1 的集合存储与序列化在 DAGui 的 DATableValidationStore（元数据束成员），
 * 求值走 DAPyScriptsDataFrame 的 da_validate_dataframe 算子（Python 判定空值与比较）。
 *
 * 列引用（R20 语义类）：按**列名** + columnOccurrence（第 n 个同名列，0 起）消歧——
 * pandas 允许重名列且插入无重名检查，必须显式处理；列改名命令负责迁移并提示影响面。
 *
 * 求值语义（钉死，与 da_validate_dataframe 一致，单测钉住）：
 * - 空值判定一律由 pandas 完成（NaN/NaT/None，C++ QVariant 不可靠，红线1）；
 * - NumericRange：值可转数值且越界 → 违规；空值与非数值**不判**（空值交给 NotEmpty 规则）；
 * - DateRange：值可转日期且越界 → 违规；空值不判；区间为闭区间 [min,max]；
 * - EnumSet：值不在集合内 → 违规（**空值也违规**，需要允许空时不启用本规则或另加规则）；
 * - NotEmpty：空值 → 违规；
 * - Unique：列内重复（duplicated keep=False，重复组的**全部**行都算违规，便于定位）；
 * - Regex：整格完全匹配（fullmatch）不通过 → 违规；空值不判；正则非法整条规则报错。
 */
class DADATA_API DATableValidationRule
{
public:
    // 规则类型
    enum class Type
    {
        NumericRange,  ///< 数值区间（闭区间，可只设一端）
        DateRange,     ///< 日期区间（闭区间，可只设一端）
        EnumSet,       ///< 枚举集合（值必须在集合内）
        NotEmpty,      ///< 非空
        Unique,        ///< 唯一（重复组全部行违规）
        Regex          ///< 正则（整格完全匹配）
    };
    // 违规级别（对应瞬态高亮层 Level::ViolationError / ViolationWarning）
    enum class Level
    {
        Error,    ///< 错误（橙红 #CE6043）
        Warning   ///< 警告（金黄 #E6C27C）
    };

    DATableValidationRule();

    QString columnName;          ///< 列名（R20 语义类引用）
    int columnOccurrence { 0 };  ///< 第 n 个同名列（0 起，重名消歧，R20）
    Type type { Type::NotEmpty };
    Level level { Level::Error };
    bool enabled { true };       ///< 启用开关（停用后校验跳过，定义保留）
    QString label;               ///< 规则显示名（报告分组用；空 = 调用方按类型+列自动生成）

    // ===== 类型参数（按 type 取用；未用字段不参与比较）=====
    double numericMin { 0.0 };   ///< 数值下界（NumericRange）
    double numericMax { 0.0 };   ///< 数值上界（NumericRange）
    bool hasNumericMin { false };  ///< 是否设下界（不设 = 不限）
    bool hasNumericMax { false };  ///< 是否设上界
    QDateTime dateMin;           ///< 日期下界（DateRange，无效 = 不限）
    QDateTime dateMax;           ///< 日期上界（DateRange，无效 = 不限）
    QStringList enumValues;      ///< 枚举允许值集合（EnumSet）
    bool enumAllowOther { true };  ///< 编辑下拉是否允许其他值（delegate createEditor 用，
                                 ///< 只影响编辑交互，不影响校验判定，§4.9）
    QString regexPattern;        ///< 正则表达式（Regex，fullmatch 语义）

    // 定义是否完整可用（列名非空 + 各类型必要参数齐备）
    bool isValid() const;
    // 参数摘要（报告/列表显示用，如 "[0, 100]"、"{a, b, c}"；已翻译类型名不含参数值）
    QString paramSummary() const;

    // 枚举 ↔ 字符串（XML 序列化的稳定英文标识，勿翻译）
    static QString typeToString(Type t);
    static bool typeFromString(const QString& s, Type* t);
    static QString levelToString(Level l);
    static bool levelFromString(const QString& s, Level* l);

    bool operator==(const DATableValidationRule& other) const;
    bool operator!=(const DATableValidationRule& other) const;
};
}  // end of namespace DA
#endif  // DATABLEVALIDATIONRULE_H
