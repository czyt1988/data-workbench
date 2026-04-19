# Python 3.10+ 升级评估分析报告

> 文档编号：00-python310-upgrade-analysis  
> 创建日期：2026-04-19  
> 目标：评估从 Python 3.7+ 升级到 Python 3.10+ 的完整影响范围

---

## 1. 当前 Python 版本要求分析

### 1.1 版本要求现状

**当前声明最低版本**：Python 3.7+（从代码兼容性推断，无显式最低版本声明）

| 文件 | 位置 | 当前版本要求 | 说明 |
|------|------|-------------|------|
| `CMakeLists.txt`（根目录） | 第271行 | 无显式版本约束 | `find_package(Python3 COMPONENTS Interpreter Development REQUIRED)` 未指定版本 |
| `cmake/daworkbench_3rdparty.cmake` | 第171行 | 无显式版本约束 | `damacro_import_Python` 宏同样未指定版本 |
| `src/DAPyBindQt/DAPyInterpreter.cpp` | 第146行 | `PY_VERSION_HEX >= 0x030B0000` | Python 3.11+ 时 `Py_SetPythonHome` 已弃用 |
| `src/DAPyBindQt/DAPyInterpreter.cpp` | 第198行 | `PY_VERSION_HEX >= 0x03080000` | Python 3.8+ 时使用 PyConfig API |
| `src/PyScripts/DAWorkbench/dataframe.py` | 第6行 | `sys.version_info >= (3, 8)` | Literal 类型仅在 3.8+ 可用 |
| `src/tst/DADataFrameTest/main.cpp` | 第12行 | `Py_SetPythonHome(L"C:\Python37")` | 硬编码 Python 3.7 路径 |
| `requirements.txt` | 第9行 | `typing_extensions` | Python 3.7 兼容性依赖 |

### 1.2 所有引用 Python 版本的文件完整清单

**CMake 配置文件（8处 find_package）**：

| 文件路径 | 行号 | 内容 |
|---------|------|------|
| `CMakeLists.txt` | 271 | `find_package(Python3 COMPONENTS Interpreter Development REQUIRED)` |
| `cmake/daworkbench_3rdparty.cmake` | 171 | `find_package(Python3 COMPONENTS Interpreter Development REQUIRED)`（在 `damacro_import_Python` 宏中） |
| `src/DAPyBindQt/CMakeLists.txt` | 57 | `damacro_import_Python(${DA_LIB_NAME})` |
| `src/DAPyScripts/CMakeLists.txt` | 48 | `damacro_import_Python(${DA_LIB_NAME})` |
| `src/DAPyCommonWidgets/CMakeLists.txt` | 45 | `damacro_import_Python(${DA_LIB_NAME})` |
| `src/DAData/CMakeLists.txt` | 73 | `damacro_import_Python(${DA_LIB_NAME})` |
| `src/DAInterface/CMakeLists.txt` | 90 | `damacro_import_Python(${DA_LIB_NAME})` |
| `src/DAGui/CMakeLists.txt` | 350 | `damacro_import_Python(${DA_LIB_NAME})` |
| `src/APP/CMakeLists.txt` | 111 | `damacro_import_Python(${DA_APP_NAME})` |
| `plugins/DataAnalysis/CMakeLists.txt` | 128 | `damacro_import_Python(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})` |

**Python DLL 复制逻辑（1处）**：

| 文件路径 | 行号 | 内容 |
|---------|------|------|
| `CMakeLists.txt` | 274 | `set(DA_PYTHON_DLL_PATH ${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll)` |

**C++源码中版本条件编译（1处，2个分支）**：

| 文件路径 | 行号 | 条件 | 内容 |
|---------|------|------|------|
| `src/DAPyBindQt/DAPyInterpreter.cpp` | 146 | `PY_VERSION_HEX >= 0x030B0000` | Python 3.11+: `Py_SetPythonHome` 已弃用，仅打印警告 |
| `src/DAPyBindQt/DAPyInterpreter.cpp` | 198 | `PY_VERSION_HEX >= 0x03080000` | Python 3.8+: 使用 `PyConfig` API 初始化解释器 |

**Python脚本中版本兼容代码（1处）**：

| 文件路径 | 行号 | 内容 |
|---------|------|------|
| `src/PyScripts/DAWorkbench/dataframe.py` | 6-9 | `if sys.version_info >= (3, 8): from typing import Literal; else: from typing_extensions import Literal` |

**测试代码中的硬编码路径（1处）**：

| 文件路径 | 行号 | 内容 |
|---------|------|------|
| `src/tst/DADataFrameTest/main.cpp` | 12 | `Py_SetPythonHome(L"C:\Python37")` — 硬编码Python 3.7路径 |

---

## 2. CMake 配置变更清单

### 2.1 需要修改的 CMakeLists.txt 文件

#### 2.1.1 根目录 CMakeLists.txt（必须修改）

**文件**: `CMakeLists.txt`

**变更1: 添加 Python 最低版本约束**（第271行附近）

```cmake
# 当前代码（第271行）:
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# 建议修改为:
find_package(Python3 3.10 COMPONENTS Interpreter Development REQUIRED)
```

**变更2: 添加 Python 最低版本约束**（第271行，`DA_ENABLE_AUTO_INSTALL_PYTHON_ENV` 分支内）

需要同时在此处添加版本约束，确保 Windows DLL 复制逻辑也受版本控制。

**变更3: README.md 中 Python 版本描述**（可选，建议更新）

README.md 当前描述为 "Python 3.7+"，需要改为 "Python 3.10+"。

#### 2.1.2 cmake/daworkbench_3rdparty.cmake（必须修改）

**文件**: `cmake/daworkbench_3rdparty.cmake`

**变更1: `damacro_import_Python` 宏添加版本约束**（第171行）

```cmake
# 当前代码（第171行）:
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# 建议修改为:
find_package(Python3 3.10 COMPONENTS Interpreter Development REQUIRED)
```

此宏被 8 个 CMakeLists.txt 文件调用（见上表），修改一处即可影响全局。

#### 2.1.3 其他 CMakeLists.txt 文件

**不需要单独修改**。所有 Python 查找都通过 `damacro_import_Python` 宏统一调用，修改宏即可覆盖所有模块。

但需要注意以下文件的 `find_package` 不通过宏调用，需要单独处理：

| 文件 | 说明 |
|------|------|
| `plugins/DataAnalysis/CMakeLists.txt`（第128行） | 使用 `damacro_import_Python`，已覆盖 |
| `plugins/CMakeLists.txt`（第65行） | 显式设置 `DA_ENABLE_PYTHON=ON`，无 Python 版本查找，不需要修改 |

### 2.2 版本约束策略

**推荐方案**: 在 `damacro_import_Python` 宏中添加 `3.10` 版本约束，这是最简洁的方案，一处修改覆盖全局。

**备选方案**: 在根 `CMakeLists.txt` 的 `DA_ENABLE_AUTO_INSTALL_PYTHON_ENV` 分支中也显式指定版本，确保 DLL 复制分支也受版本控制。

---

## 3. 源代码兼容性分析

### 3.1 DAPyInterpreter.cpp — Python 初始化代码兼容性

**文件**: `src/DAPyBindQt/DAPyInterpreter.cpp`

#### 3.1.1 `setPythonHomePath` 方法（第141-158行）

**当前代码**:
```cpp
void DAPyInterpreter::setPythonHomePath(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }
#if PY_VERSION_HEX >= 0x030B0000
    Q_UNUSED(path);
    qWarning() << "Py_SetPythonHome is deprecated in Python 3.11+, use initializePythonInterpreter with pythonHomePath parameter instead";
#else
    std::vector< wchar_t > wp((path.size() + 1) * 4, 0);
    path.toWCharArray(wp.data());
    try {
        Py_SetPythonHome(wp.data());
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
#endif
}
```

**升级影响**:
- Python 3.10 下：`PY_VERSION_HEX = 0x030A0000`，不满足 `>= 0x030B0000`，仍然走 `#else` 分支，调用 `Py_SetPythonHome`
- **`Py_SetPythonHome` 在 Python 3.10 中仍然可用**，但此函数在 Python 3.13 中已彻底移除
- **建议**: 修改条件为 `PY_VERSION_HEX >= 0x030A0000`（即 Python 3.10+），因为 PyConfig API 在 3.8+ 就已可用，建议所有 3.10+ 版本都使用 PyConfig 而非旧 API
- **或者**: 保持当前 3.11+ 的条件判断，Python 3.10 仍然可以使用 `Py_SetPythonHome`，但要注意 3.13+ 会彻底移除

#### 3.1.2 `initializePythonInterpreter(const QString& pythonHomePath)` 方法（第194-226行）

**当前代码**:
```cpp
void DAPyInterpreter::initializePythonInterpreter(const QString& pythonHomePath)
{
    try {
        std::shared_ptr< pybind11::scoped_interpreter > pythonInterpreter = nullptr;
#if PY_VERSION_HEX >= 0x03080000
        if (!pythonHomePath.isEmpty()) {
            PyConfig config;
            PyConfig_InitPythonConfig(&config);
            config.parse_argv = 0;
            std::vector< wchar_t > wp((pythonHomePath.size() + 1) * 4, 0);
            pythonHomePath.toWCharArray(wp.data());
            PyStatus status = PyConfig_SetString(&config, &config.home, wp.data());
            ...
            pythonInterpreter = std::make_shared< pybind11::scoped_interpreter >(&config);
        } else {
            pythonInterpreter = std::make_shared< pybind11::scoped_interpreter >();
        }
#else
        if (!pythonHomePath.isEmpty()) {
            setPythonHomePath(pythonHomePath);
        }
        pythonInterpreter = std::make_shared< pybind11::scoped_interpreter >();
#endif
        initializePythonInterpreter(pythonInterpreter);
    } catch (const std::exception& e) {
        qWarning() << e.what();
    }
}
```

**升级影响**:
- Python 3.10 下：`PY_VERSION_HEX = 0x030A0000`，满足 `>= 0x03080000`，走 PyConfig 分支
- **`pybind11::scoped_interpreter` 接受 `PyConfig*` 参数** — 这是 pybind11 3.x 新增的功能，当前项目使用的 pybind11 版本为 **3.0.3**，已支持此 API
- **需要注意**: `PyConfig_InitPythonConfig` 在 Python 3.12 中有变化（改为 `PyConfig_InitIsolatedConfig` 或 `PyConfig_InitPythonConfig`），但 3.10 下无变化
- **结论**: 此代码在 Python 3.10 下兼容，无需修改

#### 3.1.3 `initializePythonInterpreter(std::shared_ptr<pybind11::scoped_interpreter> interp)` 方法（第166-174行）

```cpp
void DAPyInterpreter::initializePythonInterpreter(std::shared_ptr< pybind11::scoped_interpreter > interp)
{
    qDebug() << "Python DLL version from header:" << PY_VERSION;
    qDebug() << "Python hex version:" << PY_VERSION_HEX;
    qDebug() << "Python runtime version:" << Py_GetVersion();
    qDebug() << "Python path:" << Py_GetPath();
    qDebug() << "Compiled against:" << PY_MAJOR_VERSION << "." << PY_MINOR_VERSION << "." << PY_MICRO_VERSION;
    interpreter = interp;
}
```

**升级影响**: 纯诊断代码，使用 Python C API 宏和函数，Python 3.10 完全兼容，无需修改。

### 3.2 DAPybind11InQt.h — slots 宏冲突处理

**文件**: `src/DAPyBindQt/DAPybind11InQt.h`

```cpp
#undef slots
#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#endif
#include "pybind11/pybind11.h"
...
#define slots Q_SLOTS
```

**升级影响**: 此文件处理 Qt `slots` 宏与 Python 头文件的冲突，是项目特有的兼容性方案。Python 3.10 不影响此逻辑，无需修改。

### 3.3 PYBIND11_EMBEDDED_MODULE — Python 嵌入模块

**文件清单**:
- `src/APP/PythonBinding/DAAppPythonBinding.cpp`（第25行）: `PYBIND11_EMBEDDED_MODULE(da_app, m)`
- `src/DAData/DADataPythonBinding.cpp`: `PYBIND11_EMBEDDED_MODULE(da_data, m)`
- `src/DAInterface/DAInterfacePythonBinding.cpp`: `PYBIND11_EMBEDDED_MODULE(da_interface, m)`

**升级影响**: `PYBIND11_EMBEDDED_MODULE` 是 pybind11 的核心宏，pybind11 3.0.3 完全兼容 Python 3.10，无需修改。

### 3.4 pybind11 版本兼容性

**当前 pybind11 版本**: 3.0.3（从 `src/3rdparty/pybind11/include/pybind11/detail/common.h` 第20-30行确认）

**pybind11 3.0.3 的 Python 支持范围**: Python 3.7+ 到 Python 3.13+

**结论**: 当前 pybind11 版本完全兼容 Python 3.10，无需升级 pybind11。

**但需注意**: 如果未来升级到 Python 3.13+，需要注意 `Py_SetPythonHome` 的移除和 `PyConfig` API 的变化。pybind11 3.x 已处理这些兼容性问题。

### 3.5 Python C API 变化（3.7 → 3.10）

| API | Python 3.7 | Python 3.10 | 影响 |
|-----|-----------|-------------|------|
| `Py_SetPythonHome` | 可用 | 可用（3.13移除） | 当前代码兼容 |
| `PyConfig API` | 不可用（3.8+） | 可用 | 当前代码已处理3.8+分支 |
| `PY_SSIZE_T_CLEAN` | 推荐 | 必须 | 已定义 |
| `Py_GetVersion` | 可用 | 可用 | 无变化 |
| `Py_GetPath` | 可用 | 可用 | 无变化 |
| `scoped_interpreter` | pybind11提供 | pybind11提供 | pybind11 3.0.3支持 |

**结论**: 当前C++代码对Python 3.10完全兼容，仅需调整条件编译阈值（可选）。

---

## 4. requirements.txt 变更分析

### 4.1 当前依赖清单

| 依赖 | 当前版本约束 | 3.10兼容性 | 变更建议 |
|------|-------------|-----------|---------|
| numpy | 无版本约束 | ✅ 兼容 | 保持不变 |
| loguru | 无版本约束 | ✅ 兼容 | 保持不变 |
| pandas | 无版本约束 | ✅ 兼容 | 保持不变 |
| scipy | 无版本约束 | ✅ 兼容 | 保持不变 |
| openpyxl | 无版本约束 | ✅ 兼容 | 保持不变 |
| chardet | 无版本约束 | ✅ 兼容 | 保持不变 |
| PyWavelets | 无版本约束 | ✅ 兼容 | 保持不变 |
| pyarrow | 无版本约束 | ✅ 兼容 | 保持不变 |
| typing_extensions | 无版本约束 | ✅ 兼容但可移除 | **建议移除** |
| matplotlib | 无版本约束 | ✅ 兼容 | 保持不变 |
| seaborn | 无版本约束 | ✅ 兼容 | 保持不变 |
| langgraph | 无版本约束 | ✅ 兼容（要求3.10+） | 保持不变 |
| langchain-openai | 无版本约束 | ✅ 兼容 | 保持不变 |
| langgraph-cli[inmem] | 无版本约束 | ✅ 兼容 | 保持不变 |

### 4.2 关键变更分析

#### 4.2.1 typing_extensions — 建议移除

**当前状态**: `requirements.txt` 第9行包含 `typing_extensions`

**原因**: 这是为 Python 3.7 兼容性而添加的，用于在 Python 3.7 下提供 `Literal` 类型支持。Python 3.10 已原生支持 `Literal`、`Union` 使用 `|` 操作符等特性。

**代码引用**: `src/PyScripts/DAWorkbench/dataframe.py` 第6-9行:
```python
if sys.version_info >= (3, 8):
    from typing import Literal
else:
    from typing_extensions import Literal
```

**建议变更**:
1. 从 `requirements.txt` 移除 `typing_extensions`
2. 简化 `dataframe.py` 的导入逻辑，直接使用 `from typing import Literal`
3. 移除 `sys.version_info >= (3, 8)` 的条件判断

#### 4.2.2 langgraph — Python 3.10+ 硬性要求

**重要**: `langgraph` 包实际上要求 Python >= 3.10，这是本次升级的核心驱动力之一。当前 `requirements.txt` 中无版本约束，如果用户在 Python 3.7 环境下运行 `pip install -r requirements.txt`，`langgraph` 安装可能失败或安装不兼容版本。

**建议**: 在 `requirements.txt` 中为 `langgraph` 添加最低版本约束，如:
```
langgraph>=0.2.0
```

#### 4.2.3 pandas._typing — 内部 API 使用

**文件**: `src/PyScripts/DAWorkbench/dataframe.py` 第14行:
```python
from pandas._typing import Axis, Scalar
```

**升级影响**: `pandas._typing` 是 pandas 的内部模块，不同版本的 pandas 可能改变其内容。Python 3.10 本身不影响此导入，但需要注意 pandas 版本升级后的内部 API 变化。

**建议**: 监控 pandas 版本升级后的 `_typing` 变化，必要时改用 `pandas.Axis` 或直接使用字符串类型标注。

---

## 5. DAPyInterpreter 兼性性分析

### 5.1 完整兼容性矩阵

| 方法 | Python 3.7 | Python 3.10 | Python 3.11 | Python 3.13 | 说明 |
|------|-----------|-------------|-------------|-------------|------|
| `setPythonHomePath()` | ✅ 使用`Py_SetPythonHome` | ✅ 使用`Py_SetPythonHome` | ⚠️ 弃用警告 | ❌ 移除 | 需要修改条件阈值 |
| `initializePythonInterpreter()` | ✅ | ✅ | ✅ | ✅ | 无变化 |
| `initializePythonInterpreter(QString)` | ⚠️ 走旧分支 | ✅ 走PyConfig分支 | ✅ 走PyConfig分支 | ✅ 走PyConfig分支 | PyConfig在3.8+可用 |
| `shutdown()` | ✅ | ✅ | ✅ | ✅ | 无变化 |
| `appendSysPath()` | ✅ | ✅ | ✅ | ✅ | 无变化 |
| `wherePython()` | ✅ | ✅ | ✅ | ✅ | 无变化（Windows `where` 命令） |
| `wherePythonFromConfig()` | ✅ | ✅ | ✅ | ✅ | 无变化（JSON配置） |

### 5.2 scoped_interpreter 构造函数兼容性

**Python 3.10 + pybind11 3.0.3**:
- `pybind11::scoped_interpreter()` — 无参数构造 ✅
- `pybind11::scoped_interpreter(PyConfig*)` — PyConfig构造 ✅（pybind11 3.x新增）
- `pybind11::scoped_interpreter(bool init_signal_handlers)` — 在 pybind11 2.x 中的旧构造方式

**结论**: 当前代码使用的两种构造方式在 Python 3.10 + pybind11 3.0.3 下完全兼容。

### 5.3 GIL 管理兼容性

**当前代码中的 GIL 使用**:

| 文件 | 行号 | API | 3.10兼容性 |
|------|------|-----|-----------|
| `DAPyInterpreter.cpp` | 239 | `pybind11::gil_scoped_release` | ✅ |
| `DAPyInterpreter.cpp` | 245 | `pybind11::module::import` | ✅ |
| `DAPyInterpreter.cpp` | 267 | `pybind11::module::import` | ✅ |
| `DAPythonSignalHandler.cpp` | 无直接Python调用 | 通过信号槽间接 | ✅ |

**结论**: GIL 管理代码在 Python 3.10 下完全兼容。

### 5.4 建议的 DAPyInterpreter.cpp 变更

**变更1: 将 `setPythonHomePath` 的条件阈值从 3.11 改为 3.10**（可选但推荐）

```cpp
// 当前代码（第146行）:
#if PY_VERSION_HEX >= 0x030B0000

// 建议修改为:
#if PY_VERSION_HEX >= 0x030A0000
```

**理由**: PyConfig API 在 3.8+ 就已可用，统一在 3.10+ 使用新 API 更安全，避免在 3.13+ 彻底移除 `Py_SetPythonHome` 时出现编译错误。

**变更2: 在 `setPythonHomePath` 中改用 PyConfig API**

```cpp
void DAPyInterpreter::setPythonHomePath(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }
#if PY_VERSION_HEX >= 0x030A0000
    // Python 3.10+: Py_SetPythonHome 已弃用或移除，使用 initializePythonInterpreter(QString) 代替
    Q_UNUSED(path);
    qWarning() << "Py_SetPythonHome is deprecated in Python 3.10+, use initializePythonInterpreter with pythonHomePath parameter instead";
#else
    // Python 3.7-3.9: 使用旧 API
    std::vector< wchar_t > wp((path.size() + 1) * 4, 0);
    path.toWCharArray(wp.data());
    try {
        Py_SetPythonHome(wp.data());
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
#endif
}
```

---

## 6. 升级风险评估

### 6.1 高风险项

| 风险 | 影响范围 | 严重程度 | 缓解方案 |
|------|---------|---------|---------|
| **Windows 7 不再支持** | Python 3.10 不支持 Windows 7 | ⚠️ 中 | 项目README已提及Python 3.7是为Win7兼容，升级3.10意味着放弃Win7支持。需在文档中明确声明 |
| **typing_extensions 移除** | `dataframe.py` 条件导入逻辑 | ⚠️ 中 | 简化导入，移除条件判断 |
| **Py_SetPythonHome 未来移除** | `DAPyInterpreter.cpp` | ⚠️ 中 | 调整条件阈值到 3.10+，统一使用 PyConfig |
| **langgraph 硬性要求 3.10** | requirements.txt | 🔴 高 | 添加版本约束，确保 pip 安装不会在不兼容的Python版本上失败 |

### 6.2 中风险项

| 风险 | 影响范围 | 严重程度 | 缓解方案 |
|------|---------|---------|---------|
| **pandas._typing 内部 API** | dataframe.py | ⚠️ 中低 | 监控 pandas 版本变化，必要时迁移到公共 API |
| **pyarrow 版本兼容** | requirements.txt | ⚠️ 低 | pyarrow 对 Python 3.10 完全兼容 |
| **CMake find_package 版本约束** | 构建系统 | ⚠️ 中 | 添加显式版本约束，防止意外找到低版本Python |

### 6.3 低风险项

| 风险 | 影响范围 | 严重程度 | 缓解方案 |
|------|---------|---------|---------|
| **Python 3.10 新语法特性** | PyScripts 代码 | ⚠️ 低 | `match/case` 语句、`|` 类型联合操作符目前未使用，升级后可选择使用 |
| **DLL名称变化** | CMake DLL复制 | ⚠️ 低 | `python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll` 已使用动态版本号，3.10下为 `python310.dll`，无需修改 |

### 6.4 不受影响的部分

- pybind11 3.0.3 完全兼容 Python 3.10
- `PYBIND11_EMBEDDED_MODULE` 宏无变化
- Qt 与 Python 3.10 无直接耦合
- 所有 numpy/scipy/matplotlib 依赖兼容 3.10
- `DAPythonSignalHandler` 不直接使用 Python C API
- `DAPybind11QtCaster.hpp` 类型转换逻辑与 Python 版本无关

---

## 7. 升级步骤清单

### 步骤 1: CMake 配置变更（优先级：高）

1. **修改 `cmake/daworkbench_3rdparty.cmake`** 第171行：
   - `find_package(Python3 3.10 COMPONENTS Interpreter Development REQUIRED)`
   - 此修改覆盖所有调用 `damacro_import_Python` 的模块

2. **修改根 `CMakeLists.txt`** 第271行（`DA_ENABLE_AUTO_INSTALL_PYTHON_ENV` 分支内）：
   - 同样添加 `3.10` 版本约束
   - 确保 DLL 复制逻辑仅在 Python 3.10+ 环境下执行

### 步骤 2: 源代码变更（优先级：中）

1. **修改 `src/DAPyBindQt/DAPyInterpreter.cpp`**：
   - 第146行：`PY_VERSION_HEX >= 0x030B0000` → `PY_VERSION_HEX >= 0x030A0000`
   - 确保在 Python 3.10+ 下统一使用 PyConfig API

2. **修改 `src/tst/DADataFrameTest/main.cpp`**：
   - 第12行：移除 `Py_SetPythonHome(L"C:\Python37")` 硬编码路径
   - 改为使用 `DAPyInterpreter::initializePythonInterpreter` 或动态查找 Python 路径

### 步骤 3: Python 代码变更（优先级：中）

1. **修改 `src/PyScripts/DAWorkbench/dataframe.py`**：
   - 移除 `sys.version_info >= (3, 8)` 条件判断（第6-9行）
   - 直接使用 `from typing import Literal`
   - 移除对 `typing_extensions` 的依赖

2. **可选**: 将 `Union[X, Y]` 替换为 `X | Y`（Python 3.10+ 语法）
   - `dataframe.py` 第91行: `Union[Scalar, dict, None]` → `Scalar | dict | None`
   - `dataframe.py` 第4行: `from typing import List, Dict,Tuple, Optional, Union` → 简化导入

### 步骤 4: requirements.txt 变更（优先级：高）

1. **移除 `typing_extensions`**（第9行）
2. **为 `langgraph` 添加版本约束**: `langgraph>=0.2.0`
3. **可选**: 为所有依赖添加最低版本约束，确保依赖版本与 Python 3.10 兼容

### 步骤 5: 文档更新（优先级：中）

1. **更新 README.md**: Python 3.7+ → Python 3.10+
2. **更新 AGENTS.md**: 版本要求描述
3. **更新项目文档**: 开发环境说明中的 Python 版本要求

### 步骤 6: 验证与测试（优先级：高）

1. **安装 Python 3.10 环境**
2. **重新配置 CMake**: 使用 Python 3.10 的 Qt 工具链文件重新运行 `cmake -S . -B build`
3. **完整构建**: `cmake --build build --config Release --parallel`
4. **运行测试**: 确认所有 Python 相关功能正常
5. **验证 Python 嵌入**: 确认 `DAPyInterpreter::initializePythonInterpreter` 正常工作
6. **验证 langgraph**: 确认 `langgraph` 在 Python 3.10 下正常安装和运行

### 步骤 7: CI/CD 更新（优先级：中）

1. **更新 GitHub Actions**: 确保 CI 使用 Python 3.10+ 进行构建和测试
2. **更新构建脚本**: 确保 Docker/虚拟环境使用 Python 3.10

---

## 附录 A: 文件变更汇总表

| 文件 | 变更类型 | 变更内容 | 优先级 |
|------|---------|---------|--------|
| `cmake/daworkbench_3rdparty.cmake` | 修改 | `find_package(Python3 3.10 ...)` | 🔴 高 |
| `CMakeLists.txt` | 修改 | `find_package(Python3 3.10 ...)` | 🔴 高 |
| `src/DAPyBindQt/DAPyInterpreter.cpp` | 修改 | 条件阈值 `0x030B0000` → `0x030A0000` | ⚠️ 中 |
| `src/tst/DADataFrameTest/main.cpp` | 修改 | 移除硬编码 Python 3.7 路径 | ⚠️ 中 |
| `src/PyScripts/DAWorkbench/dataframe.py` | 修改 | 移除 `typing_extensions` 条件导入 | ⚠️ 中 |
| `requirements.txt` | 修改 | 移除 `typing_extensions`，添加 `langgraph>=0.2.0` | 🔴 高 |
| `README.md` | 修改 | Python 版本描述 3.7+ → 3.10+ | ⚠️ 低 |
| `AGENTS.md` | 修改 | 开发环境 Python 版本描述 | ⚠️ 低 |

## 附录 B: Python 3.7→3.10 语言特性变化摘要

| 特性 | 3.7 | 3.10 | 项目当前使用情况 |
|------|-----|------|----------------|
| `typing.Literal` | ❌ 需 `typing_extensions` | ✅ 内置 | `dataframe.py` 使用 |
| `X | Y` 类型联合 | ❌ | ✅ | 未使用（使用 `Union`） |
| `match/case` 语句 | ❌ | ✅ | 未使用 |
| `dict | merge` (`|` 操作符) | ❌ | ✅ | 未使用 |
| `dataclass` slots | ❌ | ✅ `slots=True` | 未使用 |
| `ParamSpec` | ❌ | ✅ | 未使用 |
| `TypeAlias` | ❌ | ✅ | 未使用 |
| `zip` 严格模式 | ❌ | ✅ `strict=True` | 未使用 |

**结论**: 项目 Python 代码的升级影响非常小，主要仅需处理 `typing_extensions` / `Literal` 的导入逻辑。

## 附录 C: 全项目 Python 相关 CMake 模块依赖图

```
根 CMakeLists.txt
  ├── cmake/daworkbench_3rdparty.cmake (damacro_import_Python 宏)
  │     ├── src/DAPyBindQt/CMakeLists.txt (调用 damacro_import_Python)
  │     ├── src/DAPyScripts/CMakeLists.txt (调用 damacro_import_Python)
  │     ├── src/DAPyCommonWidgets/CMakeLists.txt (调用 damacro_import_Python)
  │     ├── src/DAData/CMakeLists.txt (调用 damacro_import_Python)
  │     ├── src/DAInterface/CMakeLists.txt (调用 damacro_import_Python)
  │     ├── src/DAGui/CMakeLists.txt (调用 damacro_import_Python)
  │     ├── src/APP/CMakeLists.txt (调用 damacro_import_Python)
  │     └── plugins/DataAnalysis/CMakeLists.txt (调用 damacro_import_Python)
  ├── cmake/daworkbench_3rdparty.cmake (damacro_import_pybind11 宏)
  │     ├── (同上所有模块也调用 damacro_import_pybind11)
  └── DA_ENABLE_AUTO_INSTALL_PYTHON_ENV (DLL复制逻辑)
```

**关键发现**: 所有 Python 查找逻辑都集中在 `damacro_import_Python` 宏中，一处修改即可覆盖全局。这是项目的良好架构设计。