#ifndef DATABLEVALIDATIONSTORE_H
#define DATABLEVALIDATIONSTORE_H
#include "DAGuiAPI.h"
#include "DATableValidationRule.h"
#include <QObject>
#include <QList>
#include <QHash>
#include <QPair>
#include <QVector>
#include <QStringList>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
/**
 * @brief 数据校验规则子存储（元数据束成员，005 T1.25）
 *
 * 职责（设计稿 §3.1/§4.9）：
 * - 规则集合管理：DATableValidationRule 列表（插入顺序）；规则定义类在 src/DAData
 *   （语义类，无 GUI 依赖，D3 归属分层），本存储负责集合与序列化；
 * - 列引用按列名 + 第 n 个同名列消歧（R20 语义类）：renameColumnReferences 批量迁移
 *   （列改名命令负责调用并提示影响面）；
 * - 唯一性违规缓存（§4.9：O(n) 的 pandas 操作按列缓存重复集合，二次校验不重算）：
 *   **瞬态**成员不序列化；数据变更/排序/插删行列由 widget 侧挂点调 invalidateUniqueCache
 *   保守失效；uniquePythonEvals 打点计数供单测钉住「二次校验不重算」；
 * - XML 段：<validation> 进 DATableMetadata 束聚合（table-metadata.xml）。
 *
 * 校验本身是只读操作（不改数据、不入撤销栈，红线2）：本存储只持有规则；规则增删改
 * 由 DACommandsTableValidation 命令驱动（旧新对象记录，D4）。
 */
class DAGUI_API DATableValidationStore : public QObject
{
    Q_OBJECT
public:
    explicit DATableValidationStore(QObject* parent = nullptr);

    // ===== 规则集合（插入顺序）=====
    bool isEmpty() const;
    int count() const;
    QList< DATableValidationRule > rules() const;
    DATableValidationRule ruleAt(int index) const;
    // 启用的规则下标（校验执行只取启用项）
    QList< int > enabledRuleIndexes() const;

    // 增删改（命令层调用，本类不入撤销栈；发 rulesChanged）
    void addRule(const DATableValidationRule& rule, int index = -1);
    void removeRule(int index);
    void updateRule(int index, const DATableValidationRule& rule);
    // 整体替换（规则对话框确定后由 setRules 命令调用）
    void setRules(const QList< DATableValidationRule >& rules);
    void clearAll();

    // ===== 列名引用联动（R20）=====
    // 引用了 columnName 的规则条数（列改名提示「有 N 条规则引用它」）
    int rulesReferencing(const QString& columnName) const;
    // 列改名批量迁移，返回受影响规则条数
    int renameColumnReferences(const QString& oldName, const QString& newName);

    // ===== 枚举下拉（delegate createEditor 用，§4.9 附加能力）=====
    // columnNames 为该表全部列名（位置索引对齐）；columnPosition 列上有启用的 EnumSet
    // 规则时返回 true 并输出允许值与「允许其他值」开关（多条枚举规则取第一条）
    bool enumOptionsForColumn(const QStringList& columnNames, int columnPosition, QStringList* values, bool* allowOther) const;

    // ===== 唯一性违规缓存（瞬态，不序列化；§4.9 按列缓存重复集合）=====
    // 键 = (列名, occurrence)；数据变更/排序/插删行列 → invalidateUniqueCache 保守失效
    bool uniqueCacheValid(const DATableValidationRule& rule) const;
    QVector< int > uniqueCacheRows(const DATableValidationRule& rule) const;
    void setUniqueCache(const DATableValidationRule& rule, const QVector< int >& rows);
    void invalidateUniqueCache();
    // 打点：唯一性规则实际下 Python 求值的累计次数（单测钉住「二次校验不重算」）
    int uniquePythonEvals() const;
    void countUniquePythonEval();

    // ===== 整店快照/恢复（命令 undo 联动）=====
    QList< DATableValidationRule > snapshot() const;
    void restore(const QList< DATableValidationRule >& rules);

    // ===== 序列化（<validation> 段，标量走短属性、枚举值走 <v> 子元素）=====
    void toXml(QDomDocument& doc, QDomElement& parent) const;
    bool fromXml(const QDomElement& e);

Q_SIGNALS:
    // 规则集合变更（增删改/迁移/恢复/清空），视图与 Ribbon 状态据此刷新
    void rulesChanged();

private:
    QList< DATableValidationRule > mRules;  ///< 规则集合（插入顺序，round-trip 稳定）
    // 唯一性违规缓存：(列名, occurrence) → 违规行（绝对行号）；瞬态不序列化
    QHash< QPair< QString, int >, QVector< int > > mUniqueCache;
    int mUniquePythonEvals { 0 };  ///< 唯一性规则下 Python 求值累计次数（打点）
};
}  // end of namespace DA
#endif  // DATABLEVALIDATIONSTORE_H
