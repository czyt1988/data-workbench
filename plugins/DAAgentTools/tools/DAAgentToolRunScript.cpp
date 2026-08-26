#include "DAAgentToolRunScript.h"
#include <QDir>
#include <QFileInfo>
#include "DAProjectInterface.h"
#include "DAPyScriptRunner.h"

namespace DA
{
/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolRunScript::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("run_script"),
                         QStringLiteral("Run a Python script from the current project's script workspace in a shared "
                                        "persistent namespace (Jupyter-like). 'path' is relative to the workspace root "
                                        "(e.g. \"scripts/analyze.py\"); use write_file to create the script first. "
                                        "Inside the script, modules da_app/da_interface/da_data are pre-imported; "
                                        "access in-memory data via "
                                        "da_app.getCore().getDataManagerInterface().getAllDataframes() (returns {name: "
                                        "dataframe} dict). Variables persist across run_script/run_code calls, so "
                                        "store intermediate results as namespace variables and reference them in later "
                                        "calls — the 'result' field is only a text summary for you to read (a script "
                                        "may set the __result__ variable as its return value), NOT a Python object you "
                                        "can operate on. Optional 'args' is injected as a dict named 'args'. "
                                        "stdout/stderr are captured. Note: scripts run on the main thread and freeze "
                                        "the UI while running; a timeout (default 300s) injects KeyboardInterrupt; "
                                        "scripts must be UTF-8 encoded; CWD is the workspace root.")};
    spec.addParam({QStringLiteral("path"),
                   QStringLiteral("Script path relative to the script workspace root"),
                   {Type::String},
                   true});
    spec.addParam({QStringLiteral("args"),
                   QStringLiteral("Optional arguments injected as the 'args' dict in the namespace"),
                   {Type::Object}});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolRunScript::execute(const QJsonObject& params)
{
    const QString relPath = params["path"].toString();
    if (relPath.isEmpty()) {
        return errorResponse("path is required");
    }
    DAProjectInterface* pi = core() ? core()->getProjectInterface() : nullptr;
    if (!pi) {
        return errorResponse("no project available, script workspace unavailable");
    }
    const QString wsRoot = QDir::cleanPath(pi->getScriptWorkspaceDir());
    if (wsRoot.isEmpty()) {
        // 已打开的工程尚未保存过（区别于"无工程"）
        return errorResponse("the project has not been saved yet, script workspace unavailable; save the project first");
    }
    // 路径遍历防护：cleanPath 词法归一 ../ 后做前缀校验（不能用 canonicalFilePath——
    // 它对不存在的文件返回空串，会把"脚本不存在"误判为"越界"）
    const QString absPath = QDir::cleanPath(QDir(wsRoot).filePath(relPath));
    if (absPath != wsRoot && !absPath.startsWith(wsRoot + QLatin1String("/"))) {
        return errorResponse("path escapes script workspace");
    }
    if (!QFileInfo::exists(absPath)) {
        return errorResponse(QString("script not found: %1").arg(relPath));
    }
    return DAPyScriptRunner::runScript(absPath, params["args"].toObject());
}
}  // namespace DA
