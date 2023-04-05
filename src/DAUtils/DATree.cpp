#include "DATree.h"
#include "DATreeItem.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QDataStream>
#include <memory>
namespace DA
{
class DATreePrivate
{
    DA_IMPL_PUBLIC(DATree)
public:
    DATreePrivate(DATree* c);
    std::unique_ptr< DATreeItem > _rootItem;  ///< 树上有个隐藏的顶层item
    QMap< QString, QVariant > m_property;     ///< 记录tree的属性
};

QJsonObject write_item_to_json(DATreeItem* item);
QJsonObject write_property_to_json(QMap< QString, QVariant >* prop);
bool read_item_from_json(const QJsonObject& json, DATreeItem* item);

}

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//==============================================================
// DATreePrivate
//==============================================================
DATreePrivate::DATreePrivate(DATree* c) : q_ptr(c)
{
}
//===================================================
//
//===================================================

DATree::DATree() : d_ptr(new DATreePrivate(this))
{
    d_ptr->_rootItem.reset(new DATreeItem());
    d_ptr->_rootItem->setTree(this);
}

DATree::DATree(const DATree& c) : d_ptr(new DATreePrivate(this))
{
    *this = c;
}

DATree::~DATree()
{
    clear();
}
/**
 * @brief 重载等号操作符实现深拷贝
 * @param tree
 * @return
 */
DATree& DATree::operator=(const DATree& tree)
{
    clear();
    QList< DATreeItem* > items = tree.getItems();
    const int c                = items.size();
    for (int i = 0; i < c; ++i) {
        DATreeItem* item = new DATreeItem();
        *item            = *(items[ c ]);
        appendItem(item);
    }
    return *this;
}

void DATree::clear()
{
    d_ptr->_rootItem.reset();
    d_ptr->m_property.clear();  //属性清除
}
/**
 * @brief 获取子节点的个数
 * @return
 */
int DATree::getItemCount() const
{
    return d_ptr->_rootItem->childItemCount();
}
/**
 * @brief 索引子条目
 * @param row 0base的行数索引
 * @return
 */
DATreeItem* DATree::getItem(int row) const
{
    return d_ptr->_rootItem->childItem(row);
}
/**
 * @brief 获取所有子节点
 * @return
 */
QList< DATreeItem* > DATree::getItems() const
{
    return d_ptr->_rootItem->getChildItems();
}
/**
 * @brief 追加子条目
 * @param item
 * item的所有权交由satree管理
 */
void DATree::appendItem(DATreeItem* item)
{
    //会自动设置树指针
    d_ptr->_rootItem->appendChild(item);
}
/**
 * @brief 插入子条目
 * @param item
 * @param row
 */
void DATree::insertItem(DATreeItem* item, int row)
{
    //会自动设置树指针
    d_ptr->_rootItem->insertChild(item, row);
}
/**
 * @brief 判断是否存在子节点
 * @param item
 * @return
 */
bool DATree::haveItem(DATreeItem* item) const
{
    return (d_ptr->_rootItem->childIndex(item)) >= 0;
}
/**
 * @brief 把item解除satree的关系
 * @param item
 */
void DATree::takeItem(DATreeItem* item)
{
    //会自动设置树指针为空
    d_ptr->_rootItem->takeChild(item);
}
/**
 * @brief 根据索引把item返回，同时解除satree的关系
 * @param row 索引
 */
DATreeItem* DATree::takeItemByIndex(int row)
{
    //会自动设置树指针为空
    return d_ptr->_rootItem->takeChild(row);
}
/**
 * @brief 返回item对应的树层级
 * @param item
 * @return
 */
int DATree::indexOfItem(DATreeItem* const item) const
{
    return d_ptr->_rootItem->childIndex(item);
}

/**
 * @brief 设置属性
 * @param name 属性名
 * @param var 属性值
 */
void DATree::setTreeProperty(const QString& name, const QVariant& var)
{
    d_ptr->m_property.insert(name, var);
}

/**
 * @brief 获取属性
 * @param name 属性名
 * @param defaultVal 属性默认值
 * @return 如果有返回对应属性值，如果没有返回默认值
 */
QVariant DATree::getTreeProperty(const QString& name, const QVariant& defaultVal) const
{
    return d_ptr->m_property.value(name, defaultVal);
}

/**
 * @brief 移除属性
 * @param name 属性名
 */
void DATree::removeTreeProperty(const QString& name)
{
    d_ptr->m_property.remove(name);
}

/**
 * @brief 获取所有属性名
 * @return 属性名列表
 */
QList< QString > DATree::getTreePropertyNames() const
{
    return d_ptr->m_property.keys();
}

/**
 * @brief 获取所有属性
 * @return
 */
QMap< QString, QVariant > DATree::getTreePropertys() const
{
    return d_ptr->m_property;
}

/**
 * @brief 参考QStandardItemModel的invisibleRootItem
 * @return
 */
DATreeItem* DATree::invisibleRootItem() const
{
    return d_ptr->_rootItem.get();
}

/**
 * @brief 判断是否为顶层item
 * @param item
 * @return
 */
bool DATree::isRootItem(const DATreeItem* item) const
{
    return (item->parent() == d_ptr->_rootItem.get());
}

/**
 * @brief 获取父级节点下所有子节点的名字
 *
 * @note 如果父级节点还是nullptr，则获取顶层节点的名字
 * @param parent 父级节点
 * @return
 */
QList< QString > DATree::getChildItemNames(const DATreeItem* parent) const
{
    if (nullptr == parent) {
        return invisibleRootItem()->getChildItemNames();
    }
    return parent->getChildItemNames();
}

QDebug& DA::operator<<(QDebug& dbg, const DA::DATree& tree)
{
    QList< DA::DATreeItem* > items = tree.getItems();
    for (const DA::DATreeItem* i : qAsConst(items)) {
        dbg << *(i);
    }
    return dbg;
}

/**
 * @brief 把satree转换为json string
 * @param tree tree指针
 * @return
 */
QString DA::toJson(const DA::DATree* tree)
{
    QList< DATreeItem* > items = tree->getItems();
    const auto c               = items.size();
    QJsonArray mainJTree;
    for (auto i = 0; i < c; ++i) {
        DATreeItem* item = items[ i ];
        mainJTree.append(write_item_to_json(item));
    }
    QJsonObject propobj;
    QList< QString > props = tree->getTreePropertyNames();
    for (const QString& k : props) {
        propobj.insert(k, QJsonValue::fromVariant(tree->getTreeProperty(k)));
    }
    QJsonObject mainobj;
    mainobj.insert("prop", propobj);
    mainobj.insert("item", mainJTree);
    QJsonDocument json(mainobj);
    return json.toJson();
}

QJsonObject DA::write_item_to_json(DATreeItem* item)
{
    QJsonObject itemObj;
    itemObj.insert("name", item->getName());
    QIcon icon = item->getIcon();
    if (!icon.isNull()) {
        QByteArray byte;
        QDataStream st(&byte, QIODevice::ReadWrite);
        st << icon;
        itemObj.insert("icon", QString(byte.toBase64()));
    }
    const auto c = item->getPropertyCount();
    if (c > 0) {
        QJsonObject propObj;
        for (auto i = 0; i < c; ++i) {
            int id;
            QVariant var;
            item->property(i, id, var);
            propObj.insert(QString::number(id), QJsonValue::fromVariant(var));
        }
        itemObj.insert("porperty", propObj);
    }
    const auto cc = item->childItemCount();
    if (cc > 0) {
        QJsonArray jArrVal;
        for (auto i = 0; i < cc; ++i) {
            QJsonObject cj = write_item_to_json(item->childItem(i));
            jArrVal.append(cj);
        }
        itemObj.insert("childItems", jArrVal);
    }
    return itemObj;
}

QJsonObject DA::write_property_to_json(QMap< QString, QVariant >* prop)
{
    QJsonObject objprop;
    for (auto i = prop->begin(); i != prop->end(); ++i) {
        if (i.value().isValid()) {
            objprop[ i.key() ] = QJsonValue::fromVariant(i.value());
        }
    }
    return objprop;
}

bool DA::read_item_from_json(const QJsonObject& json, DATreeItem* item)
{
    auto i = json.find("name");
    if (i != json.end()) {
        item->setName(i.value().toString());
    }
    i = json.find("icon");
    if (i != json.end()) {
        QIcon icon;
        QByteArray byte = QByteArray::fromBase64(i.value().toString().toLocal8Bit());
        QDataStream st(&byte, QIODevice::ReadWrite);
        st >> icon;
        if (!icon.isNull())
            item->setIcon(icon);
    }
    i = json.find("porperty");
    if (i != json.end()) {
        if (i.value().isObject()) {
            QJsonObject propObj = i.value().toObject();
            for (auto oi = propObj.begin(); oi != propObj.end(); ++oi) {
                bool isKeyOk = false;
                int propID   = oi.key().toInt(&isKeyOk);
                if (!isKeyOk)
                    continue;
                QVariant var = oi.value().toVariant();
                item->setProperty(propID, var);
            }
        }
    }
    i = json.find("childItems");
    if (i != json.end()) {
        //读取子节点
        if (i.value().isArray()) {
            QJsonArray jArrVal = i.value().toArray();
            for (auto i = jArrVal.begin(); i != jArrVal.end(); ++i) {
                std::unique_ptr< DATreeItem > childitem(new DATreeItem());
                if (read_item_from_json((*i).toObject(), childitem.get())) {
                    item->appendChild(childitem.release());
                }
            }
        }
    }
    return true;
}

/**
 * @brief 从标准json sting转换到tree
 * @param json jsonstring
 * @param tree 待修改的tree
 * @return 如果转换成功返回true
 */
bool DA::fromJson(const QString& json, DA::DATree* tree)
{
    QJsonParseError error;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (!jsonDocument.isObject()) {
        return false;
    }
    QJsonObject mainobj = jsonDocument.object();
    QJsonObject propobj = mainobj.value("prop").toObject();
    QJsonArray jsonArr  = mainobj.value("item").toArray();

    // 解析属性
    for (auto i = propobj.begin(); i != propobj.end(); ++i) {
        tree->setTreeProperty(i.key(), i.value().toVariant());
    }

    //解析值
    const auto size = jsonArr.size();
    for (int i = 0; i < size; ++i) {
        std::unique_ptr< DATreeItem > item(new DATreeItem());
        QJsonValue v = jsonArr[ i ];
        if (read_item_from_json(v.toObject(), item.get())) {
            tree->appendItem(item.release());
        }
    }
    return true;
}
