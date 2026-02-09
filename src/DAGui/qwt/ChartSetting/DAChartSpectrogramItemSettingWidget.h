#ifndef DAChartSpectrogramItemSettingWidget_H
#define DAChartSpectrogramItemSettingWidget_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
#include "DAAbstractChartItemSettingWidget.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_symbol.h"

// Qt
class QAbstractButton;
// qwt
class QwtPlot;

namespace Ui
{
class DAChartSpectrogramItemSettingWidget;
}

namespace DA
{
class DAChartPlotItemSettingWidget;
/**
 * @brief 曲线设置窗口
 *
 * @note 注意此窗口不保存item
 */
class DAGUI_API DAChartSpectrogramItemSettingWidget : public DAAbstractChartItemSettingWidget
{
    Q_OBJECT

public:
	explicit DAChartSpectrogramItemSettingWidget(QWidget* parent = nullptr);
	~DAChartSpectrogramItemSettingWidget();
    // 更新界面
    virtual void updateUI(QwtPlotItem* item) override;
    void applySetting(QwtPlotSpectrogram* item);
	// 标题
	void setTitle(const QString& t);
	QString getTitle() const;
	// DisplayMode
	void setDisplayMode(QwtPlotSpectrogram::DisplayMode v);
	QwtPlotSpectrogram::DisplayMode getDisplayMode() const;
	// Color
	void setFromColor(const QColor& v);
	QColor getFromColor() const;
	void setToColor(const QColor& v);
	QColor getToColor() const;
	// 画笔
	QPen getCurvePen() const;
	// 填充
	QBrush getFillBrush() const;
	// 清空界面
	void resetUI();
	// 获取itemplot widget
	DAChartPlotItemSettingWidget* getItemSettingWidget() const;
public slots:
	// 画笔
	void setCurvePen(const QPen& v);

protected:
	void resetDisplayModeComboBox();
private slots:
	void onDisplayModeCurrentIndexChanged(int index);
	void onFromColorChanged(const QPen& p);
	void onToColorChanged(const QPen& p);
	void onCurvePenChanged(const QPen& p);
protected slots:
	virtual void plotItemAttached(QwtPlotItem* plotItem, bool on);

private:
	Ui::DAChartSpectrogramItemSettingWidget* ui;
};
}
#endif  // DAChartSpectrogramItemSettingWidget_H
