#include "DAAppWorkFlowOperateWidget.h"
#include "DADataWorkFlow.h"
namespace DA
{
DAAppWorkFlowOperateWidget::DAAppWorkFlowOperateWidget(QWidget* parent) : DAWorkFlowOperateWidget(parent)
{
}

DAAppWorkFlowOperateWidget::~DAAppWorkFlowOperateWidget()
{
}

DAWorkFlow* DAAppWorkFlowOperateWidget::createWorkflow()
{
    return (new DADataWorkFlow());
}
}
