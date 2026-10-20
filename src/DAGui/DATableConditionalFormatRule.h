#ifndef DATABLECONDITIONALFORMATRULE_H
#define DATABLECONDITIONALFORMATRULE_H
#include "DAGuiAPI.h"
#include "DATableCellStyle.h"
#include <QColor>
#include <QDate>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
/**
 * @brief 条件格式规则（纯数据类，003 T1.5）
 *
 * 三类规则均按列绑定（列引用为位置键，插删列由 DATableConditionalFormatStore 重映射，R20）：
 * - CellRule 单元格规则：条件（Greater/Less/Between/Equal/Contains/IsEmpty/IsNotEmpty）
 *   命中后输出前景/背景/字体（DATableCellStyle，各属性带 valid 标志，可只设其中若干）；
 * - ColorScale 色阶：双色/三色，按列 min/max（三色含中位点 median）插值填充背景，前景不变；
 * - DataBar 数据条：按列 min/max 归一化条长，背景之上、文字之下绘制（§1.4-1）。
 *
 * 求值语义（钉死，单测钉住，勿改）：
 * - 取值用 Qt::EditRole 原始值；空值（invalid/null/NaN/空串）除 IsEmpty/IsNotEmpty 外一律不命中；
 * - 比较双方都能严格转 double（QVariant::toDouble ok 且非 NaN）→ 数值比较；否则按 toString 字符串比较；
 * - Greater/Less 严格比较（等于不命中）；Between 为闭区间 [v1, v2]；Contains 区分大小写；
 * - 判定失败（如无法转换）视为不命中；
 * - 归一化 t = (v-min)/(max-min) 截断到 [0,1]；max==min 退化取 0.5（色阶与数据条同语义）。
 *
 * enabled=false 的规则跳过求值；同列多条规则按列表顺序求值，后命中的覆盖先命中的（Excel 语义）。
 */
class DAGUI_API DATableConditionalFormatRule
{
public:
    // 规则类型
    enum class Type
    {
        CellRule,    ///< 单元格规则（条件 → 前景/背景/字体）
        ColorScale,  ///< 色阶（双色/三色插值背景）
        DataBar      ///< 数据条（归一化条长）
    };
    // 单元格规则条件操作符
    enum class Condition
    {
        Greater,       ///< 大于（严格，等于不命中）
        Less,          ///< 小于（严格）
        Between,       ///< 介于（闭区间 [value1, value2]）
        Equal,         ///< 等于
        Contains,      ///< 包含（字符串子串，区分大小写）
        IsEmpty,       ///< 为空（invalid/null/NaN/空串）
        IsNotEmpty,    ///< 非空
        DateOccurring, ///< 发生日期（value1 = DateOccurrence 整数值，相对"今天"的区间判定）
        Duplicate,     ///< 重复值（列内出现≥2 次；求值需列重复键集合，见 evaluateDuplicate）
        Unique         ///< 唯一值（列内仅出现 1 次；求值需列重复键集合，见 evaluateDuplicate）
    };
    // 发生日期条件的日期区间类别（value1 以其整数值序列化）
    enum class DateOccurrence
    {
        Yesterday,  ///< 昨天
        Today,      ///< 今天
        Tomorrow,   ///< 明天
        Last7Days,  ///< 最近 7 天（含今天往前共 7 天）
        LastWeek,   ///< 上周（周一~周日）
        ThisWeek,   ///< 本周（周一~周日）
        NextWeek,   ///< 下周（周一~周日）
        LastMonth,  ///< 上月
        ThisMonth,  ///< 本月
        NextMonth   ///< 下月
    };

    DATableConditionalFormatRule();

    // ===== 纯函数求值（可单测，不依赖绘制环境）=====
    // 值能否严格转数值（invalid/null/NaN/非全串数字 → false）
    static bool toNumeric(const QVariant& v, double* out);
    // 值是否为"空"（invalid/null/NaN/空串）
    static bool isCellEmpty(const QVariant& v);
    // 条件求值：空单元格除 IsEmpty/IsNotEmpty 外不命中；数值优先、退化字符串比较；
    // DateOccurring 以 QDate::currentDate() 为"今天"求值；Duplicate/Unique 一律 false（需列上下文，走 evaluateDuplicate）
    static bool evaluateCondition(Condition cond, const QVariant& cellValue, const QVariant& v1, const QVariant& v2);
    // 发生日期求值：单元格日期落在 occ 相对 today 的区间内命中（非日期值不命中）
    static bool evaluateDateOccurring(DateOccurrence occ, const QVariant& cellValue, const QDate& today);
    // 重复/唯一求值：duplicateKeys = 列内出现≥2 次的值的归一化键集合（duplicateKey 口径）；
    // keysValid=false（惰性源等无列上下文）一律不命中；空单元格不命中
    static bool evaluateDuplicate(Condition cond, const QVariant& cellValue, const QSet< QString >& duplicateKeys,
                                  bool keysValid);
    // 值的归一化键（重复判定口径）：数值类型 → 最短往返十进制串；日期/时间 → ISO 串；其余 toString 原样；空值返回空串
    static QString duplicateKey(const QVariant& v);
    // 发生日期区间端点（含首尾；供界面摘要与单测复用）
    static QPair< QDate, QDate > dateOccurrenceRange(DateOccurrence occ, const QDate& today);
    // 归一化位置：hi>lo → (v-lo)/(hi-lo) 截断 [0,1]；hi==lo 退化返回 0.5
    static double normalizedPosition(double value, double lo, double hi);
    // 颜色线性插值（RGB 分量），t 截断 [0,1]
    static QColor lerpColor(const QColor& from, const QColor& to, double t);
    // 色阶取色：双色按 [vmin,vmax] 插值 cmin→cmax；三色以 vmed 分段 cmin→cmid→cmax
    static QColor colorScaleColor(double value, double vmin, double vmax, double vmed, bool threeColor,
                                  const QColor& cmin, const QColor& cmid, const QColor& cmax);
    // 数据条归一化条长比例 [0,1]（max==min 退化 0.5，越界截断）
    static double dataBarRatio(double value, double vmin, double vmax);

    // ===== 序列化（属性名用短名，照样式段风格）=====
    // 把规则写为 <rule> 元素追加到 parent
    void toXml(QDomDocument& doc, QDomElement& parent) const;
    // 从 <rule> 元素读取（未知属性忽略，前向兼容）；类型非法返回 false
    bool fromXml(const QDomElement& e);

    // 枚举 ↔ 字符串（XML 序列化与界面显示共用）
    static QString typeToString(Type t);
    static bool typeFromString(const QString& s, Type* t);
    static QString conditionToString(Condition c);
    static bool conditionFromString(const QString& s, Condition* c);
    static QString dateOccurrenceToString(DateOccurrence o);
    static bool dateOccurrenceFromString(const QString& s, DateOccurrence* o);

    bool operator==(const DATableConditionalFormatRule& other) const;
    bool operator!=(const DATableConditionalFormatRule& other) const;

public:
    Type type { Type::CellRule };                ///< 规则类型
    Condition condition { Condition::Greater };  ///< 条件操作符（仅 CellRule 使用）
    QVariant value1;                             ///< 条件参数 1（Between 为下界）
    QVariant value2;                             ///< 条件参数 2（仅 Between 使用，上界）
    DATableCellStyle style;                      ///< 输出样式（仅 CellRule；valid 标志区分未设置）
    QColor scaleMinColor { QColor(0xBD, 0xCD, 0xE8) };  ///< 色阶低端色（官方浅蓝 #BDCDE8）
    QColor scaleMidColor { QColor(0xFF, 0xFF, 0xFF) };  ///< 色阶中点色（三色用，白）
    QColor scaleMaxColor { QColor(0x52, 0x80, 0xC1) };  ///< 色阶高端色（官方主蓝 #5280C1）
    bool threeColorScale { false };              ///< 是否三色色阶（false=双色）
    QColor barColor { QColor(0xBD, 0xCD, 0xE8) };  ///< 数据条填充色（官方浅蓝 #BDCDE8）
    bool barGradient { false };                  ///< 数据条渐变填充（false=实心；渐变沿条长方向透明度递增）
    bool enabled { true };                       ///< 启停（列表顺序即优先级）
};
}  // end of namespace DA
#endif  // DATABLECONDITIONALFORMATRULE_H
