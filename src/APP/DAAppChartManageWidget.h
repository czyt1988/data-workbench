#ifndef DAAPPCHARTMANAGEWIDGET_H
#define DAAPPCHARTMANAGEWIDGET_H
#include "DAChartManageWidget.h"

namespace DA
{

class DAAppChartManageWidget : public DAChartManageWidget
{
    Q_OBJECT
public:
    DAAppChartManageWidget(QWidget* parent = nullptr);
    ~DAAppChartManageWidget();
};

}  // end DA
#endif  // DAAPPCHARTMANAGEWIDGET_H
