#ifndef DACOMPUTEDCOLUMN_H
#define DACOMPUTEDCOLUMN_H
#include "DADataAPI.h"
#include <QString>
#include <QStringList>

namespace DA
{
/**
 * @brief 计算列定义（纯数据类，无 GUI 依赖，004 T1.23）
 *
 * 「列级公式」的定义记录：某一列（targetColumnName）的取值由表达式（expr）针对
 * 其他列求值得到。本类只承载定义，不含状态与求值逻辑——三态状态机与重算调度在
 * DAGui 的 DATableComputedColumnStore（元数据束成员），求值走 DAPyScriptsDataFrame
 * 的 da_eval_column 算子。定义类放 src/DAData 是为将来被非 GUI 消费（工作流节点、
 * agent 触发重算）留位（设计稿 §4.4 落位说明 / D3 语义类归属）。
 *
 * 磁盘契约（R10）：expr + engine + version 一起序列化（DATableComputedColumnStore
 * 的 <computed-column> 段），加载/求值失败只标失效、不阻塞打开工程、不损坏数据。
 *
 * 关键语义（R7/R8/R20）：
 * - 物化而非虚拟：求值结果写进真实 DataFrame 列，本定义只是元数据记录；
 * - 禁止自覆盖：expr 不得引用 targetColumnName（只能写新列，重算幂等）；
 * - 依赖用列名引用（dependsOn），列插删/改名时不易错位（重名以「第 n 个同名列」消歧
 *   由存储层处理）；
 * - expr 存赋值右侧表达式体（RHS），目标列名单独存 targetColumnName，求值时经
 *   da_eval_column 组装（目标列名可含空格，不依赖 eval 赋值语法）。
 */
class DADATA_API DAComputedColumn
{
public:
    // 磁盘契约版本号（当前定义结构为 1；结构变更时递增，加载侧按版本兼容）
    static constexpr int c_currentVersion = 1;
    // 求值引擎标识（v1 只有 pandas eval 单表达式；随定义持久化，R10）
    static const QString& pandasEvalEngine();

public:
    DAComputedColumn();
    DAComputedColumn(const QString& target, const QString& expression);

    QString targetColumnName;  ///< 目标列名（计算结果物化到的列）
    QString expr;              ///< 表达式体（赋值右侧 RHS，列引用可含反引号）
    QString engine;            ///< 求值引擎标识（默认 "pandas.eval"，磁盘契约 R10）
    int version { c_currentVersion };  ///< 定义结构版本（磁盘契约 R10）
    QStringList dependsOn;     ///< 依赖列名列表（R20 列名引用；由 da_expr_depends 提取）
    bool enabled { true };     ///< 启用开关（停用后不参与重算调度，定义保留）

    // 定义是否可用（目标列名与表达式都非空）
    bool isValid() const;
    // 表达式是否引用了目标列（自覆盖，R7 禁止；大小写敏感精确匹配）
    bool isSelfReferencing() const;
    bool operator==(const DAComputedColumn& other) const;
    bool operator!=(const DAComputedColumn& other) const;
};
}  // end of namespace DA
#endif  // DACOMPUTEDCOLUMN_H
