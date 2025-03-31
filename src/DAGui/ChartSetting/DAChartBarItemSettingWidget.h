#ifndef DACHARTBARITEMSETTINGWIDGET_H
#define DACHARTBARITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
#include "DAAbstractChartItemSettingWidget.h"
#include "qwt_plot_curve.h"
#include "qwt_symbol.h"
#include "qwt_column_symbol.h"
#include "qwt_plot_barchart.h"

// Qt
class QAbstractButton;
// qwt
class QwtPlot;
class QwtColumnSymbol;
class QwtPlotBarChart;

namespace Ui
{
class DAChartBarItemSettingWidget;
}

namespace DA
{
class DAChartPlotItemSettingWidget;
/**
 * @brief 曲线设置窗口
 *
 * @note 注意此窗口不保存item
 */
class DAGUI_API DAChartBarItemSettingWidget : public DAAbstractChartItemSettingWidget
{
    Q_OBJECT

public:
	explicit DAChartBarItemSettingWidget(QWidget* parent = nullptr);
	~DAChartBarItemSettingWidget();
	// item设置了
	virtual void plotItemSet(QwtPlotItem* item) override;
	// 根据QwtPlotCurve更新ui
	void updateUI(const QwtPlotBarChart* item);
	// 根据ui更新plotitem
	void updatePlotItem(QwtPlotBarChart* item);
	// 标题
	void setTitle(const QString& t);
	QString getTitle() const;
	// Bar Style
	void setBarStyle(QwtColumnSymbol::Style v);
	QwtColumnSymbol::Style getBarStyle() const;
	// Bar LegendMode
	void setBarLegendMode(QwtPlotBarChart::LegendMode v);
	QwtPlotBarChart::LegendMode getBarLegendMode() const;
	// fill编辑
	void enableFillEdit(bool on = true);
	bool isEnableFillEdit() const;
	// 填充
	QBrush getFillBrush() const;
	// 基线
	double getBaseLine() const;
	bool isHaveBaseLine() const;
	// 清空界面
	void resetUI();
	// 获取itemplot widget
	DAChartPlotItemSettingWidget* getItemSettingWidget() const;

	// 布局策略相关接口
	void setLayoutPolicy(QwtPlotAbstractBarChart::LayoutPolicy policy);
	QwtPlotAbstractBarChart::LayoutPolicy getLayoutPolicy() const;
	void setLayoutHint(double hint);
	double getLayoutHint() const;
	void setSpacing(int spacing);
	int getSpacing() const;
	void setMargin(int margin);
	int getMargin() const;

public slots:
	// 填充
	void setFillBrush(const QBrush& v);
	// 基线
	void setBaseLine(double v);

protected:
	void resetBarStyleComboBox();
private slots:
	void onBarStyleCurrentIndexChanged(int index);
	void on_checkBoxChart_clicked(bool checked);
	void on_checkBoxBar_clicked(bool checked);
	void on_checkBoxEnableFill_clicked(bool checked);
	void onBrushChanged(const QBrush& b);
	void on_lineEditBaseLine_editingFinished();
	void onLayoutPolicyChanged(int index);
	void onSpacingValueChanged(int value);
	void onMarginValueChanged(int value);
	void onLayoutHintValueChanged(double value);

protected slots:
	virtual void plotItemAttached(QwtPlotItem* plotItem, bool on) override;

private:
	Ui::DAChartBarItemSettingWidget* ui;
};
}

#endif  // DACHARTBARITEMSETTINGWIDGET_H
