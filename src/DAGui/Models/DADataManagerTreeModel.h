#ifndef DADATAMANAGERTREEMODEL_H
#define DADATAMANAGERTREEMODEL_H
#include <functional>
#include <QObject>
#include <QStandardItemModel>
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DADataManager.h"
class QMimeData;
#ifndef DAAPPDATAMANAGERTREEITEM_USERTYPE
#define DAAPPDATAMANAGERTREEITEM_USERTYPE (QStandardItem::UserType + 1)
#endif
namespace DA
{
DA_IMPL_FORWARD_DECL(DADataManagerTreeModel)

/**
 * @brief 用于存放数据的item
 */
class DAGUI_API DADataManagerTreeItem : public QStandardItem
{
    friend class DADataManagerTreeModel;

public:
    enum TreeItemType
    {
        ItemUnknow,  ///< 未知类型
        ItemData,    ///< 数据标签
        ItemFolder   ///< 文件夹标签
    };

protected:
    DADataManagerTreeItem();

public:
    DADataManagerTreeItem(const DA::DAData& d);
    DADataManagerTreeItem(const QString& n);
    ~DADataManagerTreeItem() override;
    //获取item对应的data
    DAData toData() const;
    // rtti
    int type() const override;
    //深度
    int depth() const;
    //获取树节点属性
    TreeItemType getTreeItemType() const;
    //判断是否是folder
    bool isFolder() const;
    bool isData() const;
    QStandardItem* clone() const override;

protected:
    DADataManagerTreeItem(const DADataManagerTreeItem& other);
    DADataManagerTreeItem& operator=(const DADataManagerTreeItem& other);
};

/**
 * @brief 此类用于组织变量树，和@sa DAAppDataManager 结合，对变量进行类似文件夹样式的归类
 */
class DAGUI_API DADataManagerTreeModel : public QStandardItemModel
{
    Q_OBJECT
    DA_IMPL(DADataManagerTreeModel)
public:
    DADataManagerTreeModel(QObject* parent = nullptr);
    DADataManagerTreeModel(DADataManager* p, QObject* parent = nullptr);
    ~DADataManagerTreeModel();
    //通过data查找对应的item，如果没有返回nullptr
    DADataManagerTreeItem* dataToItem(const DA::DAData& d) const;
    //添加文件夹
    DADataManagerTreeItem* addFolder(const QString& name, DADataManagerTreeItem* parentFolder = nullptr);
    //移除文件夹
    void removeFolder(DADataManagerTreeItem* f);
    //获取父级节点下所有子节点的名字,如果父级节点还是nullptr，则获取顶层节点的名字
    QList< QString > getChildItemNames(const QStandardItem* parent = nullptr) const;

    //从index获取item
    DADataManagerTreeItem* treeItemFromIndex(const QModelIndex& i) const;
    //设置datamanager
    void setDataManager(DADataManager* p);
    //获取dataMgr
    DADataManager* getDataManager() const;

public:
    virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

protected:
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

private:
    void init();
private slots:
    void onDataAdded(const DA::DAData& d);
    void onDataBeginRemoved(const DA::DAData& d, int dataIndex);
    void onDataChanged(const DA::DAData& d, DADataManager::ChangeType t);
};

//以下是递归函数用于遍历
bool standardItemIterator(QStandardItem* startItem, std::function< bool(QStandardItem*, QStandardItem*) > fun, bool firstColumnOnly = false);
//遍历所有的数据文件
bool daAppDataManagerTreeItemIterator(QStandardItem* startItem,
                                      std::function< bool(QStandardItem*, DADataManagerTreeItem*) > fun,
                                      DADataManagerTreeItem::TreeItemType type = DADataManagerTreeItem::ItemUnknow);

}

#endif  // DADATAMANAGERTREEMODEL_H
