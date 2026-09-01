#pragma once
#include "DAAbstractPlugin.h"
namespace DA
{
/**
 * @brief 论文撰写 agent 插件
 *
 * 演示"插件注册内置 agent"的标准模式（本插件是该机制的第一个使用者）：
 * initialize() 经 DAAgentInterface 完成三件事——
 * 1. registerBuiltinAgent("论文撰写助手", ...)：从 qrc 读取提示词播种到
 *    <exe>/daAgent/（仅文件不存在时写入，尊重用户已有编辑）；
 * 2. registerTool(search_literature / verify_doi)：注册文献检索与 DOI
 *    验证工具（CrossRef/OpenAlex 免费公开 API）；
 * 3. registerSystemPrompt("literature_tools")：注入工具用法提示词片段。
 *
 * 完整开发指南见 docs/zh/dev-guide/agent/register-builtin-agent-via-plugin.md。
 */
class DAPaperAgentPlugin : public QObject, public DAAbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DAABSTRACTPLUGIN_IID)
    Q_INTERFACES(DA::DAAbstractPlugin)
public:
    DAPaperAgentPlugin();
    ~DAPaperAgentPlugin() override;
    bool initialize() override;
    QString getIID() const override { return DAABSTRACTPLUGIN_IID; }
    QString getName() const override { return QStringLiteral("DAPaperAgent"); }
    QString getVersion() const override { return QStringLiteral("0.0.1"); }
    QString getDescription() const override
    {
        return tr("Built-in paper writing agent with literature search tools");  // cn:内置论文撰写 agent（附文献检索工具）
    }
};
}  // namespace DA
