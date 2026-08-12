#include "DataAnalysisBaseWorker.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataManagerInterface.h"
#include "SARibbonMainWindow.h"
#include "DACommandInterface.h"
/**
 * @brief 构造函数
 * @param par 父对象
 */
DataAnalysisBaseWorker::DataAnalysisBaseWorker(QObject* par) : QObject(par)
{
}

/**
 * @brief 析构函数
 */
DataAnalysisBaseWorker::~DataAnalysisBaseWorker()
{
}

/**
 * @brief 初始化
 * @param core 核心接口指针
 */
void DataAnalysisBaseWorker::initialize(DA::DACoreInterface* core)
{
    DA::DAInterfaceHelper::initialize(core);
}
