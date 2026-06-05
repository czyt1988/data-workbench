#include "DAAppWorkFlowOperateWidget.h"
#include "DADataWorkFlow.h"
#include "DAPyWorkFlowManager.h"
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

DAPyWorkFlowManager* DAAppWorkFlowOperateWidget::createManager()
{
    auto mgr = new DAPyWorkFlowManager();
    // 注入 DADataWorkFlow 替代默认的 DAPyWorkFlow
    mgr->setWorkflow(new DADataWorkFlow());
    return mgr;
}

void DAAppWorkFlowOperateWidget::setPluginManager(DAAppPluginManager* pluginMgr)
{
    mPluginMgr = pluginMgr;
}

void DAAppWorkFlowOperateWidget::onWorkflowCreated(DAPyWorkFlowEditWidget* wfw)
{
    cmd()->addStack(wfw->getUndoStack());
    // 注入Python节点工厂到Manager
    if (mPluginMgr) {
        DAPyWorkFlowManager* mgr = wfw->getManager();
        if (mgr) {
            mgr->setFactory(mPluginMgr->getPyNodeFactory());
        }
    }
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
