#ifndef DACHARTCANVASSETTINGWIDGET_H
#define DACHARTCANVASSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
class QwtPlot;

namespace Ui
{
class DAChartCanvasSettingWidget;
}

namespace DA
{
/**
 * @brief QwtCanvasWidget的设置
 */
class DAGUI_API DAChartCanvasSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAChartCanvasSettingWidget(QWidget* parent = nullptr);
    ~DAChartCanvasSettingWidget();

    void setPlot(QwtPlot* plot);
    QwtPlot* getPlot() const;

protected:
    void changeEvent(QEvent* e);

private:
    Ui::DAChartCanvasSettingWidget* ui;
    QPointer< QwtPlot > mPlot;
};
}  // end DA
#endif  // DACHARTCANVASSETTINGWIDGET_H
