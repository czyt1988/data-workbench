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
	// maker编辑
	void enableMarkerEdit(bool on = true);
	bool isEnableMarkerEdit() const;
	// fill编辑
	void enableFillEdit(bool on = true);
	bool isEnableFillEdit() const;
	// 画笔
	QPen getCurvePen() const;
	// 填充
	QBrush getFillBrush() const;
	// 基线
	double getBaseLine() const;
	bool isHaveBaseLine() const;
	// 清空界面
	void resetUI();
	// 获取itemplot widget
	DAChartPlotItemSettingWidget* getItemSettingWidget() const;
public slots:
	// 画笔
	void setCurvePen(const QPen& v);
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
	void on_checkBoxLegendShowLine_clicked(bool checked);
	void on_checkBoxLegendShowSymbol_clicked(bool checked);
	void on_checkBoxLegendShowBrush_clicked(bool checked);
	void on_checkBoxEnableMarker_clicked(bool checked);
	void on_checkBoxEnableFill_clicked(bool checked);
	void onSymbolStyleChanged(QwtSymbol::Style s);
	void onSymbolSizeChanged(int s);
	void onSymbolColorChanged(const QColor& s);
	void onSymbolOutlinePenChanged(const QPen& s);
	void onBrushChanged(const QBrush& b);
	void on_lineEditBaseLine_editingFinished();
	void onButtonGroupOrientationClicked(QAbstractButton* b);
	void onCurvePenChanged(const QPen& p);

protected slots:
	virtual void plotItemAttached(QwtPlotItem* plotItem, bool on);

private:
	Ui::DAChartBarItemSettingWidget* ui;
};
}

#endif  // DACHARTBARITEMSETTINGWIDGET_H
