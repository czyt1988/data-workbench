#include "DATableComputedColumnStore.h"
#include <QSet>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父对象（一般为元数据束 DATableMetadata）
 */
DATableComputedColumnStore::DATableComputedColumnStore(QObject* parent) : QObject(parent)
{
}

/**
 * @brief 按目标列名找条目下标
 * @param target 目标列名
 * @return 下标；无返回 -1
 */
int DATableComputedColumnStore::indexOfTarget(const QString& target) const
{
    for (int i = 0; i < mEntries.size(); ++i) {
        if (mEntries.at(i).def.targetColumnName == target) {
            return i;
        }
    }
    return -1;
}

bool DATableComputedColumnStore::isEmpty() const
{
    return mEntries.isEmpty();
}

int DATableComputedColumnStore::count() const
{
    return mEntries.size();
}

/**
 * @brief 全部目标列名（插入顺序）
 */
QStringList DATableComputedColumnStore::targetColumns() const
{
    QStringList res;
    res.reserve(mEntries.size());
    for (const Entry& e : mEntries) {
        res.append(e.def.targetColumnName);
    }
    return res;
}

bool DATableComputedColumnStore::hasDefinition(const QString& target) const
{
    return indexOfTarget(target) >= 0;
}

/**
 * @brief 取定义
 * @param target 目标列名
 * @return 无定义返回默认构造的无效定义（isValid()==false）
 */
DAComputedColumn DATableComputedColumnStore::definition(const QString& target) const
{
    const int i = indexOfTarget(target);
    return (i >= 0) ? mEntries.at(i).def : DAComputedColumn();
}

/**
 * @brief 取完整条目（定义 + 状态）
 * @param target 目标列名
 * @param out 输出条目
 * @return 有定义返回 true
 */
bool DATableComputedColumnStore::entryOf(const QString& target, Entry& out) const
{
    const int i = indexOfTarget(target);
    if (i < 0) {
        return false;
    }
    out = mEntries.at(i);
    return true;
}

QList< DATableComputedColumnStore::Entry > DATableComputedColumnStore::entries() const
{
    return mEntries;
}

/**
 * @brief 取状态
 * @param target 目标列名
 * @return 无定义返回 Invalid
 */
DATableComputedColumnStore::State DATableComputedColumnStore::stateOf(const QString& target) const
{
    const int i = indexOfTarget(target);
    return (i >= 0) ? mEntries.at(i).state : State::Invalid;
}

/**
 * @brief 添加/替换定义（目标列名已存在则整条替换）
 * @param def 定义
 * @param state 初始状态（创建后立即求值成功为 Valid）
 */
void DATableComputedColumnStore::addDefinition(const DAComputedColumn& def, State state)
{
    if (def.targetColumnName.isEmpty()) {
        return;
    }
    const int i = indexOfTarget(def.targetColumnName);
    if (i >= 0) {
        mEntries[ i ].def   = def;
        mEntries[ i ].state = state;
    } else {
        Entry e;
        e.def   = def;
        e.state = state;
        mEntries.append(e);
    }
    Q_EMIT definitionsChanged();
}

/**
 * @brief 移除定义（目标列被删/移除公式）
 * @param target 目标列名
 */
void DATableComputedColumnStore::removeDefinition(const QString& target)
{
    const int i = indexOfTarget(target);
    if (i < 0) {
        return;
    }
    mEntries.removeAt(i);
    Q_EMIT definitionsChanged();
}

/**
 * @brief 更新定义（编辑公式，状态不变——调用方求值成功后另行 setState）
 * @param def 新定义（按 targetColumnName 定位）
 */
void DATableComputedColumnStore::updateDefinition(const DAComputedColumn& def)
{
    const int i = indexOfTarget(def.targetColumnName);
    if (i < 0) {
        return;
    }
    mEntries[ i ].def = def;
    Q_EMIT definitionsChanged();
}

/**
 * @brief 设置状态（发 stateChanged，表头角标变色）
 * @param target 目标列名
 * @param state 新状态
 */
void DATableComputedColumnStore::setState(const QString& target, State state)
{
    const int i = indexOfTarget(target);
    if (i < 0 || mEntries.at(i).state == state) {
        return;
    }
    mEntries[ i ].state = state;
    Q_EMIT stateChanged(target, state);
}

/**
 * @brief 引用了 columnName 作依赖列的定义目标（不含目标列自身就是 columnName 的）
 * @param columnName 列名
 * @return 目标列名列表
 */
QStringList DATableComputedColumnStore::definitionsReferencing(const QString& columnName) const
{
    QStringList res;
    for (const Entry& e : mEntries) {
        if (e.def.targetColumnName != columnName && e.def.dependsOn.contains(columnName)) {
            res.append(e.def.targetColumnName);
        }
    }
    return res;
}

/**
 * @brief 数据变更标记：依赖列或目标列自身被改 → Valid 转 Stale
 *
 * Stale/Invalid 不降级（已失效的列不因数据变更转待重算，保持失效直到修复）。
 * 排序不调用本函数（行顺序变化不影响列值，R13）。
 * @param changedColumns 变更的列名集合
 * @return 本次被标记为 Stale 的目标列
 */
QStringList DATableComputedColumnStore::markStaleForChangedColumns(const QStringList& changedColumns)
{
    QStringList marked;
    if (changedColumns.isEmpty()) {
        return marked;
    }
    const QSet< QString > changed(changedColumns.begin(), changedColumns.end());
    for (Entry& e : mEntries) {
        if (e.state != State::Valid || !e.def.enabled) {
            continue;
        }
        bool affected = changed.contains(e.def.targetColumnName);
        if (!affected) {
            for (const QString& dep : std::as_const(e.def.dependsOn)) {
                if (changed.contains(dep)) {
                    affected = true;
                    break;
                }
            }
        }
        if (affected) {
            e.state = State::Stale;
            marked.append(e.def.targetColumnName);
            Q_EMIT stateChanged(e.def.targetColumnName, State::Stale);
        }
    }
    return marked;
}

/**
 * @brief 整表操作保守全标（Valid → Stale）
 * @return 被标记的目标列
 */
QStringList DATableComputedColumnStore::markAllStale()
{
    QStringList marked;
    for (Entry& e : mEntries) {
        if (e.state == State::Valid && e.def.enabled) {
            e.state = State::Stale;
            marked.append(e.def.targetColumnName);
            Q_EMIT stateChanged(e.def.targetColumnName, State::Stale);
        }
    }
    return marked;
}

/**
 * @brief 列改名批量迁移：依赖列引用与目标列自身一并改名（R20）
 * @param oldName 旧列名
 * @param newName 新列名
 * @return 受影响的定义个数
 */
int DATableComputedColumnStore::renameColumnReferences(const QString& oldName, const QString& newName)
{
    if (oldName == newName || oldName.isEmpty()) {
        return 0;
    }
    int affected = 0;
    for (Entry& e : mEntries) {
        bool touched = false;
        if (e.def.targetColumnName == oldName) {
            e.def.targetColumnName = newName;
            touched                = true;
        }
        for (QString& dep : e.def.dependsOn) {
            if (dep == oldName) {
                dep    = newName;
                touched = true;
            }
        }
        if (touched) {
            ++affected;
        }
    }
    if (affected > 0) {
        Q_EMIT definitionsChanged();
    }
    return affected;
}

/**
 * @brief 删列联动
 *
 * 目标列被删 → 定义移除（removedTargets 输出，供命令 undo 恢复）；
 * 依赖列被删 → 置 Invalid（invalidatedTargets 输出，供提示，不静默清空数据，R9）。
 * @param removedNames 被删除的列名
 * @param removedTargets 输出：定义被移除的目标列（含其定义可从 snapshot 恢复）
 * @param invalidatedTargets 输出：因依赖缺失被置失效的目标列
 */
void DATableComputedColumnStore::onColumnsRemoved(const QStringList& removedNames, QStringList* removedTargets, QStringList* invalidatedTargets)
{
    if (removedNames.isEmpty()) {
        return;
    }
    const QSet< QString > removed(removedNames.begin(), removedNames.end());
    bool defsChanged = false;
    // 先移除目标列被删的定义
    for (int i = mEntries.size() - 1; i >= 0; --i) {
        if (removed.contains(mEntries.at(i).def.targetColumnName)) {
            if (removedTargets) {
                removedTargets->append(mEntries.at(i).def.targetColumnName);
            }
            mEntries.removeAt(i);
            defsChanged = true;
        }
    }
    // 再把依赖列被删的定义置失效
    for (Entry& e : mEntries) {
        if (e.state == State::Invalid) {
            continue;
        }
        for (const QString& dep : std::as_const(e.def.dependsOn)) {
            if (removed.contains(dep)) {
                if (e.state != State::Invalid) {
                    e.state = State::Invalid;
                    if (invalidatedTargets) {
                        invalidatedTargets->append(e.def.targetColumnName);
                    }
                    Q_EMIT stateChanged(e.def.targetColumnName, State::Invalid);
                }
                break;
            }
        }
    }
    if (defsChanged) {
        Q_EMIT definitionsChanged();
    }
}

/**
 * @brief 整店快照（列删除等命令的 undo 联动）
 */
QList< DATableComputedColumnStore::Entry > DATableComputedColumnStore::snapshot() const
{
    return mEntries;
}

/**
 * @brief 从快照整店恢复（undo）
 * @param entries 快照
 */
void DATableComputedColumnStore::restore(const QList< Entry >& entries)
{
    mEntries = entries;
    Q_EMIT definitionsChanged();
}

/**
 * @brief 序列化到 <computed-column> 段
 *
 * 每个定义一个 <column>：属性 target/engine/version/state/enabled，子元素 <expr>（文本）
 * 与若干 <dep name="...">（依赖列名，用子元素而非逗号拼接，避免列名含分隔符）。
 * 磁盘契约 R10：expr + engine + version 一起写出。
 * @param doc QDomDocument
 * @param parent table 元素
 */
void DATableComputedColumnStore::toXml(QDomDocument& doc, QDomElement& parent) const
{
    if (mEntries.isEmpty()) {
        return;
    }
    QDomElement cc = doc.createElement(QStringLiteral("computed-column"));
    for (const Entry& e : mEntries) {
        QDomElement col = doc.createElement(QStringLiteral("column"));
        col.setAttribute(QStringLiteral("target"), e.def.targetColumnName);
        col.setAttribute(QStringLiteral("engine"), e.def.engine);
        col.setAttribute(QStringLiteral("version"), e.def.version);
        col.setAttribute(QStringLiteral("enabled"), e.def.enabled ? 1 : 0);
        QString stateStr;
        switch (e.state) {
        case State::Valid:
            stateStr = QStringLiteral("valid");
            break;
        case State::Stale:
            stateStr = QStringLiteral("stale");
            break;
        case State::Invalid:
            stateStr = QStringLiteral("invalid");
            break;
        }
        col.setAttribute(QStringLiteral("state"), stateStr);
        // expr 作文本子元素（可能含特殊字符/空格，CDATA 保护）
        QDomElement exprEle = doc.createElement(QStringLiteral("expr"));
        exprEle.appendChild(doc.createCDATASection(e.def.expr));
        col.appendChild(exprEle);
        // 依赖列名逐个 <dep>
        for (const QString& dep : e.def.dependsOn) {
            QDomElement depEle = doc.createElement(QStringLiteral("dep"));
            depEle.setAttribute(QStringLiteral("name"), dep);
            col.appendChild(depEle);
        }
        cc.appendChild(col);
    }
    parent.appendChild(cc);
}

/**
 * @brief 从 <computed-column> 段反序列化（先清空再写入；未知标签忽略）
 *
 * 打开工程不重算（R7）：本函数只恢复定义与状态，不触发任何求值。
 * @param e computed-column 元素
 * @return 始终返回 true
 */
bool DATableComputedColumnStore::fromXml(const QDomElement& e)
{
    mEntries.clear();
    QDomNode n = e.firstChild();
    while (!n.isNull()) {
        QDomElement col = n.toElement();
        if (!col.isNull() && col.tagName() == QLatin1String("column")) {
            Entry entry;
            entry.def.targetColumnName = col.attribute(QStringLiteral("target"));
            entry.def.engine           = col.attribute(QStringLiteral("engine"), DAComputedColumn::pandasEvalEngine());
            entry.def.version          = col.attribute(QStringLiteral("version"), QString::number(DAComputedColumn::c_currentVersion)).toInt();
            entry.def.enabled          = col.attribute(QStringLiteral("enabled"), QStringLiteral("1")).toInt() != 0;
            const QString stateStr     = col.attribute(QStringLiteral("state"), QStringLiteral("valid"));
            if (stateStr == QLatin1String("stale")) {
                entry.state = State::Stale;
            } else if (stateStr == QLatin1String("invalid")) {
                entry.state = State::Invalid;
            } else {
                entry.state = State::Valid;
            }
            // 子元素：<expr> 文本 + <dep name>
            QDomNode cn = col.firstChild();
            while (!cn.isNull()) {
                QDomElement child = cn.toElement();
                if (!child.isNull()) {
                    if (child.tagName() == QLatin1String("expr")) {
                        entry.def.expr = child.text();
                    } else if (child.tagName() == QLatin1String("dep")) {
                        const QString dep = child.attribute(QStringLiteral("name"));
                        if (!dep.isEmpty()) {
                            entry.def.dependsOn.append(dep);
                        }
                    }
                }
                cn = cn.nextSibling();
            }
            if (!entry.def.targetColumnName.isEmpty()) {
                mEntries.append(entry);
            }
        }
        n = n.nextSibling();
    }
    Q_EMIT definitionsChanged();
    return true;
}

/**
 * @brief 清空全部定义
 */
void DATableComputedColumnStore::clear()
{
    if (mEntries.isEmpty()) {
        return;
    }
    mEntries.clear();
    Q_EMIT definitionsChanged();
}

/**
 * @brief 状态显示文本（表头 tooltip / 提示，已 tr）
 * @param state 状态
 * @return 本地化文本
 */
QString DATableComputedColumnStore::stateText(State state)
{
    switch (state) {
    case State::Valid:
        return tr("Valid");  // cn:有效
    case State::Stale:
        return tr("Needs Recompute");  // cn:待重算
    case State::Invalid:
        return tr("Invalid");  // cn:失效
    }
    return QString();
}

}  // end of namespace DA
