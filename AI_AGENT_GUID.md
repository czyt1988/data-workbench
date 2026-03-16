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

### 第三方库
项目使用 git submodule 管理大部分第三方库，需要执行：
```shell
git submodule update --init --recursive
```

需要编译的第三方库：
- SARibbon
- Qt-Advanced-Docking-System
- ctk (精简版)
- qwt
- QtPropertyBrowser
- spdlog
- pybind11
- ordered-map

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

### 1️. 代码风格
- 严格保持与现有代码一致（命名规范、缩进、头文件组织等）
- 遵循 Qt 开发最佳实践（使用 `Q_PROPERTY`、`Q_SIGNALS`、`Q_SLOT` 等宏，禁止使用 `slot`、`signal` 等小写命名的宏）

### 2️. 注释规范（强制）

#### 2.1 源文件（.cpp）注释规范

所有新增代码必须使用 **Doxygen 格式**，并区分中英文：

```cpp
/**
 * \if ENGLISH
 * @brief [English brief description]
 * @param[in] param_name [English parameter description]
 * @return [English return value description]
 * @details [English detailed explanation]
 * \endif
 * 
 * \if CHINESE
 * @brief [中文简要说明]
 * @param[in] param_name [中文参数描述]
 * @return [中文返回值描述]
 * @details [中文详细说明]
 * \endif
 */
```

#### 2.2 头文件（.h）注释规范

- 头文件中的 `public` 函数声明旁，仅添加**单行英文简要注释**（使用 `//` 或简洁的 `/** */`）。
- **不要**在头文件中写入详细的双语 Doxygen 块（类的 doxygen 注释除外、信号的 doxygen 注释除外），详细内容应保留在对应的 `.cpp` 文件中，以保持头文件整洁。
- 示例：

```cpp
// 类的注释规范建见下一节
class MyClass {
public:
    // Constructor for MyClass (English only)
    MyClass(); 
};
```

- 但有些特例，例如 Qt 的信号（头文件中 Q_SIGNALS 关键字下面的函数），它没有在 cpp 中的定义，这些函数的 doxygen 注释需要在头文件中按上面中英文要求添加，你需要把信号的 doxygen 注释转换为中英双语。
- 另外类的 doxygen 注释也需要在头文件中按上面中英文要求添加。

#### 2.3 类的 doxygen 注释规范

- 类的 doxygen 注释需要在头文件中按上面中英文要求添加。
- 示例：

```cpp
/**
 * \if ENGLISH
 * @brief [English description]
 * \endif
 *
 * \if CHINESE
 * @brief [中文说明]
 * \endif
 */
class MyClass {
public:
    // Constructor for MyClass (English only)
    MyClass(); 
};
```

- 对于功能性较强的类，类的注释中应该加入使用示例，以便使用者了解如何使用

## 插件系统

插件位于 `plugins` 目录下，每个插件是一个独立的模块，可以扩展软件功能。

### 插件开发
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

## 测试与调试

### 测试策略
- 确保新功能有相应的测试用例
- 测试数据处理和图表生成的正确性
- 验证工作流执行的可靠性

### 调试技巧
- 使用 Qt 的调试工具
- 利用 spdlog 进行日志记录
- 检查 Python 集成的错误信息

## 常见问题与解决方案

### 编译问题
- 确保所有第三方库已正确拉取和编译
- 检查 Qt 版本兼容性
- 验证 CMake 配置是否正确

### 运行问题
- 检查 Python 环境和依赖
- 验证插件加载是否正常
- 查看日志文件了解错误详情

## 项目文档

项目文档位于 `docs` 目录，包括：
- 构建指南
- 开发指南
- 使用指南
- API 文档

详细文档可访问：[https://czyt1988.github.io/data-workbench](https://czyt1988.github.io/data-workbench)