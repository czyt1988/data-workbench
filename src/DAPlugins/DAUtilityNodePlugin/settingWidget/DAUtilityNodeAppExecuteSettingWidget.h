#ifndef DAUTILITYNODEAPPEXECUTESETTINGWIDGET_H
#define DAUTILITYNODEAPPEXECUTESETTINGWIDGET_H

#include <QWidget>

namespace Ui {
class DAUtilityNodeAppExecuteSettingWidget;
}

class DAUtilityNodeAppExecuteSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAUtilityNodeAppExecuteSettingWidget(QWidget *parent = nullptr);
    ~DAUtilityNodeAppExecuteSettingWidget();

private:
    Ui::DAUtilityNodeAppExecuteSettingWidget *ui;
};

#endif // DAUTILITYNODEAPPEXECUTESETTINGWIDGET_H
