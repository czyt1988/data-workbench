#ifndef DACHARTCURVEITEMSETTINGWIDGET_H
#define DACHARTCURVEITEMSETTINGWIDGET_H

#include <QWidget>
#include <QPointer>
#include "qwt_plot_curve.h"
class QwtPlot;
namespace Ui
{
class DAChartCurveItemSettingWidget;
}
namespace DA
{
/**
 * @brief QwtPlotItem的设置页面
 */
class DAChartCurveItemSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAChartCurveItemSettingWidget(QWidget* parent = nullptr);
    ~DAChartCurveItemSettingWidget();
    //设置item
    void setChartCurveItem(QwtPlotCurve* item);
    QwtPlotCurve* getChartCurveItem() const;
signals:
    /**
     * @brief 曲线样式发生改变
     * @param s
     */
    void curveStyleChanged(QwtPlotCurve::CurveStyle s);
private slots:
    void onCurveStyleComboBoxCurrentIndexChanged(int i);

private:
    //初始化ui
    void initUI();

private:
    Ui::DAChartCurveItemSettingWidget* ui;
    QwtPlotCurve* _item { nullptr };
};
}
#endif  // DACHARTCURVEITEMSETTINGWIDGET_H
