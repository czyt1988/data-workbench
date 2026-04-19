# Python 3.10+ 兼容性验证清单

> **文档说明**: 本清单用于在升级到 Python 3.10+ 后，系统性地验证 data-workbench 项目的兼容性。每个验证项都包含具体的测试命令、预期结果和失败处理指南。
> 
> **版本信息**: 
> - 目标 Python 版本：>= 3.10
> - 项目版本：v0.0.5
> - 文档创建日期：2026-04-19

---

## 目录

1. [CMake 构建验证](#1-cmake-构建验证)
2. [Python 解释器初始化验证](#2-python-解释器初始化验证)
3. [Python 模块导入验证](#3-python-模块导入验证)
4. [pandas/numpy/scipy 功能验证](#4-pandasnumphyscipy-功能验证)
5. [工作流引擎验证](#5-工作流引擎验证)
6. [图表模块验证](#6-图表模块验证)
7. [Python 脚本兼容性验证](#7-python-脚本兼容性验证)
8. [插件系统验证](#8-插件系统验证)
9. [自动化测试套件验证](#9-自动化测试套件验证)
10. [回归风险清单](#10-回归风险清单)

---

## 1. CMake 构建验证

**类别**: 构建  
**优先级**: 高

### 1.1 Python 3.10 环境检测

**测试命令**:
```powershell
# 验证 Python 版本
python --version
# 应显示：Python 3.10.x 或更高

# 验证 Python 路径
where python
```

**预期结果**:
- Python 版本 >= 3.10.0
- 路径指向正确的 Python 3.10+ 安装目录

**失败处理**:
- 检查系统 PATH 环境变量
- 确认已安装 Python 3.10+
- 如需多版本共存，使用 `py -3.10` 指定版本

### 1.2 CMake 配置验证

**测试命令**:
```powershell
# 清理旧构建
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

# 配置 CMake (Qt6 示例)
cmake -S . -B build -G Ninja `
    -DCMAKE_BUILD_TYPE:STRING=Release `
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE `
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH="D:\Qt\6.7.3\msvc2019_64\lib\cmake\Qt6\qt.toolchain.cmake" `
    -DQT_QML_GENERATE_QMLLS_INI:STRING=ON `
    "-DCMAKE_CXX_FLAGS_DEBUG_INIT:STRING=-DQT_QML_DEBUG -DQT_DECLARATIVE_DEBUG" `
    "-DCMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT:STRING=-DQT_QML_DEBUG -DQT_DECLARATIVE_DEBUG"
```

**预期结果**:
- CMake 配置成功，无错误
- 输出中应显示：
  ```
  -- find python
  --   |-Python3_VERSION = 3.10.x
  --   |-Python3_VERSION_MAJOR = 3
  --   |-Python3_VERSION_MINOR = 10
  --   |-Python3_ROOT_DIR = <Python 3.10 路径>
  --   |-Python3_EXECUTABLE = <python.exe 路径>
  ```

**失败处理**:
- 如提示找不到 Python 3.10，添加 `-DPython3_ROOT_DIR=<Python3.10 路径>`
- 检查 `cmake/daworkbench_3rdparty.cmake` 中 `damacro_import_Python` 宏
- 确认 `find_package(Python3 COMPONENTS Interpreter Development REQUIRED)` 成功

### 1.3 CMake 构建验证

**测试命令**:
```powershell
# 构建项目
cmake --build build --config Release --parallel

# 构建并安装
cmake --build build --config Release --target install
```

**预期结果**:
- 所有目标构建成功
- 无编译错误
- DLL/EXE 生成在 `build/bin` 目录

**失败处理**:
- 如出现 `PY_VERSION_HEX` 相关错误，检查 `DAPyInterpreter.cpp` 版本宏
- 如出现 pybind11 错误，确认 pybind11 版本 >= 2.9 (推荐 3.0.3)
- 检查 `Python3_LIBRARIES` 和 `Python3_INCLUDE_DIRS` 是否正确

### 1.4 Python DLL 部署验证

**测试命令**:
```powershell
# 检查 bin 目录下是否存在 Python DLL
Get-ChildItem build\bin\python*.dll

# 验证 DLL 版本
python -c "import sys; print(sys.version)"
```

**预期结果**:
- 存在 `python310.dll` 或对应版本的 DLL
- DLL 版本与 Python 解释器版本一致

**失败处理**:
- 检查 `DA_ENABLE_AUTO_INSTALL_PYTHON_ENV` 选项是否为 ON
- 手动复制 `$env:Python3_ROOT_DIR\python310.dll` 到 `build/bin`

---

## 2. Python 解释器初始化验证

**类别**: 运行  
**优先级**: 高

### 2.1 DAPyInterpreter 初始化

**测试命令**:
```cpp
// 在测试程序中运行
#include "DAPyInterpreter.h"
#include <QDebug>

int main() {
    qDebug() << "Python DLL version from header:" << PY_VERSION;
    qDebug() << "Python hex version:" << PY_VERSION_HEX;
    qDebug() << "Python runtime version:" << Py_GetVersion();
    
    DA::DAPyInterpreter interpreter;
    qDebug() << "Python initialized:" << DA::DAPyInterpreter::isPythonInitialized();
    
    return 0;
}
```

**预期结果**:
- `PY_VERSION_HEX >= 0x030A0000` (Python 3.10+)
- Python 解释器成功初始化
- 无崩溃或异常

**失败处理**:
- 检查 `DAPyInterpreter.cpp` 中 `PY_VERSION_HEX >= 0x03080000` 分支
- 确认 `PyConfig_InitPythonConfig` API 可用 (Python 3.8+)
- 如使用 Python 3.11+，检查 `Py_SetPythonHome` 弃用警告

### 2.2 Python Home 路径设置

**测试命令**:
```cpp
// 测试带 Python Home 路径的初始化
DA::DAPyInterpreter::initializePythonInterpreter("C:\\Python310");
```

**预期结果**:
- 成功设置 Python Home 路径
- 调试输出显示：`Python home path set to: C:\Python310`

**失败处理**:
- Python 3.11+ 使用 `PyConfig_SetString` 替代 `Py_SetPythonHome`
- 检查 `DAPyInterpreter.cpp:196-227` 版本分支逻辑

### 2.3 Python 解释器关闭

**测试命令**:
```cpp
// 测试关闭流程
DA::DAPyInterpreter interpreter;
// ... 使用 Python ...
DA::DAPyInterpreter::shutdown();
qDebug() << "Python shutdown complete";
```

**预期结果**:
- 成功调用 `stop_all_background_tasks()`
- 非守护线程正常结束
- 无崩溃或死锁

**失败处理**:
- 检查 `DAWorkbench/__init__.py` 中 `stop_all_background_tasks()` 函数
- 确认 `gc.collect()` 成功执行
- 增加 `QThread::msleep()` 等待时间

---

## 3. Python 模块导入验证

**类别**: 运行  
**优先级**: 高

### 3.1 pybind11 嵌入模块导入

**测试命令**:
```python
# 在 C++ 程序中通过 pybind11 导入
import pybind11
pybind11::module_ da_app = pybind11::module_::import("da_app");
pybind11::module_ da_interface = pybind11::module_::import("da_interface");
pybind11::module_ da_data = pybind11::module_::import("da_data");
```

**预期结果**:
- 三个模块成功导入
- 无 `ModuleNotFoundError`
- 无 `ImportError`

**失败处理**:
- 检查 `sys.path` 是否包含模块路径
- 确认 `PYBIND11_EMBEDDED_MODULE` 宏正确注册
- 验证 `DAPybind11QtCaster.hpp` 类型转换器

### 3.2 DAWorkbench 包导入

**测试命令**:
```python
# Python 测试脚本
import sys
print(f"Python path: {sys.path}")

try:
    import DAWorkbench
    print(f"DAWorkbench imported successfully")
    print(f"DAWorkbench version: {DAWorkbench.__version__ if hasattr(DAWorkbench, '__version__') else 'N/A'}")
except ImportError as e:
    print(f"Import failed: {e}")
```

**预期结果**:
- `DAWorkbench` 包成功导入
- 子模块 (`io`, `dataframe`, `data_processing` 等) 可访问
- 日志系统正常初始化

**失败处理**:
- 检查 `src/PyScripts/DAWorkbench/__init__.py`
- 确认 `da_logger.py` 依赖的 `loguru` 已安装
- 验证 `sys.path.append()` 路径正确性

### 3.3 typing_extensions 清理验证

**测试命令**:
```python
# 验证不再需要 typing_extensions
import sys
print(f"Python version: {sys.version_info}")

# 应该直接从 typing 导入
from typing import Literal
print("Literal imported from typing successfully")

# 不应再需要这行
# from typing_extensions import Literal
```

**预期结果**:
- `from typing import Literal` 成功 (Python 3.10+ 内置)
- `requirements.txt` 中 `typing_extensions` 可安全移除

**失败处理**:
- 如导入失败，确认 Python 版本 >= 3.8
- 更新 `dataframe.py` 中的条件导入逻辑

---

## 4. pandas/numpy/scipy 功能验证

**类别**: 功能  
**优先级**: 高

### 4.1 pandas 核心功能

**测试命令**:
```python
import pandas as pd
import numpy as np

# 测试 DataFrame 创建
df = pd.DataFrame({'A': [1, 2, 3], 'B': [4.0, 5.0, 6.0]})
print(f"DataFrame shape: {df.shape}")

# 测试 DAWorkbench 自定义函数
from DAWorkbench.dataframe import da_drop_irow, da_drop_icolumn, da_fill_na

df_test = df.copy()
da_drop_irow(df_test, [0])
print(f"After drop row: {df_test.shape}")

df_test = df.copy()
da_drop_icolumn(df_test, [0])
print(f"After drop column: {df_test.shape}")

df_nan = pd.DataFrame([[np.nan, 2], [3, np.nan]])
da_fill_na(df_nan, value=0)
print(f"After fillna:\n{df_nan}")
```

**预期结果**:
- pandas 版本 >= 2.0 (推荐 2.2.3)
- 所有 `da_*` 函数正常执行
- 无 `FutureWarning` 或 `DeprecationWarning`

**失败处理**:
- 检查 `requirements.txt` 中 pandas 版本
- 验证 `pandas._typing` 内部 API 兼容性
- 如出现类型警告，更新类型注解

### 4.2 numpy 版本兼容性

**测试命令**:
```python
import numpy as np

print(f"NumPy version: {np.__version__}")

# 测试常用函数
arr = np.array([1, 2, 3, 4, 5])
print(f"Mean: {np.mean(arr)}")
print(f"Std: {np.std(arr)}")

# 测试 datetime64
dt = np.datetime64('2021-01-01')
print(f"DateTime64: {dt}")

# 测试 linspace (da_insert_column 使用)
s = np.linspace(0, 10, 5)
print(f"Linspace: {s}")
```

**预期结果**:
- numpy 版本 < 2.0.0 (兼容性考虑)
- 所有数组操作正常
- datetime64 类型正确处理

**失败处理**:
- numpy 2.0 有破坏性变更，建议锁定在 1.x
- 检查 `da_insert_column` 中 `np.full` 和 `np.arange` 用法

### 4.3 scipy 功能验证

**测试命令**:
```python
from scipy import stats, signal
import numpy as np

# 测试统计函数
data = np.random.randn(100)
print(f"Skewness: {stats.skew(data)}")
print(f"Kurtosis: {stats.kurtosis(data)}")

# 测试信号处理 (PyWavelets 依赖)
import pywt
coeffs = pywt.wavedec(data, 'db4', level=3)
print(f"Wavelet decomposition levels: {len(coeffs)}")
```

**预期结果**:
- scipy 版本与 numpy 兼容
- PyWavelets 正常工作
- 无导入错误

**失败处理**:
- 检查 `requirements.txt` 中 scipy 和 PyWavelets 版本
- 确认 numpy 版本在 scipy 支持范围内

### 4.4 数据 IO 功能

**测试命令**:
```python
from DAWorkbench.dataframe import da_to_csv, da_to_excel, da_to_pickle, da_to_parquet
from DAWorkbench.dataframe import da_from_pickle, da_from_parquet
import pandas as pd
import tempfile
import os

df = pd.DataFrame({'A': [1, 2, 3], 'B': ['a', 'b', 'c']})

with tempfile.TemporaryDirectory() as tmpdir:
    # 测试 CSV
    csv_path = os.path.join(tmpdir, 'test.csv')
    da_to_csv(df, csv_path, sep=',')
    df_csv = pd.read_csv(csv_path)
    print(f"CSV round-trip: {df_csv.shape}")
    
    # 测试 Excel
    excel_path = os.path.join(tmpdir, 'test.xlsx')
    da_to_excel(df, excel_path)
    df_excel = pd.read_excel(excel_path)
    print(f"Excel round-trip: {df_excel.shape}")
    
    # 测试 Pickle
    pickle_path = os.path.join(tmpdir, 'test.pkl')
    da_to_pickle(df, pickle_path)
    df_pickle = df.copy()
    da_from_pickle(df_pickle, pickle_path)
    print(f"Pickle round-trip: {df_pickle.shape}")
    
    # 测试 Parquet
    parquet_path = os.path.join(tmpdir, 'test.parquet')
    da_to_parquet(df, parquet_path)
    df_parquet = df.copy()
    da_from_parquet(df_parquet, parquet_path)
    print(f"Parquet round-trip: {df_parquet.shape}")
```

**预期结果**:
- 所有 IO 格式读写成功
- 数据完整性保持
- openpyxl 版本兼容 (推荐 3.1.5)

**失败处理**:
- 检查 `openpyxl` 和 `pyarrow` 是否安装
- 验证文件路径编码 (Windows 中文路径问题)

---

## 5. 工作流引擎验证

**类别**: 功能  
**优先级**: 高

### 5.1 DAWorkFlow 创建

**测试命令**:
```cpp
#include "DAWorkFlow.h"
#include "DAAbstractNode.h"
#include <QDebug>

int main() {
    DA::DAWorkFlow workflow;
    qDebug() << "Workflow created:" << !workflow.isNull();
    
    // 测试节点创建
    auto node = DA::DAAbstractNode::SharedPointer::create();
    qDebug() << "Node created:" << !node.isNull();
    
    // 测试节点添加到工作流
    workflow.addNode(node);
    qDebug() << "Node count:" << workflow.nodeCount();
    
    return 0;
}
```

**预期结果**:
- 工作流对象成功创建
- 节点创建和添加正常
- 无内存泄漏

**失败处理**:
- 检查 Qt 版本兼容性 (Qt5 vs Qt6)
- 验证 `QSharedPointer` 和 `QWeakPointer` 行为

### 5.2 工作流执行

**测试命令**:
```cpp
#include "DAWorkFlowExecuter.h"

// 创建工作流和节点
DA::DAWorkFlow workflow;
// ... 添加节点和连接 ...

// 测试执行器
DA::DAWorkFlowExecuter executer;
executer.setWorkflow(&workflow);

// 连接信号
QObject::connect(&executer, &DA::DAWorkFlowExecuter::finished, []() {
    qDebug() << "Execution finished";
});

// 启动执行
executer.startExecute();

// 等待完成
QEventLoop loop;
QObject::connect(&executer, &DA::DAWorkFlowExecuter::finished, &loop, &QEventLoop::quit);
loop.exec();
```

**预期结果**:
- 工作流执行完成
- 所有节点 `exec()` 方法调用成功
- 信号槽连接正常

**失败处理**:
- 检查线程模型 (`moveToThread`)
- 验证 `DAAbstractNode::exec()` 返回值
- 如使用异步节点，检查 `DAAbstractNodeAsync` 状态机

### 5.3 节点数据流

**测试命令**:
```python
# 通过 Python 测试节点数据流
import DAWorkbench as dawb

# 创建测试数据
df = dawb.dataframe.make_dataframe(100)
print(f"Initial shape: {df.shape}")

# 测试数据处理节点
dawb.dataframe.da_drop_irow(df, [0, 1, 2])
print(f"After drop: {df.shape}")

# 验证数据传递
assert df.shape[0] == 97, "Row count mismatch"
print("Data flow test passed")
```

**预期结果**:
- 节点间数据传递正确
- LinkData 追踪连接正常
- 输入输出端口映射正确

**失败处理**:
- 检查 `DAAbstractNode` 的输入输出 key 定义
- 验证 `transmit()` 方法实现

---

## 6. 图表模块验证

**类别**: 功能  
**优先级**: 中

### 6.1 DAFigure 基础绘图

**测试命令**:
```cpp
#include "DAFigure.h"
#include "DAFigurePlot.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    DA::DAFigure figure;
    figure.show();
    
    // 测试添加 Plot
    auto plot = figure.addPlot();
    qDebug() << "Plot added:" << (plot != nullptr);
    
    // 测试绘图
    QVector<double> x = {1, 2, 3, 4, 5};
    QVector<double> y = {2, 4, 6, 8, 10};
    plot->graph()->setData(x, y);
    plot->replot();
    
    // 截图测试
    QPixmap pixmap = figure.grab();
    qDebug() << "Screenshot size:" << pixmap.size();
    
    return 0;
}
```

**预期结果**:
- 图表窗口正常显示
- 绘图数据正确渲染
- 截图功能正常

**失败处理**:
- 检查 Qwt 版本兼容性
- 验证 OpenGL 上下文 (Qt6 需要 `OpenGLWidgets`)
- 检查 HiDPI 缩放

### 6.2 论文级图片导出

**测试命令**:
```cpp
#include "DAFigureExport.h"

// 导出为高分辨率图片
DA::DAFigureExport exporter;
exporter.setFigure(figure);
exporter.setFormat("PNG");
exporter.setDPI(300);
exporter.setWidth(1920);
exporter.setHeight(1080);

bool success = exporter.exportToFile("output.png");
qDebug() << "Export success:" << success;
```

**预期结果**:
- 导出图片分辨率正确
- 文字和线条清晰
- 文件大小合理

**失败处理**:
- 检查 QPainter 渲染后端
- 验证字体嵌入
- 如使用 SVG，检查 QtSvg 模块

---

## 7. Python 脚本兼容性验证

**类别**: 回归  
**优先级**: 高

### 7.1 dataframe.py 验证

**测试命令**:
```python
# 测试 dataframe.py 所有公开函数
from DAWorkbench.dataframe import *
import pandas as pd
import numpy as np

print("Testing dataframe.py functions...")

# 创建测试数据
df = pd.DataFrame({
    'A': [1, 2, 3, np.nan, 5],
    'B': [5.0, np.nan, 7.0, 8.0, 9.0],
    'C': ['a', 'b', 'c', 'd', 'e']
})

# 测试删除函数
df1 = df.copy()
da_drop_irow(df1, [0, 1])
assert df1.shape[0] == 3, "da_drop_irow failed"

df2 = df.copy()
da_drop_icolumn(df2, [0])
assert df2.shape[1] == 2, "da_drop_icolumn failed"

# 测试 NaN 处理
df3 = df.copy()
da_drop_na(df3)
print(f"After dropna: {df3.shape}")

df4 = df.copy()
da_fill_na(df4, value=0)
assert not df4.isna().any().any(), "da_fill_na failed"

# 测试插值
df5 = df.copy()
da_fill_interpolate(df5, method='spline', order=2)
print(f"After interpolate: NaN count = {df5.isna().sum().sum()}")

# 测试类型转换
df6 = df.copy()
da_astype(df6, [0], np.float32)
assert df6['A'].dtype == np.float32, "da_astype failed"

print("All dataframe.py tests passed!")
```

**预期结果**:
- 所有 `da_*` 函数正常执行
- 无 Python 3.10 语法警告
- `Literal` 从 `typing` 正确导入

**失败处理**:
- 更新 `sys.version_info >= (3, 8)` 条件判断
- 移除 `typing_extensions` 依赖
- 检查 `pandas._typing` 内部 API 变更

### 7.2 io.py 验证

**测试命令**:
```python
from DAWorkbench import io
import tempfile
import os

print("Testing io.py functions...")

# 测试文件读取
with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
    f.write("A,B,C\n1,2,3\n4,5,6\n")
    temp_path = f.name

try:
    df = io.read_csv(temp_path)
    assert df.shape == (2, 3), "read_csv failed"
    print(f"CSV read: {df.shape}")
    
    df = io.read_excel(temp_path.replace('.csv', '.xlsx'))
    # 可能失败，因为临时文件是 CSV
    print("Excel read test skipped (no xlsx file)")
    
finally:
    os.unlink(temp_path)

print("io.py tests completed")
```

**预期结果**:
- 文件读取函数正常
- 编码检测正确 (chardet)
- 异常处理健全

**失败处理**:
- 检查 `chardet` 版本兼容性
- 验证文件路径 Unicode 处理

### 7.3 da_logger.py 验证

**测试命令**:
```python
from DAWorkbench.da_logger import setup_logging, shutdown_logging, log_function_call
import sys

print("Testing da_logger.py...")

# 测试日志初始化
setup_logging()
print("Logging setup complete")

# 测试装饰器
@log_function_call
def test_func(x, y):
    return x + y

result = test_func(3, 5)
print(f"Decorated function result: {result}")

# 测试日志关闭
shutdown_logging()
print("Logging shutdown complete")

print("da_logger.py tests passed")
```

**预期结果**:
- loguru 日志系统正常
- 装饰器记录函数调用
- 清理函数无副作用

**失败处理**:
- 检查 `loguru` 版本兼容性
- 验证 `atexit.register()` 注册顺序
- 确认 `sys.stderr` 重定向不影响 C++ 端

### 7.4 所有 PyScripts 脚本扫描

**测试命令**:
```powershell
# PowerShell 脚本扫描所有 Python 文件
$pyFiles = Get-ChildItem -Path "src\PyScripts" -Recurse -Filter "*.py"
$pythonExe = "python"

foreach ($file in $pyFiles) {
    Write-Host "Checking $($file.FullName)..."
    & $pythonExe -m py_compile $file.FullName
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ Syntax OK" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Syntax Error" -ForegroundColor Red
    }
}
```

**预期结果**:
- 所有 Python 文件语法检查通过
- 无 Python 3.10 不兼容语法
- 无已弃用的 API 使用

**失败处理**:
- 修复语法错误
- 更新已弃用的 Python API
- 检查 f-string 表达式限制 (Python 3.10+)

---

## 8. 插件系统验证

**类别**: 功能  
**优先级**: 中

### 8.1 插件加载验证

**测试命令**:
```cpp
#include "DAPluginManager.h"
#include "DAAbstractNodePlugin.h"
#include <QDebug>

int main() {
    DA::DAPluginManager manager;
    
    // 扫描插件目录
    manager.scanPluginDirectory("plugins");
    qDebug() << "Plugins found:" << manager.pluginCount();
    
    // 加载插件
    bool loaded = manager.loadPlugin("DataAnalysis");
    qDebug() << "DataAnalysis plugin loaded:" << loaded;
    
    // 获取节点工厂
    auto factory = manager.getNodeFactory("DataAnalysisNode");
    qDebug() << "Factory valid:" << (factory != nullptr);
    
    return 0;
}
```

**预期结果**:
- 插件扫描成功
- 插件加载无错误
- 节点工厂可访问

**失败处理**:
- 检查插件 IID 定义
- 验证 `Q_PLUGIN_METADATA` 宏
- 确认 Qt 插件系统初始化

### 8.2 插件节点创建

**测试命令**:
```cpp
#include "DAAbstractNodeFactory.h"

// 假设已加载 DataAnalysis 插件
DA::DAAbstractNodeFactory* factory = ...;

// 创建节点
auto node = factory->create("DataAnalysisNode");
if (node) {
    qDebug() << "Node created:" << node->metaObject()->className();
    
    // 测试节点执行
    bool result = node->exec();
    qDebug() << "Node exec result:" << result;
} else {
    qDebug() << "Failed to create node";
}
```

**预期结果**:
- 节点创建成功
- `exec()` 方法返回正确
- 无内存泄漏

**失败处理**:
- 检查插件与主程序版本兼容性
- 验证节点元数据注册
- 确认 pybind11 模块已初始化

---

## 9. 自动化测试套件验证

**类别**: 回归  
**优先级**: 高

### 9.1 DADataFrameTest

**测试命令**:
```powershell
# 注意：需要更新测试代码中的 Python 路径
# 修改 src/tst/DADataFrameTest/main.cpp:
# 从 Py_SetPythonHome(L"C:\Python37");
# 改为 Py_SetPythonHome(L"C:\Python310");

# 构建测试
cmake --build build --config Release --target DADataFrameTest

# 运行测试
.\build\bin\DADataFrameTest.exe
```

**预期结果**:
- 测试编译成功
- CSV 文件读取成功
- DataFrame 操作正常
- 无崩溃

**失败处理**:
- **必须**更新 `main.cpp` 中的硬编码 Python 路径
- 检查测试数据文件路径
- 验证 `DAPandas` 包装器

### 9.2 DASharedTest

**测试命令**:
```powershell
# 构建测试
cmake --build build --config Release --target DASharedTest

# 运行测试
.\build\bin\DASharedTest.exe
```

**预期结果**:
- 智能指针测试通过
- 线程安全测试通过
- 无内存泄漏

**失败处理**:
- 检查 `QSharedPointer` 行为变化
- 验证跨线程信号槽

### 9.3 MessageHandle 测试

**测试命令**:
```powershell
# 构建测试
cmake --build build --config Release --target MessageHandle

# 运行测试
.\build\bin\MessageHandle.exe
```

**预期结果**:
- 消息处理正常
- 日志系统正常
- 异常捕获正常

**失败处理**:
- 检查 spdlog 版本兼容性
- 验证日志文件路径

---

## 10. 回归风险清单

**类别**: 回归  
**优先级**: 高

### 10.1 高风险功能点

以下功能点在 Python 3.10 升级后最可能出现问题，需重点测试：

| 风险项 | 描述 | 测试重点 | 负责人 |
|--------|------|----------|--------|
| **PyConfig API** | Python 3.8+ 使用新 API，3.11+ 弃用 `Py_SetPythonHome` | `DAPyInterpreter.cpp` 版本分支逻辑 | C++ 开发 |
| **typing_extensions** | Python 3.10 内置 `Literal`，需移除依赖 | `dataframe.py` 导入语句，`requirements.txt` | Python 开发 |
| **pandas._typing** | 内部 API 可能变更 | `dataframe.py` 类型注解 | Python 开发 |
| **numpy 2.0** | numpy 2.0 有破坏性变更 | 锁定 numpy < 2.0，测试所有数组操作 | Python 开发 |
| **DADataFrameTest** | 硬编码 Python 3.7 路径 | 更新测试代码路径 | 测试开发 |
| **线程模型** | Python GIL 与 Qt 线程交互 | `DAPythonSignalHandler` 跨线程回调 | C++ 开发 |
| **日志系统** | loguru 与 Python 关闭顺序 | `atexit.register()` 清理函数 | Python 开发 |
| **插件 ABI** | pybind11 版本变化影响 ABI | 插件加载和节点创建 | C++ 开发 |

### 10.2 中风险功能点

| 风险项 | 描述 | 测试重点 |
|--------|------|----------|
| **Qt5/Qt6 兼容** | Python 升级可能影响 Qt 版本选择 | 宏 `QT_VERSION_CHECK` |
| **文件编码** | Windows 中文路径处理 | `QString` 到 Python 字符串转换 |
| **异常处理** | pybind11 异常翻译 | `error_already_set` 捕获 |
| **内存管理** | Python 对象生命周期 | `shared_ptr` 和 Python GC 交互 |

### 10.3 低风险功能点

| 风险项 | 描述 | 备注 |
|--------|------|------|
| **纯 C++ 模块** | 不依赖 Python 的模块 | DAGraphicsView, DACommonWidgets 等 |
| **UI 界面** | 纯 Qt 界面代码 | 不受 Python 升级影响 |
| **配置文件** | JSON/XML 配置读取 | 使用 Qt 原生 API |

---

## 附录 A：快速验证脚本

### A.1 Python 环境验证脚本

```python
# verify_python_env.py
import sys
import subprocess

def main():
    print("=" * 60)
    print("Python Environment Verification")
    print("=" * 60)
    
    # Python version
    print(f"\n1. Python Version: {sys.version}")
    assert sys.version_info >= (3, 10), "Python version must be >= 3.10"
    
    # Required packages
    packages = [
        'numpy', 'pandas', 'scipy', 'loguru', 'openpyxl',
        'chardet', 'PyWavelets', 'pyarrow', 'matplotlib', 'seaborn',
        'langgraph', 'langchain-openai'
    ]
    
    print("\n2. Package Versions:")
    for pkg in packages:
        try:
            module = __import__(pkg)
            version = getattr(module, '__version__', 'N/A')
            print(f"   ✓ {pkg}: {version}")
        except ImportError as e:
            print(f"   ✗ {pkg}: NOT INSTALLED ({e})")
    
    # typing_extensions should not be needed
    print("\n3. Typing Module:")
    try:
        from typing import Literal
        print("   ✓ Literal from typing (Python 3.10+)")
    except ImportError:
        print("   ✗ Literal not available in typing")
    
    try:
        from typing_extensions import Literal as LiteralExt
        print("   ⚠ typing_extensions still needed (should be removed)")
    except ImportError:
        print("   ✓ typing_extensions not installed (correct for 3.10+)")
    
    # DAWorkbench import
    print("\n4. DAWorkbench Import:")
    try:
        import DAWorkbench
        print("   ✓ DAWorkbench imported")
        print(f"   - Path: {DAWorkbench.__file__}")
    except ImportError as e:
        print(f"   ✗ DAWorkbench import failed: {e}")
    
    print("\n" + "=" * 60)
    print("Verification Complete")
    print("=" * 60)

if __name__ == '__main__':
    main()
```

### A.2 CMake 配置验证脚本

```powershell
# verify_cmake_config.ps1
param(
    [string]$BuildDir = "build"
)

Write-Host "=" * 60
Write-Host "CMake Configuration Verification"
Write-Host "=" * 60

# Check build directory
if (Test-Path $BuildDir) {
    Write-Host "`n1. Build Directory: EXISTS" -ForegroundColor Green
    
    # Check CMakeCache.txt
    $cacheFile = Join-Path $BuildDir "CMakeCache.txt"
    if (Test-Path $cacheFile) {
        Write-Host "2. CMakeCache.txt: EXISTS" -ForegroundColor Green
        
        # Extract Python info
        Write-Host "`n3. Python Configuration:"
        Select-String -Path $cacheFile -Pattern "Python3_" | ForEach-Object {
            Write-Host "   $($_.Line)"
        }
    } else {
        Write-Host "2. CMakeCache.txt: NOT FOUND" -ForegroundColor Red
    }
} else {
    Write-Host "1. Build Directory: NOT FOUND" -ForegroundColor Red
    Write-Host "Run CMake configuration first" -ForegroundColor Yellow
}

Write-Host "`n" + "=" * 60
Write-Host "Verification Complete"
Write-Host "=" * 60
```

---

## 附录 B：问题排查指南

### B.1 常见问题速查

**问题 1**: CMake 找不到 Python 3.10

**症状**:
```
CMake Error at cmake/daworkbench_3rdparty.cmake:167 (find_package):
  Could not find a package configuration file provided by "Python3"
```

**解决**:
```powershell
# 显式指定 Python 路径
cmake -S . -B build `
    -DPython3_ROOT_DIR="C:\Python310" `
    -DCMAKE_TOOLCHAIN_FILE="..."
```

**问题 2**: pybind11 模块导入失败

**症状**:
```
ImportError: ModuleNotFoundError: No module named 'da_app'
```

**解决**:
1. 检查 `PYBIND11_EMBEDDED_MODULE(da_app)` 宏是否执行
2. 确认 `DAPyInterpreter` 已初始化
3. 添加 `sys.path.append()` 到模块目录

**问题 3**: Python 3.11+ `Py_SetPythonHome` 弃用警告

**症状**:
```
DeprecationWarning: Py_SetPythonHome is deprecated in Python 3.11+
```

**解决**:
- 更新 `DAPyInterpreter.cpp` 版本判断阈值
- 从 `PY_VERSION_HEX >= 0x030B0000` 改为 `PY_VERSION_HEX >= 0x030A0000`

**问题 4**: pandas 类型警告

**症状**:
```
FutureWarning: pandas._typing is deprecated
```

**解决**:
- 更新 `dataframe.py` 中的类型导入
- 使用公共 API 替代内部 API

**问题 5**: 测试代码硬编码路径

**症状**:
```
Py_SetPythonHome(L"C:\Python37")  // 导致使用旧版本
```

**解决**:
- 更新 `src/tst/DADataFrameTest/main.cpp`
- 使用配置文件或环境变量指定路径

---

## 附录 C：版本兼容性矩阵

| 组件 | Python 3.7 | Python 3.8 | Python 3.9 | Python 3.10 | Python 3.11 | Python 3.12 |
|------|-----------|-----------|-----------|------------|------------|------------|
| **pybind11 3.0.3** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **pandas 2.2.3** | ✗ | △ | △ | ✓ | ✓ | ✓ |
| **numpy 1.26.x** | △ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **numpy 2.0.x** | ✗ | ✗ | ✗ | △ | △ | ✓ |
| **typing_extensions** | 必需 | 可选 | 可选 | 不需 | 不需 | 不需 |
| **PyConfig API** | ✗ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Py_SetPythonHome** | ✓ | ✓ | ✓ | ✓ | 弃用 | 弃用 |

**图例**:
- ✓ = 完全支持
- △ = 部分支持 (需注意)
- ✗ = 不支持
- 不需 = 不需要此依赖

---

## 验证完成检查表

完成所有验证后，请确认以下项目：

- [ ] 所有构建验证项通过
- [ ] Python 解释器初始化正常
- [ ] 所有 Python 模块导入成功
- [ ] pandas/numpy/scipy 功能测试通过
- [ ] 工作流引擎创建和执行正常
- [ ] 图表模块绘图和导出正常
- [ ] 所有 PyScripts 脚本语法检查通过
- [ ] 插件系统加载和创建节点正常
- [ ] 自动化测试套件全部通过
- [ ] 回归风险项已测试
- [ ] `requirements.txt` 已更新 (移除 `typing_extensions`)
- [ ] 测试代码路径已更新 (`DADataFrameTest/main.cpp`)
- [ ] 文档已更新 (本清单标记为完成)

**验证人**: _______________  
**验证日期**: _______________  
**Python 版本**: _______________  
**Qt 版本**: _______________  
**构建类型**: □ Debug  □ Release  □ RelWithDebInfo

---

*文档结束*
