#include "DATableValidationStore.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父对象（一般为元数据束）
 */
DATableValidationStore::DATableValidationStore(QObject* parent) : QObject(parent)
{
}

bool DATableValidationStore::isEmpty() const
{
    return mRules.isEmpty();
}

int DATableValidationStore::count() const
{
    return mRules.size();
}

QList< DATableValidationRule > DATableValidationStore::rules() const
{
    return mRules;
}

DATableValidationRule DATableValidationStore::ruleAt(int index) const
{
    if (index < 0 || index >= mRules.size()) {
        return DATableValidationRule();
    }
    return mRules[ index ];
}

QList< int > DATableValidationStore::enabledRuleIndexes() const
{
    QList< int > res;
    for (int i = 0; i < mRules.size(); ++i) {
        if (mRules[ i ].enabled) {
            res.append(i);
        }
    }
    return res;
}

/**
 * @brief 追加规则（index<0 或越界 = 末尾）
 */
void DATableValidationStore::addRule(const DATableValidationRule& rule, int index)
{
    if (index < 0 || index > mRules.size()) {
        mRules.append(rule);
    } else {
        mRules.insert(index, rule);
    }
    Q_EMIT rulesChanged();
}

/**
 * @brief 移除 index 处规则
 */
void DATableValidationStore::removeRule(int index)
{
    if (index < 0 || index >= mRules.size()) {
        return;
    }
    mRules.removeAt(index);
    Q_EMIT rulesChanged();
}

/**
 * @brief 替换 index 处规则
 */
void DATableValidationStore::updateRule(int index, const DATableValidationRule& rule)
{
    if (index < 0 || index >= mRules.size()) {
        return;
    }
    mRules[ index ] = rule;
    Q_EMIT rulesChanged();
}

/**
 * @brief 整体替换规则集合（规则对话框确定后由命令调用）
 */
void DATableValidationStore::setRules(const QList< DATableValidationRule >& rules)
{
    mRules = rules;
    // 规则变了，唯一性缓存的键可能失效，保守清空
    invalidateUniqueCache();
    Q_EMIT rulesChanged();
}

void DATableValidationStore::clearAll()
{
    mRules.clear();
    invalidateUniqueCache();
    Q_EMIT rulesChanged();
}

/**
 * @brief 引用了 columnName 的规则条数（列改名提示「有 N 条规则引用它」）
 */
int DATableValidationStore::rulesReferencing(const QString& columnName) const
{
    int n = 0;
    for (const DATableValidationRule& r : std::as_const(mRules)) {
        if (r.columnName == columnName) {
            ++n;
        }
    }
    return n;
}

/**
 * @brief 列改名批量迁移（R20：列名引用随改名命令同步）
 * @return 受影响规则条数
 */
int DATableValidationStore::renameColumnReferences(const QString& oldName, const QString& newName)
{
    if (oldName == newName) {
        return 0;
    }
    int n = 0;
    for (DATableValidationRule& r : mRules) {
        if (r.columnName == oldName) {
            r.columnName = newName;
            ++n;
        }
    }
    if (n > 0) {
        invalidateUniqueCache();
        Q_EMIT rulesChanged();
    }
    return n;
}

/**
 * @brief 列位置 → 枚举下拉选项（delegate createEditor 用）
 *
 * columnNames 位置索引对齐；先算 columnPosition 的列名与「第 n 个同名列」序号，
 * 再找该 (列名, occurrence) 上启用的第一条 EnumSet 规则。
 * @return 有启用的枚举规则返回 true
 */
bool DATableValidationStore::enumOptionsForColumn(const QStringList& columnNames, int columnPosition, QStringList* values, bool* allowOther) const
{
    if (columnPosition < 0 || columnPosition >= columnNames.size()) {
        return false;
    }
    const QString name = columnNames[ columnPosition ];
    int occurrence = 0;
    for (int i = 0; i < columnPosition; ++i) {
        if (columnNames[ i ] == name) {
            ++occurrence;
        }
    }
    for (const DATableValidationRule& r : std::as_const(mRules)) {
        if (r.enabled && r.type == DATableValidationRule::Type::EnumSet && r.columnName == name
            && r.columnOccurrence == occurrence && !r.enumValues.isEmpty()) {
            if (values) {
                *values = r.enumValues;
            }
            if (allowOther) {
                *allowOther = r.enumAllowOther;
            }
            return true;
        }
    }
    return false;
}

bool DATableValidationStore::uniqueCacheValid(const DATableValidationRule& rule) const
{
    return mUniqueCache.contains(qMakePair(rule.columnName, rule.columnOccurrence));
}

QVector< int > DATableValidationStore::uniqueCacheRows(const DATableValidationRule& rule) const
{
    return mUniqueCache.value(qMakePair(rule.columnName, rule.columnOccurrence));
}

void DATableValidationStore::setUniqueCache(const DATableValidationRule& rule, const QVector< int >& rows)
{
    mUniqueCache.insert(qMakePair(rule.columnName, rule.columnOccurrence), rows);
}

void DATableValidationStore::invalidateUniqueCache()
{
    mUniqueCache.clear();
}

int DATableValidationStore::uniquePythonEvals() const
{
    return mUniquePythonEvals;
}

void DATableValidationStore::countUniquePythonEval()
{
    ++mUniquePythonEvals;
}

QList< DATableValidationRule > DATableValidationStore::snapshot() const
{
    return mRules;
}

void DATableValidationStore::restore(const QList< DATableValidationRule >& rules)
{
    mRules = rules;
    invalidateUniqueCache();
    Q_EMIT rulesChanged();
}

/**
 * @brief 序列化 <validation> 段（标量走短属性、枚举值走 <v> 子元素，照样式段风格）
 */
void DATableValidationStore::toXml(QDomDocument& doc, QDomElement& parent) const
{
    QDomElement e = doc.createElement(QStringLiteral("validation"));
    for (const DATableValidationRule& r : std::as_const(mRules)) {
        QDomElement re = doc.createElement(QStringLiteral("rule"));
        re.setAttribute(QStringLiteral("col"), r.columnName);
        re.setAttribute(QStringLiteral("occ"), r.columnOccurrence);
        re.setAttribute(QStringLiteral("type"), DATableValidationRule::typeToString(r.type));
        re.setAttribute(QStringLiteral("level"), DATableValidationRule::levelToString(r.level));
        re.setAttribute(QStringLiteral("enabled"), r.enabled ? 1 : 0);
        if (!r.label.isEmpty()) {
            re.setAttribute(QStringLiteral("label"), r.label);
        }
        if (r.hasNumericMin) {
            re.setAttribute(QStringLiteral("nmin"), r.numericMin);
        }
        if (r.hasNumericMax) {
            re.setAttribute(QStringLiteral("nmax"), r.numericMax);
        }
        if (r.dateMin.isValid()) {
            re.setAttribute(QStringLiteral("dmin"), r.dateMin.toString(Qt::ISODateWithMs));
        }
        if (r.dateMax.isValid()) {
            re.setAttribute(QStringLiteral("dmax"), r.dateMax.toString(Qt::ISODateWithMs));
        }
        re.setAttribute(QStringLiteral("allow-other"), r.enumAllowOther ? 1 : 0);
        if (r.type == DATableValidationRule::Type::Regex) {
            re.setAttribute(QStringLiteral("regex"), r.regexPattern);
        }
        for (const QString& v : std::as_const(r.enumValues)) {
            QDomElement ve = doc.createElement(QStringLiteral("v"));
            ve.appendChild(doc.createTextNode(v));
            re.appendChild(ve);
        }
        e.appendChild(re);
    }
    parent.appendChild(e);
}

/**
 * @brief 从 <validation> 段反序列化（先自清空；未知属性忽略，前向兼容）
 * @return 始终返回 true
 */
bool DATableValidationStore::fromXml(const QDomElement& e)
{
    mRules.clear();
    invalidateUniqueCache();
    QDomElement re = e.firstChildElement(QStringLiteral("rule"));
    while (!re.isNull()) {
        DATableValidationRule r;
        r.columnName        = re.attribute(QStringLiteral("col"));
        r.columnOccurrence  = re.attribute(QStringLiteral("occ"), QStringLiteral("0")).toInt();
        DATableValidationRule::typeFromString(re.attribute(QStringLiteral("type"), QStringLiteral("not-empty")), &r.type);
        DATableValidationRule::levelFromString(re.attribute(QStringLiteral("level"), QStringLiteral("error")), &r.level);
        r.enabled           = re.attribute(QStringLiteral("enabled"), QStringLiteral("1")).toInt() != 0;
        r.label             = re.attribute(QStringLiteral("label"));
        if (re.hasAttribute(QStringLiteral("nmin"))) {
            r.hasNumericMin = true;
            r.numericMin    = re.attribute(QStringLiteral("nmin")).toDouble();
        }
        if (re.hasAttribute(QStringLiteral("nmax"))) {
            r.hasNumericMax = true;
            r.numericMax    = re.attribute(QStringLiteral("nmax")).toDouble();
        }
        if (re.hasAttribute(QStringLiteral("dmin"))) {
            r.dateMin = QDateTime::fromString(re.attribute(QStringLiteral("dmin")), Qt::ISODateWithMs);
        }
        if (re.hasAttribute(QStringLiteral("dmax"))) {
            r.dateMax = QDateTime::fromString(re.attribute(QStringLiteral("dmax")), Qt::ISODateWithMs);
        }
        r.enumAllowOther = re.attribute(QStringLiteral("allow-other"), QStringLiteral("1")).toInt() != 0;
        r.regexPattern   = re.attribute(QStringLiteral("regex"));
        QDomElement ve   = re.firstChildElement(QStringLiteral("v"));
        while (!ve.isNull()) {
            r.enumValues.append(ve.text());
            ve = ve.nextSiblingElement(QStringLiteral("v"));
        }
        if (!r.columnName.isEmpty()) {
            mRules.append(r);
        }
        re = re.nextSiblingElement(QStringLiteral("rule"));
    }
    Q_EMIT rulesChanged();
    return true;
}

}  // end of namespace DA
