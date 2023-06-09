﻿#include "DADataManagerTreeModel.h"
// Qt
#include <QHash>
#include <QDebug>
#include <QMimeData>
// stl
#include <limits>

// DAData
#include "DADataManager.h"
#include "DAData.h"
#include "pandas/DAPyDataFrame.h"
//
#include "DADataManagerTableModel.h"
/**
 * @def 定义角色，是否为数据，（bool）
 */
#ifndef DAAPPDATAMANAGERTREE_ROLE_ITEM_TYPE
#define DAAPPDATAMANAGERTREE_ROLE_ITEM_TYPE (Qt::UserRole + 10)
#endif
/**
 * @def 定义角色，是否为文件夹，（bool）
 */
#ifndef DAAPPDATAMANAGERTREE_ROLE_DATA
#define DAAPPDATAMANAGERTREE_ROLE_DATA (Qt::UserRole + 20)
#endif

namespace DA
{
class DADataManagerTreeModelPrivate
{
    DA_IMPL_PUBLIC(DADataManagerTreeModel)
public:
    DADataManagerTreeModelPrivate(DADataManagerTreeModel* p);

public:
    DADataManager* _dataMgr;
};

//===================================================
// DAAppDataManagerTreePrivate
//===================================================

DADataManagerTreeModelPrivate::DADataManagerTreeModelPrivate(DADataManagerTreeModel* p) : q_ptr(p)
{
}

//===================================================
// DATreeDataItem
//===================================================
/**
 * @brief 构造为数据item
 * @param d
 */
DADataManagerTreeItem::DADataManagerTreeItem() : QStandardItem()
{
    setData((int)ItemUnknow, DAAPPDATAMANAGERTREE_ROLE_ITEM_TYPE);
    setEditable(false);
}

DADataManagerTreeItem::DADataManagerTreeItem(const DAData& d) : QStandardItem(d.getName())
{
    setIcon(DADataManagerTableModel::dataToIcon(d));
    setData((int)ItemData, DAAPPDATAMANAGERTREE_ROLE_ITEM_TYPE);
    setData(d.id(), DAAPPDATAMANAGERTREE_ROLE_DATA);
    setEditable(false);
    setDropEnabled(false);
}

/**
 * @brief 构造为文件夹item
 * @param n
 */
DADataManagerTreeItem::DADataManagerTreeItem(const QString& n) : QStandardItem(n)
{
    setIcon(QIcon(":/icon/icon/folder.svg"));
    setData((int)ItemFolder, DAAPPDATAMANAGERTREE_ROLE_ITEM_TYPE);
    setEditable(false);
    setDropEnabled(true);
    setColumnCount(2);
}

DADataManagerTreeItem::DADataManagerTreeItem(const DADataManagerTreeItem& other) : QStandardItem(other)
{
}

DADataManagerTreeItem::~DADataManagerTreeItem()
{
}

/**
 * @brief 获取item对应的data
 * @return
 */
DAData DADataManagerTreeItem::toData() const
{
    QVariant v = data(DAAPPDATAMANAGERTREE_ROLE_DATA);
    if (!v.isValid()) {
        return DAData();
    }
    bool isok         = false;
    DAData::IdType id = v.toULongLong(&isok);
    if (!isok) {
        return DAData();
    }
    DADataManagerTreeModel* tree = qobject_cast< DADataManagerTreeModel* >(model());
    if (nullptr == tree) {
        return DAData();
    }
    return tree->getDataManager()->getDataById(id);
}

int DADataManagerTreeItem::type() const
{
    return DAAPPDATAMANAGERTREEITEM_USERTYPE;
}

/**
 * @brief 深度
 * @return
 */
int DADataManagerTreeItem::depth() const
{
    int d            = 0;
    QStandardItem* p = parent();
    while (p != nullptr) {
        ++d;
        p = p->parent();
    }
    return d;
}

/**
 * @brief 获取item的类型
 * @return
 */
DADataManagerTreeItem::TreeItemType DADataManagerTreeItem::getTreeItemType() const
{
    QVariant v = data(DAAPPDATAMANAGERTREE_ROLE_ITEM_TYPE);
    if (!v.isValid()) {
        return ItemUnknow;
    }
    return static_cast< DADataManagerTreeItem::TreeItemType >(v.toInt());
}

/**
 * @brief 判断是否是folder
 * @return
 */
bool DADataManagerTreeItem::isFolder() const
{
    return (getTreeItemType() == ItemFolder);
}
/**
 * @brief 判断是否是data
 * @return
 */
bool DADataManagerTreeItem::isData() const
{
    return (getTreeItemType() == ItemData);
}

QStandardItem* DADataManagerTreeItem::clone() const
{
    return new DADataManagerTreeItem(*this);
}

DADataManagerTreeItem& DADataManagerTreeItem::operator=(const DADataManagerTreeItem& other)
{
    QStandardItem::operator=(other);
    return *this;
}

//===================================================
// DAAppDataManagerTree
//===================================================
DADataManagerTreeModel::DADataManagerTreeModel(QObject* parent)
    : QStandardItemModel(parent), d_ptr(new DADataManagerTreeModelPrivate(this))
{
    init();
}

DADataManagerTreeModel::DADataManagerTreeModel(DADataManager* p, QObject* parent)
    : QStandardItemModel(parent), d_ptr(new DADataManagerTreeModelPrivate(this))
{
    init();
    setDataManager(p);
}

void DADataManagerTreeModel::init()
{
    qRegisterMetaType< DA::DAData >("DA::DAData");
    setItemPrototype(new DADataManagerTreeItem());
    setHorizontalHeaderItem(0, new QStandardItem(tr("name")));
    setHorizontalHeaderItem(1, new QStandardItem(tr("property")));
}

DADataManagerTreeModel::~DADataManagerTreeModel()
{
}

/**
 * @brief 通过data查找对应的item，如果没有返回nullptr
 * @param d
 * @note 注意此操作是O(n)
 * @return
 */
DADataManagerTreeItem* DADataManagerTreeModel::dataToItem(const DAData& d) const
{
    DADataManagerTreeItem* res = nullptr;
    DA::daAppDataManagerTreeItemIterator(
    invisibleRootItem(),
    [ &res, &d ](QStandardItem* par, DADataManagerTreeItem* i) -> bool {
        qDebug() << "daAppDataManagerTreeItemIterator";
        Q_UNUSED(par);
        DAData innerd = i->toData();
        if (innerd == d) {
            res = i;
            return false;  //结束迭代
        }
        return true;  //继续迭代
    },
    DADataManagerTreeItem::ItemData);
    return res;
}

/**
 * @brief 添加文件夹
 * @param text
 * @return
 */
DADataManagerTreeItem* DADataManagerTreeModel::addFolder(const QString& name, DADataManagerTreeItem* parentFolder)
{
    DADataManagerTreeItem* folder = new DADataManagerTreeItem(name);
    if (nullptr == parentFolder) {
        //添加到顶层
        appendRow(folder);
    } else {
        //添加到子层级
        if (!parentFolder->isFolder()) {
            // parent不是folder添加到顶层
            appendRow(folder);
        } else {
            parentFolder->appendRow(folder);
        }
    }
    return folder;
}

/**
 * @brief 移除文件夹
 * @note 如果文件夹下有其他的数据文件，则数据文件将统一移动到上级，数据并不会被删除
 * @param f 注意，次函数会对f指针进行删除
 */
void DADataManagerTreeModel::removeFolder(DADataManagerTreeItem* f)
{
    Q_CHECK_PTR(f);
    QVector< QPair< QStandardItem*, DADataManagerTreeItem* > > willTakeItem;
    //把子目录下的item获取
    daAppDataManagerTreeItemIterator(f, [ &willTakeItem ](QStandardItem* parItem, DADataManagerTreeItem* i) -> bool {
        willTakeItem.append(qMakePair(parItem, i));
        return true;
    });
    //获取节点要转移的父级
    QStandardItem* parfolder = f->parent();
    if (nullptr == parfolder) {
        parfolder = invisibleRootItem();
    } else {
        if ((parfolder->type() != DAAPPDATAMANAGERTREEITEM_USERTYPE)
            || !(static_cast< DADataManagerTreeItem* >(parfolder))->isFolder()) {
            parfolder = invisibleRootItem();
        }
    }
    //节点的抽取,抽取完成后willTakeItem的second都脱离model管理
    for (const auto& p : willTakeItem) {
        if (p.first && p.second) {
            p.first->takeChild(p.second->row(), p.second->column());
            parfolder->appendRow(p.second);
        }
    }
    //最后删除Floder
    delete f;
}
/**
 * @brief 获取父级节点下所有子节点的名字
 *
 * @note 如果父级节点还是nullptr，则获取顶层节点的名字
 * @param parent 父级节点
 * @return
 */
QList< QString > DADataManagerTreeModel::getChildItemNames(const QStandardItem* parent) const
{
    QList< QString > res;
    if (nullptr == parent) {
        int rc = rowCount();
        for (int r = 0; r < rc; ++r) {
            res.append(item(r, 0)->text());
        }
    } else {
        int rc = parent->rowCount();
        for (int r = 0; r < rc; ++r) {
            res.append(parent->child(r)->text());
        }
    }
    return res;
}

/**
 * @brief 获取data mgr
 * @return
 */
DADataManager* DADataManagerTreeModel::getDataManager() const
{
    return d_ptr->_dataMgr;
}
/**
 * @brief 把index转换为tree item
 * @param i
 * @return 如果无法转换返回nullptr
 */
DADataManagerTreeItem* DADataManagerTreeModel::treeItemFromIndex(const QModelIndex& i) const
{
    QStandardItem* si = itemFromIndex(i);
    if (si) {
        if (DAAPPDATAMANAGERTREEITEM_USERTYPE == si->type()) {
            return static_cast< DADataManagerTreeItem* >(si);
        }
    }
    return nullptr;
}

/**
 * @brief 设置datamanager
 * @todo 这里再设置的时候应该把原有的DADataManager内容构建进去，由于目前用图都是开始就设置进去，因此暂时不实现也不影响
 * @param p
 */
void DADataManagerTreeModel::setDataManager(DADataManager* p)
{
    // TODO 这里再设置的时候应该把原有的DADataManager内容构建进去，由于目前用图都是开始就设置进去，因此暂时不实现也不影响
    d_ptr->_dataMgr = p;

    connect(p, &DADataManager::dataAdded, this, &DADataManagerTreeModel::onDataAdded);
    connect(p, &DADataManager::dataBeginRemove, this, &DADataManagerTreeModel::onDataBeginRemoved);
    connect(p, &DADataManager::dataChanged, this, &DADataManagerTreeModel::onDataChanged);
}

Qt::ItemFlags DADataManagerTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QStandardItemModel::flags(index);
    if (index.column() != 0) {
        f.setFlag(Qt::ItemIsEditable, false);
        f.setFlag(Qt::ItemIsDragEnabled, false);
        f.setFlag(Qt::ItemIsDropEnabled, false);
    }
    return f;
}

QVariant DADataManagerTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (0 == index.column()) {
        return QStandardItemModel::data(index, role);
    }
    if (Qt::DisplayRole != role) {
        return QVariant();
    }
    if (1 == index.column()) {
        DADataManagerTreeItem* ti = treeItemFromIndex(index.siblingAtColumn(0));
        if (nullptr == ti) {
            qDebug() << "can not cast DAAppDataManagerTreeItem";
            return QVariant();
        }
        qDebug() << "1 == index.column,item text=" << ti->text() << ",item depth=" << ti->depth();
        if (!ti->isData()) {
            return QVariant();
        }
        DAData d = ti->toData();
        if (d.isNull()) {
            qDebug() << ti->text() << "is data but to data get null";
            return QVariant();
        }

        if (d.isDataFrame()) {
            qDebug() << ti->text() << " is df";
            DAPyDataFrame df = d.toDataFrame();
            auto shape       = df.shape();
            qDebug() << QString("[%1,%2]").arg(shape.first).arg(shape.second);
            return QString("[%1,%2]").arg(shape.first).arg(shape.second);
        }
        qDebug() << ti->text() << ":end";
    }
    return QVariant();
}

bool DADataManagerTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    return false;
}

/**
 * @brief DAAppDataManagerTree::mimeData
 * @param indexes
 * @return
 */
QMimeData* DADataManagerTreeModel::mimeData(const QModelIndexList& indexes) const
{
    //如果选了多个item，包含了folder和data，则只选中data
    //如果选择了多个folder，则选中最顶层的folder
    if (0 == indexes.size()) {
        return QStandardItemModel::mimeData(indexes);
    }
    QSet< DADataManagerTreeItem* > items;
    for (const QModelIndex& i : qAsConst(indexes)) {
        qDebug() << "mimeData(" << i.row() << "," << i.column() << ")";
        QStandardItem* si = nullptr;
        if (i.column() != 0) {
            si = itemFromIndex(i.siblingAtColumn(0));
        } else {
            si = itemFromIndex(i);
        }
        if (nullptr == si) {
            continue;
        }
        if (si->type() == DAAPPDATAMANAGERTREEITEM_USERTYPE) {
            items.insert(static_cast< DADataManagerTreeItem* >(si));
        }
    }
    QModelIndexList needToMimeData;

    bool isData = true;
    //判断是否有data
    for (const DADataManagerTreeItem* i : qAsConst(items)) {
        if (i->isData()) {
            //有data，就把data加入
            needToMimeData.append(indexFromItem(i));
            isData = true;
        }
    }
    if (0 == needToMimeData.size()) {
        //说明没有data，处理文件夹
        int minDepth = std::numeric_limits< int >::max();  //初始赋予最大值
        for (const DADataManagerTreeItem* i : qAsConst(items)) {
            if (i->isFolder()) {
                //获取深度
                int dp = i->depth();
                if (dp < minDepth) {
                    minDepth = dp;
                    if (needToMimeData.isEmpty()) {
                        needToMimeData.append(indexFromItem(i));
                    } else {
                        needToMimeData[ 0 ] = indexFromItem(i);
                    }
                }
                isData = false;
            }
        }
    }
    QMimeData* md = QStandardItemModel::mimeData(needToMimeData);
    if (md == nullptr) {
        return nullptr;
    }
    if (isData) {
        md->setData("da/tree-data", QByteArray());
    } else {
        md->setData("da/tree-folder", QByteArray());
    }
    return md;
}

bool DADataManagerTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!parent.isValid()) {
        //顶层都可以移动
        return QStandardItemModel::dropMimeData(data, action, row, column, parent);
    }
    QStandardItem* parentItem = itemFromIndex(parent);
    if (nullptr == parentItem) {
        return false;
    }
    if (parentItem->type() != DAAPPDATAMANAGERTREEITEM_USERTYPE) {
        // parent 不能是非DAAppDataManagerTreeItem
        return false;
        DADataManagerTreeItem* treeitem = static_cast< DADataManagerTreeItem* >(parentItem);
        if (!treeitem->isFolder()) {
            return false;
        }
    }
    return QStandardItemModel::dropMimeData(data, action, row, column, parent);
}

/**
 * @brief 参数加入
 * @param d
 * @param willAddIndex
 */
void DADataManagerTreeModel::onDataAdded(const DA::DAData& d)
{
    DADataManagerTreeItem* i = new DADataManagerTreeItem(d);
    appendRow(i);
}

void DADataManagerTreeModel::onDataBeginRemoved(const DA::DAData& d, int dataIndex)
{
    Q_UNUSED(dataIndex);
    DADataManagerTreeItem* item = dataToItem(d);
    if (nullptr == item) {
        qWarning() << tr("The data(%1) cannot find its corresponding item "
                         "in the data management tree during the removal process")
                      .arg(d.getName());  // cn:数据在移除过程中无法找到其对应的数据管理树中的条目
        return;
    }
    removeRow(item->row(), indexFromItem(item->parent()));
}

void DADataManagerTreeModel::onDataChanged(const DAData& d, DADataManager::ChangeType t)
{
    DADataManagerTreeItem* item = dataToItem(d);
    if (nullptr == item) {
        qWarning() << tr("The data(%1) cannot find its corresponding item "
                         "in the data management tree during the removal process")
                      .arg(d.getName());  // cn:数据在移除过程中无法找到其对应的数据管理树中的条目
        return;
    }
    switch (t) {
    case DADataManager::ChangeName:
        item->setText(d.getName());
        break;
    default:
        break;
    }
}

/**
 * @brief 递归遍历startItem下所有的QStandardItem，迭代过程会调用函数指针，函数指针第一个参数为父节点，第二个参数为遍历到的子节点
 * @param startItem
 * @param fun 回调函数第一个参数为父节点，第二个参数为遍历到的子节点,如果回调返回true，则继续递归，
 * 如果返回false则终止回调
 * @param firstColumnOnly 只迭代第一列，其余列不迭代@default false
 * @return 如果返回true，说明整个递归过程遍历了所有节点，如果返回false，说明遍历过程中断，
 * 此返回取决于回调函数，如果回调函数返回过false，则此函数必会返回false
 */
bool DA::standardItemIterator(QStandardItem* startItem, std::function< bool(QStandardItem*, QStandardItem*) > fun, bool firstColumnOnly)
{
    if (startItem == nullptr) {
        return false;
    }
    int rc = startItem->rowCount();
    int cc = (firstColumnOnly ? 1 : startItem->columnCount());
    for (int r = 0; r < rc; ++r) {
        for (int c = 0; c < cc; ++c) {
            QStandardItem* citem = startItem->child(r, c);
            if (nullptr == citem) {
                continue;
            }
            if (!fun(startItem, citem)) {
                return false;
            }
            if (!standardItemIterator(citem, fun, firstColumnOnly)) {
                return false;
            }
        }
    }
    return true;
}
/**
 * @brief 递归遍历startItem下所有的DATreeDataItem,迭代过程会调用函数指针，函数指针第一个参数为父节点，第二个参数为遍历到的子节点
 * @param startItem
 * @param fun 回调函数第一个参数为父节点，第二个参数为遍历到的子节点,如果回调返回true，则继续递归，
 * 如果返回false则终止回调
 * @param type 希望遍历的item类型，如果不做限定，传入@ref DAAppDataManagerTreeItem::ItemUnknow
 * @return 如果返回true，说明整个递归过程遍历了所有节点，如果返回false，说明遍历过程中断，
 * 此返回取决于回调函数，如果回调函数返回过false，则此函数必会返回false
 */
bool DA::daAppDataManagerTreeItemIterator(QStandardItem* startItem,
                                          std::function< bool(QStandardItem*, DADataManagerTreeItem*) > fun,
                                          DADataManagerTreeItem::TreeItemType type)
{
    return standardItemIterator(
    startItem,
    [ &fun, type ](QStandardItem* par, QStandardItem* item) -> bool {
        if (item && item->type() == DAAPPDATAMANAGERTREEITEM_USERTYPE) {
            DADataManagerTreeItem* ti = static_cast< DADataManagerTreeItem* >(item);
            if (type == DADataManagerTreeItem::ItemUnknow) {
                //不做限制
                return fun(par, ti);
            } else if (type == DADataManagerTreeItem::ItemData) {
                if (ti->isData()) {
                    return fun(par, ti);
                }
            } else if (type == DADataManagerTreeItem::ItemFolder) {
                if (ti->isFolder()) {
                    return fun(par, ti);
                }
            }
        }
        return true;
    },
    true);
}

}  // end da
