#include "DAInterfacePythonBinding.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

#include "DACoreInterface.h"
#include "DADataManagerInterface.h"
#include "DAProjectInterface.h"
#include "DACommandInterface.h"
#include "DAUIInterface.h"
#include "DAData.h"
#include "DADataManager.h"
#if DA_ENABLE_PYTHON
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
#endif


PYBIND11_EMBEDDED_MODULE(da_interface, m)
{
    pybind11::class_< DA::DADataManagerInterface >(m, "DADataManagerInterface")
        .def("add_data", &DA::DADataManagerInterface::addData, pybind11::arg("data"), "Add data immediately")
        .def("add_data_", &DA::DADataManagerInterface::addData_, pybind11::arg("data"), "Add data with undo/redo")
        .def("remove_data", &DA::DADataManagerInterface::removeData, pybind11::arg("data"))
        .def("remove_data_", &DA::DADataManagerInterface::removeData_, pybind11::arg("data"))
        .def("get_count", &DA::DADataManagerInterface::getDataCount)
        .def("get_data", &DA::DADataManagerInterface::getData, pybind11::arg("index"))
        .def("get_index", &DA::DADataManagerInterface::getDataIndex, pybind11::arg("data"))
        .def("get_data_by_id", &DA::DADataManagerInterface::getDataById, pybind11::arg("id"))
        // 快捷：pandas → DAData
        .def(
            "add_dataframe",
            [](DA::DADataManagerInterface& self, pybind11::object df, const std::string& name) {
                DA::DAData data((DA::DAPyDataFrame(df)));
                data.setName(QString::fromStdString(name));
                self.addData(data);
            },
            pybind11::arg("df"),
            pybind11::arg("name"))
        .def(
            "add_series",
            [](DA::DADataManagerInterface& self, pybind11::object se, const std::string& name) {
                DA::DAData data((DA::DAPySeries(se)));
                data.setName(QString::fromStdString(name));
                self.addData(data);
            },
            pybind11::arg("series"),
            pybind11::arg("name"));

    /* 4. DACoreInterface 补充 */
    pybind11::class_< DA::DACoreInterface >(m, "DACoreInterface")
        .def("data_manager",
             &DA::DACoreInterface::getDataManagerInterface,
             pybind11::return_value_policy::reference_internal)  // 返回DADataManagerInterface
        .def("is_project_dirty", &DA::DACoreInterface::isProjectDirty)
        .def("set_project_dirty", &DA::DACoreInterface::setProjectDirty, pybind11::arg("on"));
}
