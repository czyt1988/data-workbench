# DAPybind11QtCaster.hpp — pybind11 与 Qt 类型转换器

本文档详细说明 `src/DAPyBindQt/DAPybind11QtCaster.hpp` 提供的 pybind11 ↔ Qt 类型自动双向转换机制。**所有需要处理 Python 对象与 Qt 类型互转的代码都应依赖此文件，不应自行实现类型转换逻辑。**

## 概述

`DAPybind11QtCaster.hpp` 通过 pybind11 的 `type_caster<T>` 模板特化机制，为以下 Qt 类型注册了自动双向类型转换器：

- **基本类型**: `QString`, `QByteArray`, `QVariant`
- **日期时间**: `QDate`, `QTime`, `QDateTime`（支持 Python `datetime`、`pandas.Timestamp`、`numpy.datetime64`）
- **容器类型**: `QList<T>`, `QVector<T>`, `QSet<T>`, `QHash<K,V>`, `QMap<K,V>`
- **辅助工具**: `DA::PY::` 命名空间下的便捷转换函数、`safe_pyobject` 安全 Python 对象包装器、`canCast*` 类型检测函数

## 包含方式

**不要直接 `#include <pybind11/pybind11.h>`**，这样会导致 Qt 的 `slots` 宏与 Python 头文件冲突。

**正确方式**：使用 `DAPybind11InQt.h`（它已包含 `DAPybind11QtCaster.hpp` 及所有必要的 pybind11 头文件）：

```cpp
#include "DAPybind11InQt.h"  // 正确！内部已包含 DAPybind11QtCaster.hpp
```

`DAPybind11InQt.h` 的处理逻辑：

```cpp
// 1. 取消 Qt 的空 slots 宏定义
#undef slots
// 2. 定义 Python 要求的宏
#define PY_SSIZE_T_CLEAN
// 3. 安全引入 pybind11 系列头文件
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "pybind11/cast.h"
#include "pybind11/embed.h"
#include "pybind11/stl.h"
#include "pybind11/chrono.h"
#include "pybind11/operators.h"
// 4. 恢复 Qt 的 slots 宏为 Q_SLOTS
#define slots Q_SLOTS
```

所有进行 pybind11 绑定的 `.cpp` 文件必须以 `#include "DAPybind11InQt.h"` 作为第一个 pybind11 相关 include。

## 类型映射总表

| Qt 类型 | Python 类型 | 方向 | 特殊说明 |
|---------|-------------|:--:|----------|
| `QString` | `str` | ↔ | UTF-8 编码自动转换，也接受 `bytes` |
| `QByteArray` | `bytes` / `bytearray` | ↔ | 支持 `None` → 空 QByteArray；也接受 `str`/`list[int]` |
| `QDate` | `datetime.date` | ↔ | 无效日期 → `None` |
| `QTime` | `datetime.time` | ↔ | 微秒 → 毫秒精度转换 |
| `QDateTime` | `datetime.datetime` | ↔ | 还支持 `pandas.Timestamp`、`numpy.datetime64`，含时区处理 |
| `QList<T>` | `list` | ↔ | `T` 需已有 `type_caster` |
| `QVector<T>` | `list` | ↔ | 仅 Qt5；Qt6 中 `QVector` 即 `QList` |
| `QSet<T>` | `set` | ↔ | 也接受 Python `list` 输入 |
| `QHash<K,V>` | `dict` | ↔ | 哈希映射 |
| `QMap<K,V>` | `dict` | ↔ | 有序映射 |
| `QVariant` | `Any` | ↔ | **万能桥接器**：支持 int/float/bool/str/bytes/list/dict/set/date/time/datetime/numpy |

## 自动转换原理

pybind11 的 `type_caster<T>` 模板特化包含两个核心方法：

```
load(handle src, bool convert) → bool     // Python → C++ 方向
cast(const T& src, ...) → handle           // C++ → Python 方向
```

一旦定义了 `type_caster<T>`，pybind11 会在以下场景自动使用它：

- **函数参数绑定**：Python `str` 自动转为 `QString` 参数
- **函数返回值**：C++ `QList<int>` 返回值自动转为 Python `list`
- **`.cast<T>()` 调用**：`py::object.cast<QString>()` 自动使用 caster

```cpp
// 场景 1：绑定 C++ 方法 — 字符串参数自动转换
py::class_<DA::SomeClass>(m, "SomeClass")
    .def("setName", &DA::SomeClass::setName);  // Python str → QString 自动
// Python: obj.setName("hello")

// 场景 2：绑定 C++ 方法 — 容器返回值自动转换
py::class_<DA::SomeClass>(m, "SomeClass")
    .def("getValues", &DA::SomeClass::getValues);  // QList<int> → Python list 自动
// Python: result = obj.getValues()  # 返回 [1, 2, 3]

// 场景 3：手动调用 cast
py::object py_obj = ...;
QString str = py_obj.cast<QString>();  // 自动使用 type_caster<QString>::load()
QVariant var = py_obj.cast<QVariant>(); // 自动使用 type_caster<QVariant>::load()
```

## 各类型转换详解

### QString ↔ Python str

```cpp
// === Python → QString ===
// 方式 A（自动转换）：pybind11 绑定的函数参数直接用 QString 类型
py::class_<Foo>(m, "Foo")
    .def("setName", &Foo::setName);  // Python: foo.setName("你好")

// 方式 B（手动调用）：
QString qstr = py_obj.cast<QString>();
QString qstr = DA::PY::fromPyString(py_obj);

// === QString → Python ===
// 方式 A（自动转换）：返回值直接声明为 QString
py::class_<Foo>(m, "Foo")
    .def("getName", &Foo::getName);  // 返回值 QString 自动转 str

// 方式 B（手动调用）：
py::object py_obj = py::cast(qstr);
py::object py_obj = DA::PY::toPyObject(qstr);
```

**注意事项**：
- 自动转字符串时，允许 `bytes` → `QString` 转换（使用 UTF-8 解码）
- 不支持 `list`/`dict` 等容器类型自动转为字符串，必须显式转换

### QByteArray ↔ Python bytes

```cpp
// === Python → QByteArray ===
QByteArray ba = py_obj.cast<QByteArray>();

// === QByteArray → Python ===
py::object py_bytes = py::cast(ba);
```

**支持的 Python 输入类型**：
- `None` → 空 QByteArray
- `bytes` / `bytearray` → QByteArray
- `str` → QByteArray (UTF-8 编码)
- `list[int]` → QByteArray（每个元素 0-255）

### QDateTime ↔ Python datetime / pandas.Timestamp / numpy.datetime64

`QDateTime` 是项目中覆盖最全面的类型转换器。它不仅支持 Python 标准库的 `datetime.datetime`，还专门适配了 `pandas.Timestamp` 和 `numpy.datetime64`。

```cpp
// === Python → QDateTime ===
QDateTime dt = py_obj.cast<QDateTime>();
// 自动识别 datetime.datetime / pandas.Timestamp / numpy.datetime64

// === QDateTime → Python ===
py::object py_dt = py::cast(dt);  // 返回 datetime.datetime

// === 辅助函数 ===
QDateTime dt = DA::PY::fromPyDateTime(py_obj);
py::object py_obj = DA::PY::toPyObject(dt);
```

**QDateTime cast（C++ → Python）的关键行为**：
- 使用"墙上时间"戳调用 `datetime.fromtimestamp(ts)`，**不调用 `toUTC()`**
- 生成的 Python `datetime` 对象直接反映本地时间
- 无效的 QDateTime → `None`

**QDateTime load（Python → C++）的时区处理**：
- 如果源 Python 对象有时区信息（`tzinfo` 不为 `None`）→ 使用 `Qt::UTC`
- 如果源 Python 对象无时区信息 → 使用本地时间构造

**pandas.Timestamp 处理**：
- `pandas.NaT` 会被 `canCastToQDateTime()` 显式排除（返回 `false`）
- `Timestamp.tz` 是否为 `None` 决定时区策略
- 无时区的 Timestamp 先通过 `to_pydatetime()` 转换为 Python datetime 再处理

**numpy.datetime64 处理**：
- 先通过 `astype("int64")` 获取纳秒时间戳
- 转换为毫秒后使用 `QDateTime::fromMSecsSinceEpoch`

### 日期类型检测辅助函数

```cpp
// 检测 pybind11::handle 能否转换为 QDateTime
bool canCastToQDateTime(pybind11::handle src);
// 支持: datetime / pandas.Timestamp / numpy.datetime64
// 排除: pandas.NaT

bool canCastToQDate(pybind11::handle src);
// 仅支持: datetime.date

bool canCastToQTime(pybind11::handle src);
// 仅支持: datetime.time

bool canCastToQString(pybind11::handle src);
// 支持: str (PyUnicode) / bytes
```

### 容器类型：QList<T>、QVector<T>、QSet<T>

**通用规则**：容器的 `type_caster` 依赖元素类型 `T` 已有自己的 `type_caster`。如果 `T` 是基本类型（`int`, `QString`, `double` 等），可直接转换；如果 `T` 是自定义绑定类型，可能需要额外处理。

```cpp
// === Python list → QList ===
QList<int> ints = py_obj.cast<QList<int>>();            // ✅ 自动
QList<QString> strs = py_obj.cast<QList<QString>>();    // ✅ 自动
QList<QString> strs = DA::PY::fromPyList<QString>(py_obj);

// === QList → Python list ===
py::object py_list = py::cast(qlist);                    // ✅ 自动
py::object py_list = DA::PY::toPyObject(qlist);

// === QVector<T>（仅 Qt5）===
// QVector 的行为与 QList 完全一致，区别仅在于 C++ 类型
QVector<double> vec = py_obj.cast<QVector<double>>();

// === QSet<T> ===
// 接受 Python set 或 list 作为输入
QSet<int> s = py_obj.cast<QSet<int>>();
// 也支持从 list 创建：DA::PY::fromPyListToSet<QString>(py_obj)
```

**`QList<自定义绑定类型>` 的注意事项**：

当 `QList<T>` 中 `T` 是自定义绑定类型（如 `QList<DAData>`）时，自动 caster 可能将元素转为 pybind11 内部类型而非正确的绑定类型。此时推荐手动遍历：

```cpp
// Lambda 手动包装，确保每个元素以正确的绑定类型返回
.def("getAllDatas",
    [](DA::DADataManagerInterface& self) {
        QList<DA::DAData> datas = self.getAllDatas();
        pybind11::list pyList;
        for (const DA::DAData& data : datas) {
            pyList.append(data);  // 每个 DAData 以绑定类型加入
        }
        return pyList;
    })
```

### 映射类型：QHash<K,V> 和 QMap<K,V>

```cpp
// === Python dict → QHash / QMap ===
QHash<int, QString> hash = py_obj.cast<QHash<int, QString>>();
QMap<QString, double> map = py_obj.cast<QMap<QString, double>>();

// 辅助函数
QHash<K,V> h = DA::PY::fromPyDict<K,V>(py_dict);
QMap<K,V> m  = DA::PY::fromPyMap<K,V>(py_dict);

// === QHash / QMap → Python dict ===
py::object py_dict = py::cast(hash);
py::object py_dict = py::cast(map);
py::object py_dict = DA::PY::toPyObject(hash);
```

### QVariant — 通用类型桥接器

`QVariant` 是项目中**最重要的转换器**，作为 Qt 和 Python 类型系统之间的万能桥接器。它处理几乎所有 Python 类型的转换，并在 C++ → Python 方向保留了完整的元类型信息。

```cpp
// === Python → QVariant（自动类型推断）===
QVariant var = py_obj.cast<QVariant>();
QVariant var = DA::PY::fromPyVariant(py_obj);

// === QVariant → Python（保留元类型）===
py::object py_obj = py::cast(var);
py::object py_obj = DA::PY::toPyObject(var);
```

#### Python → QVariant 的自动类型检测顺序

`load()` 方法按以下优先级检测类型：

| 优先级 | Python 类型 | QVariant 内部类型 |
|--------|-----------|------------------|
| 1 | `None` | `QVariant()` (无效) |
| 2 | numpy `ndarray` / `generic` | 标量 → 对应类型；数组 → `QVariantList` |
| 3 | `int` (`PyLong_Check`) | `int` → `qlonglong` |
| 4 | `float` (`PyFloat_Check`) | `double` → `float` |
| 5 | `bool` (`PyBool_Check`) | `bool` |
| 6 | `str` (`PyUnicode_Check`) | `QString` |
| 7 | `bytes` / `bytearray` | `QByteArray` |
| 8 | `list` / `tuple` | `QVariantList` |
| 9 | `dict` | `QVariantMap` → `QVariantHash` |
| 10 | `set` | `QVariantList`（元素递归转换） |
| 11 | `datetime.datetime` | `QDateTime` |
| 12 | `datetime.date` | `QDate` |
| 13 | `datetime.time` | `QTime` |
| 14 | 其他（`convert=True`） | 降级为 `QString` |

#### numpy 数组的 QVariant 处理

当 PyObject 是 numpy 数组时，`handle_numpy_object()` 提供完整的 dtype 映射：

| numpy dtype | `dtype.char` | QVariant 内部类型 |
|-------------|-------------|------------------|
| `bool_` | `'?'` | `bool` |
| `int8` | `'b'` | `QChar` |
| `int16`/`int32` | `'h'/'l'` | `int` |
| `int64` | `'q'` | `long long` |
| `uint8` | `'B'` | `QChar` |
| `uint16`/`uint32` | `'H'/'L'` | `unsigned int` |
| `uint64` | `'Q'` | `unsigned long long` |
| `float16`/`float32` | `'e'/'f'` | `float` |
| `float64` | `'d'` | `double` |
| `complex64`/`complex128` | `'F'/'D'` | `QString` |
| `str_` | `'U'` | `QString` |
| `bytes_` | `'S'` | `QByteArray` |
| `datetime64` | `'M'` | `QDateTime` |
| `timedelta64` | `'m'` | `QString` |
| `void`（记录数组）| `'V'` | `QByteArray` |
| `object_` | `'O'` | 递归 QVariant 转换 |

**多维 numpy 数组**通过 `tolist()` 转为 Python list，然后递归转为 `QVariantList`。

#### QVariant → Python 的 QMetaType 路由

`cast()` 方法根据 `src.userType()` 精确还原 Python 类型：

- `QMetaType::Int/Double/Bool` → Python 对应数值类型
- `QMetaType::QString/QByteArray/QChar` → Python str/bytes/str
- `QMetaType::QDate/QTime/QDateTime` → Python datetime 类型
- `QMetaType::QVariantList` → Python list（元素递归转换）
- `QMetaType::QVariantMap/QVariantHash` → Python dict
- 其他已知命名类型（`QList<T>`、`QMap<K,V>` 等）→ 通过 `typeName()` 检测
- 兜底 → `toString()` → Python str

## DA::PY 命名空间中的辅助函数

除了自动 type_caster，`DAPybind11QtCaster.hpp` 在 `DA::PY` 命名空间中提供了大量便捷函数，适用于需要手动控制转换的场景。

### 标量类型转换

| 函数签名 | 方向 | 说明 |
|---------|:--:|------|
| `fromPyString(handle)` → `QString` | Py→Qt | Python str/bytes → QString |
| `toPyObject(const QString&)` → `object` | Qt→Py | QString → Python str |
| `fromPyDate(handle)` → `QDate` | Py→Qt | Python date → QDate |
| `toPyObject(const QDate&)` → `object` | Qt→Py | QDate → Python date |
| `fromPyTime(handle)` → `QTime` | Py→Qt | Python time → QTime |
| `toPyObject(const QTime&)` → `object` | Qt→Py | QTime → Python time |
| `fromPyDateTime(handle)` → `QDateTime` | Py→Qt | Python datetime → QDateTime |
| `toPyObject(const QDateTime&)` → `object` | Qt→Py | QDateTime → Python datetime |
| `fromPyVariant(handle)` → `QVariant` | Py→Qt | Python any → QVariant |
| `toPyObject(const QVariant&)` → `object` | Qt→Py | QVariant → Python any |
| `toPyObject(const QVariant&, const dtype&)` → `object` | Qt→Py | QVariant → numpy typed |

### 容器类型转换

| 函数签名 | 方向 | 说明 |
|---------|:--:|------|
| `fromPyList<T>(handle)` → `QList<T>` | Py→Qt | Python list → QList |
| `toPyObject(const QList<T>&)` → `object` | Qt→Py | QList → Python list |
| `fromPyVector<T>(handle)` → `QVector<T>` | Py→Qt | Python list → QVector |
| `fromPySet<T>(handle)` → `QSet<T>` | Py→Qt | Python set → QSet |
| `toPyObject(const QSet<T>&)` → `object` | Qt→Py | QSet → Python set |
| `fromPyListToSet<T>(handle)` → `QSet<T>` | Py→Qt | Python list → QSet (去重) |
| `fromPyDict<K,V>(handle)` → `QHash<K,V>` | Py→Qt | Python dict → QHash |
| `toPyObject(const QHash<K,V>&)` → `object` | Qt→Py | QHash → Python dict |
| `fromPyMap<K,V>(handle)` → `QMap<K,V>` | Py→Qt | Python dict → QMap |
| `toPyObject(const QMap<K,V>&)` → `object` | Qt→Py | QMap → Python dict |

### 类型检测函数

```cpp
// 检测 Python 对象是否能安全转换为特定 Qt 类型
bool DA::PY::canCastToQDateTime(pybind11::handle src);
bool DA::PY::canCastToQDate(pybind11::handle src);
bool DA::PY::canCastToQTime(pybind11::handle src);
bool DA::PY::canCastToQString(pybind11::handle src);
```

## safe_pyobject — 安全 Python 对象包装器

`DA::PY::safe_pyobject` 是一个 RAII 风格的 PyObject* 持有者，用于在 C++ 代码中安全地持有 Python 对象，避免 GIL 未初始化时的崩溃。

```cpp
class safe_pyobject {
public:
    // 默认构造 — 持有 nullptr
    safe_pyobject();

    // 移动构造：接管 pybind11::object 的所有权（release + dec_ref 责任转移）
    safe_pyobject(pybind11::object&& obj);

    // 禁止拷贝（引用计数由移动语义管理）
    safe_pyobject(const safe_pyobject&) = delete;
    safe_pyobject& operator=(const safe_pyobject&) = delete;

    // 移动构造/赋值
    safe_pyobject(safe_pyobject&& other) noexcept;
    safe_pyobject& operator=(safe_pyobject&& other) noexcept;

    // 析构：如果 Py_IsInitialized()，调用 Py_DECREF
    ~safe_pyobject();

    // 检查是否为 nullptr 或 Py_None
    bool is_none() const;

    // 返回 pybind11::handle（不增加引用计数）
    pybind11::handle get() const;

    // 隐式 bool：非空且非 None 为 true
    operator bool() const;

    // 返回 pybind11::object（borrow 引用）
    pybind11::object object() const;
};
```

**典型用途**：作为静态变量的延迟初始化容器，安全存储 Python 类型对象（如 `datetime.datetime`、`pandas.Timestamp` 等）。

```cpp
// 模式 1：在 type_caster 中缓存 Python 类型对象
static DA::PY::safe_pyobject& get_datetime_type()
{
    static DA::PY::safe_pyobject datetime_type =
        DA::PY::import_type_safe("datetime", "datetime");
    return datetime_type;
}

// 模式 2：安全导入 Python 模块/类型
DA::PY::safe_pyobject np_type = DA::PY::import_type_safe("numpy", "ndarray");
if (np_type && pybind11::isinstance(src, np_type.get())) {
    // 处理 numpy 数组
}
```

**`import_type_safe()` 辅助函数**：

```cpp
// 安全导入 Python 模块中的指定类型
// 如果 Python 未初始化或导入失败，返回空的 safe_pyobject
inline safe_pyobject import_type_safe(const char* module_name, const char* type_name);
```

## 使用场景与最佳实践

### 场景 1：绑定 C++ 方法时直接声明 Qt 类型

最简单的用法 — 只要 `DAPybind11InQt.h` 已包含，pybind11 自动处理类型转换：

```cpp
#include "DAPybind11InQt.h"

PYBIND11_EMBEDDED_MODULE(my_module, m)
{
    py::class_<MyWidget>(m, "MyWidget")
        // ✅ QString 参数自动从 Python str 转换
        .def("setTitle", &MyWidget::setTitle)
        // ✅ QList<int> 返回值自动转为 Python list
        .def("getValues", &MyWidget::getValues)
        // ✅ QDateTime 参数自动从 Python datetime 转换
        .def("setTimestamp", &MyWidget::setTimestamp);
}
```

### 场景 2：Lambda 包装需要默认参数

当方法需要默认参数值时，用 Lambda 显式包装：

```cpp
// ❌ 直接绑定 — caster 不支持参数默认值
.def("addInfoLogMessage", &DAUIInterface::addInfoLogMessage)
// 这样绑定后 Python 调用必须提供所有参数

// ✅ Lambda 包装 — 提供默认参数
.def("addInfoLogMessage",
    [](DA::DAUIInterface& self, const std::string& msg, bool showInStatusBar) {
        self.addInfoLogMessage(
            QString::fromStdString(msg), showInStatusBar);
    },
    py::arg("msg"),
    py::arg("showInStatusBar") = true);
// Python: ui.addInfoLogMessage("hello")  # showInStatusBar 默认为 true
```

### 场景 3：手动转换无法自动处理的对象

当 pybind11 无法自动推断转换时，手动使用 `DA::PY::` 函数：

```cpp
.def("processData",
    [](MyClass& self, py::object py_data) {
        // 手动检测类型并转换
        if (DA::PY::canCastToQDateTime(py_data)) {
            QDateTime dt = DA::PY::fromPyDateTime(py_data);
            self.processDateTime(dt);
        } else if (DA::PY::canCastToQString(py_data)) {
            QString str = DA::PY::fromPyString(py_data);
            self.processString(str);
        } else {
            QVariant var = DA::PY::fromPyVariant(py_data);
            self.processVariant(var);
        }
    });
```

### 场景 4：自定义 C++ 函数中进行 Qt ↔ Python 转换

在任何需要 C++ 代码处理 Python 对象的场景：

```cpp
#include "DAPybind11InQt.h"

void handlePythonResult(py::object result) {
    // 方式 A：使用 py::cast（自动利用 type_caster）
    QString str = py::cast<QString>(result);
    QList<int> list = py::cast<QList<int>>(result);

    // 方式 B：使用 DA::PY:: 便捷函数
    QString str = DA::PY::fromPyString(result);
    QVariant var = DA::PY::fromPyVariant(result);
}

py::object createPythonString(const QString& str) {
    // 方式 A：
    return py::cast(str);

    // 方式 B：
    return DA::PY::toPyObject(str);
}
```

### 何时使用自动转换 vs Lambda 包装

| 场景 | 推荐方式 | 原因 |
|------|---------|------|
| 方法参数/返回值可自动匹配类型 | 直接绑定 (`&Class::method`) | 代码简洁，无额外开销 |
| 需要参数默认值 | Lambda 包装 | caster 不支持参数默认值 |
| `QList<自定义绑定类型>` 返回值 | Lambda 手动遍历 | 确保元素为正确绑定类型 |
| `QJsonObject` / `QRegularExpression` | Lambda + 辅助函数 | caster 未覆盖这些类型 |
| 需要参数验证或额外逻辑 | Lambda 包装 | 可在 Lambda 中添加检查/日志 |
| 性能敏感的批量转换 | 直接绑定 | Lambda 有间接调用开销 |

## 常见陷阱

### 陷阱 1：忘记 include DAPybind11InQt.h

```
症状：编译错误，slots 相关语法错误
原因：直接 #include <pybind11/pybind11.h> 导致 Qt slots 宏冲突
```

```cpp
// ❌ 错误
#include <pybind11/pybind11.h>

// ✅ 正确
#include "DAPybind11InQt.h"
```

### 陷阱 2：QVector<T> 仅在 Qt5 下可用

```cpp
// ❌ 在 Qt6 下编译失败 — QVector 的 type_caster 在 Qt5 条件宏内
QVector<int> vec = py_obj.cast<QVector<int>>();

// ✅ 使用 QList 代替（Qt6 中 QVector ≡ QList）
QList<int> list = py_obj.cast<QList<int>>();
```

### 陷阱 3：QDateTime 时区混淆

C++ `QDateTime` 转换为 Python `datetime` 时，使用"墙上时间"戳调用 `datetime.fromtimestamp(ts)`。这会导致：

- 如果源 QDateTime 是 UTC 时间，Python 端会显示为本地时间
- 如果源 QDateTime 是本地时间，Python 端时间值不变

如果需要精确的 UTC 时间传递，建议使用时间戳（毫秒）而非 QDateTime 对象。

### 陷阱 4：pandas.NaT 与 canCastToQDateTime

```cpp
// NaT 是 pandas.Timestamp 的子类，isinstance(src, pd_Timestamp) 返回 true
// 但 NaT 无法转换为 QDateTime
// canCastToQDateTime() 已专门排除 NaT
if (DA::PY::canCastToQDateTime(py_obj)) {
    // 安全 — NaT 已排除
    QDateTime dt = DA::PY::fromPyDateTime(py_obj);
}
```

### 陷阱 5：QVariant 的 int 类型可能丢失精度

```cpp
// Python int 可能超出 C++ int 范围
// QVariant caster 先尝试 int，失败后尝试 long long / unsigned long long
// 极端大的 Python int 会被降级为 QString
```

## 相关文档

- [Python/C++ 集成总览](./python-in-cpp.md) — 导航入口
- [Python 绑定开发](./python-binding/python-binding-development.md) — 完整的绑定开发流程
- [故障排除与最佳实践](./python-binding/troubleshooting-and-best-practices.md) — 常见错误与调试技巧
- [嵌入式 Python 调试](./embedded-python-debugging.md) — 调试 pybind11 模块
- [模块依赖关系](./module-dependency.md) — DAPyBindQt 模块的定位与职责
