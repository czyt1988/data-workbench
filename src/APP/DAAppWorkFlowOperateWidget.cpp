#include "DAAppWorkFlowOperateWidget.h"
#include "DADataWorkFlow.h"
#include "DAWorkFlowEditWidget.h"
#include "DAAppCore.h"
#include "DAAppCommand.h"
#include "DAAppPluginManager.h"
#include "DAAbstractNodeFactory.h"
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
    DADataWorkFlow* wf = new DADataWorkFlow();
    if (!mPluginMgr) {
        return wf;
    }
    const auto factorys = mPluginMgr->createNodeFactorys();
    for (const auto& factory : factorys) {
        // 注册工厂
        wf->registFactory(factory);
    }
    return wf;
}

void DAAppWorkFlowOperateWidget::setPluginManager(DAAppPluginManager* pluginMgr)
{
    mPluginMgr = pluginMgr;
}

void DAAppWorkFlowOperateWidget::onWorkflowCreated(DAWorkFlowEditWidget* wfw)
{
    cmd()->addStack(wfw->getUndoStack());
}

/**
 * @brief 切换workflow
 * @param w
 */
void DAAppWorkFlowOperateWidget::onCurrentWorkFlowWidgetChanged(DAWorkFlowEditWidget* w)
{
    if (w) {
        w->getUndoStack()->setActive(true);
    }
}

void DAAppWorkFlowOperateWidget::onWorkflowRemoving(DAWorkFlowEditWidget* w)
{
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
