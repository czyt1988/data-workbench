#include "DAAbstractPlugin.h"

namespace DA
{

//===================================================
// DAAbstractPluginPrivate
//===================================================
class DAAbstractPluginPrivate
{
    DA_IMPL_PUBLIC(DAAbstractPlugin)
public:
    DAAbstractPluginPrivate(DAAbstractPlugin* p);
    DACoreInterface* _core;
};

DAAbstractPluginPrivate::DAAbstractPluginPrivate(DAAbstractPlugin* p) : q_ptr(p), _core(nullptr)
{
}

//===================================================
// DAAbstractPlugin
//===================================================

DAAbstractPlugin::DAAbstractPlugin() : d_ptr(new DAAbstractPluginPrivate(this))
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

/**
 * @brief 获取core接口
 * @return
 */
DACoreInterface* DAAbstractPlugin::core() const
{
    return d_ptr->_core;
}

void DAAbstractPlugin::setCore(DACoreInterface* c)
{
    d_ptr->_core = c;
}
}  // end DA
