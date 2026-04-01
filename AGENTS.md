# data-workbench 项目指南

## 项目概述

data-workbench 是一个数据工作流设计器，旨在实现工作流驱动的数据 ETL（提取、转换、加载），集成 pandas 的数据处理能力，实现高效的交互式数据可视化，并能固定输出论文级别的图片。

### 核心功能

- **工作流驱动**：通过有向图描述工作流，解决固定流程的数据处理问题
- **数据处理**：集成 pandas 核心功能，提供类似 Excel 的操作界面，无需编程即可使用
- **数据可视化**：交互式图表编辑，支持生成论文级别的图片
- **插件系统**：支持自定义扩展模块，集成自己的数据清洗和分析方法
- **一维仿真**：基于工作流框架，可实现类似 Amesim 的一维仿真

### 技术栈

- C++17
- Qt 5.14+ 或 Qt 6
- Python (pandas, numpy, scipy)
- 第三方库：SARibbon, Qt-Advanced-Docking-System, ctk, qwt, QtPropertyBrowser, spdlog, pybind11, ordered-map

## 目录结构

```
data-workbench/
├── .github/          # GitHub 工作流配置
├── cmake/            # CMake 配置文件
├── docs/             # 项目文档
├── plugins/          # 插件目录
├── src/              # 源码目录
│   ├── 3rdparty/     # 第三方库
│   ├── APP/          # 应用主程序
│   ├── DACommonWidgets/ # 通用 widgets
│   ├── DAData/       # 数据处理模块
│   ├── DAFigure/     # 图表模块
│   ├── DAGraphicsView/ # 图形视图模块
│   ├── DAGui/        # GUI 相关模块
├── AI_AGENT_GUID.md  # AI Agent 指南
├── CMakeLists.txt    # 主 CMake 文件
├── LICENSE           # 许可证
├── readme.md         # 项目说明
└── requirements.txt  # Python 依赖
```

### 核心模块说明

1. **APP**：应用主程序，包含主窗口、控制器、项目管理等核心功能
2. **DAData**：数据处理模块，负责数据管理、数据结构定义等
3. **DAFigure**：图表模块，负责图表绘制、编辑和导出
4. **DAGui**：GUI 相关模块，包含各种界面组件和工具
5. **DAGraphicsView**：图形视图模块，提供图形编辑功能
6. **DACommonWidgets**：通用 widgets，提供基础 UI 组件

## 开发环境

### 编译依赖

- CMake 3.16+
- C++17 兼容编译器
- Qt 5.14+ 或 Qt 6
- Python 3.7+

## Qt 集成方案

### 1. 信号槽设计

充分发挥 Qt 的信号槽机制，工具类使用信号和槽进行事件通讯。

### 2. 属性暴露方式

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

## 注释与文档规范

### 代码风格

- 严格保持与现有代码一致（命名规范、缩进、头文件组织等）
- 代码文件、类名统一`DA`开头，并放入DA命名空间
- 遵循 Qt 开发最佳实践（使用 `Q_PROPERTY`、`Q_SIGNALS`、`Q_SLOT` 等宏，禁止使用 `slot`、`signal` 等小写命名的宏）

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

## 编译构建

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

## 注意事项

- QwtPlotItem相关的类不继承QObject，不要使用Qt的信号槽机制，继承 Qwt 非 QObject 类时**不能使用 Q_OBJECT 宏**
- 项目使用PIMPL模式，PIMPL相关宏定义在`src/DAGlobals.h`中,主要有如下宏需要使用：
  - `DA_DECLARE_PRIVATE`:在`MyClass`中定义
  - `DA_DECLARE_PUBLIC`:在`MyClass::PrivateData`中声明
  - `DA_PIMPL_CONSTRUCT`:在`MyClass::在MyClass`构造函数中初始化
  - `DA_D`:在`MyClass::fun()`中获取`MyClass::PrivateData`的指针
  - `DA_DC`:在`MyClass::fun() const`中获取`MyClass::PrivateData`的const指针
