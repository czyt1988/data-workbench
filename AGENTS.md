# data-workbench 项目指南

**Generated:** 2026-04-28
**Commit:** `9676dc3`
**Branch:** `workflow-rebuild`

## OVERVIEW

AI Agent 驱动的数据分析工作平台 — C++17/Qt 有向图工作流引擎 + 内嵌 Python (pandas/numpy) + 交互式图表。核心栈: Qt 5.14+/6, pybind11, SARibbon, qwt, Qt-Advanced-Docking-System。

## STRUCTURE

```
data-workbench/
├── .github/workflows/  # CI (build.yml, page.yml)
├── cmake/              # CMake 工具链 (daworkbench_utils.cmake, 3rdparty.cmake, plugin_utils.cmake)
├── docs/zh/            # 中文技术文档 (dev-guide/, use-guide/, build/)
├── plugins/            # 插件 (DataAnalysis, CrewAIAdapter, plugin-template)
├── src/
│   ├── 3rdparty/       # 第三方库 (qwt, SARibbon, qt-advanced-docking 等)
│   ├── DAShared/       # 共享基础类型 (20 files)
│   ├── DAUtils/        # 工具类 (40 files)
│   ├── DAAxOfficeWrapper/ # Windows Office 封装 (10 files, 仅 Win)
│   ├── DAMessageHandler/  # 日志/消息 (9 files)
│   ├── DAPyBindQt/     # Python↔Qt 绑定层 (28 files)
│   ├── DAPyScripts/    # Python 脚本加载 (12 files)
│   ├── DAPyCommonWidgets/ # Python 通用 Widgets (13 files)
│   ├── DAPyWorkFlow/   # Python 工作流节点 (43 files)
│   ├── DAData/         # 数据管理/结构 (22 files)
│   ├── DACommonWidgets/ # 通用 UI 组件 (82 files)
│   ├── DAGraphicsView/ # 图形视图框架 (54 files)
│   ├── DAFigure/       # 图表/Figure 容器 (103 files)
│   ├── DAGui/          # GUI Widgets/面板/对话框 (326 files, 最大模块)
│   │   ├── ChartSetting/  # 图表属性设置面板
│   │   ├── Commands/      # QUndoCommand 子类
│   │   ├── Dialog/        # 各种对话框
│   │   ├── MimeData/      # 拖放 MIME 数据
│   │   └── Models/        # Qt Model/View 模型
│   ├── DAInterface/    # 核心接口定义 (28 files)
│   ├── DAPluginSupport/ # 插件框架 (10 files)
│   ├── APP/            # 应用主程序 (190 files)
│   │   ├── Dialog/     # 应用级对话框
│   │   ├── Icon/       # 图标资源
│   │   ├── PythonBinding/ # Python 绑定初始化
│   │   └── SettingPages/  # 设置页面
│   ├── PyScripts/      # 内置 Python 脚本 (23 files)
│   ├── i18n/           # 国际化翻译
│   └── tst/            # 测试 (7 files)
├── CMakeLists.txt      # 顶层 CMake (项目名 DAWorkbench)
└── requirements.txt    # Python 依赖
```

### 模块构建依赖顺序

```
DAShared → DAUtils → DAAxOfficeWrapper(win) → DAMessageHandler
→ DAPyBindQt → DAPyScripts → DAPyCommonWidgets → DAPyWorkFlow
→ DAData → DACommonWidgets → DAGraphicsView → DAFigure
→ DAGui → DAInterface → DAPluginSupport → APP
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| 程序入口 / main() | `src/APP/main.cpp` | QApplication, Python 初始化, AppMainWindow |
| 主窗口 | `src/APP/AppMainWindow.h/.cpp` | 继承 SARibbonMainWindow, 管理 Docking/UI/Controller |
| 应用核心(单例) | `src/APP/DAAppCore.h/.cpp` | 初始化所有子系统 |
| 插件管理 | `src/APP/DAAppPluginManager.h/.cpp` | 加载/卸载插件 |
| 项目管理 | `src/APP/DAAppProject.h/.cpp` | 工程文件读写 |
| 数据处理接口 | `src/DAData/` | 数据容器, DataFrame 封装 |
| 图表创建/编辑 | `src/DAFigure/`, `src/DAGui/ChartSetting/` | QwtFigure 容器 + 属性面板 |
| 图形视图交互 | `src/DAGraphicsView/` | QGraphicsView 子类, 节点/连线编辑 |
| Python 绑定 | `src/DAPyBindQt/` | pybind11 胶水代码 |
| Python 工作流节点 | `src/DAPyWorkFlow/` | Python 脚本节点执行 |
| 命令行参数 | `src/APP/main.cpp` → `initCommandLine()` | --version, --help, import-data, 工程文件 |
| UI 状态持久化 | `src/APP/AppMainWindow` | saveUIState/restoreUIState |
| 翻译/国际化 | `src/i18n/`, `src/APP/App_zh_CN.ts` | DATranslatorManeger |
| 崩溃转储 | `src/APP/DADumpCapture.h` | Windows dump 文件生成 |
| PIMPL 宏定义 | `src/DAGlobals.h` | DA_DECLARE_PRIVATE, DA_D, DA_DC 等 |
| Qt5/Qt6 兼容宏 | `src/DAGlobals.h` | Qt5Qt6Compat_* 系列宏 |
| 通用 Widgets | `src/DACommonWidgets/` | 按钮、列表、树等基础组件 |
| 插件开发参考 | `plugins/DataAnalysis/` | 最完整的插件示例 |
| 插件模板 | `plugins/plugin-template/` | 新插件脚手架 |
| 文档源码 | `docs/zh/` | Doxygen Wiki 中文 |

## CODE MAP

| Symbol | Type | Location | Role |
|--------|------|----------|------|
| `DA::AppMainWindow` | class | `src/APP/AppMainWindow.h` | 主窗口, 继承 SARibbonMainWindow |
| `DA::DAAppCore` | class | `src/APP/DAAppCore.h` | 核心单例, 子系统初始化 |
| `DA::DAAppController` | class | `src/APP/DAAppController.h` | MVC 控制器 |
| `DA::DAAppUI` | class | `src/APP/DAAppUI.h` | Ribbon/Docking 布局管理 |
| `DA::DAAppPluginManager` | class | `src/APP/DAAppPluginManager.h` | 插件生命周期管理 |
| `DA::DAAppProject` | class | `src/APP/DAAppProject.h` | 工程文件序列化 |
| `DAWorkbenchFeatureType` | enum | `src/DAGlobals.h` | Workflow/Data/Chart 功能域标识 |
| `QwtFigure` | class | `src/3rdparty/qwt/` | 多绘图布局容器 (类似 matplotlib Figure) |
| `DA_DECLARE_PRIVATE` | macro | `src/DAGlobals.h` | PIMPL 私有数据声明 |
| `DA_D` / `DA_DC` | macro | `src/DAGlobals.h` | PIMPL d-pointer 访问 |
| `DA_PIMPL_CONSTRUCT` | macro | `src/DAGlobals.h` | PIMPL 构造函数初始化 |

## CONVENTIONS

### 代码风格

- 严格保持与现有代码一致（命名规范、缩进、头文件组织等）
- 代码文件、类名统一`DA`开头，并放入DA命名空间
- 遵循 Qt 开发最佳实践（使用 `Q_PROPERTY`、`Q_SIGNALS`、`Q_SLOT` 等宏，禁止使用 `slot`、`signal` 等小写命名的宏）

### 信号槽设计

充分发挥 Qt 的信号槽机制，工具类使用信号和槽进行事件通讯。

### 属性暴露方式

为了贴合 Qt 框架，类的属性使用 `Q_PROPERTY` 暴露：

```cpp
// 数据属性
Q_PROPERTY(QImAbstractDataSeries* data READ data WRITE setData NOTIFY dataChanged)

// 样式属性
Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
Q_PROPERTY(float size READ size WRITE setSize NOTIFY sizeChanged)
Q_PROPERTY(float opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)

// 可见性属性
Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibilityChanged)
```

### Qt 版本兼容性

代码需兼容Qt5和Qt6,如果一些方法在Qt6中取消了，需要使用宏来判断使用那种版本的方法，例如：

```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // 使用Qt5的方法
#else
    // 使用Qt6的方法
#endif
```

Qt5/Qt6 兼容宏定义在 `src/DAGlobals.h`（`Qt5Qt6Compat_*` 系列）

## 注释与文档规范

### 注释规范（强制）

#### 2.1 源文件（.cpp）注释规范

函数的doxygen注释写在cpp文件中，不要把函数的doxygen注释写在头文件中，以保证头文件的简洁。

cpp文件的doxygen注释示例

```cpp
/**
 * @brief [中文简要说明]
 * 
 * [空一行后，写中文详细说明]
 * 如果有代码示例，可添加代码示例在 `@code` 和 `@endcode` 之间。
 * @code
 * @endcode
 * 
 * @param[in] param_name [中文参数描述]
 * @return [中文返回值描述]
 * @note [中文备注][如有可添加]
 * @see [相关函数][如有可添加]
 */
void MyClass::myFunction(int param_name)
{
    // 函数体
}
```

**注意:**原则上详细函数注释应该写在对应的 `.cpp` 文件中，而不是头文件中。

#### 2.2 头文件（.h）注释规范

- 头文件中的 `public` 函数声明旁，添加**单行中文简要注释**（使用 `//`）。
- **不要**在头文件中写入**类成员函数**的 Doxygen 块

头文件中可写的doxygen注释包括：

- 类的 doxygen 注释
- 信号的 doxygen 注释(因为信号没有在cpp的实现内容，需要在头文件中添加)
- 枚举的 doxygen 注释
- 枚举值的 doxygen 注释

类成员函数的doxygen注释在对应的 `.cpp` 文件中，以保持头文件整洁。

头文件注释示例：

```cpp
/**
 * @brief [中文说明]
 * 
 * [空一行后，写中文详细说明]
 * 如果有代码示例，可添加代码示例在 `@code` 和 `@endcode` 之间。对于功能性较强的类，类的注释中应该加入使用示例，以便使用者了解如何使用
 * @code
 * @endcode
 * @see [相关类][如有可添加]
 */
class MyClass {
public:
    /**
     * @brief 枚举说明
     */
    enum EnumType{
        Type1 ///< 枚举的注释方式
    }
public:
    // （默认构造函数、析构函数可不用写注释）
    MyClass();

    // 中文简要说明（详细说明位于.cpp文件中）
    void myFunction(int param_name);
};
```

**注：** Qt 的信号（头文件中 Q_SIGNALS 关键字下面的函数），它没有在 cpp 中的定义，这些函数的 doxygen 注释需要在头文件中添加

## 插件系统

插件位于 `plugins` 目录下，每个插件是一个独立的模块，可以扩展软件功能。

### 插件开发

如果涉及插件开发，你可以阅读:

- [创建插件项目](./docs/zh/dev-guide/plugin-project-create.md)
- [插件与接口](./docs/zh/dev-guide/plugins-interfaces.md)
- [插件模块DAPluginSupport](./docs/zh/dev-guide/plugin-module.md)
- [插件开发创建 UI](./docs/zh/dev-guide/plugin-dev-create-ui.md)

- 参考现有的插件结构（如 DataAnalysis 插件）
- 遵循项目的代码规范和架构设计
- 确保插件与主程序的接口兼容性

## Git 提交规范

在完成当前任务后，需提交所有更改到 Git 仓库。
创建有意义的提交信息保证下次任务能清楚了解这次任务的实现情况
提交信息最好包含以下信息：

- 任务类型（例如：实现、修复、文档更新）
- 实现内容的简要描述
- 相关文件列表
- 关联到计划书（如果适用）

## COMMANDS

项目使用cmake构建，如果项目目录下存在build目录，说明已经生成过，直接在此目录下编译即可

构建此项目**必须**使用 Qt 工具链文件，否则会出现 Windows SDK 头文件找不到的问题

下面是构建参考：

```powershell
# 正确的配置命令
cmake -S . -B build -G Ninja `
    -DCMAKE_BUILD_TYPE:STRING=Release `
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE `
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH="D:\Qt\6.7.3\msvc2019_64\lib\cmake\Qt6\qt.toolchain.cmake" `
    -DQT_QML_GENERATE_QMLLS_INI:STRING=ON `
    "-DCMAKE_CXX_FLAGS_DEBUG_INIT:STRING=-DQT_QML_DEBUG -DQT_DECLARATIVE_DEBUG" `
    "-DCMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT:STRING=-DQT_QML_DEBUG -DQT_DECLARATIVE_DEBUG"
```

### 参数说明

| 参数 | 说明 |
|------|------|
| `-DCMAKE_TOOLCHAIN_FILE` | **必须**指定 Qt 工具链文件，否则无法正确找到 Windows SDK |
| `-DCMAKE_BUILD_TYPE` | 构建类型：`Debug` 或 `Release` |
| `-G Ninja` | 使用 Ninja 生成器 |

### 构建命令

```powershell
# 构建项目
cmake --build build --config Release --parallel

# 构建并安装
cmake --build build --config Release --target install

# 仅构建特定目标
cmake --build build --config Release --target DAFigure
```

## ANTI-PATTERNS (THIS PROJECT)

- QwtPlotItem相关的类不继承QObject，不要使用Qt的信号槽机制，继承 Qwt 非 QObject 类时**不能使用 Q_OBJECT 宏**
- 禁止使用 `slot`、`signal` 小写命名的宏，统一使用 `Q_SLOTS`、`Q_SIGNALS`
- 禁止在头文件中写入类成员函数的 Doxygen 块注释（仅限类的注释、信号注释、枚举注释）
- 禁止在 QwtPlotItem 子类中使用信号槽

## UNIQUE STYLES

- 项目使用PIMPL模式，PIMPL相关宏定义在`src/DAGlobals.h`中,主要有如下宏需要使用：
  - `DA_DECLARE_PRIVATE`:在`MyClass`中定义
  - `DA_DECLARE_PUBLIC`:在`MyClass::PrivateData`中声明
  - `DA_PIMPL_CONSTRUCT`:在`MyClass::在MyClass`构造函数中初始化
  - `DA_D`:在`MyClass::fun()`中获取`MyClass::PrivateData`的指针
  - `DA_DC`:在`MyClass::fun() const`中获取`MyClass::PrivateData`的const指针
- 所有类、文件名统一 `DA` 前缀，放入 `DA` 命名空间
- Doxygen 注释使用中文
- 头文件保持简洁：仅单行中文注释，详细文档在 .cpp 中

## NOTES

- Python 解释器路径通过 `DAPyInterpreter::getPythonInterpreterPath()` 自动检测
- 第三方库 (SARibbon, qwt, Qt-Advanced-Docking-System 等) 需先通过 `src/3rdparty/CMakeLists.txt` 编译安装
- `src/3rdparty/qwt/` 是 Qwt 7.x 维护分支, 有自己的 AGENTS.md
- CI 通过 `.github/workflows/build.yml` 自动构建
- 崩溃转储 (.dmp) 生成在 `dumps/` 目录，由 `DADumpCapture` 管理
- 翻译文件通过 CMake option `DA_ENABLE_AUTO_TRANSLATE` 自动生成
