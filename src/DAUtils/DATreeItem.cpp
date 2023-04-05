#include "DATreeItem.h"
#include <QList>
#include <QVector>
#include <QDebug>
#include "DATree.h"
#include "da_order_map.hpp"

namespace DA
{

class DATreeItemPrivate
{
    DA_IMPL_PUBLIC(DATreeItem)
public:
    DATreeItem* m_parent;
    QList< DATreeItem* > m_childs;
    int _index;  ///<用于记录当前所处的层级，如果parent不为nullptr，这个将返回parent下item对应的层级
    DATreeItem::id_type m_id;
    DATree* m_tree;                                                                  ///< 绑定的树
    da_order_map< int, QVariant, QVector< int >, QVector< QVariant > > m_propertys;  ///< 定义属性
    DATreeItemPrivate(DATreeItem* par)
        : q_ptr(par), m_parent(nullptr), _index(-1), m_id(reinterpret_cast< DATreeItem::id_type >(par)), m_tree(nullptr)
    {
        m_propertys[ DATreeItem::RoleName ]  = QString();
        m_propertys[ DATreeItem::RoleIcon ]  = QIcon();
        m_propertys[ DATreeItem::RoleValue ] = QVariant();
    }

    ~DATreeItemPrivate()
    {
        clearChild();
    }

    void clearChild()
    {
        QList< DATreeItem* > childs = m_childs;

        for (int i = 0; i < childs.size(); ++i) {
            delete childs[ i ];
        }
        m_childs.clear();
    }

    void updateFieldCount(int startRow = 0)
    {
        const int cc = m_childs.size();

        for (int i = startRow; i < cc; ++i) {
            m_childs[ i ]->d_ptr->_index = i;
        }
    }
};

//把from的子对象都复制一份到to
void copy_childs(const DATreeItem* from, DATreeItem* to);

//打印一个item内容
QDebug& print_one_item(QDebug& dbg, const DATreeItem& item, const QString& prefix, bool isNewline = true);
QDebug& print_item_and_child_items(QDebug& dbg, const DATreeItem& item, int indent);
}
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
//
//===================================================

void DA::copy_childs(const DATreeItem* from, DATreeItem* to)
{
    QList< DATreeItem* > items = from->getChildItems();
    const auto size            = items.size();

    for (auto i = 0; i < size; ++i) {
        DATreeItem* tmp = new DATreeItem();
        //如果还有子item，会触发递归
        *tmp = *(items[ i ]);
        to->appendChild(tmp);
    }
}

DATreeItem::DATreeItem(DATreeItem* parentItem) : d_ptr(new DATreeItemPrivate(this))
{
    if (parentItem) {
        parentItem->appendChild(this);
    }
}

DATreeItem::DATreeItem(const QString& text, DATreeItem* parentItem) : d_ptr(new DATreeItemPrivate(this))
{
    setName(text);
    if (parentItem) {
        parentItem->appendChild(this);
    }
}

DATreeItem::DATreeItem(const QIcon& icon, const QString& text, DATreeItem* parentItem)
    : d_ptr(new DATreeItemPrivate(this))
{
    setName(text);
    setIcon(icon);
    if (parentItem) {
        parentItem->appendChild(this);
    }
}

/**
 * @brief 拷贝构造函数
 * @param c
 */
DATreeItem::DATreeItem(const DATreeItem& c) : d_ptr(new DATreeItemPrivate(this))
{
    *this = c;
}

DATreeItem::~DATreeItem()
{
    clearChild();
    DATreeItem* par = parent();

    if (par) {
        int indexOfPar = par->childIndex(this);
        if (indexOfPar >= 0) {
            par->d_ptr->m_childs.removeAt(indexOfPar);
            par->d_ptr->updateFieldCount(indexOfPar);
        }
    }
    if (d_ptr->m_tree) {
        // d_ptr->m_tree->
    }
}

/**
 * @brief 等号操作符
 * @param item 另外等待拷贝的item
 * @return 返回自身引用
 * @note m_parent,m_fieldRow,id 不会发生拷贝
 */
DATreeItem& DATreeItem::operator=(const DATreeItem& item)
{
    clearChild();
    d_ptr->m_propertys = item.d_ptr->m_propertys;
    //复制子对象
    copy_childs(&item, this);
    return (*this);
}

///
/// \brief 设置条目名称
/// \param name 名称
///
void DATreeItem::setName(const QString& name)
{
    d_ptr->m_propertys.orderValue(static_cast< int >(RoleName)) = name;
}

///
/// \brief 条目名称
/// \return
///
QString DATreeItem::getName() const
{
    return (d_ptr->m_propertys.orderValue(static_cast< int >(RoleName)).toString());
}

///
/// \brief 设置条目图标
/// \param icon 图标
///
void DATreeItem::setIcon(const QIcon& icon)
{
    d_ptr->m_propertys.orderValue(static_cast< int >(RoleIcon)) = icon;
}

///
/// \brief 获取条目图标
/// \return
///
QIcon DATreeItem::getIcon() const
{
    return (d_ptr->m_propertys.orderValue(static_cast< int >(RoleIcon)).value< QIcon >());
}

///
/// \brief 条目id
///
/// \return
///
DATreeItem::id_type DATreeItem::getID() const
{
    return (d_ptr->m_id);
}

///
/// \brief 设置扩展数据
/// \param roleID 标示id
/// \param var 数据内容
///
void DATreeItem::setProperty(int roleID, const QVariant& var)
{
    d_ptr->m_propertys[ roleID ] = var;
}

///
/// \brief 判断是否存在id对应的扩展数据
/// \param id 标示id
/// \return
///
bool DATreeItem::isHaveProperty(int roleID) const
{
    return (d_ptr->m_propertys.contains(roleID));
}

///
/// \brief 扩展数据的个数
/// \return
///
int DATreeItem::getPropertyCount() const
{
    return (d_ptr->m_propertys.size());
}

///
/// \brief 根据id获取扩展数据
/// \param id
/// \return
///
const QVariant& DATreeItem::property(int id) const
{
    return (d_ptr->m_propertys[ id ]);
}

///
/// \brief 根据id获取扩展数据
/// \param id
/// \return 获取为引用，修改将直接影响条目保存的数据内容
///
QVariant& DATreeItem::property(int id)
{
    return (d_ptr->m_propertys[ id ]);
}

///
/// \brief 根据索引顺序获取扩展数据，此函数仅仅为了方便遍历所有扩展数据用
/// \param index 索引顺序
/// \param id 返回hash的key
/// \param var 返回hash的value
///
void DATreeItem::property(int index, int& id, QVariant& var) const
{
    auto ite = d_ptr->m_propertys.cbegin();

    ite = ite + index;
    id  = ite.key();
    var = ite.value();
}

/**
 * @brief 获取属性值
 * @param id
 * @param defaultvar
 * @return
 */
QVariant DATreeItem::getProperty(int id, const QVariant& defaultvar) const
{
    return (d_ptr->m_propertys.value(id, defaultvar));
}

/**
 * @brief 获取所有属性
 * @return
 */
QMap< int, QVariant > DATreeItem::getPropertys() const
{
    return (d_ptr->m_propertys.toMap());
}

///
/// \brief 子条目的数目
/// \return
///
int DATreeItem::childItemCount() const
{
    return (d_ptr->m_childs.size());
}

///
/// \brief 索引子条目
/// \param row 0base的行数索引
/// \return
///
DATreeItem* DATreeItem::childItem(int row) const
{
    return (d_ptr->m_childs[ row ]);
}

/**
 * @brief 获取所有子节点
 * @return
 */
QList< DATreeItem* > DATreeItem::getChildItems() const
{
    return (d_ptr->m_childs);
}

/**
 * @brief 获取所有子节点的名字
 * @return
 */
QList< QString > DATreeItem::getChildItemNames() const
{
    QList< QString > res;
    QList< DATreeItem* > ci = getChildItems();
    for (auto i : ci) {
        res.append(i->getName());
    }
    return res;
}

///
/// \brief 追加子条目
/// \param item
/// item的所有权交由父级item管理
///
void DATreeItem::appendChild(DATreeItem* item)
{
    item->d_ptr->_index   = d_ptr->m_childs.size();
    item->d_ptr->m_parent = this;
    item->d_ptr->m_tree   = this->d_ptr->m_tree;
    d_ptr->m_childs.append(item);
}

/**
 * @brief 插入子条目
 * @param item
 * @param row 如果row大于等于childcount,row=childcount,如果row小于0，则row等于0
 */
void DATreeItem::insertChild(DATreeItem* item, int row)
{
    //如果row大于等于childcount,row=childcount
    if (row >= d_ptr->m_childs.size()) {
        row = d_ptr->m_childs.size();
    } else if (row < 0) {
        row = 0;
    }
    item->d_ptr->_index = row;
    d_ptr->m_childs.insert(row, item);
    item->d_ptr->m_parent = this;
    item->d_ptr->m_tree   = this->d_ptr->m_tree;
    //修改后面的item的m_fieldRow
    d_ptr->updateFieldCount(row + 1);
}

///
/// \brief 清除所有字条目，包括内存
///
void DATreeItem::clearChild()
{
    d_ptr->clearChild();
}

/**
 * @brief 判断是否存在子节点
 * @param item 节点
 * @return 如果存在返回true
 */
bool DATreeItem::haveChild(DATreeItem* const item) const
{
    return (d_ptr->m_childs.contains(item));
}

///
/// \brief 提取条目，此时字条目的内容将不归此条目管理
/// \param row
/// \return
///
DATreeItem* DATreeItem::takeChild(int row)
{
    DATreeItem* item = d_ptr->m_childs.takeAt(row);

    item->d_ptr->m_parent = nullptr;
    item->d_ptr->m_tree   = nullptr;
    //修改后面的item的m_fieldRow
    d_ptr->updateFieldCount(row + 1);
    return (item);
}

/**
 * @brief 提取出子节点
 * @param childItem
 */
bool DATreeItem::takeChild(DATreeItem* const item)
{
    int index = childIndex(item);

    if (index < 0) {
        return (false);
    }
    takeChild(index);
    return (true);
}

/**
 * @brief 返回child的索引
 * @param item
 * @return 返回这个child对应的索引
 * @note 此函数操作的时间复杂度为O(n),若没有，返回-1
 */
int DATreeItem::childIndex(DATreeItem* const item) const
{
    return (d_ptr->m_childs.indexOf(item));
}

/**
 * @brief 删除子对象
 * @param item 子对象的指针，如果没有将忽略
 * @note 此操作会回收内存
 */
void DATreeItem::removeChild(DATreeItem* item)
{
    int index = childIndex(item);

    if (index >= 0) {
        //删除会自动和父节点的m_childs脱离关系
        delete item;
        //修改索引item的m_fieldRow
        d_ptr->updateFieldCount(index);
    }
}

///
/// \brief 获取条目的父级条目，如果没有，返回nullptr
/// \return  如果没有，返回nullptr
///
DATreeItem* DATreeItem::parent() const
{
    if (d_ptr->m_parent) {
        if (d_ptr->m_parent->isRootItem()) {
            return nullptr;
        }
    }
    return (d_ptr->m_parent);
}

///
/// \brief 获取当前条目所在父级条目的行数，如果当前条目是子条目，这个函数返回这个字条目是对应父级条目的第几行
/// \note \sa takeChild \sa insertChild 都会影响此函数的结果
/// \return
///
int DATreeItem::index() const
{
    return (d_ptr->_index);
}

/**
 * @brief 判断是否在树节点上
 * @return 如果此item是在satree上，此函数返回true，否则为false
 */
bool DATreeItem::isOnTree() const
{
    return ((d_ptr->m_tree) != nullptr);
}

/**
 * @brief 获取树的指针
 * @return
 */
DATree* DATreeItem::getTree() const
{
    return d_ptr->m_tree;
}

/**
 * @brief 判断是否是顶层
 *
 * @note 如果是一个悬挂item返回false
 * @return
 */
bool DATreeItem::isRootItem() const
{
    if (d_ptr->m_tree) {
        return d_ptr->m_tree->isRootItem(this);
    }
    return false;
}

void DATreeItem::setID(id_type id)
{
    d_ptr->m_id = id;
}

/**
 * @brief 设置树
 * @param tree
 */
void DATreeItem::setTree(DATree* tree)
{
    if (d_ptr->m_tree == tree) {
        return;
    } else if (d_ptr->m_tree) {
        d_ptr->m_tree->takeItem(this);
    }
    d_ptr->m_tree = tree;
}

QDebug& DA::print_one_item(QDebug& dbg, const DATreeItem& item, const QString& prefix, bool isNewline)
{
    int pc = item.getPropertyCount();

    if (pc > 0) {
        dbg.noquote() << prefix << item.getName() << "{";
        int id;
        QVariant val;
        item.property(0, id, val);
        dbg.noquote() << id << ":" << val;
        for (int i = 1; i < pc; ++i) {
            item.property(i, id, val);
            dbg.noquote() << "," << id << ":" << val;
        }
        dbg.noquote() << "}";
    } else {
        dbg.noquote() << prefix << item.getName();
    }
    if (isNewline) {
        dbg << "\n";
    }
    return (dbg);
}

QDebug& DA::print_item_and_child_items(QDebug& dbg, const DATreeItem& item, int indent)
{
    QString str(indent, ' ');

    str += QStringLiteral("└");
    print_one_item(dbg, item, str);

    QList< DA::DATreeItem* > cis = item.getChildItems();
    for (const DA::DATreeItem* i : qAsConst(cis)) {
        print_item_and_child_items(dbg, *i, indent + 2);
    }
    return (dbg);
}

/**
 * @brief 输出到qdebug
 * @param dbg
 * @param item
 * @return
 */
QDebug& DA::operator<<(QDebug& dbg, const DA::DATreeItem& item)
{
    dbg = DA::print_one_item(dbg, item, "");

    QList< DA::DATreeItem* > cis = item.getChildItems();
    for (const DA::DATreeItem* i : qAsConst(cis)) {
        dbg = DA::print_item_and_child_items(dbg, *i, 2);
    }
    return (dbg);
}
