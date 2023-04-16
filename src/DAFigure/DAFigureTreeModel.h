#ifndef DAFIGURETREEMODEL_H
#define DAFIGURETREEMODEL_H
#include "DAFigureAPI.h"
#include <QAbstractItemModel>
#include <QList>
#include <qwt_plot_item.h>
#include <QStandardItemModel>
#ifndef DAChartWidgetStandardItem_Type
#define DAChartWidgetStandardItem_Type QStandardItem::UserType + 1
#endif

#ifndef DAChartItemStandardItem_Type
#define DAChartItemStandardItem_Type QStandardItem::UserType + 2
#endif
class QwtPlot;
namespace DA
{
class DAFigureWidget;
class DAChartWidget;

/**
 * @brief 针对QwtPlotItem的QStandardItem
 */
class DAFIGURE_API DAChartItemStandardItem : public QStandardItem
{
public:
    DAChartItemStandardItem(DAChartWidget* c, QwtPlotItem* i, int col = 0);
    virtual int type() const override
    {
        return DAChartItemStandardItem_Type;
    }

    virtual QVariant data(int role) const override;
    virtual void setData(const QVariant& value, int role = Qt::UserRole + 1) override;

    DAChartWidget* getChart() const;
    void setChart(DAChartWidget* c);

    QwtPlotItem* getItem() const;
    void setItem(QwtPlotItem* i);

    QVariant dataDisplayRole(QwtPlotItem* item, int c) const;
    void setDataEditRole(const QVariant& value, QwtPlotItem* item, int c);
    QVariant dataDecorationRole(QwtPlotItem* item, int c) const;
    //获取item的名字
    QString getItemName(QwtPlotItem* item) const;

private:
    DAChartWidget* _chart { nullptr };
    QwtPlotItem* _item { nullptr };
};

/**
 * @brief 用于显示DAChartWidget的QStandardItem
 */
class DAFIGURE_API DAChartWidgetStandardItem : public QStandardItem
{
public:
    DAChartWidgetStandardItem(DAFigureWidget* fig, DAChartWidget* w, int col = 0);

    virtual int type() const override
    {
        return DAChartWidgetStandardItem_Type;
    }

    virtual QVariant data(int role) const override;
    QVariant dataDisplayRole(int c) const;
    QVariant dataDecorationRole(int c) const;
    DAFigureWidget* getFigure() const;
    void setFigure(DAFigureWidget* getFigure);

    DAChartWidget* getChart() const;
    void setChart(DAChartWidget* getChart);

    //添加item
    void appendChartItem(QwtPlotItem* i);

private:
    DAFigureWidget* _figure { nullptr };
    DAChartWidget* _chart { nullptr };
};

/**
 * @brief 一共有3列
 *
 * 名字|颜色|可见性
 */
class DAFIGURE_API DAFigureTreeModel : public QStandardItemModel
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigureTreeModel)
public:
    DAFigureTreeModel(QObject* parent = 0);
    ~DAFigureTreeModel();
    //设置fig
    void setFigure(DAFigureWidget* fig);
    DAFigureWidget* getFigure() const;
    // chart的索引
    int indexOfChart(DAChartWidget* c) const;
    //查找和QwtPlotItem相关的QStandardItem,O(n)
    QList< QStandardItem* > findChartItems(QwtPlotItem* i);

public:
    // item转icon
    static QIcon chartItemToIcon(const QwtPlotItem* i);
    //通过color获取icon
    static QIcon colorIcon(const QColor& c, bool drawBorder = true);
    //通过QBrush获取icon
    static QIcon brushIcon(const QBrush& b, bool drawBorder = true);
private slots:
    //图表移除触发的槽
    void onChartWillRemove(DA::DAChartWidget* c);
    //图表增加触发的槽
    void onChartAdded(DA::DAChartWidget* c);
    //有chartitem加入或移除触发的槽
    void onChartItemAttached(QwtPlotItem* plotItem, bool on);
    // chartitem的LegendData改变槽
    void onLegendDataChanged(const QVariant& itemInfo, const QList< QwtLegendData >& data);

private:
    void addChartItem(QwtPlotItem* i);
    void removeChartItem(QwtPlotItem* i);
};

}  // End Of Namespace DA
#endif  // DAFIGURETREEMODEL_H
