#ifndef DACHARTPLOTITEMSETTINGWIDGET_H
#define DACHARTPLOTITEMSETTINGWIDGET_H

#include <QWidget>
#include <QPointer>
class QAbstractButton;
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

    // 设置plotitem
    void setPlotItem(QwtPlotItem* item);
    QwtPlotItem* getPlotItem() const;
    // 清除
    void clear();
    // 根据item值刷新ui内容，此函数不会触发信号
    void updateUI();
    // 更新坐标轴的设置
    void updateAxis();
private slots:
    void onItemTitleEditingFinished();
    void onItemZValueChanged(double z);

private:
    void onPlotItemAttached(QwtPlotItem* plotItem, bool on);
    void onButtonGroupAxisClicked(QAbstractButton* btn);

private:
    Ui::DAChartPlotItemSettingWidget* ui;
    QwtPlotItem* mItem { nullptr };
    QPointer< QwtPlot > mPlot { nullptr };
};
}
#endif  // DACHARTPLOTITEMSETTINGWIDGET_H
