#ifndef DACHARTAXISSETWIDGET_H
#define DACHARTAXISSETWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
class QwtPlot;
class QButtonGroup;
namespace Ui
{
class DAChartAxisSetWidget;
}
namespace DA
{

class DAGUI_API DAChartAxisSetWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DAChartAxisSetWidget(QWidget* parent = 0);
	~DAChartAxisSetWidget();
	QwtPlot* getChart() const;
	void setChart(QwtPlot* chart, int axisID);
	void updateUI();
	void resetAxisValue();
	// axis enable
	void setEnableAxis(bool on = true);
	bool isEnableAxis() const;
	//
	void enableWidget(bool enable = true);
	// 设置启用axis checkbox的图标
	void setEnableCheckBoxIcon(const QIcon& icon);
	QIcon getEnableCheckBoxIcon() const;
signals:
	///
	/// \brief 允许或禁止坐标轴时发送的信号
	/// \param enable
	/// \param axid
	///
	void enableAxis(bool enable, int axid);
private slots:
	Q_SLOT void onCheckBoxEnableCliecked(bool on);
	Q_SLOT void onLineEditTextChanged(const QString& text);
	Q_SLOT void onAxisFontChanged(const QFont& font);
	Q_SLOT void onAxisLabelAligmentChanged(Qt::Alignment al);
	Q_SLOT void onAxisLabelRotationChanged(double v);
	Q_SLOT void onAxisMarginValueChanged(int v);
	Q_SLOT void onAxisMaxScaleChanged(double v);
	Q_SLOT void onAxisMinScaleChanged(double v);
	Q_SLOT void onScaleDivChanged();
	Q_SLOT void onScaleStyleChanged(int id);

private:
	void updateUI(QwtPlot* chart, int axisID);

private:
	enum ScaleStyle
	{
		NormalScale,
		DateTimeScale
	};

	void connectChartAxis();
	void disconnectChartAxis();
	void connectChart();
	void disconnectChart();

private:
	Ui::DAChartAxisSetWidget* ui;
	QPointer< QwtPlot > m_chart;
	QButtonGroup* m_buttonGroup;
	int m_axisID;
};
}  // end DA
#endif  // DAChartAxisSetWidget_H
