#include "DAStatusBar.h"
#include "DAStatusBarWidget.h"
namespace DA
{
DAStatusBar::DAStatusBar(QWidget* par) : QStatusBar(par)
{
    mStatusWidget = new DAStatusBarWidget(this);
    addWidget(mStatusWidget, 1);  // 1表示拉伸因子
}

DAStatusBarWidget* DAStatusBar::getStatusWidget() const
{
    return mStatusWidget;
}
}
