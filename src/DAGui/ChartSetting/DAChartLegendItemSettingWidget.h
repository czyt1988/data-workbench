#ifndef DACHARTLEGENDITEMSETTINGWIDGET_H
#define DACHARTLEGENDITEMSETTINGWIDGET_H

#include "DAGuiAPI.h"
#include "DAAbstractChartItemSettingWidget.h"
class QwtPlotLegendItem;
namespace Ui
{
class DAChartLegendItemSettingWidget;
}
namespace DA
{
class DAGUI_API DAChartLegendItemSettingWidget : public DAAbstractChartItemSettingWidget
{
	Q_OBJECT

public:
	explicit DAChartLegendItemSettingWidget(QWidget* parent = nullptr);
	~DAChartLegendItemSettingWidget();
	// setPlotItem之后调用的虚函数
    virtual void updateUI(QwtPlotItem* item) override;

protected:
	void changeEvent(QEvent* e);
private slots:
	void onAligmentPositionChanged(Qt::Alignment al);
	void onSpinBoxHorizontalOffsetValueChanged(int v);
	void onSpinBoxVerticalOffsetValueChanged(int v);
	void onSpinBoxMarginValueChanged(int v);
	void onSpinBoxSpacingValueChanged(int v);
	void onSpinBoxItemMarginValueChanged(int v);
	void onSpinBoxItemSpacingValueChanged(int v);
	void onSpinBoxMaxColumnsValueChanged(int v);
	void onDoubleSpinBoxRadiusValueChanged(double v);
	void onBorderPenChanged(const QPen& v);
	void onLegendFontChanged(const QFont& v);
	void onLegendFontColorChanged(const QColor& v);
	void onLegendBKBrushChanged(const QBrush& v);

private:
	Ui::DAChartLegendItemSettingWidget* ui;
};
}  // end DA
#endif  // DACHARTLEGENDITEMSETTINGWIDGET_H
