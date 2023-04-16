#ifndef DACHARTPLOTITEMSETTINGWIDGET_H
#define DACHARTPLOTITEMSETTINGWIDGET_H

#include <QWidget>
#include <QPointer>
class QActionGroup;
class QwtPlotItem;
class QwtPlot;
namespace Ui
{
class DAChartPlotItemSettingWidget;
}
namespace DA
{
/**
 * @brief QwtPlotItem的设置窗口
 */
class DAChartPlotItemSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAChartPlotItemSettingWidget(QWidget* parent = nullptr);
    ~DAChartPlotItemSettingWidget();
    // plotitem
    void setPlotItem(QwtPlotItem* item);
    QwtPlotItem* getPlotItem() const;
    //清除
    void clear();
    //根据item值刷新ui内容，此函数不会触发信号
    void updateUI();
    //更新坐标轴的设置
    void updateAxis();
private slots:
    void onItemTitleEditingFinished();
    void onItemZValueChanged(double z);

private:
    void onPlotItemAttached(QwtPlotItem* plotItem, bool on);
    void onActionGroupAxisTriggered(QAction* act);

private:
    Ui::DAChartPlotItemSettingWidget* ui;
    QActionGroup* _actionGroupAxis { nullptr };
    QAction* _actionAxisLeftBottom { nullptr };
    QAction* _actionAxisLeftTop { nullptr };
    QAction* _actionAxisRightBottom { nullptr };
    QAction* _actionAxisRightTop { nullptr };
    QwtPlotItem* _item { nullptr };
    QPointer< QwtPlot > _plot { nullptr };
};
}
#endif  // DACHARTPLOTITEMSETTINGWIDGET_H
