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
    return DAAbstractPlugin::initialize();  // base default returns true
}
}  // namespace DA
