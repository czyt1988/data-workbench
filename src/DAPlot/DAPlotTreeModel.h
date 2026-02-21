#ifndef DAPLOTTREEMODEL_H
#define DAPLOTTREEMODEL_H
#include "DAPlotAPI.h"
#include <QAbstractItemModel>
#include <QList>
#include <QStandardItemModel>
#include <QPointer>

namespace QIM
{
class QImAbstractNode;
class QImFigureWidget;
class QImPlotNode;
class QImPlotItemNode;
class QImPlotAxisInfo;
}
namespace DA
{
class DAFigureWidget;

/**
 * @brief The DAFigureTreeModel class
 */
class DAPLOT_API DAPlotTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit DAPlotTreeModel(QObject* parent = nullptr);
    ~DAPlotTreeModel();

    void setFigure(QIM::QImFigureWidget* figure);
    QIM::QImFigureWidget* figure() const;

    void refresh();

    // item的类型
    static DAPlotTreeItemType itemType(const QStandardItem* item);
    static bool checkItemType(DAPlotTreeItemType type, const QStandardItem* item);

    template< typename T >
    static T pointerFromItem(const QStandardItem* item);

    template< typename T >
    T pointerFromIndex(const QModelIndex& index) const;

    template< typename T >
    QStandardItem* findTreeItem(DAPlotTreeItemType itemType, T pointer, QStandardItem* parent) const;

    QIM::QImPlotNode* plotFromItem(const QStandardItem* item) const;
    QIM::QImPlotAxisInfo* axisFromItem(const QStandardItem* item) const;


    // 创建一个纯颜色图标
    virtual QIcon generateBrushIcon(const QBrush& b, const QSize& size = QSize(22, 22)) const;
Q_SIGNALS:

private Q_SLOTS:

    void onFigureCleared();
    void onPlotNodeAttached(QIM::QImPlotNode* plot, bool on);
    void onPlotChildNodeAdded(QIM::QImAbstractNode* c);
    void onPlotChildNodeRemoved(QIM::QImAbstractNode* c);

    void onAxisLabelChanged(const QString& label);

private:
    void setupModel();
    void clearAllConnections();
    void addPlotNode(QIM::QImPlotNode* plot, QStandardItem* parentItem);
    void addAllPlotItems(QIM::QImPlotNode* plot, QStandardItem* plotItem);
    void addPlotItemNode(QIM::QImPlotItemNode* item, QStandardItem* parentItem);
    void addAllAxisInfos(QIM::QImPlotNode* plot, QStandardItem* plotItem);

    void removePlotNode(QIM::QImPlotNode* plot);
    void removePlotItemNode(QIM::QImPlotItemNode* plotitem);
    // 创建一个空item，用于树形节点没有对应的2,3列的情况
    QStandardItem* createEmptyItem() const;
    QStandardItem* findTreeItem(QIM::QImPlotNode* plotNode) const;
    QStandardItem* findTreeItem(QIM::QImPlotItemNode* plotItemNode) const;
    QStandardItem* findTreeItem(QIM::QImPlotAxisInfo* axisNode) const;

private:
    struct ConnectionsInfo
    {
        QList< QMetaObject::Connection > plotConnections;
        QList< QMetaObject::Connection > axisConnections;
        void disconnectAll();
    };

    QPointer< QIM::QImFigureWidget > m_figure;
    QStandardItem* m_axesFolderItem { nullptr };
    QStandardItem* m_plotItemFolderItem { nullptr };
    // 连接管理
    QMap< QIM::QImPlotNode*, ConnectionsInfo > m_plotConnections;
};

template< typename T >
inline QStandardItem* DAPlotTreeModel::findTreeItem(DAPlotTreeItemType itemType, T pointer, QStandardItem* parent) const
{
    if (!parent) {
        return nullptr;
    }
    const int rcnt = parent->rowCount();
    for (int i = 0; i < rcnt; ++i) {
        QStandardItem* c = parent->child(i, 0);
        if (!checkItemType(DAPlotTreeItemType::PlotItem, c)) {
            continue;
        }
        T node = pointerFromItem< T >(c);
        if (!node) {
            continue;
        }
        if (pointer == node) {
            return c;
        }
    }
    return nullptr;
}

template< typename T >
inline T DAPlotTreeModel::pointerFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return nullptr;
    QStandardItem* item = itemFromIndex(index);
    return pointerFromItem< T >(item);
}

template< typename T >
inline T DAPlotTreeModel::pointerFromItem(const QStandardItem* item)
{
    if (!item) {
        return nullptr;
    }
    QVariant v = item->data(static_cast< int >(DAPlotTreeItemRole::RoleInnerPointer));
    return v.isValid() ? reinterpret_cast< T >(v.value< quintptr >()) : nullptr;
}

}  // End Of Namespace DA
#endif  // DAFIGURETREEMODEL_H
