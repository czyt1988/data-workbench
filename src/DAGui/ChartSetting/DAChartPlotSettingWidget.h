#ifndef DACHARTPLOTSETTINGWIDGET_H
#define DACHARTPLOTSETTINGWIDGET_H

#include <QWidget>
#include <QPointer>
#include "DAGuiAPI.h"
#include "qwt_plot.h"
namespace Ui
{
class DAChartPlotSettingWidget;
}
namespace DA
{
/**
 * @brief 图表设置窗口
 */
class DAGUI_API DAChartPlotSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAChartPlotSettingWidget(QWidget* parent = nullptr);
    ~DAChartPlotSettingWidget();
    // 设置chart
    void setPlot(QwtPlot* w);
    QwtPlot* getPlot() const;
    // 更新ui
    void updateUI();

private slots:
    // 标题内容设置
    void onTitleTextChanged(const QString& t);
    void onTitleFontChanged(const QFont& f);
    void onTitleColorChanged(const QColor& c);
    // footer内容设置
    void onFooterTextChanged(const QString& t);
    void onFooterFontChanged(const QFont& f);
    void onFooterColorChanged(const QColor& c);

private:
    Ui::DAChartPlotSettingWidget* ui;
    QPointer< QwtPlot > mChartPlot;
};
}  // end DA
#endif  // DACHARTPLOTSETTINGWIDGET_H
