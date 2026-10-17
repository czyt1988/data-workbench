#include "DATableViewStateStore.h"

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
 * @brief 列改名迁移
 *
 * 只改键的列名部分，occurrence 不变（改名不改变列的位置序）。
 * 若目标 (newName, occurrence) 已有记录，以迁移过来的值覆盖（后到的改名生效）。
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
}

/**
 * @brief 序列化为 <view-state> 段
 *
 * 结构：
 * @code
 * <view-state row-header-width="60">
 *   <column-width name="value" occurrence="0" width="120"/>
 * </view-state>
 * @endcode
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
    for (auto it = mColumnWidths.constBegin(); it != mColumnWidths.constEnd(); ++it) {
        QDomElement cw = doc.createElement(QStringLiteral("column-width"));
        cw.setAttribute(QStringLiteral("name"), it.key().first);
        cw.setAttribute(QStringLiteral("occurrence"), it.key().second);
        cw.setAttribute(QStringLiteral("width"), it.value());
        vs.appendChild(cw);
    }
    parent.appendChild(vs);
}

/**
 * @brief 从 <view-state> 元素反序列化
 * @param e view-state 元素
 * @return 始终返回 true（未知子标签忽略，前向兼容）
 */
bool DATableViewStateStore::fromXml(const QDomElement& e)
{
    clear();
    if (e.hasAttribute(QStringLiteral("row-header-width"))) {
        mRowHeaderWidth = e.attribute(QStringLiteral("row-header-width")).toInt();
    }
    QDomNode n = e.firstChild();
    while (!n.isNull()) {
        QDomElement child = n.toElement();
        if (!child.isNull() && child.tagName() == QLatin1String("column-width")) {
            const QString name = child.attribute(QStringLiteral("name"));
            const int occ      = child.attribute(QStringLiteral("occurrence")).toInt();
            const int width    = child.attribute(QStringLiteral("width")).toInt();
            if (!name.isEmpty() && width > 0) {
                mColumnWidths.insert(qMakePair(name, occ), width);
            }
        }
        n = n.nextSibling();
    }
    return true;
}

/**
 * @brief 是否没有任何记录
 * @return 全空返回 true
 */
bool DATableViewStateStore::isEmpty() const
{
    return mColumnWidths.isEmpty() && mRowHeaderWidth <= 0;
}

/**
 * @brief 清空全部记录
 */
void DATableViewStateStore::clear()
{
    mColumnWidths.clear();
    mRowHeaderWidth = -1;
}

}  // end of namespace DA
