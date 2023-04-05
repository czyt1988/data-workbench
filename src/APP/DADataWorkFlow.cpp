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
    DAAppPluginManager& pluginmgr            = DAAppPluginManager::instance();
    QList< DAAbstractNodeFactory* > factorys = pluginmgr.getNodeFactorys();
    //提取所有的元数据
    QList< DANodeMetaData > nodeMetaDatas;

    for (DAAbstractNodeFactory* factory : qAsConst(factorys)) {
        nodeMetaDatas += factory->getNodesMetaData();
        //注册工厂
        registFactory(factory);
    }
}
