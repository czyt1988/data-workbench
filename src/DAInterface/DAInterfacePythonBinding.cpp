#include "DAInterfacePythonBinding.h"
#include <QMainWindow>
#include "DACoreInterface.h"
#include "DADataManagerInterface.h"
#include "DAProjectInterface.h"
#include "DACommandInterface.h"
#include "DAStatusBarInterface.h"
#include "DAUIInterface.h"
#include "DAData.h"
#include "DADataManager.h"
#include "DAPythonSignalHandler.h"
#include "DAPybind11InQt.h"
#include "DAPybind11QtTypeCast.h"
#include "DAPyJsonCast.h"

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
        // 添加返回QList<DAData>的函数
        .def(
            "getAllDatas",
            [](DA::DADataManagerInterface& self) {
                QList< DA::DAData > datas = self.getAllDatas();
                pybind11::list pyList;
                for (const DA::DAData& data : datas) {
                    pyList.append(data);
                }
                return pyList;
            },
            "Get all data objects as a list")
        .def(
            "getAllDataframes",
            [](DA::DADataManagerInterface& self) {
                QList< DA::DAData > datas = self.getAllDatas();
                pybind11::dict pydict;
                for (const DA::DAData& data : datas) {
                    if (data.isDataFrame()) {
                        pydict[ DA::PY::toPyStr(data.getName()) ] = data.toPyObject();
                    }
                }
                return pydict;
            },
            "Get all dataframe objects as a dict {dataname,dataframe}")  // 辅助函数获取所有的dataframe
        .def(
            "getSelectDatas",
            [](DA::DADataManagerInterface& self) {
                QList< DA::DAData > datas = self.getSelectDatas();
                pybind11::list pyList;
                for (const DA::DAData& data : datas) {
                    pyList.append(data);
                }
                return pyList;
            },
            "Get selected data objects as a list")
        .def("getOperateData", &DA::DADataManagerInterface::getOperateData, "get current operating data")
        .def(
            "getOperateDataSeries",
            [](DA::DADataManagerInterface& self) {
                QList< int > colindex = self.getOperateDataSeries();
                pybind11::list list;
                for (int v : colindex) {
                    list.append(v);
                }
                return list;
            },
            "get current operating data selected series index")
        .def(
            "getSelectDataframes",
            [](DA::DADataManagerInterface& self) {
                QList< DA::DAData > datas = self.getSelectDatas();
                pybind11::dict pydict;
                for (const DA::DAData& data : datas) {
                    if (data.isDataFrame()) {
                        pydict[ DA::PY::toPyStr(data.getName()) ] = data.toPyObject();
                    }
                }
                return pydict;
            },
            "Get selected data objects as a dict {dataname,dataframe}")
        .def(
            "findDatas",
            [](DA::DADataManagerInterface& self, const std::string& pattern, int cs) {
                QList< DA::DAData > datas =
                    self.findDatas(QString::fromStdString(pattern), static_cast< Qt::CaseSensitivity >(cs));
                pybind11::list pyList;
                for (const DA::DAData& data : datas) {
                    pyList.append(data);
                }
                return pyList;
            },
            pybind11::arg("pattern"),
            pybind11::arg("cs") = static_cast< int >(Qt::CaseInsensitive),
            "Find datas by name pattern,0:CaseInsensitive,1:CaseSensitive")
        .def(
            "findDatasReg",
            [](DA::DADataManagerInterface& self, const std::string& regexPattern) {
                QRegularExpression regex(QString::fromStdString(regexPattern));
                QList< DA::DAData > datas = self.findDatasReg(regex);
                pybind11::list pyList;
                for (const DA::DAData& data : datas) {
                    pyList.append(data);
                }
                return pyList;
            },
            pybind11::arg("regex_pattern"),
            "Find datas by regular expression")

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
            pybind11::arg("name"))  // 添加系列
        ;

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
        .def("isBusy", &DA::DAStatusBarInterface::isBusy)
        .def("resetProgress", &DA::DAStatusBarInterface::resetProgress)
        .def("isProgressBarVisible", &DA::DAStatusBarInterface::isProgressBarVisible);

    /*DACommandInterface*/
    pybind11::class_< DA::DACommandInterface >(m, "DACommandInterface")
        .def("ui",
             &DA::DACommandInterface::ui,
             pybind11::return_value_policy::reference)  // 返回DAUIInterface
        .def(
            "beginDataOperateCommand",
            [](DA::DACommandInterface& self,
               const DA::DAData& data,
               const std::string& text,
               bool isObjectPersist = false,
               bool isSkipFirstRedo = true) {
                self.beginDataOperateCommand(data, QString::fromStdString(text), isObjectPersist, isSkipFirstRedo);
            },
            pybind11::arg("data"),
            pybind11::arg("text"),
            pybind11::arg("isObjectPersist") = true,
            pybind11::arg("isSkipFirstRedo") = true,
            "Start a data operation command, which will be pushed onto the undo stack of the currently active data "
            "operation window")
        .def("endDataOperateCommand",
             &DA::DACommandInterface::endDataOperateCommand,
             pybind11::arg("data"),
             "A function paired with beginDataOperateCommand, used to end the current command and push it onto the "
             "command stack.");

    /*DAUIInterface*/
    pybind11::class_< DA::DAUIInterface >(m, "DAUIInterface")
        .def("getStatusBar",
             &DA::DAUIInterface::getStatusBar,
             pybind11::return_value_policy::reference_internal)  // 返回DAStatusBarInterface
        .def("processEvents", &DA::DAUIInterface::processEvents)
        .def(
            "addInfoLogMessage",
            [](DA::DAUIInterface& self, const std::string& msg, bool showInStatusBar) {
                self.addInfoLogMessage(QString::fromStdString(msg), showInStatusBar);
            },
            pybind11::arg("msg"),
            pybind11::arg("showInStatusBar") = true)
        .def(
            "addWarningLogMessage",
            [](DA::DAUIInterface& self, const std::string& msg, bool showInStatusBar) {
                self.addWarningLogMessage(QString::fromStdString(msg), showInStatusBar);
            },
            pybind11::arg("msg"),
            pybind11::arg("showInStatusBar") = true)
        .def(
            "addCriticalLogMessage",
            [](DA::DAUIInterface& self, const std::string& msg, bool showInStatusBar) {
                self.addCriticalLogMessage(QString::fromStdString(msg), showInStatusBar);
            },
            pybind11::arg("msg"),
            pybind11::arg("showInStatusBar") = true)
        .def("getCommandInterface",
             &DA::DAUIInterface::getCommandInterface,
             pybind11::return_value_policy::reference,
             "Get the command interface")
        .def(
            "getConfigValues",
            [](DA::DAUIInterface& self, const std::string& jsonConfig, const std::string& cacheKey = std::string()) {
                QString qjsonConfig = QString::fromStdString(jsonConfig);
                QString qcacheKey   = QString::fromStdString(cacheKey);
                QJsonObject jsonObj = self.getConfigValues(qjsonConfig, self.getMainWindow(), qcacheKey);
                return DA::PY::qjsonObjectToPyDict(jsonObj);
            },
            pybind11::arg("jsonConfig"),
            pybind11::arg("cacheKey") = "",
            "Execute a generic settings dialog to retrieve configuration information. The input parameter is the JSON "
            "data used to construct the dialog. A cache key can be specified to avoid repeated dialog construction. "
            "This function will launch a modal dialog for users to input parameters.");

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
