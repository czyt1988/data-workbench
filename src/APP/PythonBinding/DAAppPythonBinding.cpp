#include "DAAppPythonBinding.h"
#include "DAAppCore.h"
#include "DAPybind11InQt.h"
#include <QDebug>
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
}
