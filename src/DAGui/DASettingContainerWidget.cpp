#include "DASettingContainerWidget.h"
#include "DAWorkFlowNodeItemSettingWidget.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DASettingContainerWidget
//===================================================
DASettingContainerWidget::DASettingContainerWidget(QWidget* parent) : QStackedWidget(parent)
{
    initWorkFlowSettingWidgets();
}

DASettingContainerWidget::~DASettingContainerWidget()
{
}

void DASettingContainerWidget::initWorkFlowSettingWidgets()
{
    _workFlowNodeItemSettingWidget = new DAWorkFlowNodeItemSettingWidget();
    addWidget(_workFlowNodeItemSettingWidget);
}

DAWorkFlowNodeItemSettingWidget* DASettingContainerWidget::getWorkFlowNodeItemSettingWidget()
{
    return _workFlowNodeItemSettingWidget;
}
