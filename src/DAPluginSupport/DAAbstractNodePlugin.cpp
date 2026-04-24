#include "DAAbstractNodePlugin.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DAPyWorkFlowOperateWidget.h"
namespace DA
{
DAAbstractNodePlugin::DAAbstractNodePlugin() : DAAbstractPlugin()
{
}

DAAbstractNodePlugin::~DAAbstractNodePlugin()
{
}

void DAAbstractNodePlugin::afterLoadedNodes()
{
}

DAPyWorkFlowOperateWidget* DAAbstractNodePlugin::getCurrentActiveWorkflowOperateWidget() const
{
    auto c = core();
    if (!c) {
        return nullptr;
    }
    auto ui = c->getUiInterface();
    if (!ui) {
        return nullptr;
    }
    auto d = ui->getDockingArea();
    if (!d) {
        return nullptr;
    }
    return d->getWorkFlowOperateWidget();
}

DAPyWorkFlow* DAAbstractNodePlugin::getCurrentActiveWorkFlow() const
{
    auto optWF = getCurrentActiveWorkflowOperateWidget();
	if (!optWF) {
		return nullptr;
	}
	return optWF->getCurrentWorkflow();
}

}  // end DA
