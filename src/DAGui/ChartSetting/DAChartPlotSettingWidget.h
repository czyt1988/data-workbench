#ifndef DACHARTPLOTSETTINGWIDGET_H
#define DACHARTPLOTSETTINGWIDGET_H

#include <QWidget>
#include <QPointer>
#include "DAGuiAPI.h"
#include "DAChartWidget.h"
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
	void setChartWidget(DAChartWidget* w);
    DAChartWidget* getChartWidget() const;
public slots:
    // 标题内容设置
    void setTitleText(const QString& t);
    void setTitleFont(const QFont& f);
    void setTitleColor(const QColor& c);
    // footer内容设置
    void setFooterText(const QString& t);
    void setFooterFont(const QFont& f);
    void setFooterColor(const QColor& c);
private slots:

private:
	Ui::DAChartPlotSettingWidget* ui;
	QPointer< DAChartWidget > mChartPlot;
};
}  // end DA
#endif  // DACHARTPLOTSETTINGWIDGET_H
