---
name: python-script-integration
description: Use when integrating Python scripts with C++ in a DAWorkbench plugin — calling Python from C++, Python multithreading without UI conflicts, Python-to-C++ data exchange, Python UI dialog invocation, thread_status_manager usage, GIL management. Triggers: Python call, python thread, DAPyModule, pybind11, GIL, callInMainThread, thread_status_manager, python script, data exchange, python UI.
---

# Python 脚本与 C++ 集成

本技能指导在 DAWorkbench 插件中实现 Python 脚本调用、Python 多线程、Python 与 C++ 界面的数据交互和跨线程通信。

## 决策树

```
你要做什么？
├── C++ 调用 Python 函数 → 场景 A：Python 模块导入与函数调用
├── Python 后台线程处理数据，不阻塞 UI → 场景 B：Python 多线程
├── Python 线程向 C++ 主线程推送数据 → 场景 C：跨线程数据回传
├── Python 脚本访问 C++ 数据管理器 → 场景 D：Python 访问 C++ 数据
├── Python 脚本弹出 UI 对话框 → 场景 E：Python 调用 UI
├── Python 脚本向日志窗口输出 → 场景 F：Python 日志通道
└── Python 脚本部署与更新 → 场景 G：脚本部署
```

## 核心架构

```
┌──────── C++ 主线程（Qt 事件循环） ────────┐
│                                           │
│  MyWorker (C++ Worker)                    │
│    ├ DAPyModule m_pyModule                 │
│    │   └ import("MyPackage")              │
│    ├ DAPyModule m_pyDataAnalysisModule      │
│    │   └ = m_pyModule.attr("data_analysis")│
│    ├ DAPyModule m_threadStatusMgrModule     │
│    │   └ = DAWorkbench.DAPyBase             │
│    │      .attr("thread_status_manager")    │
│    │                                        │
│    ├ 调用 Python 函数 (同步)                │
│    │   └ m_pyDataAnalysisModule->attr(     │
│    │      "process_zip_data_thread")(path)  │
│    │      → 返回 taskid                     │
│    │                                        │
│    └ QTimer::singleShot 轮询线程状态        │
│        └ get_task_status(taskid)           │
│            → 返回 dict {is_running, progress, message, ...} │
│                                           │
└───────────────────────────────────────────┘
          ↑ callInMainThread          ↑ QTimer 轮询
          │ (Qt 信号槽跨线程)           │ (C++ 主动查询)
┌───────── Python 后台线程 ──────────────────┐
│                                           │
│  threading.Thread(target=process_zip_data)│
│    ├ 处理数据（pandas/numpy）              │
│    ├ status.update_progress(p, msg)        │
│    ├ status.update_custom_data(k, v)       │
│    ├ status.finish(success, msg)           │
│    └ 回调函数: signal_handler              │
│        .callInMainThread(add_data_func)    │
│        → 在主线程中操作 DataManager         │
│                                           │
└───────────────────────────────────────────┘
```

## !!!danger GIL 安全规则（所有场景适用）

1. **C++ → Python 调用**必须在持有 GIL 时进行。pybind11 的函数调用会自动获取 GIL，但如果你在 C++ 线程中手动操作 Python 对象，需用 `pybind11::gil_scoped_acquire` / `pybind11::gil_scoped_release`
2. **释放 GIL 让 Python 线程运行**：C++ 在等待 Python 后台线程时，必须释放 GIL
3. **`pybind11::error_already_set` 异常**必须在 GIL 作用域内 catch 并消费
4. **Python 后台线程**中不要直接操作 Qt 界面控件，必须通过 `callInMainThread`

## 场景 A：Python 模块导入与函数调用

### A1. C++ 导入 Python 模块

```cpp
#include "DAPyModule.h"
#include "DAPybind11InQt.h"  // 必须在所有 pybind11 头文件之前
#include "DAPybind11QtCaster.hpp"

bool MyWorker::initializePythonEnv()
{
    try {
        // 1. 导入插件自己的 Python 包
        m_pyModule = std::make_unique<DA::DAPyModule>();
        if (!m_pyModule->import("MyPackage")) {
            daCritical << "Failed to import MyPackage";
            return false;
        }

        // 2. 获取子模块（包内的 .py 文件）
        m_pyDataModule = std::make_unique<DA::DAPyModule>();
        *m_pyDataModule = m_pyModule->attr("data_analysis");

        // 3. 导入平台提供的线程状态管理器
        DA::DAPyModule daWorkbench("DAWorkbench.DAPyBase");
        m_threadStatusMgr = std::make_unique<DA::DAPyModule>();
        *m_threadStatusMgr = daWorkbench.attr("thread_status_manager");

        m_isPythonValid = true;
        return true;
    } catch (const std::exception& e) {
        m_pyModule.reset();
        m_pyDataModule.reset();
        qCritical() << e.what();
        return false;
    }
}
```

### A2. 调用 Python 函数并传递参数

```cpp
#include "DAPybind11QtCaster.hpp"  // Qt ↔ Python 类型转换

void MyWorker::callPythonFunction()
{
    try {
        // 获取函数对象
        auto func = m_pyDataModule->attr("my_function");
        if (func.is_none()) {
            qCritical() << "function not found";
            return;
        }

        // 传递 QString 参数（DAPybind11QtCaster 自动转换）
        auto arg = DA::PY::toPyObject(QString("some_path"));
        auto result = func(arg);

        // 获取返回值
        std::string taskid = result.cast<std::string>();
        m_taskID = taskid;

    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
}
```

### A3. Python 包结构

```
PyScripts/
└── MyPackage/              ← 包名（C++ import 的名字）
    ├── __init__.py         ← 包入口，导出子模块
    ├── data_analysis.py    ← 数据分析脚本
    └── utils.py            ← 工具函数
```

```python
# __init__.py
from . import data_analysis, utils
```

```python
# data_analysis.py
def my_function(path: str) -> str:
    """C++ 通过 attr("my_function") 调用此函数"""
    import pandas as pd
    df = pd.read_csv(path)
    return f"processed_{len(df)}_rows"
```

### A4. DAPybind11QtCaster 自动类型转换

| C++ 类型 | Python 类型 | 说明 |
|---------|-------------|------|
| `QString` | `str` | 自动 UTF-8 转换 |
| `QList<T>` | `list` | 泛型容器 |
| `QHash<K,V>` / `QMap<K,V>` | `dict` | 字典转换 |
| `QVariant` | `Any` | 支持 numpy 数组/标量 |
| `QDateTime` | `datetime.datetime` | 支持 pandas.Timestamp |
| `QColor` | `tuple` | (r,g,b) 或 (r,g,b,a) |

手动转换辅助函数：
```cpp
// QString → pybind11::object
auto pyStr = DA::PY::toPyObject(QString("hello"));

// pybind11::dict → QVariant
QVariant var = DA::PY::fromPyVariant(pyDict);

// 判断能否转为 QDateTime
bool canConvert = DA::PY::canCastToQDateTime(pyObj);
```

## 场景 B：Python 多线程

### B1. Python 端：启动后台线程

```python
# data_analysis.py
import threading
import DAWorkbench.DAPyBase.thread_status_manager as tsm

def process_zip_data_thread(zip_path: str) -> str:
    """
    主入口：启动后台线程处理数据
    返回: taskid（字符串），空字符串表示启动失败
    """
    try:
        # 创建任务状态追踪器
        taskid, status = tsm.create_task_with_status("process zip data")

        # 启动后台线程
        thread = threading.Thread(
            target=_process_data_internal,
            args=(zip_path, status),
            daemon=True
        )
        thread.start()
        return taskid
    except Exception as e:
        logger.error(f"启动线程失败: {e}")
        return ""

def _process_data_internal(zip_path: str, status: tsm.ProcessingStatus):
    """后台线程的实际处理函数"""
    try:
        status.start()
        status.update_progress(0, "开始处理")
        status.update_custom_data("zip_path", zip_path)

        # ... 数据处理逻辑 ...
        for i, file in enumerate(files):
            status.update_progress(
                (i / len(files)) * 100,
                f"处理文件 {i}/{len(files)}: {file}"
            )
            # ... 处理每个文件 ...

        status.update_custom_data("result_count", len(results))
        status.finish(True, f"完成，共处理 {len(results)} 个文件")
    except Exception as e:
        status.finish(False, f"失败: {e}")
```

### B2. C++ 端：启动线程并轮询状态

```cpp
void MyWorker::startBackgroundTask()
{
    if (!isPythonValid()) return;

    try {
        // 调用 Python 函数启动后台线程
        auto func = m_pyDataModule->attr("process_zip_data_thread");
        auto arg = DA::PY::toPyObject(m_zipPath);
        auto result = func(arg);

        if (result.is_none()) return;

        m_taskID = result.cast<std::string>();

        // 显示进度条
        DA::DAStatusBarInterface* statusBar = m_ui->getStatusBar();
        statusBar->showProgressBar();
        statusBar->setProgressText(QString(u8"正在处理..."));

        // 启动轮询
        checkTaskStatus();
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
}

void MyWorker::checkTaskStatus()
{
    try {
        // 释放 GIL，让 Python 后台线程能运行
        {
            pybind11::gil_scoped_release release;
            QThread::sleep(1);  // 短暂休眠
        }

        // 调用 Python 获取任务状态
        auto get_status = m_threadStatusMgr->attr("get_task_status");
        pybind11::dict status = get_status(m_taskID);

        bool is_running = status["is_running"].cast<bool>();

        if (is_running) {
            // 更新进度
            double progress = status["progress"].cast<double>();
            std::string message = status["message"].cast<std::string>();
            double elapsed = status["elapsed_seconds"].cast<double>();

            DA::DAStatusBarInterface* statusBar = m_ui->getStatusBar();
            statusBar->setProgress(progress);
            statusBar->setProgressText(
                QString::fromStdString(message) +
                QString(u8" 已用时: %1:%2")
                    .arg(static_cast<int>(elapsed) / 60, 2, 10, QChar('0'))
                    .arg(static_cast<int>(elapsed) % 60, 2, 10, QChar('0'))
            );

            // 继续轮询（20ms 后再次检查）
            QTimer::singleShot(20, this, &MyWorker::checkTaskStatus);
        } else {
            // 任务完成
            bool is_success = status["is_success"].cast<bool>();
            QString msg = QString::fromStdString(status["message"].cast<std::string>());

            DA::DAStatusBarInterface* statusBar = m_ui->getStatusBar();
            statusBar->hideProgressBar();
            statusBar->setBusy(false);

            if (is_success) {
                statusBar->showMessage(QString(u8"完成!"));
                // 获取自定义数据
                pybind11::dict custom_data = status["custom_data"];
                QString zip_path = QString::fromStdString(
                    custom_data["zip_path"].cast<std::string>());
                // 处理完成后的逻辑
            } else {
                statusBar->showMessage(QString(u8"失败! ") + msg);
            }

            QApplication::processEvents();
        }
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }
}
```

### B3. 状态字典字段说明

`get_task_status(taskid)` 返回的字典包含以下字段：

| 字段 | 类型 | 说明 |
|------|------|------|
| `task_id` | str | 任务唯一 ID |
| `task_name` | str | 任务名称 |
| `is_running` | bool | 是否正在运行（未暂停、未取消、未完成） |
| `is_paused` | bool | 是否暂停 |
| `is_canceled` | bool | 是否取消 |
| `is_success` | bool | 是否成功完成 |
| `current_stage` | str | 当前阶段描述 |
| `progress` | float | 进度百分比 (0-100) |
| `elapsed_seconds` | float | 已用时间（秒） |
| `message` | str | 当前状态消息 |
| `custom_data` | dict | 自定义数据副本 |

### B4. ProcessingStatus API 速查

```python
# 创建任务
taskid, status = tsm.create_task_with_status("task name")

# 状态控制
status.start()                    # 开始
status.update_progress(50, "处理中")  # 更新进度
status.pause()                    # 暂停
status.resume()                   # 恢复
status.cancel("用户取消")           # 取消
status.finish(True, "完成消息")     # 完成（成功/失败）

# 自定义数据
status.update_custom_data("key", value)
value = status.get_custom_data("key", default)

# 查询
info = status.get_status()        # 返回状态字典
is_active = status.is_active()    # 是否活跃
is_finished = status.is_finished() # 是否已结束
```

## 场景 C：跨线程数据回传

Python 后台线程处理完数据后，需要将结果写入 C++ 的 DataManager。但**不能直接操作 Qt 界面**，必须通过 `DAPythonSignalHandler::callInMainThread` 在主线程执行。

### C1. Python 端：通过 callInMainThread 回传数据

```python
# data_analysis.py
import da_app, da_data

def process_zip_data_thread(zip_path: str) -> str:
    taskid, status = tsm.create_task_with_status("process zip data")

    def internal_callback(result_dict):
        """后台线程完成后的回调，通过 callInMainThread 在主线程写入数据"""
        if result_dict is not None:
            signal_handler = da_app.getCore().getPythonSignalHandler()
            if signal_handler:
                def add_data_in_main_thread():
                    """此函数在 Qt 主线程中执行"""
                    datamanager = da_app.getCore().getDataManagerInterface()
                    for name, df in result_dict.items():
                        data = da_data.DAData(df)
                        data.setName(name)
                        data.setDescribe(name)
                        datamanager.addData(data)

                signal_handler.callInMainThread(add_data_in_main_thread)

    thread = threading.Thread(
        target=_process_data,
        args=(zip_path, internal_callback, status),
        daemon=True
    )
    thread.start()
    return taskid
```

### C2. C++ 端：监听数据添加信号

```cpp
// 在 Worker 初始化时连接数据添加信号
void MyWorker::initialize(DA::DACoreInterface* core, MyUI* ui)
{
    m_core = core;
    m_ui = core->getUiInterface();

    DA::DADataManagerInterface* datamgr = m_core->getDataManagerInterface();
    // 连接数据添加信号
    connect(datamgr, &DA::DADataManagerInterface::dataAdded,
            this, &MyWorker::onDataAdded);
}

void MyWorker::onDataAdded()
{
    // 数据已通过 Python 线程 → callInMainThread → DataManager::addData 添加
    // 在这里重新分析数据
    analysisDatas();
    Q_EMIT datasAdded();  // 通知 UI 更新
}
```

### C3. callInMainThread 机制详解

```
Python 后台线程                     C++ 主线程（Qt 事件循环）
    │                                     │
    ├ signal_handler.callInMainThread(    │
    │   lambda: add_data())               │
    │   └ 函数包装为 FunctionWrapper       │
    │   └ 存入 m_functionMap (mutex)       │
    │   └ emit executeRequested(id) ──────┼──→ Qt 跨线程队列连接
    │                                     │
    │   (Python 线程继续执行)              ├ onExecuteRequested(id)
    │                                     │   └ 从 map 取出 FunctionWrapper
    │                                     │   └ 执行函数（操作 DataManager）
    │                                     │
```

## 场景 D：Python 访问 C++ 数据

### D1. 从 DataManager 查找数据

```python
import da_app, da_data

def my_python_function():
    """Python 脚本中访问 C++ DataManager"""
    datamanager = da_app.getCore().getDataManagerInterface()

    # 按通配符查找数据表
    datas = datamanager.findDatas("*module*")
    for d in datas:
        name = d.getName()
        df = d.toDataFrame()  # 转为 pandas DataFrame
        # 处理 DataFrame...

    # 获取特定名称的数据
    datas = datamanager.findDatas("*module_8")
    if datas:
        df = datas[0].toDataFrame()
```

### D2. 向 DataManager 写入数据

```python
import da_app, da_data
import pandas as pd

def write_data_to_manager():
    """将 DataFrame 写入 DataManager（必须在主线程调用）"""
    signal_handler = da_app.getCore().getPythonSignalHandler()
    signal_handler.callInMainThread(lambda: _do_write())

def _do_write():
    datamanager = da_app.getCore().getDataManagerInterface()
    df = pd.DataFrame({"A": [1, 2, 3], "B": [4, 5, 6]})
    data = da_data.DAData(df)
    data.setName("my_data_table")
    data.setDescribe("description")
    datamanager.addData(data)
```

### D3. DAData 在 Python 中的操作

```python
import da_data

# DAData 包装 pandas DataFrame
data = da_data.DAData(df)    # 从 DataFrame 创建
df = data.toDataFrame()      # 转回 DataFrame
data.setName("name")        # 设置名称
data.setDescribe("desc")    # 设置描述
name = data.getName()        # 获取名称
is_null = data.isNull()     # 是否为空
data_id = data.id()          # 获取 ID
```

## 场景 E：Python 调用 UI

### E1. 弹出文件选择对话框

```python
import da_app

def my_function():
    ui = da_app.getCore().getUiInterface()

    # 选择文件夹
    path = ui.getExistingDirectory("选择保存路径")
    if len(path) == 0:
        return  # 用户取消

    # 选择文件
    # file_path = ui.getOpenFileName("选择文件", "CSV (*.csv)")
```

### E2. 弹出表单配置对话框

平台提供 `FormBuilder` 用于构建参数配置对话框，C++ 端通过 `getConfigValues` 弹出对话框。

```python
from DAWorkbench.DAPyBase.form_builder import FormBuilder, option

def show_config_dialog():
    ui = da_app.getCore().getUiInterface()

    config = (FormBuilder("参数设置")
        .group("basic", "基本设置")
        .enum("machine", label="机型",
              default="GMV-224WM/S",
              options=[option("GMV-224WM/S", "GMV-224WM/S"),
                       option("GMV-252WM/S", "GMV-252WM/S")],
              description="选择机型")
        .bool("save_word", label="是否生成报告",
              default=True,
              description="勾选将生成 Word 报告")
        .end_group()
        .build()
    )

    result = ui.getConfigValues(config, "my_config_cache_key")
    if len(result) == 0:
        return  # 用户取消

    machine = result["machine"]
    save_word = result["save_word"]
```

### E3. FormBuilder API 速查

| 方法 | 用途 |
|------|------|
| `FormBuilder(title)` | 创建表单构建器 |
| `.group(id, title)` | 开始一个分组 |
| `.end_group()` | 结束当前分组 |
| `.str(name, label, default, description)` | 字符串输入 |
| `.int(name, label, default, description)` | 整数输入 |
| `.float(name, label, default, description)` | 浮点输入 |
| `.bool(name, label, default, description)` | 布尔选择 |
| `.enum(name, label, default, options, description)` | 下拉枚举 |
| `.option(value, label)` | 枚举选项（在 options 列表中） |
| `.build()` | 构建 JSON 配置 |

## 场景 F：Python 日志通道

### F1. 向界面日志窗口输出

```python
import da_app

def my_function():
    ui = da_app.getCore().getUiInterface()

    # 第二个参数 showInStatusBar：
    #   True = 同时显示在状态栏（仅主线程安全）
    #   False = 仅写入日志窗口（后台线程安全）
    ui.addInfoLogMessage("信息消息", False)
    ui.addWarningLogMessage("警告消息", False)
    ui.addCriticalLogMessage("错误消息", False)
```

!!!danger 后台线程中必须传 `showInStatusBar=False`
    Python 后台线程中调用日志方法时，第二个参数必须为 `False`，否则会触发 GUI 操作导致崩溃。`False` 表示仅走 spdlog 通道（线程安全），不触碰状态栏等 GUI 控件。

### F2. 完整的后台线程日志封装

```python
import logging
from loguru import logger

try:
    import da_app
    _has_da_app = True
except Exception:
    _has_da_app = False

def emit_ui_message(msg: str, level: str = "warning") -> None:
    """线程安全的 UI 日志输出"""
    if not _has_da_app:
        return
    try:
        ui = da_app.getCore().getUiInterface()
        if level == "critical":
            ui.addCriticalLogMessage(msg, False)
        elif level == "info":
            ui.addInfoLogMessage(msg, False)
        else:
            ui.addWarningLogMessage(msg, False)
    except Exception:
        pass  # UI 推送失败不应影响数据处理主流程
```

## 场景 G：脚本部署

### G1. CMake 部署（配置阶段自动复制）

```cmake
# src/CMakeLists.txt
# Python 脚本在 CMake 配置阶段自动复制到安装目录
file(COPY "${CMAKE_CURRENT_LIST_DIR}/PyScripts/MyPackage"
     DESTINATION ${DAWorkbench_INSTALL_DIR}/bin/PyScripts)
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/PyScripts/MyPackage"
        DESTINATION ${CMAKE_INSTALL_BINDIR}/PyScripts)
```

### G2. 运行时目录结构

```
bin_Release_qtX.Y.Z_MSVC_x64/bin/
├── PyScripts/
│   ├── DAWorkbench/          ← 平台内置 Python 包
│   │   └── DAPyBase/
│   │       └── thread_status_manager.py
│   └── MyPackage/            ← 插件 Python 包
│       ├── __init__.py
│       └── data_analysis.py
├── plugins/
│   └── MyPlugin.dll          ← 插件 DLL
└── DAWorkbench.exe            ← 主程序
```

### G3. 修改 Python 脚本后的生效方式

!!!danger 修改 .py 文件后必须重新部署并重启应用
    DAWorkbench 嵌入式 Python 在启动时加载脚本到内存，修改 `.py` 文件后：
    1. **重新运行 CMake configure**（触发 `file(COPY)` 复制到 bin 目录），或手动复制 `.py` 到 `bin/PyScripts/MyPackage/`
    2. **重启 DAWorkbench 主程序**（Python 解释器重新初始化）
    3. 使用 `DAPyModule::reload()` 可在运行时重新加载模块，但仅对已导入的模块有效

## 完整调用链示例

以下是一个完整的"C++ 触发 → Python 后台处理 → C++ 轮询进度 → Python 回传数据 → C++ 更新界面"的链路：

```
1. 用户点击 Ribbon Action
   → C++ MyUI::onActionTriggered()
   → MyWorker::importZipData()

2. C++ 调用 Python 启动后台线程
   → m_pyDataAnalysisModule->attr("process_zip_data_thread")(zipPath)
   → Python: threading.Thread(target=_process_data).start()
   → 返回 taskid

3. C++ 启动 QTimer 轮询
   → checkZipDataImportStatus()
   → 释放 GIL: pybind11::gil_scoped_release + QThread::sleep(1)
   → 获取状态: get_task_status(taskid) → dict
   → 更新状态栏: statusBar->setProgress(progress)
   → QTimer::singleShot(20, continue)

4. Python 后台线程处理数据
   → pandas 读取 CSV → 清洗 → 时间对齐 → 统计计算
   → status.update_progress(x, "处理中")
   → status.update_custom_data("key", value)

5. Python 线程完成，回调函数通过 callInMainThread 写入数据
   → signal_handler.callInMainThread(add_data_func)
   → Qt 信号 → 主线程执行: datamanager.addData(data)

6. C++ 收到 dataAdded 信号
   → onDataAdded() → analysisDatas()
   → Q_EMIT datasAdded()

7. UI 收到 datasAdded 信号
   → MyUI::onDataAdded()
   → 更新设备选择列表
   → 创建默认绘图
```

## C++ 侧 Python 对象管理

### DAPyModule 使用模式

```cpp
// 成员变量声明（使用 unique_ptr 延迟初始化）
std::unique_ptr<DA::DAPyModule> m_pyModule;
std::unique_ptr<DA::DAPyModule> m_pyDataModule;
std::unique_ptr<DA::DAPyModule> m_threadStatusMgr;

// 初始化
m_pyModule = std::make_unique<DA::DAPyModule>();
m_pyModule->import("MyPackage");

// 获取子模块
m_pyDataModule = std::make_unique<DA::DAPyModule>();
*m_pyDataModule = m_pyModule->attr("data_analysis");

// 调用函数
auto func = m_pyDataModule->attr("my_function");
auto result = func(arg1, arg2);

// 从 dict 读取值
pybind11::dict status = get_status(taskid);
bool running = status["is_running"].cast<bool>();
double progress = status["progress"].cast<double>();
std::string message = status["message"].cast<std::string>();
pybind11::dict custom_data = status["custom_data"];
```

### 头文件引入顺序

```cpp
// 1. Qt 头文件（正常引入）
#include <QObject>
#include <QString>

// 2. pybind11 桥接头（必须在这些之前引入所有 Qt 头文件）
#include "DAPybind11InQt.h"        // slots 宏冲突解决
#include "DAPybind11QtCaster.hpp"  // Qt ↔ Python 类型转换

// 3. DA Python 封装
#include "DAPyModule.h"

// 4. 其他
#include "DALog.h"
```

## 常见陷阱

1. **后台线程中不能直接操作 Qt 控件** — 必须通过 `callInMainThread`
2. **GIL 死锁** — C++ 在等待 Python 线程时必须释放 GIL (`gil_scoped_release`)
3. **日志 `showInStatusBar`** — 后台线程必须传 `False`
4. **Python 脚本修改后不生效** — 需要复制到 bin 目录并重启应用
5. **`DAPybind11InQt.h` 必须第一个引入** — 解决 Qt `slots` 宏与 Python.h 的冲突
6. **`DAWidgets` 需要显式 `find_package`** — DAGui 依赖它但不自动加载
7. **Python 异常必须在 GIL 作用域内 catch** — `pybind11::error_already_set` 的析构需要 GIL
8. **`DAPyModule` 继承自 `DAPyObjectWrapper`（非 QObject）** — 不能使用 Qt 信号槽

## 参考文件

| 文件 | 说明 |
|------|------|
| `src/MyWorker.cpp` | C++ 调用 Python + QTimer 轮询线程状态完整示例 |
| `src/PyScripts/MyPackage/data_analysis.py` | Python 多线程 + callInMainThread 回传数据完整示例 |
| `src/PyScripts/MyPackage/zip_csv_file_handle.py` | 后台线程处理数据 + thread_status_manager 使用示例 |
| `src/PyScripts/MyPackage/plot_map.py` | Python 写入 DataManager + 生成 Word 报告示例 |
| `data-workbench/src/DAPyBindQt/DAPyModule.h` | DAPyModule 基类 API |
| `data-workbench/src/DAPyBindQt/DAPybind11InQt.h` | slots 宏冲突解决 |
| `data-workbench/src/DAPyBindQt/DAPybind11QtCaster.hpp` | Qt ↔ Python 类型转换器 |
| `data-workbench/src/DAPyBindQt/DAPythonSignalHandler.h` | 跨线程通信机制 |
| `data-workbench/src/DAInterface/DACoreInterface.h` | 核心接口（含 getPythonSignalHandler） |
| `data-workbench/src/PyScripts/DAWorkbench/DAPyBase/thread_status_manager.py` | 线程状态管理器完整 API |
| `data-workbench/skills/add-python-binding/SKILL.md` | 添加 Python 绑定（暴露 C++ 给 Python）的技能 |
