#include "DAInterfacePythonBinding.h"
#include "DACoreInterface.h"
#include "DADataManagerInterface.h"
#include "DAProjectInterface.h"
#include "DACommandInterface.h"
#include "DAStatusBarInterface.h"
#include "DAUIInterface.h"
#include "DAData.h"
#include "DADataManager.h"
#include "DAPythonSignalHandler.h"
#if DA_ENABLE_PYTHON
#include "DAPybind11InQt.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
#endif

PYBIND11_EMBEDDED_MODULE(da_interface, m)
{
    // 绑定 DAPythonSignalHandler 类型
    pybind11::class_< DA::DAPythonSignalHandler >(m, "DAPythonSignalHandler")
        .def(pybind11::init<>())  // 可以构造，但通常不会在Python中构造
        .def(
            "callInMainThread",
            [](DA::DAPythonSignalHandler& self, pybind11::function pyFunc) {
                // 将Python函数包装成std::function
                self.callInMainThread([ pyFunc ]() {
                    try {
                        pybind11::gil_scoped_acquire acquire;  // 获取GIL
                        pyFunc();                              // 调用Python函数
                    } catch (const pybind11::error_already_set& e) {
                        qCritical() << "Python error in main thread callback:" << e.what();
                    } catch (const std::exception& e) {
                        qCritical() << "C++ error in main thread callback:" << e.what();
                    }
                });
            },
            pybind11::arg("func"),
            "Schedule a Python function to be executed in the Qt main thread");

    /*DADataManagerInterface*/
    pybind11::class_< DA::DADataManagerInterface >(m, "DADataManagerInterface")
        .def("addData", &DA::DADataManagerInterface::addData, pybind11::arg("data"), "Add data immediately")
        .def("addData_", &DA::DADataManagerInterface::addData_, pybind11::arg("data"), "Add data with undo/redo")
        .def("removeData", &DA::DADataManagerInterface::removeData, pybind11::arg("data"))
        .def("removeData_", &DA::DADataManagerInterface::removeData_, pybind11::arg("data"))
        .def("getDataCount", &DA::DADataManagerInterface::getDataCount)
        .def("getData", &DA::DADataManagerInterface::getData, pybind11::arg("index"))
        .def("getDataIndex", &DA::DADataManagerInterface::getDataIndex, pybind11::arg("data"))
        .def("getDataById", &DA::DADataManagerInterface::getDataById, pybind11::arg("id"))
        // 快捷：pandas → DAData
        .def(
            "addDataframe",
            [](DA::DADataManagerInterface& self, pybind11::object df, const std::string& name) {
                DA::DAData data((DA::DAPyDataFrame(df)));
                data.setName(QString::fromStdString(name));
                self.addData(data);
            },
            pybind11::arg("df"),
            pybind11::arg("name"))
        .def(
            "addSeries",
            [](DA::DADataManagerInterface& self, pybind11::object se, const std::string& name) {
                DA::DAData data((DA::DAPySeries(se)));
                data.setName(QString::fromStdString(name));
                self.addData(data);
            },
            pybind11::arg("series"),
            pybind11::arg("name"));

    /*DAStatusBarInterface*/
    pybind11::class_< DA::DAStatusBarInterface >(m, "DAStatusBarInterface")
        .def(
            "showMessage",
            [](DA::DAStatusBarInterface& self, const std::string& message, int timeout = 15000) {
                self.showMessage(QString::fromStdString(message), timeout);
            },
            pybind11::arg("message"),
            pybind11::arg("timeout") = 15000)
        .def("clearMessage", &DA::DAStatusBarInterface::clearMessage)
        .def("showProgressBar", &DA::DAStatusBarInterface::showProgressBar)
        .def("hideProgressBar", &DA::DAStatusBarInterface::hideProgressBar)
        .def("setProgress", &DA::DAStatusBarInterface::setProgress, pybind11::arg("value"))
        .def(
            "setProgressText",
            [](DA::DAStatusBarInterface& self, const std::string& text) {
                self.setProgressText(QString::fromStdString(text));
            },
            pybind11::arg("text"))
        .def("clearProgressText", &DA::DAStatusBarInterface::clearProgressText)
        .def("setBusy", &DA::DAStatusBarInterface::setBusy, pybind11::arg("busy"))
        .def("resetProgress", &DA::DAStatusBarInterface::resetProgress)
        .def("isProgressBarVisible", &DA::DAStatusBarInterface::isProgressBarVisible);

    /*DAUIInterface*/
    pybind11::class_< DA::DAUIInterface >(m, "DAUIInterface")
        .def("getStatusBar",
             &DA::DAUIInterface::getStatusBar,
             pybind11::return_value_policy::reference_internal)  // 返回DAStatusBarInterface
        .def("processEvents", &DA::DAUIInterface::processEvents)
        .def(
            "addInfoLogMessage",
            [](DA::DAUIInterface& self, const std::string& msg) { self.addInfoLogMessage(QString::fromStdString(msg)); },
            pybind11::arg("msg"))
        .def(
            "addWarningLogMessage",
            [](DA::DAUIInterface& self, const std::string& msg) {
                self.addWarningLogMessage(QString::fromStdString(msg));
            },
            pybind11::arg("msg"))
        .def(
            "addCriticalLogMessage",
            [](DA::DAUIInterface& self, const std::string& msg) {
                self.addCriticalLogMessage(QString::fromStdString(msg));
            },
            pybind11::arg("msg"));

    /* 4. DACoreInterface 补充 */
    pybind11::class_< DA::DACoreInterface >(m, "DACoreInterface")
        .def("getUiInterface",
             &DA::DACoreInterface::getUiInterface,
             pybind11::return_value_policy::reference_internal)  // 返回getUiInterface
        .def("getDataManagerInterface",
             &DA::DACoreInterface::getDataManagerInterface,
             pybind11::return_value_policy::reference_internal)  // 返回DADataManagerInterface
        .def("getPythonSignalHandler",
             &DA::DACoreInterface::getPythonSignalHandler,
             pybind11::return_value_policy::reference_internal,
             "Get the Python signal handler for cross-thread communication")
        .def("isProjectDirty", &DA::DACoreInterface::isProjectDirty)
        .def("setProjectDirty", &DA::DACoreInterface::setProjectDirty, pybind11::arg("on"));
}
