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
    DAAppPluginManager& pluginmgr = DAAppPluginManager::instance();
    const auto factorys           = pluginmgr.createNodeFactorys();
    // 提取所有的元数据
    QList< DANodeMetaData > nodeMetaDatas;

    for (auto factory : factorys) {
        nodeMetaDatas += factory->getNodesMetaData();
        // 注册工厂
        registFactory(factory);
    }
}
