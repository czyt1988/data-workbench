---
name: add-python-binding
description: 当需要为DAWorkBench项目增加新的Python绑定（将C++类/函数暴露给Python），或修改现有绑定时使用此skill。触发词：python绑定、pybind11绑定、暴露给python、增加绑定、导出python模块、绑定C++到Python。
---

# Add Python Binding (Chinese)

## 概述

本技能指导在 DAWorkBench 项目中为 C++ 代码添加 Python 绑定。项目使用 pybind11 嵌入模式 (`PYBIND11_EMBEDDED_MODULE`)，将 C++ 类和函数暴露给 Python 脚本环境。绑定分为四种场景，每种有不同的模式和处理要点。

## 决策树：你要绑定什么？

```
你要绑定什么？
├── 单个自由函数或单例访问器 → Scenario A: 简单函数绑定 (Simple Function Binding)
├── Qt接口类（有QString/QList等Qt类型参数） → Scenario B: Qt接口类绑定 (Qt-Interfacing Class Binding)
├── 数据包装类（pandas/numpy对象包装） → Scenario C: 数据包装类绑定 (Data Wrapper Binding)
└── C++需要调用Python函数 → Scenario D: Python模块导入 (Python Module Import)
```

## !!!danger slots workaround（所有场景第一步）

**在包含任何 pybind11 头文件之前，必须先包含 `DAPybind11InQt.h`。** Qt 的 `slots` 宏与 pybind11 冲突，此头文件处理了 `#undef slots` → 引入 pybind11 → `#define slots Q_SLOTS` 的切换。

```cpp
#include "DAPybind11InQt.h"  // 必须是第一个pybind11相关头文件
```

## Scenario A: 简单函数绑定

**适用于：** 独立自由函数、单例访问器（无 Qt 类型参数）。

### 骨架

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

### 关键步骤

1. `#include "DAPybind11InQt.h"` 作为第一个 pybind11 相关头文件
2. 用 `PYBIND11_EMBEDDED_MODULE(module_name, m)` 定义模块
3. `.def()` 绑定自由函数
4. 单例返回值必须指定 `pybind11::return_value_policy::reference`，否则 pybind11 会尝试析构它
5. 模块名遵循 `da_xxx` 前缀约定

### 参考文件

| 文件 | 用途 |
|------|------|
| `src/APP/PythonBinding/DAAppPythonBinding.cpp` | 完整 Scenario A 示例 |
| `src/APP/PythonBinding/DAAppPythonBinding.h` | 函数声明模板 |
| `src/DAPyBindQt/DAPybind11InQt.h` | slots workaround |

## Scenario B: Qt接口类绑定

**适用于：** 含 `QString`、`QList`、`QJsonObject` 等 Qt 类型参数的接口类。

### 骨架

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

### 关键步骤

1. `#include "DAPybind11InQt.h"` 作为第一个 pybind11 相关头文件（!!!danger 必须）
2. `#include "DAPybind11QtCaster.hpp"` 引入 Qt 类型转换器
3. `pybind11::class_<>()` 绑定接口类
4. `QString` 参数：lambda 包装，接收 `std::string` 后用 `QString::fromStdString()` 转换
5. 返回内部对象：`pybind11::return_value_policy::reference_internal`（生命周期绑定到父对象）
6. `QList<T>` 返回值：lambda 手动遍历转为 `pybind11::list`（不能依赖自动 QList caster）
7. `getConfigValues`：`std::string` JSON → `QString` → 弹对话框 → `QJsonObject` → `DA::PY::qjsonObjectToPyDict()` 转 `pybind11::dict`
8. 跨线程回调：`DAPythonSignalHandler::callInMainThread` + `pybind11::gil_scoped_acquire`

### 参考文件

| 文件 | 用途 |
|------|------|
| `src/DAInterface/DAInterfacePythonBinding.cpp` | 完整 Scenario B 示例 |
| `src/DAPyBindQt/DAPybind11QtCaster.hpp` | Qt 类型转换器 |
| `src/DAPyBindQt/DAPythonSignalHandler.h` | 跨线程通信 |
| `src/DAPyBindQt/DAPyJsonCast.h` | QJsonObject ↔ pybind11::dict |

## Scenario C: 数据包装类绑定

**适用于：** 包装 pandas/numpy Python 对象的 C++ 类（如 `DAData`、`DAPyDataFrame`）。

### 骨架

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

### 关键步骤

1. `#include "DAPybind11InQt.h"` 作为第一个 pybind11 相关头文件
2. `pybind11::init<pybind11::object>()` 构造器接受 pandas 对象
3. `QString` 属性：lambda 包装 `std::string` ↔ `QString` 转换
4. `pybind11::enum_<>()` + `.value()` + `.export_values()` 导出枚举
5. `QList<T>` 返回值：lambda 手动转为 `pybind11::list`
6. 重载函数：用 `static_cast<函数签名>(&Class::method)` 选择正确重载
7. **推荐**：在 `.h` 文件中写完整的 doxygen API 参考（类型一览表 + 构造说明 + 成员说明 + 示例代码 + 限制说明）

### 参考文件

| 文件 | 用途 |
|------|------|
| `src/DAData/DADataPythonBinding.cpp` | 完整 Scenario C 示例 |
| `src/DAData/DADataPythonBinding.h` | .h doxygen API 参考范例（143行，最佳实践） |

## Scenario D: Python模块导入

**适用于：** C++ 需要调用 Python 函数（如 numpy/pandas API），不是暴露 C++ 给 Python。

### 骨架

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

### 关键步骤

1. 继承 `DAPyModule`（它继承 `DAPyObjectWrapper`，不是 QObject）
2. 使用 PIMPL 模式（`DA_DECLARE_PRIVATE` / `DA_PIMPL_CONSTRUCT`）
3. 构造函数中调用 `import()` → `DAPyModule::import("module_name")` 导入 Python 模块
4. 用 `attr("name")` 获取 Python 对象引用，缓存到 `PrivateData` 中（避免每次查找开销）
5. 用 `pybind11::isinstance(obj, cached_type)` 判断类型
6. 单例模式：`getInstance()` 返回 `static` 实例

### 参考文件

| 文件 | 用途 |
|------|------|
| `src/DAPyBindQt/numpy/DAPyModuleNumpy.h` | Scenario D 头文件模板 |
| `src/DAPyBindQt/numpy/DAPyModuleNumpy.cpp` | Scenario D 完整实现示例 |
| `src/DAPyBindQt/DAPyModule.h` | DAPyModule 基类 API |

## 关键差异对比

| 差异点 | Scenario A | Scenario B | Scenario C | Scenario D |
|--------|-----------|-----------|-----------|-----------|
| **基础模式** | `PYBIND11_EMBEDDED_MODULE` + `.def()` | `PYBIND11_EMBEDDED_MODULE` + `class_<>()` | `PYBIND11_EMBEDDED_MODULE` + `class_<>()` + `enum_<>()` | 继承 `DAPyModule`，无 `PYBIND11_EMBEDDED_MODULE` |
| **Qt类型处理** | 无（纯 `std::string`） | lambda包装 `QString` / `QList` 手动转换 | lambda包装 `QString` | 无（C++调用Python，不暴露Qt类型） |
| **所有权策略** | `reference`（单例） | `reference_internal`（子对象绑定父对象生命周期） | 默认（包装类自管理） | N/A（C++侧持有 `pybind11::object`） |
| **枚举导出** | 不适用 | 不适用 | `pybind11::enum_<>()` + `.export_values()` | 不适用 |
| **.h doxygen API参考** | 简洁函数声明 | 可省略（参考 DAInterfacePythonBinding.h 仅4行） | **推荐写**（参考 DADataPythonBinding.h 143行） | 不适用（PIMPL类） |
| **stub更新** | 新增 `da_my_module.pyi` | 新增 `da_my_interface.pyi` | 新增 `da_my_data.pyi` | 不适用 |

## 绑定后必须更新的文件

每添加一个新 Python 绑定模块后，以下文件必须同步更新：

- ✅ **stubs/** — 新增 `.pyi` 文件，参考现有 stub 格式（中文 docstring + 类型注解 + 跨模块导入）。注意循环导入 edge case：如果新模块被 `da_interface` 引用，`da_interface/__init__.pyi` 的 import 语句也需要更新
- ✅ **stubs/mock/** — 新增 mock `.py` 文件，参考现有 mock 格式。注意懒加载模式：参考 `stubs/mock/da_interface.py` 的懒加载避免循环依赖
- ✅ **docs/zh/dev-guide/python-in-cpp.md** — 更新第九部分路线图表格，新增已完成模块行
- ✅ **docs/zh/dev-guide/embedded-python-debugging.md** — 更新第二部分模块概览表格，新增模块行
- ✅ **CMakeLists.txt** — 在 `DA_ENABLE_PYTHON` 条件块中添加新绑定文件
- ✅ **mkdocs.yml** — 如果导航结构需要更新（新模块文档页面）

## 兼容性说明

### Qt5 / Qt6

- **slots workaround**：`DAPybind11InQt.h` 处理 `#undef slots` → 引入 pybind11 → `#define slots Q_SLOTS`，所有绑定文件必须先包含此头文件
- `QVector` 仅 Qt5 有，Qt6 中 `QVector` = `QList`，如需使用需加版本判断宏
- `QButtonGroup::buttonClicked` Qt5 用 `QOverload<int>::of()`，Qt6 用 `idClicked`

### DA_ENABLE_PYTHON 条件编译

所有绑定代码需 `#ifdef DA_ENABLE_PYTHON` 保护。CMakeLists.txt 中绑定文件只在 `DA_ENABLE_PYTHON=ON` 时编译。

### PIMPL类

只绑 `public` 方法。不要尝试绑定 `DA_D` 指针或 `PrivateData` 成员。PIMPL 类的私有数据不应暴露给 Python。

### QwtPlotItem 子类

QwtPlotItem 相关类不继承 QObject，不能用 `Q_OBJECT` 宏，绑定时不使用动态属性。

### QList<绑定类型>

需要 lambda 手动转换为 `pybind11::list`（遍历逐个 append）。不能依赖自动 QList caster，因为绑定类型可能没有注册 QList caster。

### QVariant caster

`DAPybind11QtCaster.hpp` 提供了 QVariant caster，支持 numpy 对象转换，但并非所有 Qt 类型组合都可用作 `QHash` key。

## 参考文件表

| 文件 | 路径 | 用途 |
|------|------|------|
| DAAppPythonBinding.cpp | src/APP/PythonBinding/ | Scenario A 参考 |
| DAAppPythonBinding.h | src/APP/PythonBinding/ | Scenario A 函数声明模板 |
| DAInterfacePythonBinding.cpp | src/DAInterface/ | Scenario B 参考（Qt接口类+lambda包装+QList转换） |
| DADataPythonBinding.cpp | src/DAData/ | Scenario C 参考（数据包装类+枚举导出） |
| DADataPythonBinding.h | src/DAData/ | .h doxygen API 参考范例（143行） |
| DAPyModuleNumpy.h/.cpp | src/DAPyBindQt/numpy/ | Scenario D 参考（模块导入+惰性缓存） |
| DAPyModule.h | src/DAPyBindQt/ | DAPyModule 基类 API |
| DAPybind11InQt.h | src/DAPyBindQt/ | slots workaround（所有场景必须） |
| DAPybind11QtCaster.hpp | src/DAPyBindQt/ | Qt 类型转换器 |
| DAPyInterpreter.h | src/DAPyBindQt/ | Python 解释器管理 |
| DAPythonSignalHandler.h | src/DAPyBindQt/ | 跨线程通信 |
| stubs/da_interface/__init__.pyi | stubs/da_interface/ | stub 格式参考 |
| stubs/mock/da_interface.py | stubs/mock/ | mock 格式参考（懒加载模式） |