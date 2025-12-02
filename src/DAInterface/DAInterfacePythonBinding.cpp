#include "DAInterfacePythonBinding.h"
#include "DACoreInterface.h"
#include "DADataManagerInterface.h"
#include "DAProjectInterface.h"
#include "DACommandInterface.h"
#include "DAUIInterface.h"
#include "DAData.h"
#include "DADataManager.h"
#if DA_ENABLE_PYTHON
#include "DAPybind11InQt.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
#endif

PYBIND11_EMBEDDED_MODULE(da_interface, m)
{
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

    /*DAUIInterface*/
    pybind11::class_< DA::DAUIInterface >(m, "DAUIInterface")
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
        .def("isProjectDirty", &DA::DACoreInterface::isProjectDirty)
        .def("setProjectDirty", &DA::DACoreInterface::setProjectDirty, pybind11::arg("on"));
}
