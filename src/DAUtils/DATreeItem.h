#ifndef DATREEITEM_H
#define DATREEITEM_H
#include "DAUtilsAPI.h"
#include <QString>
#include <QIcon>
#include <QVariant>
namespace DA
{

class DATree;
///
/// \ingroup SALib
/// \brief 基本树形结构的条目，是SAAbstractData的基类，提供了名称和图标以及父子关系管理功能
/// SAItem可用SATree进行管理，形成树形结构
///
class DAUTILS_API DATreeItem
{
    DA_DECLARE_PRIVATE(DATreeItem)
    friend class DATree;

public:
    using id_type = quintptr;
    /**
     * @brief 预设好的一些属性角色
     */
    enum Role
    {
        RoleName = 0,          ///< 名字
        RoleIcon,              ///< 图标
        RoleValue,             ///< 值
        RoleUserDefine = 1000  ///< 用户自定义
    };
    DATreeItem(DATreeItem* parentItem = nullptr);
    DATreeItem(const QString& text, DATreeItem* parentItem = nullptr);
    DATreeItem(const QIcon& icon, const QString& text, DATreeItem* parentItem = nullptr);
    DATreeItem(const DATreeItem& c);
    virtual ~DATreeItem();
    //重载等号操作符
    DATreeItem& operator=(const DATreeItem& item);

    //名字
    void setName(const QString& name);
    QString getName() const;

    //图标
    void setIcon(const QIcon& icon);
    QIcon getIcon() const;

    // id
    void setID(id_type id);
    id_type getID() const;

    //扩展数据操作相关
    void setProperty(int roleID, const QVariant& var);
    bool isHaveProperty(int roleID) const;
    int getPropertyCount() const;

    //扩展数据的获取操作
    const QVariant& property(int id) const;
    QVariant& property(int id);
    void property(int index, int& id, QVariant& var) const;
    QVariant getProperty(int id, const QVariant& defaultvar = QVariant()) const;
    QMap< int, QVariant > getPropertys() const;

    //父子条目操作相关
    int childItemCount() const;

    //索引子条目
    DATreeItem* childItem(int row) const;

    //获取当前下的所有子节点
    QList< DATreeItem* > getChildItems() const;
    //获取所有子节点的名字
    QList< QString > getChildItemNames() const;

    //追加子条目 item的所有权交由父级item管理
    void appendChild(DATreeItem* item);

    //插入子条目
    void insertChild(DATreeItem* item, int row);

    //清空所有
    void clearChild();

    //判断是否存在子节点
    bool haveChild(DATreeItem* const item) const;

    //提取子节点
    DATreeItem* takeChild(int row);
    bool takeChild(DATreeItem* const item);

    //返回child的索引 O(n)
    int childIndex(DATreeItem* const item) const;

    //删除子对象
    void removeChild(DATreeItem* item);

    //获取父级指针
    DATreeItem* parent() const;

    //用于记录当前所处的层级，如果parent不为nullptr，这个将返回parent下次item对应的层级,如果没有parent，返回-1
    int index() const;

    //判断是否在树节点上，如果此item是在satree上，此函数返回true，否则为false
    bool isOnTree() const;
    //获取树的指针
    DATree* getTree() const;
    //判断是否是顶层，parent为nullptr既是说明在顶层
    bool isRootItem() const;
    //设置树
    void setTree(DATree* tree);
};

DAUTILS_API QDebug& operator<<(QDebug& dbg, const DA::DATreeItem& item);

}  // end of DA

#endif  // SAITEM_H
