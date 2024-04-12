#ifndef DACHARTMANAGEWIDGET_H
#define DACHARTMANAGEWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
namespace Ui
{
class DAChartManageWidget;
}
namespace DA
{
class DAChartOperateWidget;
class DAFigureWidget;
class DAChartWidget;
class DAChartItemStandardItem;
class DAChartWidgetStandardItem;
// qwt
class QwtPlotItem;
DA_IMPL_FORWARD_DECL(DAChartManageWidget)
/**
 * @brief 绘图管理窗口
 */
class DAGUI_API DAChartManageWidget : public QWidget
{
    Q_OBJECT
    DA_IMPL(DAChartManageWidget)
public:
    DAChartManageWidget(QWidget* parent = nullptr);
    ~DAChartManageWidget();
    // 设置绘图炒作窗口，只有设置了绘图操作窗口，管理窗口才可以工作
    void setChartOperateWidget(DAChartOperateWidget* cow);
    // 设置item点击时如果不是当前chart，设置为当前chart
    void setCurrentChartOnItemClicked(bool on);
    bool isSetCurrentChartOnItemClicked() const;
    // 设置item双击时如果不是当前chart，设置为当前chart
    void setCurrentChartOnItemDoubleClicked(bool on);
    bool isSetCurrentChartOnItemDoubleClicked() const;

protected:
    virtual void treeviewChartItemClicked(DAChartItemStandardItem* item);
    virtual void treeviewChartWidgetClicked(DAChartWidgetStandardItem* item);
    virtual void treeviewChartItemDoubleClicked(DAChartItemStandardItem* item);
    virtual void treeviewChartWidgetDoubleClicked(DAChartWidgetStandardItem* item);

private:
    // 设置当前显示的fig对应的view
    void setCurrentDisplayView(DA::DAFigureWidget* fig);
private slots:
    void onFigureCreated(DA::DAFigureWidget* fig);
    void onFigureCloseing(DA::DAFigureWidget* fig);
    void onCurrentFigureChanged(DA::DAFigureWidget* fig, int index);
    void onTreeViewClicked(const QModelIndex& index);
    void onTreeViewDoubleClicked(const QModelIndex& index);
signals:
    /**
     * @brief 绘图图元选中信号
     * @param fig figure，注意如果只选中fig，只有fig有指针内容
     * @param chart 对应的chart ，有可能为nullptr，注意如果只选中chart,fig和chart的指针有内容
     * @param item 对应的item ，有可能为nullptr，注意如果选中item,fig和chart及item的指针都有内容
     */
    void figureItemClicked(DAFigureWidget* fig, DAChartWidget* chart, QwtPlotItem* item);
    void figureItemDoubleClicked(DAFigureWidget* fig, DAChartWidget* chart, QwtPlotItem* item);

private:
    Ui::DAChartManageWidget* ui;
};
}  // end of namespace DA
#endif  // DACHARTMANAGEWIDGET_H
