#ifndef DACHARTSETTINGWIDGET_H
#define DACHARTSETTINGWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
#include "DAChartWidget.h"
namespace Ui
{
class DAChartSettingWidget;
}
namespace DA
{
/**
 * @brief 图表设置窗口
 */
class DAGUI_API DAChartSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAChartSettingWidget(QWidget* parent = nullptr);
    ~DAChartSettingWidget();
    // 设置chart
    void setChartWidget(DAChartWidget* w);

private:
    Ui::DAChartSettingWidget* ui;
};
}  // end DA
#endif  // DACHARTSETTINGWIDGET_H
