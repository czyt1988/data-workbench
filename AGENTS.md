# data-workbench 项目指南

## 项目概述

data-workbench 是基于有向图工作流引擎的AI Agent驱动的数据分析平台。C++/Qt 桌面应用，内嵌 Python，集成 pandas 数据处理与交互式可视化。
三大板块：工作流（WorkFlow）、数据处理（Data）、图表（Chart），模块间通过 Python 双向互通。

## 技术栈

- **C++17**，**Qt 5.14+ / Qt 6**（必须兼容两者）
- Python 3.7+（pandas, numpy, scipy, matplotlib）
- 第三方库（submodule 管理）：SARibbon, Qt-Advanced-Docking-System, DALiteCtk(精简 ctk), qwt, QtPropertyBrowser, spdlog, pybind11, ordered-map

## 目录结构与模块依赖

所有模块在 `DA` 命名空间下，类名 `DA` 前缀。

### src/ 完整模块列表（编译顺序 = 依赖顺序）

| 模块 | 说明 | 关键依赖 |
|------|------|---------|
| `DAShared` | 共享定义/全局宏 | 无 |
| `DAUtils` | 工具类（XML, CSV, 翻译, 树结构） | DAShared |
| `DAAxOfficeWrapper` | Windows Office 互操作 | DAUtils（仅 Win32） |
| `DAMessageHandler` | 日志/消息处理（基于 spdlog） | DAUtils |
| `DAPyBindQt` | Python-C++ 绑定（pybind11） | Qt + Python（DA_ENABLE_PYTHON） |
| `DAPyScripts` | Python 脚本引擎封装 | DAPyBindQt |
| `DAPyCommonWidgets` | Python 相关 GUI 组件 | DAPyBindQt |
| `DAData` | 数据管理（DataFrame/Series 封装） | DAUtils + Python模块 |
| `DACommonWidgets` | 通用 UI 组件（属性面板、笔刷选择器） | DAUtils |
| `DAGraphicsView` | 图形视图框架（场景/图元基类） | DAUtils |
| `DAWorkFlow` | 工作流引擎（节点、有向图、执行器） | DAGraphicsView |
| `DAFigure` | 图表模块（基于 qwt 的 FigureWidget） | — |
| `DAGui` | 高级 GUI（工作流操作面板、图表管理、存档） | — |
| `DAInterface` | 抽象接口层（插件与主程序间的 API） | DAGui |
| `DAPluginSupport` | 插件框架（DAAbstractPlugin / DAAbstractNodePlugin） | DAInterface |
| `APP` | 可执行文件入口（DAWorkBench.exe），`main.cpp` | DAPluginSupport |

### plugins/ 目录

- `DataAnalysis/`：数据处理插件（唯一已激活），实现 `DAAbstractNodePlugin`
- `plugin-template/`：插件脚手架工具（`make-plugin.py`）

## 构建系统

### 关键前提：构建顺序不可变

```
1. 拉取 submodule     git submodule update --init --recursive
2. 编译第三方库        cd src/3rdparty && cmake + build + install
3. 编译主项目          cmake + build（自动 install 到 bin/ 目录）
4. 编译插件（可选）    依赖主项目已 install
```

### 构建命令

**必须指定 Qt 工具链文件**，否则 Windows SDK 头文件找不到（当前 build 目录已存在，使用 MSVC 生成）：

```powershell
# 配置（如果不存在 build 目录）
cmake -S . -B build -G Ninja `
    -DCMAKE_BUILD_TYPE:STRING=Release `
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH="D:\Qt\6.7.3\msvc2019_64\lib\cmake\Qt6\qt.toolchain.cmake" `
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE

# 构建
cmake --build build --config Release --parallel

# 构建单个模块
cmake --build build --config Release --target DAFigure
```

### 重要 CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `DA_ENABLE_PYTHON` | ON | 开启 Python 绑定（决定 DAData 等模块的 Python 依赖） |
| `DA_BUILD_PLUGINS` | ON | 是否编译 plugins/ |
| `DA_AUTO_INSTALL_PREFIX` | ON | 自动安装到本地 bin/ 目录 |
| `DA_AUTO_GENERATE_CONFIG_INFO` | OFF | 自动生成 `DAConfigs.h`（仅库开发者开启） |

### DAConfigs.h

`src/DAConfigs.h` 是 CMake 从 `src/DAConfigs.h.in` **自动生成**的，不要手动编辑。包含版本号、编译日期、`DA_ENABLE_PYTHON` 等构建时宏。

### CMake 工具宏

- `cmake/daworkbench_utils.cmake`：`damacro_app_setting`、`damacro_import_*`、`dafun_set_bin_name`
- `cmake/daworkbench_3rdparty.cmake`：第三方库 find_package 封装
- `cmake/daworkbench_plugin_utils.cmake`：插件构建辅助

## 代码规范

### PIMPL 模式（强制，来自 `src/DAGlobals.h`）

项目使用自研 PIMPL 宏，基于 `std::unique_ptr`，**不是** Qt 的 `Q_DECLARE_PRIVATE`：

```cpp
// MyClass.h
class MyClass {
    DA_DECLARE_PRIVATE(MyClass)   // 声明 PrivateData + d_ptr + d_func()
public:
    MyClass();
    void doSomething();
};

// MyClass.cpp
class MyClass::PrivateData {
    DA_DECLARE_PUBLIC(MyClass)    // 声明 q_ptr + q_func()
public:
    int value;
};
MyClass::MyClass() : DA_PIMPL_CONSTRUCT {}  // 初始化 d_ptr

void MyClass::doSomething() {
    DA_D(d);      // PrivateData* d = d_func()
    d->value = 1;
}
```

常用宏速查：`DA_DECLARE_PRIVATE` → `DA_DECLARE_PUBLIC` → `DA_PIMPL_CONSTRUCT` → `DA_D`/`DA_DC` → `DA_Q`/`DA_QC`

### 命名与 Qt 兼容

- 类名 `DA` 前缀，`DA` 命名空间
- 使用 `Q_SIGNALS`、`Q_SLOTS`（非小写 `signal`/`slot`）
- 属性用 `Q_PROPERTY` 暴露
- Qt5/Qt6 兼容：用 `#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)` 宏区分，`DAGlobals.h` 提供 `Qt5Qt6Compat_*` 系列兼容宏

### clang-format

项目根目录有 `.clang-format`（基于 WebKit 风格，4 空格缩进，120 列宽，`PointerAlignment: Left`）。提交前应格式化。

### 注释规范（Doxygen + 中文）

- `.cpp` 文件写函数 Doxygen 块（`@brief`、`@param`、`@return`），中文内容
- `.h` 文件仅写**单行** `//` 简要注释在 public 函数旁；**不要**在头文件写成员函数 Doxygen 块
- 例外：类的 Doxygen、信号的 Doxygen、枚举的 Doxygen 可以在 `.h` 中

### 关键禁忌

- **QwtPlotItem 继承链不继承 QObject → 不能用 `Q_OBJECT` 宏、不能使用信号槽**
- **不要写 `as any`、`@ts-ignore` 等类型规避**（C++ 中对应 `static_cast` 也需谨慎）
- `DA_AUTO_REGISTER_META_TYPE` 宏只能写在 `.cpp` 文件中，且必须在命名空间外

## 插件系统

插件继承链：`DAAbstractPlugin`（基础接口）→ `DAAbstractNodePlugin`（工作流节点插件）

创建插件的关键步骤：
1. 类继承 `QObject, public DA::DAAbstractNodePlugin`（QObject 必须是第一个基类）
2. 头文件加入 `Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)` 和 `Q_INTERFACES(DA::DAAbstractNodePlugin)`
3. 实现 `getName()`, `getIID()`, `getVersion()`, `getDescription()`, `createNodeFactory()`, `initialize()`

参考：`plugins/DataAnalysis/DataAnalysisPlugin.h`

## 测试

测试位于 `src/tst/`，使用 CTest：

```powershell
# 从 build/src/tst/ 运行
cd build/src/tst
ctest --output-on-failure

# 或构建自定义目标
cmake --build build --target run_all_tests
```

## 国际化

翻译文件 `src/i18n/da_zh_CN.ts` / `da_en_US.ts`。Qt 6.2+ 使用分离 API：
- 更新 `.ts`（手动）：`cmake --build build --target update_translations`
- 生成 `.qm`（自动随构建）：`da_translations` 目标

## 相关文档

| 文档 | 何时查阅 |
|------|---------|
| 项目主页 | https://czyt1988.github.io/data-workbench |
| `docs/Doxyfile-wiki-cn` | C++ API 参考 |
| `submodule.md` | 第三方库拉取、更新、源切换 |

### 关键开发指引（`docs/zh/dev-guide/`）

按任务查表，无需遍历 25 个文件：

| 任务 | 文档 | 要点 |
|------|------|------|
| 创建/修改工作流节点 | `workflow-lifecycle.md` | 节点创建→连接→执行→移除四大回调 |
| 开发插件 | `plugin-architecture.md`、`plugin-project-create.md`、`plugins-interfaces.md` | DACoreInterface 入口、插件项目脚手架 |
| 调试嵌入式 Python | `embedded-python-debugging.md` | `.pyi` stub 代码提示、mock 验证、debugpy 远程调试 |
| 保存/加载项目 | `project-serialization-architecture.md` | ZIP 存档结构、多线程任务队列 |
| 创建设置/配置窗口 | `settingwidget-standard.md` | QPointer 生命周期、6 个标准函数 |
| 操作图表（Figure/Chart/Item） | `figure-abstract.md` | 三层模型与 undo/redo 集成 |
| Python/C++ 互调 | `python-in-cpp.md` | 导航入口（含 GIL、绑定开发、故障排除等 5 个子文档） |
| 命名、调试宏等细节 | `coding-standard.md` | `mFoo`（私成）、`s_`（静态）、`g_`（全局）命名 |
