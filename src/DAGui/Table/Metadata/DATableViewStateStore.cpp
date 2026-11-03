#include "Table/Metadata/DATableViewStateStore.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父对象
 */
DATableViewStateStore::DATableViewStateStore(QObject* parent) : QObject(parent)
{
}

/**
 * @brief 查询列宽
 * @param name 列名
 * @param occurrence 第几次出现（重名列消歧位）
 * @return 宽度（像素），未记录返回 -1
 */
int DATableViewStateStore::columnWidth(const QString& name, int occurrence) const
{
    auto it = mColumnWidths.constFind(qMakePair(name, occurrence));
    return (it != mColumnWidths.constEnd()) ? it.value() : -1;
}

/**
 * @brief 是否记录了某列宽
 * @param name 列名
 * @param occurrence 第几次出现
 * @return 已记录返回 true
 */
bool DATableViewStateStore::hasColumnWidth(const QString& name, int occurrence) const
{
    return mColumnWidths.contains(qMakePair(name, occurrence));
}

/**
 * @brief 记录列宽
 * @param name 列名
 * @param occurrence 第几次出现
 * @param width 宽度（像素），<=0 视为清除
 */
void DATableViewStateStore::setColumnWidth(const QString& name, int occurrence, int width)
{
    const QPair< QString, int > key = qMakePair(name, occurrence);
    if (width <= 0) {
        mColumnWidths.remove(key);
        return;
    }
    mColumnWidths.insert(key, width);
}

/**
 * @brief 移除某列宽记录
 * @param name 列名
 * @param occurrence 第几次出现
 */
void DATableViewStateStore::removeColumnWidth(const QString& name, int occurrence)
{
    mColumnWidths.remove(qMakePair(name, occurrence));
}

/**
 * @brief 全部列宽条目
 * @return 条目列表（顺序不保证，应用方逐条按名匹配）
 */
QList< DATableViewStateStore::ColumnWidthEntry > DATableViewStateStore::columnWidthEntries() const
{
    QList< ColumnWidthEntry > res;
    res.reserve(mColumnWidths.size());
    for (auto it = mColumnWidths.constBegin(); it != mColumnWidths.constEnd(); ++it) {
        ColumnWidthEntry e;
        e.name       = it.key().first;
        e.occurrence = it.key().second;
        e.width      = it.value();
        res.append(e);
    }
    return res;
}

/**
 * @brief 查询行头宽
 * @return 宽度（像素），未记录返回 -1
 */
int DATableViewStateStore::rowHeaderWidth() const
{
    return mRowHeaderWidth;
}

/**
 * @brief 记录行头宽
 * @param width 宽度（像素），<=0 视为清除
 */
void DATableViewStateStore::setRowHeaderWidth(int width)
{
    mRowHeaderWidth = (width > 0) ? width : -1;
}

/**
 * @brief 查询统一行高
 * @return 高度（像素），未记录返回 -1
 */
int DATableViewStateStore::rowHeight() const
{
    return mRowHeight;
}

/**
 * @brief 记录统一行高
 * @param height 高度（像素），<=0 视为清除
 */
void DATableViewStateStore::setRowHeight(int height)
{
    mRowHeight = (height > 0) ? height : -1;
}

/**
 * @brief 查询水平表头高
 * @return 高度（像素），未记录返回 -1
 */
int DATableViewStateStore::horizontalHeaderHeight() const
{
    return mHorizontalHeaderHeight;
}

/**
 * @brief 记录水平表头高
 * @param height 高度（像素），<=0 视为清除
 */
void DATableViewStateStore::setHorizontalHeaderHeight(int height)
{
    mHorizontalHeaderHeight = (height > 0) ? height : -1;
}

/**
 * @brief 查询列序
 * @return 列名有序表（空 = 未记录 = 数据列序）
 */
QList< QPair< QString, int > > DATableViewStateStore::columnOrder() const
{
    return mColumnOrder;
}

/**
 * @brief 记录列序
 * @param order 列名有序表（空表 = 恢复默认顺序）
 */
void DATableViewStateStore::setColumnOrder(const QList< QPair< QString, int > >& order)
{
    mColumnOrder = order;
}

/**
 * @brief 查询列是否隐藏
 * @param name 列名
 * @param occurrence 第几次出现（重名列消歧位）
 * @return 已隐藏返回 true
 */
bool DATableViewStateStore::isColumnHidden(const QString& name, int occurrence) const
{
    return mHiddenColumns.contains(qMakePair(name, occurrence));
}

/**
 * @brief 记录列隐藏状态
 *
 * hidden 为 false 时移除条目（未记录 = 可见），保证「显示所有列」清空集合。
 * @param name 列名
 * @param occurrence 第几次出现
 * @param hidden 是否隐藏
 */
void DATableViewStateStore::setColumnHidden(const QString& name, int occurrence, bool hidden)
{
    const QPair< QString, int > key = qMakePair(name, occurrence);
    if (hidden) {
        mHiddenColumns.insert(key);
    } else {
        mHiddenColumns.remove(key);
    }
}

/**
 * @brief 全部隐藏条目
 * @return 隐藏集合（顺序不保证，调用方按集合用）
 */
QList< QPair< QString, int > > DATableViewStateStore::hiddenColumns() const
{
    return mHiddenColumns.values();
}

/**
 * @brief 冻结列列表
 * @return 冻结列身份（有序 = 冻结区显示顺序；空 = 未冻结）
 */
QList< QPair< QString, int > > DATableViewStateStore::frozenColumns() const
{
    return mFrozenColumns;
}

/**
 * @brief 整体替换冻结列列表
 *
 * 去重保序（同一身份只收一次），空表 = 取消全部冻结。
 * @param frozen 冻结列身份（有序 = 冻结区显示顺序）
 */
void DATableViewStateStore::setFrozenColumns(const QList< QPair< QString, int > >& frozen)
{
    mFrozenColumns.clear();
    mFrozenColumns.reserve(frozen.size());
    for (const QPair< QString, int >& ident : frozen) {
        if (!mFrozenColumns.contains(ident)) {
            mFrozenColumns.append(ident);
        }
    }
}

/**
 * @brief 查询列是否冻结
 * @param name 列名
 * @param occurrence 第几次出现（重名列消歧位）
 * @return 已冻结返回 true
 */
bool DATableViewStateStore::isColumnFrozen(const QString& name, int occurrence) const
{
    return mFrozenColumns.contains(qMakePair(name, occurrence));
}

/**
 * @brief 记录列冻结状态
 *
 * frozen 为 true 时追加到冻结列表末尾（调用方按全局列序插入请用 setFrozenColumns）；
 * false 时移除条目。
 * @param name 列名
 * @param occurrence 第几次出现
 * @param frozen 是否冻结
 */
void DATableViewStateStore::setColumnFrozen(const QString& name, int occurrence, bool frozen)
{
    const QPair< QString, int > key = qMakePair(name, occurrence);
    for (int i = mFrozenColumns.size() - 1; i >= 0; --i) {
        if (mFrozenColumns.at(i) == key) {
            mFrozenColumns.removeAt(i);
        }
    }
    if (frozen) {
        mFrozenColumns.append(key);
    }
}

/**
 * @brief 是否已做过加载收敛
 * @return 已收敛返回 true（页面据此只收敛一次）
 */
bool DATableViewStateStore::isColumnSetNormalized() const
{
    return mColumnSetNormalized;
}

/**
 * @brief 加载收敛：按实际列身份表剪除孤儿条目、把新列追加到列序末尾
 *
 * 列序为空时保持为空（空 = 未记录 = 数据列序——不用 actual 填充，否则每个工程
 * 都会被写出一条覆盖全列的列序，破坏「空=默认」语义）。非空时保序保留身份在
 * actual 里的条目，剪除其余，再按 actualColumns 的顺序把未出现的身份追加到末尾。
 * 隐藏集合剪除身份不在 actual 里的条目。冻结列同理剪除孤儿条目；旧格式
 * <freeze columns="N"/> 的计数在这一步展开为「全局列序（列序为空则实际列序）
 * 前 N 个列身份」，保证旧工程打开后行为不变。一次调用即视为已收敛
 * （改没改都置位）。
 * @param actualColumns 实际列身份表（数据列序升序）
 * @return true 表示列序、隐藏集合或冻结列与进入时不同（调用方据此发一条用户可见提示）
 */
bool DATableViewStateStore::normalizeToColumns(const QList< QPair< QString, int > >& actualColumns)
{
    bool changed = false;
    QSet< QPair< QString, int > > actualSet;
    for (const QPair< QString, int >& ident : actualColumns) {
        actualSet.insert(ident);
    }
    if (!mColumnOrder.isEmpty()) {
        QList< QPair< QString, int > > newOrder;
        newOrder.reserve(mColumnOrder.size());
        QSet< QPair< QString, int > > used;
        for (const QPair< QString, int >& ident : std::as_const(mColumnOrder)) {
            if (actualSet.contains(ident) && !used.contains(ident)) {
                newOrder.append(ident);
                used.insert(ident);
            }
        }
        for (const QPair< QString, int >& ident : actualColumns) {
            if (!used.contains(ident)) {
                newOrder.append(ident);  // 新列按实际顺序追加到末尾
            }
        }
        if (newOrder != mColumnOrder) {
            mColumnOrder = newOrder;
            changed      = true;
        }
    }
    QSet< QPair< QString, int > > newHidden;
    for (const QPair< QString, int >& ident : std::as_const(mHiddenColumns)) {
        if (actualSet.contains(ident)) {
            newHidden.insert(ident);
        }
    }
    if (newHidden != mHiddenColumns) {
        mHiddenColumns = newHidden;
        changed        = true;
    }
    // 旧格式冻结计数展开：按收敛后的全局列序（空 = 实际列序）取前 N 个身份
    if (mLegacyFreezeCount > 0) {
        const QList< QPair< QString, int > >& order = mColumnOrder.isEmpty() ? actualColumns : mColumnOrder;
        QList< QPair< QString, int > > legacy;
        legacy.reserve(qMin(mLegacyFreezeCount, order.size()));
        for (int i = 0; i < order.size() && i < mLegacyFreezeCount; ++i) {
            legacy.append(order.at(i));
        }
        if (legacy != mFrozenColumns) {
            mFrozenColumns = legacy;
            changed        = true;
        }
        mLegacyFreezeCount = -1;
    }
    // 冻结列剪除孤儿身份（保序去重）
    QList< QPair< QString, int > > newFrozen;
    newFrozen.reserve(mFrozenColumns.size());
    for (const QPair< QString, int >& ident : std::as_const(mFrozenColumns)) {
        if (actualSet.contains(ident) && !newFrozen.contains(ident)) {
            newFrozen.append(ident);
        }
    }
    if (newFrozen != mFrozenColumns) {
        mFrozenColumns = newFrozen;
        changed        = true;
    }
    mColumnSetNormalized = true;
    return changed;
}

/**
 * @brief 列改名迁移
 *
 * 只改键的列名部分，occurrence 不变（改名不改变列的位置序）。列宽、列序、隐藏、
 * 冻结四份键一并迁移（列序/冻结保持位置，只改名）。若目标 (newName, occurrence)
 * 已有记录，以迁移过来的值覆盖（后到的改名生效）。
 * @param oldName 旧列名
 * @param newName 新列名
 */
void DATableViewStateStore::renameColumn(const QString& oldName, const QString& newName)
{
    if (oldName == newName) {
        return;
    }
    for (auto it = mColumnWidths.begin(); it != mColumnWidths.end();) {
        if (it.key().first == oldName) {
            const int occ   = it.key().second;
            const int width = it.value();
            it              = mColumnWidths.erase(it);
            mColumnWidths.insert(qMakePair(newName, occ), width);
        } else {
            ++it;
        }
    }
    for (int i = 0; i < mColumnOrder.size(); ++i) {
        if (mColumnOrder[ i ].first == oldName) {
            mColumnOrder[ i ].first = newName;  // 保持位置，只改名
        }
    }
    QSet< QPair< QString, int > > newHidden;
    for (const QPair< QString, int >& ident : std::as_const(mHiddenColumns)) {
        newHidden.insert(ident.first == oldName ? qMakePair(newName, ident.second) : ident);
    }
    mHiddenColumns = newHidden;
    for (int i = 0; i < mFrozenColumns.size(); ++i) {
        if (mFrozenColumns[ i ].first == oldName) {
            mFrozenColumns[ i ].first = newName;  // 保持冻结区顺序，只改名
        }
    }
}

/**
 * @brief 列删除：移除该列名的全部条目
 * @param name 列名
 */
void DATableViewStateStore::removeColumn(const QString& name)
{
    for (auto it = mColumnWidths.begin(); it != mColumnWidths.end();) {
        if (it.key().first == name) {
            it = mColumnWidths.erase(it);
        } else {
            ++it;
        }
    }
    for (int i = mColumnOrder.size() - 1; i >= 0; --i) {
        if (mColumnOrder[ i ].first == name) {
            mColumnOrder.removeAt(i);
        }
    }
    for (auto it = mHiddenColumns.begin(); it != mHiddenColumns.end();) {
        if (it->first == name) {
            it = mHiddenColumns.erase(it);
        } else {
            ++it;
        }
    }
    for (int i = mFrozenColumns.size() - 1; i >= 0; --i) {
        if (mFrozenColumns[ i ].first == name) {
            mFrozenColumns.removeAt(i);
        }
    }
}

/**
 * @brief 序列化为 <view-state> 段
 *
 * 结构：
 * @code
 * <view-state row-header-width="60">
 *   <column-width name="value" occurrence="0" width="120"/>
 *   <column-order>
 *     <column name="f" occurrence="0"/>
 *     <column name="g" occurrence="0"/>
 *   </column-order>
 *   <column name="h" occurrence="0" hidden="1"/>
 *   <freeze>
 *     <column name="a" occurrence="0"/>
 *     <column name="c" occurrence="0"/>
 *   </freeze>
 * </view-state>
 * @endcode
 * 三个新子元素（006）没有内容就不写；既有 column-width 与 row-header-width 形态不变。
 * <freeze> 的子元素顺序即冻结区显示顺序。旧格式 <freeze columns="N"/> 仍可读
 * （由 normalizeToColumns 展开为前 N 列）。
 * @param doc QDomDocument
 * @param parent 目标父元素（table 元素）
 */
void DATableViewStateStore::toXml(QDomDocument& doc, QDomElement& parent) const
{
    if (isEmpty()) {
        return;
    }
    QDomElement vs = doc.createElement(QStringLiteral("view-state"));
    if (mRowHeaderWidth > 0) {
        vs.setAttribute(QStringLiteral("row-header-width"), mRowHeaderWidth);
    }
    if (mRowHeight > 0) {
        vs.setAttribute(QStringLiteral("row-height"), mRowHeight);
    }
    if (mHorizontalHeaderHeight > 0) {
        vs.setAttribute(QStringLiteral("horizontal-header-height"), mHorizontalHeaderHeight);
    }
    for (auto it = mColumnWidths.constBegin(); it != mColumnWidths.constEnd(); ++it) {
        QDomElement cw = doc.createElement(QStringLiteral("column-width"));
        cw.setAttribute(QStringLiteral("name"), it.key().first);
        cw.setAttribute(QStringLiteral("occurrence"), it.key().second);
        cw.setAttribute(QStringLiteral("width"), it.value());
        vs.appendChild(cw);
    }
    if (!mColumnOrder.isEmpty()) {
        QDomElement co = doc.createElement(QStringLiteral("column-order"));
        for (const QPair< QString, int >& ident : mColumnOrder) {
            QDomElement c = doc.createElement(QStringLiteral("column"));
            c.setAttribute(QStringLiteral("name"), ident.first);
            c.setAttribute(QStringLiteral("occurrence"), ident.second);
            co.appendChild(c);
        }
        vs.appendChild(co);
    }
    for (const QPair< QString, int >& ident : std::as_const(mHiddenColumns)) {
        QDomElement c = doc.createElement(QStringLiteral("column"));
        c.setAttribute(QStringLiteral("name"), ident.first);
        c.setAttribute(QStringLiteral("occurrence"), ident.second);
        c.setAttribute(QStringLiteral("hidden"), 1);
        vs.appendChild(c);
    }
    if (!mFrozenColumns.isEmpty()) {
        QDomElement fz = doc.createElement(QStringLiteral("freeze"));
        for (const QPair< QString, int >& ident : mFrozenColumns) {
            QDomElement c = doc.createElement(QStringLiteral("column"));
            c.setAttribute(QStringLiteral("name"), ident.first);
            c.setAttribute(QStringLiteral("occurrence"), ident.second);
            fz.appendChild(c);
        }
        vs.appendChild(fz);
    }
    parent.appendChild(vs);
}

/**
 * @brief 从 <view-state> 元素反序列化
 *
 * 按直接子节点分派（column-order 的子元素与隐藏条目同名 column，只有遍历直接子节点
 * 才能区分）。未知子标签忽略（前向/后向兼容）。加载后 mColumnSetNormalized 为 false，
 * 页面据此做一次 normalizeToColumns 收敛。
 * @param e view-state 元素
 * @return 始终返回 true（未知子标签忽略，前向兼容）
 */
bool DATableViewStateStore::fromXml(const QDomElement& e)
{
    clear();
    if (e.hasAttribute(QStringLiteral("row-header-width"))) {
        mRowHeaderWidth = e.attribute(QStringLiteral("row-header-width")).toInt();
    }
    if (e.hasAttribute(QStringLiteral("row-height"))) {
        mRowHeight = e.attribute(QStringLiteral("row-height")).toInt();
    }
    if (e.hasAttribute(QStringLiteral("horizontal-header-height"))) {
        mHorizontalHeaderHeight = e.attribute(QStringLiteral("horizontal-header-height")).toInt();
    }
    QDomNode n = e.firstChild();
    while (!n.isNull()) {
        QDomElement child = n.toElement();
        if (!child.isNull()) {
            if (child.tagName() == QLatin1String("column-width")) {
                const QString name = child.attribute(QStringLiteral("name"));
                const int occ      = child.attribute(QStringLiteral("occurrence")).toInt();
                const int width    = child.attribute(QStringLiteral("width")).toInt();
                if (!name.isEmpty() && width > 0) {
                    mColumnWidths.insert(qMakePair(name, occ), width);
                }
            } else if (child.tagName() == QLatin1String("column-order")) {
                // 列名有序表：遍历它的子元素里 tagName == column 的项（name 空则跳过）
                QDomNode cn = child.firstChild();
                while (!cn.isNull()) {
                    QDomElement ce = cn.toElement();
                    if (!ce.isNull() && ce.tagName() == QLatin1String("column")) {
                        const QString name = ce.attribute(QStringLiteral("name"));
                        if (!name.isEmpty()) {
                            mColumnOrder.append(qMakePair(name, ce.attribute(QStringLiteral("occurrence")).toInt()));
                        }
                    }
                    cn = cn.nextSibling();
                }
            } else if (child.tagName() == QLatin1String("column")) {
                // 隐藏条目：hidden 属性非空且非 "0" 才收进集合
                const QString name = child.attribute(QStringLiteral("name"));
                const QString hiddenAttr = child.attribute(QStringLiteral("hidden"));
                if (!name.isEmpty() && !hiddenAttr.isEmpty() && hiddenAttr != QLatin1String("0")) {
                    mHiddenColumns.insert(qMakePair(name, child.attribute(QStringLiteral("occurrence")).toInt()));
                }
            } else if (child.tagName() == QLatin1String("freeze")) {
                // 新格式：子元素 column 列表（顺序 = 冻结区显示顺序）；
                // 旧格式 columns="N" 计数先记下，由 normalizeToColumns 展开
                if (child.hasAttribute(QStringLiteral("columns"))) {
                    const int cols = child.attribute(QStringLiteral("columns")).toInt();
                    if (cols > 0) {
                        mLegacyFreezeCount = cols;
                    }
                } else {
                    QDomNode cn = child.firstChild();
                    while (!cn.isNull()) {
                        QDomElement ce = cn.toElement();
                        if (!ce.isNull() && ce.tagName() == QLatin1String("column")) {
                            const QString name = ce.attribute(QStringLiteral("name"));
                            if (!name.isEmpty()) {
                                const QPair< QString, int > ident
                                    = qMakePair(name, ce.attribute(QStringLiteral("occurrence")).toInt());
                                if (!mFrozenColumns.contains(ident)) {
                                    mFrozenColumns.append(ident);
                                }
                            }
                        }
                        cn = cn.nextSibling();
                    }
                }
            }
        }
        n = n.nextSibling();
    }
    mColumnSetNormalized = false;  // 待页面收敛
    return true;
}

/**
 * @brief 是否没有任何记录
 *
 * 列宽、行头宽、列序（非空）、隐藏（非空）、冻结（非空）任一非空即不为空。
 * @return 全空返回 true
 */
bool DATableViewStateStore::isEmpty() const
{
    return mColumnWidths.isEmpty() && mRowHeaderWidth <= 0 && mRowHeight <= 0 && mHorizontalHeaderHeight <= 0
           && mColumnOrder.isEmpty() && mHiddenColumns.isEmpty() && mFrozenColumns.isEmpty()
           && mLegacyFreezeCount <= 0;
}

/**
 * @brief 清空全部记录
 *
 * 只清数据；收敛标志不在此处置位（fromXml 显式置 false、normalizeToColumns 置 true）。
 */
void DATableViewStateStore::clear()
{
    mColumnWidths.clear();
    mRowHeaderWidth         = -1;
    mRowHeight              = -1;
    mHorizontalHeaderHeight = -1;
    mColumnOrder.clear();
    mHiddenColumns.clear();
    mFrozenColumns.clear();
    mLegacyFreezeCount = -1;
}

}  // end of namespace DA
