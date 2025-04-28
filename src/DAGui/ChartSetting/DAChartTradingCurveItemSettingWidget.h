#ifndef DACHARTTRADINGCURVEITEMSETTINGWIDGET_H
#define DACHARTTRADINGCURVEITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
#include "DAAbstractChartItemSettingWidget.h"
#include "qwt_symbol.h"

// Qt
class QAbstractButton;
// qwt
class QwtPlotTradingCurve;

namespace Ui
{
class DAChartTradingCurveItemSettingWidget;
}

namespace DA
{
class DAChartPlotItemSettingWidget;
/**
 * @brief 曲线设置窗口
 *
 * @note 注意此窗口不保存item
 */
class DAGUI_API DAChartTradingCurveItemSettingWidget : public DAAbstractChartItemSettingWidget
{
    Q_OBJECT

public:
    explicit DAChartTradingCurveItemSettingWidget(QWidget* parent = nullptr);
    ~DAChartTradingCurveItemSettingWidget();
	// item设置了
	virtual void plotItemSet(QwtPlotItem* item) override;
	// 根据QwtPlotCurve更新ui
	void updateUI(const QwtPlotTradingCurve* item);
	// 根据ui更新plotitem
	void updatePlotItem(QwtPlotTradingCurve* item);

	// 方向
    Qt::Orientation getOrientationFromUI() const;
	// 清空界面
	void resetUI();
	// 获取itemplot widget
	DAChartPlotItemSettingWidget* getItemSettingWidget() const;
    // 更新symbol brush
    void updateSymbolFillBrushFromUI(QwtPlotTradingCurve* c);
    // 更新方向
    void updateOrientationFromUI(QwtPlotTradingCurve* c);

private slots:
    void onRadioButtonBarClicked(bool on);
    void onRadioButtonStickClicked(bool on);
    void onIncreasingBrushChanged(const QBrush& b);
    void onDecreasingBrushChanged(const QBrush& b);
	void onButtonGroupOrientationClicked(QAbstractButton* b);
	void onCurvePenChanged(const QPen& p);
    void onDoubleSpinBoxExternValueChanged(double v);
    void onDoubleSpinBoxMinValueChanged(double v);
    void onDoubleSpinBoxMaxValueChanged(double v);
protected slots:
	virtual void plotItemAttached(QwtPlotItem* plotItem, bool on);

private:
    Ui::DAChartTradingCurveItemSettingWidget* ui;
};
}
#endif  // DACHARTBOXITEMSETTINGWIDGET_H
