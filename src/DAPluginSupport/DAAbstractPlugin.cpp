#include "DAAbstractPlugin.h"

namespace DA
{

//===================================================
// DAAbstractPluginPrivate
//===================================================
class DAAbstractPlugin::PrivateData
{
    DA_DECLARE_PUBLIC(DAAbstractPlugin)
public:
    PrivateData(DAAbstractPlugin* p);
    DACoreInterface* mCore { nullptr };
};

DAAbstractPlugin::PrivateData::PrivateData(DAAbstractPlugin* p) : q_ptr(p)
{
}

//===================================================
// DAAbstractPlugin
//===================================================

DAAbstractPlugin::DAAbstractPlugin() : DA_PIMPL_CONSTRUCT
{
}

DAAbstractPlugin::~DAAbstractPlugin()
{
}

void DAAbstractPlugin::retranslate()
{
}

bool DAAbstractPlugin::initialize()
{
    return true;
}

DAAbstractSettingPage* DAAbstractPlugin::createSettingPage()
{
    return nullptr;
}

/**
 * @brief 获取core接口
 * @return
 */
DACoreInterface* DAAbstractPlugin::core() const
{
    return d_ptr->mCore;
}

/**
 * @brief 设置core
 * @param c
 */
void DAAbstractPlugin::setCore(DACoreInterface* c)
{
    d_ptr->mCore = c;
}
}  // end DA
