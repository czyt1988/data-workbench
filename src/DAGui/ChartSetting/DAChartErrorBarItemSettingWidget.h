#ifndef DACHARTERRORBARITEMSETTINGWIDGET_H
#define DACHARTERRORBARITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
#include "DAAbstractChartItemSettingWidget.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_symbol.h"

// Qt
class QAbstractButton;
// qwt
class QwtPlot;

namespace Ui
{
class DAChartErrorBarItemSettingWidget;
}

namespace DA
{
class DAChartPlotItemSettingWidget;
/**
 * @brief 曲线设置窗口
 *
 * @note 注意此窗口不保存item
 */
class DAGUI_API DAChartErrorBarItemSettingWidget : public DAAbstractChartItemSettingWidget
{
    Q_OBJECT

public:
	explicit DAChartErrorBarItemSettingWidget(QWidget* parent = nullptr);
	~DAChartErrorBarItemSettingWidget();
	// item设置了
	virtual void plotItemSet(QwtPlotItem* item) override;
	// 根据QwtPlotCurve更新ui
	void updateUI(const QwtPlotIntervalCurve* item);
	// 根据ui更新plotitem
	void updatePlotItem(QwtPlotIntervalCurve* item);
	// 标题
	void setTitle(const QString& t);
	QString getTitle() const;
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
	// 方向
	void setOrientation(Qt::Orientation v);
	Qt::Orientation getOrientation() const;
	// 清空界面
	void resetUI();
	// 获取itemplot widget
	DAChartPlotItemSettingWidget* getItemSettingWidget() const;
public slots:
	// 画笔
	void setCurvePen(const QPen& v);
	// 填充
	void setFillBrush(const QBrush& v);

private slots:
	void on_checkBoxEnableMarker_clicked(bool checked);
	void on_checkBoxEnableFill_clicked(bool checked);
	void onSymbolStyleChanged(QwtSymbol::Style s);
	void onSymbolSizeChanged(int s);
	void onSymbolColorChanged(const QColor& s);
	void onSymbolOutlinePenChanged(const QPen& s);
	void onBrushChanged(const QBrush& b);
	void onButtonGroupOrientationClicked(QAbstractButton* b);
	void onCurvePenChanged(const QPen& p);
protected slots:
	virtual void plotItemAttached(QwtPlotItem* plotItem, bool on);

private:
	Ui::DAChartErrorBarItemSettingWidget* ui;
};
}

#endif  // DACHARTERRORBARITEMSETTINGWIDGET_H
