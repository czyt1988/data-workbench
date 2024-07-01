#include "DAAbstractNodePlugin.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DAWorkFlowOperateWidget.h"
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

DAWorkFlow* DAAbstractNodePlugin::getCurrentActiveWorkFlow() const
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
	auto optWF = d->getWorkFlowOperateWidget();
	if (!optWF) {
		return nullptr;
	}
	return optWF->getCurrentWorkflow();
}

}  // end DA
