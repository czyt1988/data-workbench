#ifndef DACHARTSYMBOLEDITWIDGET_H
#define DACHARTSYMBOLEDITWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
namespace Ui
{
class DAChartSymbolEditWidget;
}
namespace DA
{
class DAGUI_API DAChartSymbolEditWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAChartSymbolEditWidget(QWidget* parent = nullptr);
    ~DAChartSymbolEditWidget();

private:
    Ui::DAChartSymbolEditWidget* ui;
};
}

#endif  // DACHARTSYMBOLEDITWIDGET_H
