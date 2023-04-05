#include "DATreeModel.h"
#include "DATree.h"
#include "DATreeItem.h"
#include <QVector>
#include <QList>
#include <iostream>
#include <memory>
#include <QDebug>
namespace DA
{

class DATreeModelPrivate
{
    DA_IMPL_PUBLIC(DATreeModel)
public:
    DATreeModelPrivate(DATreeModel* p);
    bool isNull() const;
    void setColumnCount(int col);
    DATree* _treePtr;
    int _columnCount;
    QVector< QString > _headerLabels;                ///< 记录头部标签
    QHash< DATreeItem*, QModelIndex > _itemToIndex;  ///< 记录item和index的关系
};
}  // end of DA

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DATreeModelPrivate
//===================================================

DATreeModelPrivate::DATreeModelPrivate(DATreeModel* p) : q_ptr(p), _treePtr(nullptr), _columnCount(1)
{
}

bool DATreeModelPrivate::isNull() const
{
    return nullptr == _treePtr;
}

void DATreeModelPrivate::setColumnCount(int col)
{
    _columnCount = col;
    _headerLabels.resize(col);
}

//===================================================
// DATreeModel
//===================================================
DATreeModel::DATreeModel(QObject* par, int colCount) : QAbstractItemModel(par), d_ptr(new DATreeModelPrivate(this))
{
    d_ptr->setColumnCount(colCount);
}

DATreeModel::DATreeModel(DATree* t, QObject* par, int colCount)
    : QAbstractItemModel(par), d_ptr(new DATreeModelPrivate(this))
{
    d_ptr->_treePtr = t;
    d_ptr->setColumnCount(colCount);
}

DATreeModel::~DATreeModel()
{
}

QModelIndex DATreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || column < 0)
        return QModelIndex();
    if (d_ptr->isNull())
        return QModelIndex();
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    if (!parent.isValid())  //说明是顶层
    {
        if ((row >= d_ptr->_treePtr->getItemCount()) || (column >= d_ptr->_columnCount)) {
            return QModelIndex();
        }
        DATreeItem* rootItem            = d_ptr->_treePtr->getItem(row);
        QModelIndex rootIndex           = createIndex(row, column, rootItem);  //顶层节点
        d_ptr->_itemToIndex[ rootItem ] = rootIndex;
        return rootIndex;
    }
    DATreeItem* parItem = indexToItem(parent);
    if ((nullptr == parItem) || (row >= parItem->childItemCount()) || (column >= d_ptr->_columnCount)) {
        return QModelIndex();  //不正常情况
    }
    DATreeItem* leafItem            = parItem->childItem(row);
    QModelIndex leafIndex           = createIndex(row, column, leafItem);  //叶子节点
    d_ptr->_itemToIndex[ leafItem ] = leafIndex;
    return leafIndex;
}

QModelIndex DATreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    DATreeItem* item = indexToItem(index);
    if (nullptr == item)
        return QModelIndex();
    if (item->isRootItem()) {
        return QModelIndex();
    }
    DATreeItem* parItem = item->parent();
    if (nullptr == parItem) {
        // parItem 这种属于异常
        qCritical() << tr("DATreeModel get invalid item");
        return QModelIndex();
    }
    return createIndex(parItem->index(), 0, parItem);  //挂载parent的都是只有一列的
}

int DATreeModel::rowCount(const QModelIndex& p) const
{
    if (!p.isValid()) {
        //顶层
        return d_ptr->_treePtr->getItemCount();
    }
    DATreeItem* parItem = indexToItem(p);
    return parItem ? parItem->childItemCount() : 0;
}

int DATreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return d_ptr->_columnCount;
}
/**
 * @brief 表头
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant DATreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (Qt::Horizontal == orientation) {
        if (section < d_ptr->_headerLabels.size()) {
            return d_ptr->_headerLabels.at(section);
        }
    }
    return QVariant();
}

Qt::ItemFlags DATreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant DATreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    DATreeItem* item = indexToItem(index);
    if (nullptr == item) {
        return QVariant();
    }
    return dataTreeItem(item, index, role);
}

/**
 * @brief 强制刷新
 */
void DATreeModel::update()
{
    beginResetModel();
    endResetModel();
}

/**
 * @brief 针对DATreeItem的data重载
 * @param i
 * @param index
 * @param role
 * @return
 */
QVariant DATreeModel::dataTreeItem(DATreeItem* i, const QModelIndex& index, int role) const
{
    if (nullptr == i) {
        return QVariant();
    }
    if (Qt::DisplayRole == role) {
        if (index.isValid() && 0 == index.column()) {
            return i->getName();
        }
    }
    return QVariant();
}

/**
 * @brief 设置列的数量
 * @param col 列的数量
 */
void DATreeModel::setColumnCount(int col)
{
    beginResetModel();
    d_ptr->setColumnCount(col);
    endResetModel();
}

/**
 * @brief 设置表头
 * @param column 如果column大于设置的列数量，此函数无响应
 * @param s
 */
void DATreeModel::setHeaderLabel(int column, const QString& s)
{
    if (column < d_ptr->_headerLabels.size()) {
        d_ptr->_headerLabels[ column ] = s;
    }
}

/**
 * @brief 设置树
 * @note 此操作并不会把之前设置的数销毁
 * @param t
 */
void DATreeModel::setTree(DATree* t)
{
    beginResetModel();
    d_ptr->_treePtr = t;
    endResetModel();
}

/**
 * @brief 获取树的指针
 * @return
 */
DATree* DATreeModel::getTree()
{
    return d_ptr->_treePtr;
}
/**
 * @brief 获取树的指针
 * @return
 */
const DATree* DATreeModel::getTree() const
{
    return d_ptr->_treePtr;
}

DATreeItem* DATreeModel::indexToItem(const QModelIndex& index) const
{
    if (index.isValid()) {
        return static_cast< DATreeItem* >(index.internalPointer());
    }
    return nullptr;
}

QModelIndex DATreeModel::itemToIndex(const DATreeItem* i) const
{
    return d_ptr->_itemToIndex.value(const_cast< DATreeItem* >(i), QModelIndex());
}
