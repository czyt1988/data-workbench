#include "DataAnalysisBaseWorker.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataManagerInterface.h"
#include "SARibbonMainWindow.h"
#include "DACommandInterface.h"
DataAnalysisBaseWorker::DataAnalysisBaseWorker(QObject* par) : QObject(par)
{
}

DataAnalysisBaseWorker::~DataAnalysisBaseWorker()
{
}

void DataAnalysisBaseWorker::initialize(DA::DACoreInterface* core)
{
    DA::DAInterfaceHelper::initialize(core);
}
