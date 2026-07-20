#include "DAAppPythonBinding.h"
#include "DAAppCore.h"
#include "DAPybind11InQt.h"
#include <QDebug>
#include "DAFigurePythonBinding.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DAChartOperateWidget.h"
namespace DA
{

void addInfoLogMessage(const std::string& msg)
{
    qInfo() << QString::fromStdString(msg);
}

void addWarningLogMessage(const std::string& msg)
{
    qWarning() << QString::fromStdString(msg);
}

void addCriticalLogMessage(const std::string& msg)
{
    qCritical() << QString::fromStdString(msg);
}

}

PYBIND11_EMBEDDED_MODULE(da_app, m)
{
    /* 5. 全局入口函数 */
    m.def("getCore",
          &DA::getAppCorePtr,
          "Return the application core interface (singleton)",
          pybind11::return_value_policy::reference);  // 务必要制定pybind11::return_value_policy::reference，否则pybind11会析构它

    m.def("addInfoLogMessage", &DA::addInfoLogMessage, "add the info message");
    m.def("addWarningLogMessage", &DA::addWarningLogMessage, "add the warning message");
    m.def("addCriticalLogMessage", &DA::addCriticalLogMessage, "add the critical message");

    // Register the chart getter callback for the da_figure module.
    // DAFigure cannot depend on the APP module (circular dependency), so we use
    // a function pointer that's set here during da_app module initialization.
    // The lambda has no captures and converts to a function pointer.
    da_figure::setCurrentChartGetter([]() -> DA::DAChartWidget* {
        auto& core = DA::DAAppCore::getInstance();
        auto ui = core.getUiInterface();
        if (!ui) return nullptr;
        auto dock = ui->getDockingArea();
        if (!dock) return nullptr;
        auto chartOperate = dock->getChartOperateWidget();
        if (!chartOperate) return nullptr;
        return chartOperate->getCurrentChart();
    });
}
