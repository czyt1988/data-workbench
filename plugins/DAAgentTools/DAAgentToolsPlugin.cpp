#include "DAAgentToolsPlugin.h"
#include "DAAgentInterface.h"
#include "DACoreInterface.h"
#include "DAAbstractAgentTool.h"
// 16 built-in tools (moved from src/DAAgent/tools/ by plan-03)
// Data tools (5) -- inherit DAAgentToolBase
#include "tools/DAAgentToolListData.h"
#include "tools/DAAgentToolDataInfo.h"
#include "tools/DAAgentToolQueryData.h"
#include "tools/DAAgentToolColumnStats.h"
#include "tools/DAAgentToolExportData.h"
// Chart tools (8) -- inherit DAAgentChartToolBase
#include "tools/DAAgentToolCreateChart.h"
#include "tools/DAAgentToolAddCurve.h"
#include "tools/DAAgentToolSetChartStyle.h"
#include "tools/DAAgentToolAddAnnotation.h"
#include "tools/DAAgentToolAddRegion.h"
#include "tools/DAAgentToolCreateSubplots.h"
#include "tools/DAAgentToolSaveChartImage.h"
#include "tools/DAAgentToolListFigures.h"
// File/report tools (3) -- inherit DAAgentToolBase
#include "tools/DAAgentToolReadFile.h"
#include "tools/DAAgentToolWriteFile.h"
#include "tools/DAAgentToolSaveReport.h"

namespace DA
{
DAAgentToolsPlugin::DAAgentToolsPlugin() : DAAbstractPlugin() {}

DAAgentToolsPlugin::~DAAgentToolsPlugin() = default;

bool DAAgentToolsPlugin::initialize()
{
    auto* c = core();
    if (!c) {
        return false;  // core not ready, do not register
    }
    auto* agent = c->getAgentInterface();
    if (!agent) {
        return false;  // agent interface not ready, do not register
    }
    // Data tools (5) -- inherit DAAgentToolBase
    agent->registerTool(new DAAgentToolListData(c, this));
    agent->registerTool(new DAAgentToolDataInfo(c, this));
    agent->registerTool(new DAAgentToolQueryData(c, this));
    agent->registerTool(new DAAgentToolColumnStats(c, this));
    agent->registerTool(new DAAgentToolExportData(c, this));
    // Chart tools (8) -- inherit DAAgentChartToolBase
    agent->registerTool(new DAAgentToolCreateChart(c, this));
    agent->registerTool(new DAAgentToolAddCurve(c, this));
    agent->registerTool(new DAAgentToolSetChartStyle(c, this));
    agent->registerTool(new DAAgentToolAddAnnotation(c, this));
    agent->registerTool(new DAAgentToolAddRegion(c, this));
    agent->registerTool(new DAAgentToolCreateSubplots(c, this));
    agent->registerTool(new DAAgentToolSaveChartImage(c, this));
    agent->registerTool(new DAAgentToolListFigures(c, this));
    // File/report tools (3) -- inherit DAAgentToolBase
    agent->registerTool(new DAAgentToolReadFile(c, this));
    agent->registerTool(new DAAgentToolWriteFile(c, this));
    agent->registerTool(new DAAgentToolSaveReport(c, this));
    // 绘图引用提示词：教 agent 用 da-figure: 超链接在回复中引用创建的绘图，
    // 用户点击后由程序 raise 绘图区域并定位到对应 figure。
    agent->registerSystemPrompt(QStringLiteral("figure_reference"), QStringLiteral(
        "## Referencing Figures in Your Reply\n"
        "When you create a chart with `create_chart` / `create_subplots`, the tool returns `figure_name` and `figure_id`.\n"
        "To let the user open a figure directly from your reply, insert a Markdown hyperlink using the `da-figure:` scheme:\n\n"
        "  [human-readable chart name](da-figure:<figure_name>)\n"
        "  # precise form (use when names may duplicate):\n"
        "  [human-readable chart name](da-figure:id=<figure_id>)\n\n"
        "Rules:\n"
        "- The link text should be a short human-readable name (use the chart title or figure_name).\n"
        "- Prefer the name form for brevity; use the id form when figure names may duplicate.\n"
        "- Only reference figures you have created in the current session (the user can click the link to raise and focus that figure).\n"
        "- Use `list_figures` to discover existing figures and their names/ids.\n"
    ));
    return DAAbstractPlugin::initialize();  // base default returns true
}
}  // namespace DA
