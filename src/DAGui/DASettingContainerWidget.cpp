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
    //初始化工作流相关的配置窗口
    initWorkFlowSettingWidgets();
}

DASettingContainerWidget::~DASettingContainerWidget()
{
}

void DASettingContainerWidget::initWorkFlowSettingWidgets()
{
    mWorkFlowNodeItemSettingWidget = new DAWorkFlowNodeItemSettingWidget();
    addWidget(mWorkFlowNodeItemSettingWidget);
}

DAWorkFlowNodeItemSettingWidget* DASettingContainerWidget::getWorkFlowNodeItemSettingWidget()
{
    return mWorkFlowNodeItemSettingWidget;
}
