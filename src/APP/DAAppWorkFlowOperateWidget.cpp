#include "DAAppWorkFlowOperateWidget.h"
#include "DADataWorkFlow.h"
#include "DAWorkFlowEditWidget.h"
#include "DAAppCore.h"
#include "DAAppCommand.h"
#include <QDebug>
namespace DA
{
DAAppWorkFlowOperateWidget::DAAppWorkFlowOperateWidget(QWidget* parent) : DAWorkFlowOperateWidget(parent)
{
    connect(this, &DAAppWorkFlowOperateWidget::workflowCreated, this, &DAAppWorkFlowOperateWidget::onWorkflowCreated);
    connect(this, &DAAppWorkFlowOperateWidget::workflowRemoving, this, &DAAppWorkFlowOperateWidget::onWorkflowRemoving);
    connect(this,
            &DAAppWorkFlowOperateWidget::currentWorkFlowWidgetChanged,
            this,
            &DAAppWorkFlowOperateWidget::onCurrentWorkFlowWidgetChanged);
    connect(this, &DAAppWorkFlowOperateWidget::workflowClearing, this, &DAAppWorkFlowOperateWidget::onWorkflowClearing);
}

DAAppWorkFlowOperateWidget::~DAAppWorkFlowOperateWidget()
{
}

DAWorkFlow* DAAppWorkFlowOperateWidget::createWorkflow()
{
    return (new DADataWorkFlow());
}

void DAAppWorkFlowOperateWidget::onWorkflowCreated(DAWorkFlowEditWidget* wfw)
{
    cmd()->addStack(wfw->getUndoStack());
    qInfo() << "workflow created";
}

void DAAppWorkFlowOperateWidget::onCurrentWorkFlowWidgetChanged(DAWorkFlowEditWidget* w)
{
    if (w) {
        qInfo() << "workflow cmd active";
        w->getUndoStack()->setActive(true);
    }
}

void DAAppWorkFlowOperateWidget::onWorkflowRemoving(DAWorkFlowEditWidget* w)
{
    qInfo() << "workflow remove";
    cmd()->removeStack(w->getUndoStack());
}

void DAAppWorkFlowOperateWidget::onWorkflowClearing()
{
    // 把所有的stack脱离
    const QList< DAWorkFlowEditWidget* > all = getAllWorkFlowWidgets();
    for (DAWorkFlowEditWidget* w : all) {
        cmd()->removeStack(w->getUndoStack());
    }
}

DAAppCommand* DAAppWorkFlowOperateWidget::cmd() const
{
    return DAAppCore::getInstance().getAppCmd();
}
}
