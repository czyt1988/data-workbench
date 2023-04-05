#ifndef SATREEMODEL_H
#define SATREEMODEL_H
#include <QAbstractItemModel>
#include "DAGuiAPI.h"
namespace DA
{
class DATreeModelPrivate;
class DATree;
class DATreeItem;
/**
 * @brief 针对DATree的通用model
 */
class DAGUI_API DATreeModel : public QAbstractItemModel
{
    Q_OBJECT
    DA_IMPL(DATreeModel)
public:
    explicit DATreeModel(QObject* par = nullptr, int colCount = 1);
    explicit DATreeModel(DATree* t, QObject* par = nullptr, int colCount = 1);
    ~DATreeModel();
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& p) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    void update();

public:
    virtual QVariant dataTreeItem(DATreeItem* i, const QModelIndex& index, int role) const;

public:
    //设置列数
    void setColumnCount(int col);
    //设置表头，需要先setColumnCount
    void setHeaderLabel(int column, const QString& s);
    //设置tree
    void setTree(DATree* t);
    //获取树的指针
    DATree* getTree();
    const DATree* getTree() const;

public:
    DATreeItem* indexToItem(const QModelIndex& index) const;
    QModelIndex itemToIndex(const DATreeItem* i) const;
};
}  // end of DA
#endif  // SATREEMODEL_H
