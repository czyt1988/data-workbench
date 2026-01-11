#ifndef DAFIGURETREEMODEL_H
#define DAFIGURETREEMODEL_H
#include "DAFigureAPI.h"
#include <QAbstractItemModel>
#include <QList>
#include <qwt_axis_id.h>
#include <QStandardItemModel>

class QwtPlot;
class QwtFigure;
class QwtScaleWidget;
class QwtPlotItem;
namespace DA
{
class DAFigureWidget;

class DAFIGURE_API DAFigureTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum NodeType
    {
        NodeTypeUnknow = 1001,
        NodeTypeFigure,
        NodeTypePlotFolder,  ///< RolePlot有效，可提取host QwtPlot指针
        NodeTypePlot,        ///< RolePlot有效，可提取QwtPlot指针(含host和parasite)
        NodeTypeAxesFolder,  ///< RolePlot有效，可提取host QwtPlot指针
        NodeTypeAxis,  ///< RoleScale有效，可提取QwtScaleWidget指针;RoleAxisId有效，可提取QwtAxisId;RolePlot有效，可提取对应QwtPlot指针
        NodeTypeItemsFolder,  ///< RolePlot有效，可提取QwtPlot指针
        NodeTypePlotItem  ///< RolePlotItem有效，可提取QwtPlotItem指针;RolePlot有效，可提取QwtPlot指针
    };

    enum CustomRoles
    {
        RolePlot     = Qt::UserRole + 1,  // 存储QwtPlot指针
        RoleScale    = Qt::UserRole + 2,  // 存储QwtScaleWidget指针
        RolePlotItem = Qt::UserRole + 3,  // 存储QwtPlotItem指针
        RoleAxisId   = Qt::UserRole + 4,  // 存储QwtAxisId
        RoleNodeType = Qt::UserRole + 5   // 存储节点类型
    };

    explicit DAFigureTreeModel(QObject* parent = nullptr);
    ~DAFigureTreeModel();

    void setFigure(QwtFigure* figure);
    QwtFigure* figure() const
    {
        return m_figure;
    }

    void refresh();

    // item的类型
    NodeType itemType(QStandardItem* item) const;

    template< typename T >
    T* pointerFromItem(const QStandardItem* item, CustomRoles role) const
    {
        if (!item) {
            return nullptr;
        }
        QVariant v = item->data(role);
        return v.isValid() ? reinterpret_cast< T* >(v.value< quintptr >()) : nullptr;
    }

    template< typename T >
    T* pointerFromIndex(const QModelIndex& index, CustomRoles role) const
    {
        if (!index.isValid())
            return nullptr;
        QStandardItem* item = itemFromIndex(index);
        return pointerFromItem< T >(item, role);
    }
    QwtPlot* plotFromItem(const QStandardItem* item) const;
    QwtPlot* plotFromIndex(const QModelIndex& index) const;
    QwtScaleWidget* scaleFromItem(const QStandardItem* item) const;
    QwtScaleWidget* scaleFromIndex(const QModelIndex& index) const;
    QwtPlotItem* plotItemFromItem(const QStandardItem* item) const;
    QwtPlotItem* plotItemFromIndex(const QModelIndex& index) const;
    QwtAxisId axisIdFromItem(const QStandardItem* item) const;
    QwtAxisId axisIdFromItem(const QModelIndex& index) const;

    // 返回绘图的名字
    virtual QString generatePlotTitleText(QwtPlot* plot) const;
    // 返回QwtPlotItem的名字
    virtual QString generatePlotItemName(QwtPlotItem* item) const;
    // 返回QwtPlotItem对应的图标
    virtual QIcon generatePlotItemIcon(QwtPlotItem* item) const;
    // 创建一个纯颜色图标
    virtual QIcon generateBrushIcon(const QBrush& b) const;
Q_SIGNALS:
    void chartItemAttached(QwtPlotItem* item, bool on);
private slots:
    void onAxesAdded(QwtPlot* plot);
    void onAxesRemoved(QwtPlot* plot);
    void onFigureCleared();
    void onCurrentAxesChanged(QwtPlot* plot);
    void onItemAttached(QwtPlotItem* item, bool on);

private:
    void setupModel();
    void clearAllConnections();
    void addPlotToModel(QwtPlot* plot, QStandardItem* parentItem);
    void addLayerToModel(QwtPlot* plot, QStandardItem* parentItem);
    void addAxesToLayer(QwtPlot* plot, QStandardItem* layerItem);
    void addPlotItemsToLayer(QwtPlot* plot, QStandardItem* layerItem);
    void addPlotItem(QwtPlotItem* item, QStandardItem* parentItem);
    void removePlotItem(QwtPlotItem* item, QStandardItem* parentItem);
    void removePlotFromModel(QwtPlot* plot);
    // 创建一个空item，用于树形节点没有对应的2,3列的情况
    QStandardItem* createEmptyItem() const;
    // 创建绘图属性item
    QStandardItem* createAxesPropertyItem(QwtPlot* plot) const;
    // 更新绘图属性，把当前选中的绘图更新掉
    void updateAxesPropertyItem();

    QStandardItem* findPlotItem(QwtPlot* plot) const;
    QStandardItem* findItemsFolderForPlot(QStandardItem* plotItem, QwtPlot* plot) const;

private:
    QwtFigure* m_figure;
    QHash< QwtPlot*, QStandardItem* > m_plotItems;
    QHash< QwtPlotItem*, QStandardItem* > m_plotItemItems;

    // 连接管理
    QList< QMetaObject::Connection > m_figureConnections;
    QHash< QwtPlot*, QList< QMetaObject::Connection > > m_plotConnections;
};

}  // End Of Namespace DA
#endif  // DAFIGURETREEMODEL_H
