#include "DASystemNodesPlugin.h"
#include <QDebug>

DASystemNodesPlugin::DASystemNodesPlugin() : DA::DAAbstractNodePlugin()
{
}

DASystemNodesPlugin::~DASystemNodesPlugin()
{
}

bool DASystemNodesPlugin::initialize()
{
    return DA::DAAbstractNodePlugin::initialize();
}

QString DASystemNodesPlugin::getIID() const
{
    return "DA.Plugin.DASystemNodes";
}

QString DASystemNodesPlugin::getName() const
{
    return u8"DA System Nodes Plugin";
}

QString DASystemNodesPlugin::getVersion() const
{
    return "1.0.0";
}

QString DASystemNodesPlugin::getDescription() const
{
    return u8"Provides system-level workflow nodes for UI interaction, flow control and data display";
}

DA::DAPyNodeFactory* DASystemNodesPlugin::createNodeFactory()
{
    // 节点由 Python 包 DASystemNodes 通过 pyplugins 目录扫描自动注册
    return nullptr;
}

void DASystemNodesPlugin::destroyNodeFactory(DA::DAPyNodeFactory* p)
{
    Q_UNUSED(p)
}

DA::DAAbstractSettingPage* DASystemNodesPlugin::createSettingPage()
{
    return nullptr;
}

void DASystemNodesPlugin::retranslate()
{
}
