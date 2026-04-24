#include "DAAppWorkFlowOperateWidget.h"
#include "DADataWorkFlow.h"
#include "DAPyWorkFlowEditWidget.h"
#include "DAAppCore.h"
#include "DAAppCommand.h"
#include "DAAppPluginManager.h"
#include "DAPyNodeFactory.h"
#include <QDebug>
namespace DA
{
DAAppWorkFlowOperateWidget::DAAppWorkFlowOperateWidget(QWidget* parent) : DAPyWorkFlowOperateWidget(parent)
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

DAPyWorkFlow* DAAppWorkFlowOperateWidget::createWorkflow()
{
    DADataWorkFlow* wf = new DADataWorkFlow();
    if (!mPluginMgr) {
        return wf;
    }
    // TODO: DAPyWorkFlow不再使用registFactory模式，工厂注册需要通过DAPyNodeFactory管理
    // 旧的registFactory接口已移除，后续需要适配新的插件加载方式
    return wf;
}

void DAAppWorkFlowOperateWidget::setPluginManager(DAAppPluginManager* pluginMgr)
{
    mPluginMgr = pluginMgr;
}

void DAAppWorkFlowOperateWidget::onWorkflowCreated(DAPyWorkFlowEditWidget* wfw)
{
    cmd()->addStack(wfw->getUndoStack());
}

/**
 * @brief 切换workflow
 * @param w
 */
void DAAppWorkFlowOperateWidget::onCurrentWorkFlowWidgetChanged(DAPyWorkFlowEditWidget* w)
{
    if (w) {
        w->getUndoStack()->setActive(true);
    }
}

void DAAppWorkFlowOperateWidget::onWorkflowRemoving(DAPyWorkFlowEditWidget* w)
{
    cmd()->removeStack(w->getUndoStack());
}

void DAAppWorkFlowOperateWidget::onWorkflowClearing()
{
    // 把所有的stack脱离
    const QList< DAPyWorkFlowEditWidget* > all = getAllWorkFlowWidgets();
    for (DAPyWorkFlowEditWidget* w : all) {
        cmd()->removeStack(w->getUndoStack());
    }
}

DAAppCommand* DAAppWorkFlowOperateWidget::cmd() const
{
    return DAAppCore::getInstance().getAppCmd();
}
}
