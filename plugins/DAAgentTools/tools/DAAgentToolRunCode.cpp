#include "DAAgentToolRunCode.h"
#include "DAPyScriptRunner.h"

namespace DA
{
/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
QJsonObject DAAgentToolRunCode::getToolSpec() const
{
    return QJsonObject{
        {"name", "run_code"},
        {"description",
         "Execute inline Python code in a shared persistent namespace (Jupyter-like). Modules "
         "da_app/da_interface/da_data are pre-imported; access in-memory data via "
         "da_app.getCore().getDataManagerInterface().getAllDataframes() (returns {name: dataframe} dict). "
         "Variables persist across run_code/run_script calls — use it for multi-step analysis: define a "
         "variable (e.g. df) in one call and reference it in later calls. The 'result' field is only a text "
         "summary for you to read (set the __result__ variable to return a value), NOT a Python object you "
         "can operate on. Optional 'args' is injected as a dict named 'args'. stdout/stderr are captured. "
         "Note: code runs on the main thread and freezes the UI while running; a timeout (default 300s) "
         "injects KeyboardInterrupt; when a project with a script workspace is open, CWD is the workspace root."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"code", QJsonObject{{"type", "string"}, {"description", "Python code to execute"}}},
                {"args", QJsonObject{{"type", "object"}, {"description", "Optional arguments injected as the 'args' dict in the namespace"}}}
            }},
            {"required", QJsonArray{"code"}}
        }}
    };
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolRunCode::execute(const QJsonObject& params)
{
    const QString code = params["code"].toString();
    if (code.isEmpty()) {
        return errorResponse("code is required");
    }
    return DAPyScriptRunner::runCode(code, params["args"].toObject());
}
}  // namespace DA
