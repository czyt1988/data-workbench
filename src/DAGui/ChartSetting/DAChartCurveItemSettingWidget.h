#ifndef DACHARTCURVEITEMSETTINGWIDGET_H
#define DACHARTCURVEITEMSETTINGWIDGET_H

#include <QWidget>

namespace Ui
{
class DAChartCurveItemSettingWidget;
}

namespace DA
{

class DAChartCurveItemSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAChartCurveItemSettingWidget(QWidget* parent = nullptr);
    ~DAChartCurveItemSettingWidget();
    // 标题
    void setTitle(const QString& t);
    QString getTitle() const;

private:
    Ui::DAChartCurveItemSettingWidget* ui;
};
}

#endif  // DACHARTCURVEITEMSETTINGWIDGET_H
