#ifndef SATREE_H
#define SATREE_H
#include "DAUtilsAPI.h"
#include "DATreeItem.h"
#include <QVariant>

namespace DA
{
class DATreePrivate;
/**
 * @brief 通用树形结构数据存储
 * 支持任意拷贝和赋值
 */
class DAUTILS_API DATree
{
    DA_IMPL(DATree)
    friend class DATreeItem;

public:
    DATree();
    DATree(const DATree& c);
    virtual ~DATree();
    //重载等号操作符
    DATree& operator=(const DATree& tree);

    //清空所有节点和属性
    void clear();

    //父子条目操作相关
    int getItemCount() const;

    //索引子条目
    DATreeItem* getItem(int row) const;

    //获取当前下的所有子节点
    QList< DATreeItem* > getItems() const;

    //追加子条目
    void appendItem(DATreeItem* item);

    //插入子条目
    void insertItem(DATreeItem* item, int row);

    //判断是否存在子节点
    bool haveItem(DATreeItem* item) const;

    //把item解除satree的关系
    void takeItem(DATreeItem* item);
    DATreeItem* takeItemByIndex(int row);

    //返回item对应的树层级
    int indexOfItem(DATreeItem* const item) const;

    //设置tree的属性，tree可以携带附加信息
    void setTreeProperty(const QString& name, const QVariant& var);

    //获取属性
    QVariant getTreeProperty(const QString& name, const QVariant& defaultVal = QVariant()) const;

    //移除属性
    void removeTreeProperty(const QString& name);

    //获取所有属性名
    QList< QString > getTreePropertyNames() const;

    //获取所有属性
    QMap< QString, QVariant > getTreePropertys() const;

    //参考QStandardItemModel的invisibleRootItem
    DATreeItem* invisibleRootItem() const;

    //判断是否为顶层item
    bool isRootItem(const DATreeItem* item) const;

    //获取父级节点下所有子节点的名字,如果父级节点还是nullptr，则获取顶层节点的名字
    QList< QString > getChildItemNames(const DATreeItem* parent = nullptr) const;
};

DAUTILS_API QDebug& operator<<(QDebug& dbg, const DA::DATree& tree);
//把satree转换为json string
DAUTILS_API QString toJson(const DA::DATree* tree);

//从标准json sting转换到tree
DAUTILS_API bool fromJson(const QString& json, DA::DATree* tree);
}  // end of DA
Q_DECLARE_METATYPE(DA::DATree)

// debug输出

#endif  // SATREE_H
