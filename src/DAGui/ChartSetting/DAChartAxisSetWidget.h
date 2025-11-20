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
    QwtPlot* getPlot() const;
    void setPlot(QwtPlot* chart, int axisID);
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
private Q_SLOTS:
    void onCheckBoxEnableCliecked(bool on);
    void onLineEditTextChanged(const QString& text);
    void onAxisFontChanged(const QFont& font);
    void onAxisLabelAligmentChanged(Qt::Alignment al);
    void onAxisLabelRotationChanged(double v);
    void onAxisMarginValueChanged(int v);
    void onAxisMaxScaleChanged(double v);
    void onAxisMinScaleChanged(double v);
    void onScaleDivChanged();
    void onScaleStyleChanged(int id);

private:
    void updateUI(QwtPlot* chart, int axisID);

private:
    enum ScaleStyle
    {
        NormalScale,
        DateTimeScale
    };

    void bindTarget();
    void unbindTarget();
    void connectChart();
    void disconnectChart();

private:
    Ui::DAChartAxisSetWidget* ui;
    QPointer< QwtPlot > m_plot;
    QButtonGroup* m_buttonGroup;
    int m_axisID;
};
}  // end DA
#endif  // DAChartAxisSetWidget_H
