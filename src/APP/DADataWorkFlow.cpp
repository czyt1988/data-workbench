#include "DADataWorkFlow.h"
#include "DAAppPluginManager.h"
#include "DAAbstractNodeFactory.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DADataWorkFlow
//===================================================
DADataWorkFlow::DADataWorkFlow(QObject* p) : DA::DAWorkFlow(p)
{
}

DADataWorkFlow::~DADataWorkFlow()
{
}
