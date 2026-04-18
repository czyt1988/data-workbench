---
name: add-python-binding
description: Use when adding new Python bindings (exposing C++ classes/functions to Python) in the DAWorkBench project, or modifying existing bindings. Trigger words: python binding, pybind11 binding, expose to python, add binding, export python module, bind C++ to Python.
---

# Add Python Binding

## Overview

This skill guides adding Python bindings for C++ code in the DAWorkBench project. The project uses pybind11 embedded mode (`PYBIND11_EMBEDDED_MODULE`) to expose C++ classes and functions to the Python scripting environment. Bindings fall into four scenarios, each with distinct patterns and key points.

## Decision Tree: What Are You Binding?

```
What are you binding?
├── A standalone free function or singleton accessor → Scenario A: Simple Function Binding
├── A Qt-interfacing class (with QString/QList etc. parameters) → Scenario B: Qt-Interfacing Class Binding
├── A data wrapper class (pandas/numpy object wrapper) → Scenario C: Data Wrapper Binding
└── C++ needs to call Python functions → Scenario D: Python Module Import
```

## !!!danger slots workaround (First Step for All Scenarios)

**Before including any pybind11 header, you must first include `DAPybind11InQt.h`.** Qt's `slots` macro conflicts with pybind11. This header handles the `#undef slots` → include pybind11 → `#define slots Q_SLOTS` toggle.

```cpp
#include "DAPybind11InQt.h"  // 必须是第一个pybind11相关头文件
```

## Scenario A: Simple Function Binding

**Use for:** Standalone free functions, singleton accessors (no Qt type parameters).

### Skeleton

```cpp
// MyPythonBinding.h
#ifndef MYPYTHONBINDING_H
#define MYPYTHONBINDING_H
#include <string>
namespace DA {
void myHelperFunction(const std::string& msg);
DA::DACoreInterface* getMyCorePtr();
}
#endif

// MyPythonBinding.cpp
#include "MyPythonBinding.h"
#include "DAPybind11InQt.h"  // slots workaround，必须第一个
#include "DACoreInterface.h"

namespace DA {
void myHelperFunction(const std::string& msg) {
    qInfo() << QString::fromStdString(msg);
}
}

PYBIND11_EMBEDDED_MODULE(da_my_module, m)
{
    // 单例访问器：必须使用 return_value_policy::reference
    m.def("getCore",
          &DA::getMyCorePtr,
          "Return the core interface (singleton)",
          pybind11::return_value_policy::reference);

    // 自由函数：默认策略即可
    m.def("myHelper", &DA::myHelperFunction, "A helper function");
}
```

### Key Steps

1. `#include "DAPybind11InQt.h"` as the first pybind11-related header
2. Define module with `PYBIND11_EMBEDDED_MODULE(module_name, m)`
3. `.def()` for free functions
4. Singleton returns must specify `pybind11::return_value_policy::reference`, otherwise pybind11 will try to destruct it
5. Module names follow the `da_xxx` prefix convention

### Reference Files

| File | Purpose |
|------|---------|
| `src/APP/PythonBinding/DAAppPythonBinding.cpp` | Full Scenario A example |
| `src/APP/PythonBinding/DAAppPythonBinding.h` | Function declaration template |
| `src/DAPyBindQt/DAPybind11InQt.h` | slots workaround |

## Scenario B: Qt-Interfacing Class Binding

**Use for:** Interface classes with `QString`, `QList`, `QJsonObject` and other Qt type parameters.

### Skeleton

```cpp
#include "DAPybind11InQt.h"  // slots workaround，必须第一个
#include "DAPybind11QtCaster.hpp"  // Qt类型转换器
#include "DAPythonSignalHandler.h"
#include "MyQtInterface.h"

PYBIND11_EMBEDDED_MODULE(da_my_interface, m)
{
    // 1. 绑定类
    pybind11::class_<DA::MyQtInterface>(m, "MyQtInterface")
        // 2. QString参数 → lambda包装，用 std::string 接收再转换
        .def("showMessage",
             [](DA::MyQtInterface& self, const std::string& message, int timeout) {
                 self.showMessage(QString::fromStdString(message), timeout);
             },
             pybind11::arg("message"),
             pybind11::arg("timeout") = 15000)
        // 3. 返回内部对象 → reference_internal
        .def("getSubInterface",
             &DA::MyQtInterface::getSubInterface,
             pybind11::return_value_policy::reference_internal)
        // 4. QList<T>返回值 → lambda手动转为 pybind11::list
        .def("getAllItems",
             [](DA::MyQtInterface& self) {
                 QList<DA::MyItem> items = self.getAllItems();
                 pybind11::list pyList;
                 for (const DA::MyItem& item : items) {
                     pyList.append(item);
                 }
                 return pyList;
             },
             "Get all items as a Python list")
        // 5. JSON配置对话框（getConfigValues模式）
        .def("getConfigValues",
             [](DA::MyQtInterface& self, const std::string& jsonConfig, const std::string& cacheKey) {
                 QJsonObject jsonObj = self.getConfigValues(
                     QString::fromStdString(jsonConfig),
                     self.getMainWindow(),
                     QString::fromStdString(cacheKey));
                 return DA::PY::qjsonObjectToPyDict(jsonObj);
             },
             pybind11::arg("jsonConfig"),
             pybind11::arg("cacheKey") = "")
        ;

    // 6. 跨线程通信（如需从Python回调到Qt主线程）
    pybind11::class_<DA::DAPythonSignalHandler>(m, "DAPythonSignalHandler")
        .def(pybind11::init<>())
        .def("callInMainThread",
             [](DA::DAPythonSignalHandler& self, pybind11::function pyFunc) {
                 self.callInMainThread([pyFunc]() {
                     try {
                         pybind11::gil_scoped_acquire acquire;
                         pyFunc();
                     } catch (const pybind11::error_already_set& e) {
                         qCritical() << "Python error in main thread callback:" << e.what();
                     }
                 });
             },
             pybind11::arg("func"));
}
```

### Key Steps

1. `#include "DAPybind11InQt.h"` as the first pybind11-related header (!!!danger mandatory)
2. `#include "DAPybind11QtCaster.hpp"` to bring in Qt type casters
3. `pybind11::class_<>()` to bind the interface class
4. `QString` parameters: lambda wrapper, receive `std::string` then convert with `QString::fromStdString()`
5. Return internal objects: `pybind11::return_value_policy::reference_internal` (lifetime bound to parent object)
6. `QList<T>` returns: lambda manually iterates and converts to `pybind11::list` (cannot rely on automatic QList caster)
7. `getConfigValues`: `std::string` JSON → `QString` → dialog → `QJsonObject` → `DA::PY::qjsonObjectToPyDict()` to `pybind11::dict`
8. Cross-thread callbacks: `DAPythonSignalHandler::callInMainThread` + `pybind11::gil_scoped_acquire`

### Reference Files

| File | Purpose |
|------|---------|
| `src/DAInterface/DAInterfacePythonBinding.cpp` | Full Scenario B example |
| `src/DAPyBindQt/DAPybind11QtCaster.hpp` | Qt type casters |
| `src/DAPyBindQt/DAPythonSignalHandler.h` | Cross-thread communication |
| `src/DAPyBindQt/DAPyJsonCast.h` | QJsonObject ↔ pybind11::dict |

## Scenario C: Data Wrapper Binding

**Use for:** C++ classes that wrap pandas/numpy Python objects (e.g. `DAData`, `DAPyDataFrame`).

### Skeleton

```cpp
// MyDataPythonBinding.h — 推荐：写doxygen API参考
#ifndef MYDATAPYTHONBINDING_H
#define MYDATAPYTHONBINDING_H
/**
 * @file MyDataPythonBinding
 * @brief Python 绑定模块 da_my_data 的完整 API 清单与调用示例
 * 
 * @section py_overview 一、模块功能
 * da_my_data 模块把 C++ 的数据包装类暴露给 Python，使脚本能够：
 * - 直接读取/构造 pandas 对象并封装为 MyData；
 * - 将 MyData 加入管理器；
 * - 查询、删除等常规管理操作。
 * 
 * @section py_class_list 二、导出类型一览
 * | Python 类/枚举 | 对应 C++ 类型 | 说明 |
 * |----------------|---------------|------|
 * | MyData         | DA::MyData    | 轻量包装 |
 * | MyEnum         | DA::MyEnum    | 状态枚举 |
 * 
 * @section py_example 三、完整示例
 * @code{.py}
 * import pandas as pd
 * import da_my_data
 * df = pd.read_csv("example.csv")
 * data = da_my_data.MyData(df)
 * data.setName("csv_example")
 * @endcode
 */
#include "DAPybind11InQt.h"
#include "MyData.h"

DA::DAPyDataFrame pyDataFrameToDAPyDataFrame(pybind11::object df);
void addDataFrameFromPy(DA::MyDataManager& mgr, pybind11::object df, const std::string& name);
#endif

// MyDataPythonBinding.cpp
#include "MyDataPythonBinding.h"
#include "DAPybind11InQt.h"  // slots workaround
#include "MyData.h"

DA::DAPyDataFrame pyDataFrameToDAPyDataFrame(pybind11::object df)
{
    return DA::DAPyDataFrame(df);
}

void addDataFrameFromPy(DA::MyDataManager& mgr, pybind11::object df, const std::string& name)
{
    DA::DAPyDataFrame daDf = pyDataFrameToDAPyDataFrame(df);
    DA::MyData data(daDf);
    data.setName(QString::fromStdString(name));
    mgr.addData(data);
}

PYBIND11_EMBEDDED_MODULE(da_my_data, m)
{
    // 1. 绑定数据包装类
    pybind11::class_<DA::MyData>(m, "MyData")
        .def(pybind11::init<>())                    // 默认构造
        .def(pybind11::init<pybind11::object>())    // 直接接受 pandas 对象
        .def("toPyObject", &DA::MyData::toPyObject, "Return the underlying pandas object")
        .def("getName", [](const DA::MyData& self) { return self.getName().toStdString(); })
        .def("setName", [](DA::MyData& self, const std::string& n) { self.setName(QString::fromStdString(n)); })
        .def("isNull", &DA::MyData::isNull, "Check if the data is null")
        .def("id", &DA::MyData::id, "Return the data id");

    // 2. 导出枚举
    pybind11::enum_<DA::MyDataManager::ChangeType>(m, "DataChangeType")
        .value("Name", DA::MyDataManager::ChangeName)
        .value("Value", DA::MyDataManager::ChangeValue)
        .export_values();

    // 3. 绑定数据管理器
    pybind11::class_<DA::MyDataManager>(m, "MyDataManager")
        .def("addDataFrame", &addDataFrameFromPy, "Add a pandas DataFrame to manager")
        .def("addData",
             static_cast<void(DA::MyDataManager::*)(DA::MyData&)>(&DA::MyDataManager::addData),
             "Add a MyData object to manager")
        .def("getDataCount", &DA::MyDataManager::getDataCount)
        // QList → pybind11::list 手动转换
        .def("getAllDatas",
             [](DA::MyDataManager& self) {
                 QList<DA::MyData> datas = self.getAllDatas();
                 pybind11::list pyList;
                 for (const DA::MyData& data : datas) { pyList.append(data); }
                 return pyList;
             });
}
```

### Key Steps

1. `#include "DAPybind11InQt.h"` as the first pybind11-related header
2. `pybind11::init<pybind11::object>()` constructor accepts pandas objects
3. `QString` properties: lambda wrapper with `std::string` ↔ `QString` conversion
4. `pybind11::enum_<>()` + `.value()` + `.export_values()` to export enums
5. `QList<T>` returns: lambda manually converts to `pybind11::list`
6. Overloaded functions: use `static_cast<function_signature>(&Class::method)` to select the correct overload
7. **Recommended**: Write a complete doxygen API reference in the `.h` file (type table + construction notes + member docs + examples + limitations)

### Reference Files

| File | Purpose |
|------|---------|
| `src/DAData/DADataPythonBinding.cpp` | Full Scenario C example |
| `src/DAData/DADataPythonBinding.h` | .h doxygen API reference example (143 lines, best practice) |

## Scenario D: Python Module Import

**Use for:** C++ needs to call Python functions (e.g. numpy/pandas APIs), not exposing C++ to Python.

### Skeleton

```cpp
// DAPyModuleMyLib.h
#ifndef DAPYMODULEMYLIB_H
#define DAPYMODULEMYLIB_H
#include "DAPyBindQtGlobal.h"
#include "DAPyModule.h"
namespace DA {
class DAPYBINDQT_API DAPyModuleMyLib : public DAPyModule
{
    DA_DECLARE_PRIVATE(DAPyModuleMyLib)
    DAPyModuleMyLib();
public:
    ~DAPyModuleMyLib();
    static DAPyModuleMyLib& getInstance();
    void finalize();
    bool import();
    // 暴露缓存好的Python函数/类型
    bool isInstanceMyType(const pybind11::object& obj) const;
private:
    // PrivateData 中缓存 Python 对象引用
};
}  // namespace DA
#endif

// DAPyModuleMyLib.cpp
#include "DAPyModuleMyLib.h"
#include "DAPybind11InQt.h"  // slots workaround
#include <QDebug>

namespace DA {
class DAPyModuleMyLib::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyModuleMyLib)
public:
    PrivateData(DAPyModuleMyLib* p);
    QString mLastErrorString;
    // 缓存 Python 函数/类型引用，避免每次 attr() 查找
    pybind11::object mObjMyType;
    pybind11::object mObjMyFunc;
};

DAPyModuleMyLib::PrivateData::PrivateData(DAPyModuleMyLib* p) : q_ptr(p) {}

DAPyModuleMyLib::DAPyModuleMyLib() : DAPyModule(), DA_PIMPL_CONSTRUCT
{
    import();  // 1. 先导入模块
    try {
        // 2. 惰性缓存关键 Python 对象
        d_ptr->mObjMyType = attr("MyType");
        d_ptr->mObjMyFunc = attr("my_function");
    } catch (const std::exception& e) {
        d_ptr->mLastErrorString = e.what();
    }
}

bool DAPyModuleMyLib::import()
{
    return DAPyModule::import("my_lib");  // 3. 调用基类 importModule
}

bool DAPyModuleMyLib::isInstanceMyType(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjMyType);
}

DAPyModuleMyLib& DAPyModuleMyLib::getInstance()
{
    static DAPyModuleMyLib s_instance;
    return s_instance;
}
}
```

### Key Steps

1. Inherit `DAPyModule` (which inherits `DAPyObjectWrapper`, not QObject)
2. Use PIMPL pattern (`DA_DECLARE_PRIVATE` / `DA_PIMPL_CONSTRUCT`)
3. Call `import()` in constructor → `DAPyModule::import("module_name")` to import the Python module
4. Use `attr("name")` to get Python object references, cache them in `PrivateData` (avoids repeated lookup overhead)
5. Use `pybind11::isinstance(obj, cached_type)` for type checking
6. Singleton pattern: `getInstance()` returns a `static` instance

### Reference Files

| File | Purpose |
|------|---------|
| `src/DAPyBindQt/numpy/DAPyModuleNumpy.h` | Scenario D header template |
| `src/DAPyBindQt/numpy/DAPyModuleNumpy.cpp` | Scenario D full implementation example |
| `src/DAPyBindQt/DAPyModule.h` | DAPyModule base class API |

## Key Differences

| Aspect | Scenario A | Scenario B | Scenario C | Scenario D |
|--------|-----------|-----------|-----------|-----------|
| **Base pattern** | `PYBIND11_EMBEDDED_MODULE` + `.def()` | `PYBIND11_EMBEDDED_MODULE` + `class_<>()` | `PYBIND11_EMBEDDED_MODULE` + `class_<>()` + `enum_<>()` | Inherit `DAPyModule`, no `PYBIND11_EMBEDDED_MODULE` |
| **Qt type handling** | None (pure `std::string`) | Lambda wrapping `QString` / manual `QList` conversion | Lambda wrapping `QString` | None (C++ calls Python, no Qt types exposed) |
| **Ownership policy** | `reference` (singleton) | `reference_internal` (child bound to parent lifetime) | Default (wrapper manages itself) | N/A (C++ side holds `pybind11::object`) |
| **Enum export** | Not applicable | Not applicable | `pybind11::enum_<>()` + `.export_values()` | Not applicable |
| **.h doxygen API reference** | Concise function declarations | Can be omitted (see DAInterfacePythonBinding.h, only 4 lines) | **Recommended** (see DADataPythonBinding.h, 143 lines) | Not applicable (PIMPL class) |
| **Stub update** | Add `da_my_module.pyi` | Add `da_my_interface.pyi` | Add `da_my_data.pyi` | Not applicable |

## Post-Binding Checklist

After adding a new Python binding module, these files must be updated synchronously:

- ✅ **stubs/** — Add a `.pyi` file following existing stub format (Chinese docstring + type annotations + cross-module imports). Watch for circular import edge cases: if the new module is referenced by `da_interface`, update `da_interface/__init__.pyi` imports too
- ✅ **stubs/mock/** — Add a mock `.py` file following existing mock format. Note lazy-loading pattern: see `stubs/mock/da_interface.py` for avoiding circular dependencies
- ✅ **docs/zh/dev-guide/python-in-cpp.md** — Update section 9 roadmap table, add row for the new module
- ✅ **docs/zh/dev-guide/embedded-python-debugging.md** — Update section 2 module overview table, add row for the new module
- ✅ **CMakeLists.txt** — Add new binding files in the `DA_ENABLE_PYTHON` conditional block
- ✅ **mkdocs.yml** — If navigation structure needs updating (new module docs page)

## Compatibility Notes

### Qt5 / Qt6

- **slots workaround**: `DAPybind11InQt.h` handles `#undef slots` → include pybind11 → `#define slots Q_SLOTS`. All binding files must include this header first
- `QVector` only exists in Qt5; in Qt6 `QVector` = `QList`. Use version-check macros if needed
- `QButtonGroup::buttonClicked` in Qt5 uses `QOverload<int>::of()`, Qt6 uses `idClicked`

### DA_ENABLE_PYTHON Conditional Compilation

All binding code must be guarded with `#ifdef DA_ENABLE_PYTHON`. In CMakeLists.txt, binding files are only compiled when `DA_ENABLE_PYTHON=ON`.

### PIMPL Classes

Only bind `public` methods. Never attempt to bind `DA_D` pointers or `PrivateData` members. PIMPL private data must not be exposed to Python.

### QwtPlotItem Subclasses

QwtPlotItem-related classes do not inherit QObject, so `Q_OBJECT` macro is not allowed. Do not use dynamic properties when binding them.

### QList<BoundType>

Requires lambda manual conversion to `pybind11::list` (iterate and append each item). Cannot rely on automatic QList caster because bound types may not have registered one.

### QVariant caster

`DAPybind11QtCaster.hpp` provides a QVariant caster supporting numpy object conversion, but not all Qt type combinations can serve as `QHash` keys.

## Reference Files

| File | Path | Purpose |
|------|------|---------|
| DAAppPythonBinding.cpp | src/APP/PythonBinding/ | Scenario A reference |
| DAAppPythonBinding.h | src/APP/PythonBinding/ | Scenario A function declaration template |
| DAInterfacePythonBinding.cpp | src/DAInterface/ | Scenario B reference (Qt-interfacing class + lambda wrapping + QList conversion) |
| DADataPythonBinding.cpp | src/DAData/ | Scenario C reference (data wrapper class + enum export) |
| DADataPythonBinding.h | src/DAData/ | .h doxygen API reference example (143 lines) |
| DAPyModuleNumpy.h/.cpp | src/DAPyBindQt/numpy/ | Scenario D reference (module import + lazy caching) |
| DAPyModule.h | src/DAPyBindQt/ | DAPyModule base class API |
| DAPybind11InQt.h | src/DAPyBindQt/ | slots workaround (mandatory for all scenarios) |
| DAPybind11QtCaster.hpp | src/DAPyBindQt/ | Qt type casters |
| DAPyInterpreter.h | src/DAPyBindQt/ | Python interpreter management |
| DAPythonSignalHandler.h | src/DAPyBindQt/ | Cross-thread communication |
| stubs/da_interface/__init__.pyi | stubs/da_interface/ | Stub format reference |
| stubs/mock/da_interface.py | stubs/mock/ | Mock format reference (lazy-loading pattern) |