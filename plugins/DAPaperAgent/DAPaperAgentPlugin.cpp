#include "DAPaperAgentPlugin.h"
#include "DAAgentInterface.h"
#include "DACoreInterface.h"
#include "DAAbstractAgentTool.h"
// Literature tools (2) -- inherit DAAgentToolBase
#include "tools/DAPaperToolSearchLiterature.h"
#include "tools/DAPaperToolVerifyDoi.h"
#include <QFile>

namespace DA
{
/**
 * @brief 构造函数
 */
DAPaperAgentPlugin::DAPaperAgentPlugin() : DAAbstractPlugin() {}

/**
 * @brief 析构函数
 */
DAPaperAgentPlugin::~DAPaperAgentPlugin() = default;

/**
 * @brief 初始化插件：注册内置论文撰写 agent、文献工具与工具用法提示词
 * @return 初始化成功返回 true，若核心接口或 Agent 接口不可用则返回 false
 */
bool DAPaperAgentPlugin::initialize()
{
    auto* c = core();
    if (!c) {
        return false;  // core not ready, do not register
    }
    auto* agent = c->getAgentInterface();
    if (!agent) {
        return false;  // agent interface not ready, do not register
    }
    // 注册内置 agent：提示词经 qrc 内嵌随插件部署，仅当 <daAgent>/论文撰写助手.md
    // 不存在时写入（尊重用户已有编辑/删除）
    QFile promptFile(QStringLiteral(":/da/paper/paper-agent.md"));
    if (promptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(promptFile.readAll());
        promptFile.close();
        if (!content.trimmed().isEmpty()) {
            agent->registerBuiltinAgent(QStringLiteral("论文撰写助手"), content);
        } else {
            qWarning("DAPaperAgentPlugin::initialize: embedded paper-agent.md is empty, skip agent registration");
        }
    } else {
        qWarning("DAPaperAgentPlugin::initialize: cannot open :/da/paper/paper-agent.md from resources");
    }
    // Literature tools (2) -- inherit DAAgentToolBase
    agent->registerTool(new DAPaperToolSearchLiterature(c, this));
    agent->registerTool(new DAPaperToolVerifyDoi(c, this));
    // 文献工具用法提示词：教 agent 在论文写作场景中先检索后引用、引用前必验 DOI。
    agent->registerSystemPrompt(QStringLiteral("literature_tools"), QStringLiteral(
        "## Literature Search and DOI Verification\n"
        "Two tools are available for academic writing:\n"
        "- `search_literature(query, max_results, source)`: search real academic papers via the free\n"
        "  CrossRef / OpenAlex metadata APIs. Returns normalized entries (DOI, title, authors, journal,\n"
        "  year, type). Try to combine keywords of the research topic; use `source` to force one API.\n"
        "- `verify_doi(doi)`: verify whether a DOI exists and fetch its exact metadata (title, journal,\n"
        "  year, authors) from CrossRef.\n\n"
        "Rules:\n"
        "- NEVER invent a citation. Every reference you list must come from `search_literature` results,\n"
        "  user-provided material, or a `verify_doi` lookup.\n"
        "- Before including any DOI in a reference list, verify it with `verify_doi` (or confirm it came\n"
        "  directly from `search_literature` output).\n"
        "- If both APIs are unreachable (network error), say so honestly and switch to offline mode:\n"
        "  build the reference list only from literature supplied by the user, clearly marked as\n"
        "  \"pending manual verification\".\n"
    ), this);
    return DAAbstractPlugin::initialize();  // base default returns true
}
}  // namespace DA
