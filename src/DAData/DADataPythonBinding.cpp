#include "DADataPythonBinding.h"
#include "DAData.h"
#include "DAPybind11InQt.h"

DA::DAPyDataFrame pyDataFrameToDAPyDataFrame(pybind11::object df)
{
    return DA::DAPyDataFrame(df);
}

void addDataFrameFromPy(DA::DADataManager& mgr, pybind11::object df, const std::string& name)
{
    DA::DAPyDataFrame daDf = pyDataFrameToDAPyDataFrame(df);
    DA::DAData data(daDf);
    data.setName(QString::fromStdString(name));
    mgr.addData(data);  // 调用 C++ 的 addData
}

PYBIND11_EMBEDDED_MODULE(da_data, m)
{
    // 绑定 DAData（轻量包装类，无虚函数问题）
    /**
     * 导出da_data.DAData
     */
    pybind11::class_< DA::DAData >(m, "DAData")
        .def(pybind11::init<>())                    // 默认构造
        .def(pybind11::init< pybind11::object >())  // 直接接受 pandas.DataFrame/Series
        .def("toPyObject", &DA::DAData::toPyObject, "Return the underlying pandas DataFrame as a Python object")
        .def(
            "toDataFrame", [](const DA::DAData& self) { return self.toDataFrame().object(); }, "Convert to DataFrame")
        .def(
            "toSeries", [](const DA::DAData& self) { return self.toSeries().object(); }, "Convert to Series")
        .def("getName", [](const DA::DAData& self) { return self.getName().toStdString(); })
        .def("setName", [](DA::DAData& self, const std::string& n) { self.setName(QString::fromStdString(n)); })
        .def("getDescribe", [](const DA::DAData& self) { return self.getDescribe().toStdString(); })
        .def("setDescribe", [](DA::DAData& self, const std::string& n) { self.setDescribe(QString::fromStdString(n)); })
        .def("isNull", &DA::DAData::isNull, "Check if the data is null")
        .def("id", &DA::DAData::id, "Return the data id")
        .def("isDataFrame", &DA::DAData::isDataFrame, "Check if the data type is DataFrame")
        .def("isSeries", &DA::DAData::isSeries, "Check if the data type is Series")
        .def("setPyObject", &DA::DAData::setPyObject, pybind11::arg("obj"), "Check if the data type is Series")
        .def("getDataManager", &DA::DAData::getDataManager, pybind11::return_value_policy::reference, "get the DataManager");

    /**
     * 导出da_data.DataChangeType
     */
    pybind11::enum_< DA::DADataManager::ChangeType >(m, "DataChangeType")
        .value("Name", DA::DADataManager::ChangeName)
        .value("Describe", DA::DADataManager::ChangeDescribe)
        .value("Value", DA::DADataManager::ChangeValue)
        .value("ColumnName", DA::DADataManager::ChangeDataframeColumnName)
        .export_values();

    /**
     * 导出da_data.DADataManager
     */
    pybind11::class_< DA::DADataManager >(m, "DADataManager")
        .def("addDataFrame", &addDataFrameFromPy, "Add a pandas DataFrame to DA Data Manager")
        .def("getDataCount", &DA::DADataManager::getDataCount)
        .def("getDataName", [](DA::DADataManager& mgr, int i) { return mgr.getData(i).getName().toStdString(); })
        .def("addData",
             static_cast< void (DA::DADataManager::*)(DA::DAData&) >(&DA::DADataManager::addData),
             "Add a DAData object to manager")
        .def("addData_",
             static_cast< void (DA::DADataManager::*)(DA::DAData&) >(&DA::DADataManager::addData_),
             "Add data with undo/redo support")
        // 添加返回QList<DAData>的函数
        .def(
            "getAllDatas",
            [](DA::DADataManager& self) {
                QList< DA::DAData > datas = self.getAllDatas();
                pybind11::list pyList;
                for (const DA::DAData& data : datas) {
                    pyList.append(data);
                }
                return pyList;
            },
            "Get all data objects as a list")
        .def(
            "findDatas",
            [](DA::DADataManager& self, const std::string& pattern, int cs) {
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
            [](DA::DADataManager& self, const std::string& regexPattern) {
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
        // 移除
        .def("removeData", &DA::DADataManager::removeData, pybind11::arg("data"), "Remove data without undo/redo")
        .def("removeData_", &DA::DADataManager::removeData_, pybind11::arg("data"), "Remove data with undo/redo")

        // 索引 & 取值
        .def("getDataIndex", &DA::DADataManager::getDataIndex, pybind11::arg("data"), "Return index of data in manager")
        .def("getData", &DA::DADataManager::getData, pybind11::arg("index"), "Return DAData at index")

        // dirty 标志
        .def("isDirty", &DA::DADataManager::isDirty, "Return true if manager has unsaved changes")
        .def("setDirtyFlag", &DA::DADataManager::setDirtyFlag, pybind11::arg("on"), "Set/unset dirty flag manually")
        // 信号触发器
        .def("notifyDataChangedSignal",
             &DA::DADataManager::notifyDataChangedSignal,
             "Notify the UI that a data item has changed",
             pybind11::arg("data"),
             pybind11::arg("changeType"))  // 通知刷新
        ;
}
