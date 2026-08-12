#ifndef DAAPPCHARTMANAGEWIDGET_H
#define DAAPPCHARTMANAGEWIDGET_H
#include "Chart/DAChartManageWidget.h"

namespace DA
{

class DAAppChartManageWidget : public DAChartManageWidget
{
    Q_OBJECT
public:
    DAAppChartManageWidget(QWidget* parent = nullptr);
    virtual ~DAAppChartManageWidget() override;
};

}  // end DA
#endif  // DAAPPCHARTMANAGEWIDGET_H
