#pragma once
#include "DAAbstractPlugin.h"
namespace DA
{
/**
 * @brief Plugin that registers the platform's 18 built-in agent tools.
 *
 * This is the first plugin in the repo to derive directly from DAAbstractPlugin
 * (IID "org.da.abstract.plugin"); unlike DataAnalysis/DASystemNodes it does not
 * need a node factory. initialize() fetches the agent interface from the core and
 * registers the 18 tools (5 data + 10 chart + 3 file/report) that previously lived
 * inside DAAgentModule::registerBuiltinTools (plan-03搬迁).
 *
 * The plugin owns the 18 tool QObjects (parented to this), so they are released
 * together with the plugin.
 */
class DAAgentToolsPlugin : public QObject, public DAAbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DAABSTRACTPLUGIN_IID)
    Q_INTERFACES(DA::DAAbstractPlugin)
public:
    DAAgentToolsPlugin();
    ~DAAgentToolsPlugin() override;
    bool initialize() override;
    QString getIID() const override { return DAABSTRACTPLUGIN_IID; }
    QString getName() const override { return QStringLiteral("DAAgentTools"); }
    QString getVersion() const override { return QStringLiteral("0.0.1"); }
    QString getDescription() const override
    {
        return tr("Platform built-in agent tools"); //cn: 平台内置 agent 工具
    }
};
}  // namespace DA
