#ifndef DAFIGURETREEMODEL_H
#define DAFIGURETREEMODEL_H
#include "DAFigureAPI.h"
#include <QAbstractItemModel>
#include <QList>
#include <QPointer>
#include <qwt_axis_id.h>
#include <QStandardItemModel>

class QwtPlot;
class QwtFigure;
class QwtScaleWidget;
class QwtPlotItem;
class Qwt3DPlotItem;
namespace DA
{
class DAFigureWidget;
class DAChart3DWidget;

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
        NodeTypePlotItem,  ///< RolePlotItem有效，可提取QwtPlotItem指针;RolePlot有效，可提取QwtPlot指针
        // 3D 节点类型
        NodeTypePlot3DFolder,  ///< RolePlot3D有效，可提取DAChart3DWidget指针
        NodeTypePlot3D,        ///< RolePlot3D有效，可提取DAChart3DWidget指针
        NodeTypePlot3DAxesFolder,  ///< RolePlot3D有效，可提取DAChart3DWidget指针
        NodeTypePlot3DAxis,   ///< RolePlot3D有效;RoleAxis3DId有效，存储AXIS枚举值(X1/Y1/Z1)
        NodeTypePlot3DItemsFolder,  ///< RolePlot3D有效
        NodeTypePlot3DItem    ///< RolePlot3DItem有效，可提取Qwt3DPlotItem指针;RolePlot3D有效
    };

    enum CustomRoles
    {
        RolePlot     = Qt::UserRole + 1,  // 存储QwtPlot指针
        RoleScale    = Qt::UserRole + 2,  // 存储QwtScaleWidget指针
        RolePlotItem = Qt::UserRole + 3,  // 存储QwtPlotItem指针
        RoleAxisId   = Qt::UserRole + 4,  // 存储QwtAxisId
        RoleNodeType = Qt::UserRole + 5,  // 存储节点类型
        // 3D data roles
        RolePlot3D     = Qt::UserRole + 6,  // 存储DAChart3DWidget指针
        RolePlot3DItem = Qt::UserRole + 7,  // 存储Qwt3DPlotItem指针
        RoleAxis3DId   = Qt::UserRole + 8   // 存储3D轴ID（AXIS枚举值：X1=0, Y1=1, Z1=2）
    };

    explicit DAFigureTreeModel(QObject* parent = nullptr);
    ~DAFigureTreeModel();

    void setFigure(QwtFigure* figure);
    QwtFigure* figure() const
    {
        return m_figure;
    }

    // 设置 DAFigureWidget（包含 3D chart 管理入口）
    void setFigureWidget(DA::DAFigureWidget* figWidget);

    void refresh();

    // item的类型
    NodeType itemType(QStandardItem* item) const;

    // 返回指定plotItem对应的QModelIndex（column 0）
    QModelIndex indexFromPlotItem(QwtPlotItem* item) const;

    // 拖拽支持：PlotItem可拖出，Plot/ItemsFolder/PlotItem/PlotFolder可接收
    Qt::ItemFlags flags(const QModelIndex& index) const override;

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

    // 通知指定plotItem的可见性列刷新（触发dataChanged信号）
    void notifyPlotItemVisibilityChanged(QwtPlotItem* item);
    // 通知指定坐标轴的可见性列刷新（触发dataChanged信号）
    void notifyAxisVisibilityChanged(QwtPlot* plot, QwtAxisId axisId);
    // 通知指定plotItem的文字列刷新（用于重命名后刷新显示）
    void notifyPlotItemTextChanged(QwtPlotItem* item);
    // 通知指定坐标轴的文字列刷新（用于重命名后刷新显示）
    void notifyAxisTextChanged(QwtPlot* plot, QwtAxisId axisId);
    // 通知指定chart节点的文字列刷新（用于重命名后刷新显示）
    void notifyPlotFolderTextChanged(QwtPlot* plot);

    // === 3D 相关方法 ===
    // 返回指定3D plotItem对应的QModelIndex（column 0）
    QModelIndex indexFrom3DPlotItem(Qwt3DPlotItem* item) const;
    // 3D 指针提取
    DAChart3DWidget* plot3DFromItem(const QStandardItem* item) const;
    DAChart3DWidget* plot3DFromIndex(const QModelIndex& index) const;
    Qwt3DPlotItem* plot3DItemFromItem(const QStandardItem* item) const;
    Qwt3DPlotItem* plot3DItemFromIndex(const QModelIndex& index) const;
    // 返回3D item的名字
    virtual QString generate3DPlotItemName(Qwt3DPlotItem* item) const;
    // 返回3D item对应的图标
    virtual QIcon generate3DPlotItemIcon(Qwt3DPlotItem* item) const;
    // 通知指定3D plotItem的可见性列刷新
    void notify3DPlotItemVisibilityChanged(Qwt3DPlotItem* item);
    // 通知指定3D plotItem的文字列刷新
    void notify3DPlotItemTextChanged(Qwt3DPlotItem* item);
    // 通知指定3D chart节点的文字列刷新
    void notify3DPlot3DTextChanged(DAChart3DWidget* chart);
    // 显式从树中移除指定3D plot item节点（确保树和hash一致，避免信号处理时序依赖）
    void remove3DPlotItemFromTree(Qwt3DPlotItem* item);

Q_SIGNALS:
    void chartItemAttached(QwtPlotItem* item, bool on);
    // 3D item 挂载/卸载信号
    void chart3DItemAttached(Qwt3DPlotItem* item, bool on);
private Q_SLOTS:
    void onAxesAdded(QwtPlot* plot);
    void onAxesRemoved(QwtPlot* plot);
    void onFigureCleared();
    void onCurrentAxesChanged(QwtPlot* plot);
    void onItemAttached(QwtPlotItem* item, bool on);
    // 3D 槽函数
    void on3DChartAdded(DA::DAChart3DWidget* chart);
    void on3DChartRemoved(DA::DAChart3DWidget* chart);
    void on3DItemAttached(Qwt3DPlotItem* item, bool on);

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

    // === 3D 私有方法 ===
    void add3DChartToModel(DAChart3DWidget* chart, QStandardItem* parentItem);
    void add3DLayerToModel(DAChart3DWidget* chart, QStandardItem* parentItem);
    void add3DAxesToLayer(DAChart3DWidget* chart, QStandardItem* layerItem);
    void add3DPlotItemsToLayer(DAChart3DWidget* chart, QStandardItem* layerItem);
    void add3DPlotItem(Qwt3DPlotItem* item, QStandardItem* parentItem);
    void remove3DPlotItem(Qwt3DPlotItem* item, QStandardItem* parentItem);
    void remove3DChartFromModel(DAChart3DWidget* chart);
    QStandardItem* find3DChartItem(DAChart3DWidget* chart) const;
    QStandardItem* find3DItemsFolderForChart(QStandardItem* chartItem) const;

private:
    QwtFigure* m_figure;
    QHash< QwtPlot*, QStandardItem* > m_plotItems;
    QHash< QwtPlotItem*, QStandardItem* > m_plotItemItems;
    // 3D 成员变量
    QHash< DAChart3DWidget*, QStandardItem* > m_plot3DItems;       ///< 3D chart → layer 树节点
    QHash< Qwt3DPlotItem*, QStandardItem* > m_plot3DItemItems;    ///< 3D plot item → 树节点
    QHash< DAChart3DWidget*, QList< QMetaObject::Connection > > m_plot3DConnections;  ///< 3D chart 信号连接
    QPointer< DAFigureWidget > m_figureWidget;  ///< DAFigureWidget 缓存

    // 连接管理
    QList< QMetaObject::Connection > m_figureConnections;
    QHash< QwtPlot*, QList< QMetaObject::Connection > > m_plotConnections;
};

}  // End Of Namespace DA
#endif  // DAFIGURETREEMODEL_H
