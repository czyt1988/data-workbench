# 插件开发全流程

本指南详细说明从创建插件项目到实现完整功能的全部流程，帮助开发者快速开发高质量的 DAWorkBench 插件。

## 主要功能特性

**特性**

- ✅ **创建新插件**：模板生成工具和手动创建两种方式
- ✅ **CMake 配置**：根目录和 src 目录的完整 CMakeLists.txt 配置
- ✅ **插件主类实现**：MyPlugin 类的头文件和实现文件详解
- ✅ **节点工厂实现**：MyNodeFactory 类的节点创建和元数据注册
- ✅ **工作节点实现**：MyWorker 类的执行逻辑和图元创建
- ✅ **内置服务使用**：日志、配置、数据管理等服务的调用方法
- ✅ **构建和安装**：完整的构建流程和验证方法

---

## 创建新插件

### 方式一：使用模板生成工具

!!! tip "推荐方式"
    使用 `plugins/plugin-template/make-plugin.py` 自动生成插件项目，无需手动创建文件结构。

1. 配置 `template.json`：

下面的 JSON 示例展示了模板配置文件的格式：

```json
{
    "plugin-base-name": "My",                           # 插件基础名称
    "plugin-display-name": "My Plugin",                 # 插件显示名称
    "plugin-description": "This is My Plugin for DAWorkbench",  # 插件描述
    "plugin-iid": "Plugin.MyPlugin",                    # 插件唯一标识符
    "factory-prototypes": "My.Factory",                 # 节点工厂原型前缀
    "factory-name": "My Factory",                       # 节点工厂名称
    "factory-description": "My Plugin Node Factory"     # 节点工厂描述
}
```

上述配置定义了插件的基本信息，脚本将根据此配置生成完整的插件项目。

2. 运行生成脚本：

下面的命令展示了如何运行模板生成脚本：

```bash
cd plugins/plugin-template
python make-plugin.py
```

脚本将在上级目录生成完整的插件项目结构，包含所有必需的文件和目录。

### 方式二：手动创建

#### 项目目录结构

下面的目录结构展示了标准插件项目的组织方式：

```text
MyPlugin/
├── CMakeLists.txt            # 插件构建配置文件 - 定义编译选项和依赖
├── src/
│   ├── MyPlugin.h            # 插件主类头文件 - 插件入口类定义
│   ├── MyPlugin.cpp          # 插件主类实现 - 初始化和生命周期管理
│   ├── MyNodeFactory.h       # 节点工厂头文件 - 节点创建和管理
│   ├── MyNodeFactory.cpp     # 节点工厂实现 - 节点元数据注册
│   ├── MyWorker.h            # 工作节点头文件 - 数据处理逻辑
│   ├── MyWorker.cpp          # 工作节点实现 - exec() 方法实现
│   ├── MyResource.qrc        # Qt 资源文件 - 图标、翻译等资源
│   └── icon/
│       └── my-icon.png       # 节点图标 - 显示在节点列表中
└── data-workbench/           # 主项目子模块引用 - 确保版本兼容
```

上述结构将插件功能按职责分离，便于代码维护和团队协作。

#### 添加 data-workbench 子模块

下面的命令展示了如何添加主项目作为子模块：

```bash
cd MyPlugin
git submodule add https://gitee.com/czyt1988/data-workbench.git ./data-workbench
git submodule update --init --recursive
```

添加主项目子模块确保插件能正确引用主程序提供的接口和库。

---

## CMake 配置

### 根目录 CMakeLists.txt

根目录 CMake 文件负责查找 Qt、指定 data-workbench 安装目录、引入辅助工具等基础配置。

下面的 CMake 示例展示了根目录配置文件的完整内容：

```cmake
cmake_minimum_required(VERSION 3.5)
project(MyPlugin)

# 系统位数判断 - 用于生成安装目录名称
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(_platform_name "x86")
else()
    set(_platform_name "x64")
endif()

# Qt 依赖 - 自动选择 Qt6 或 Qt5
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

# 指定 data-workbench 安装目录 - 必须与主程序安装目录一致
set(DAWorkbench_INSTALL_FOLDER_NAME "bin_${CMAKE_BUILD_TYPE}_qt${QT_VERSION}_${CMAKE_CXX_COMPILER_ID}_${_platform_name}")
set(DAWorkbench_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/data-workbench/${DAWorkbench_INSTALL_FOLDER_NAME}")
set(DAWorkbench_DIR "${DAWorkbench_INSTALL_DIR}/lib/cmake/DAWorkbench")

# 引入辅助工具 - 使用主项目提供的 cmake 宏
list(APPEND CMAKE_MODULE_PATH ${DAWorkbench_DIR})
include(${DAWorkbench_DIR}/daworkbench_plugin_utils.cmake)

# 设置安装目录 - 插件安装到主程序目录
set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_DIR})

# 添加 src 目录
add_subdirectory(src)
```

### src 目录 CMakeLists.txt

src 目录 CMake 文件负责具体的插件配置，包括设置插件信息、链接依赖、设置属性等。

下面的 CMake 示例展示了 src 目录配置文件的完整内容：

```cmake
cmake_minimum_required(VERSION 3.5)

# 插件信息设置 - 使用辅助宏简化配置
damacro_plugin_setting(
    "MyPlugin"                       # 插件名称
    "My Plugin for DAWorkbench"      # 插件描述
    0                                # 主版本号
    0                                # 次版本号
    1                                # 补丁版本号
    ${DAWorkbench_INSTALL_DIR}       # 安装目录
)

# Qt 依赖 - 查找所需的 Qt 模块
find_package(Qt${QT_VERSION_MAJOR} 5.14 COMPONENTS
    Core Gui Widgets Xml Svg PrintSupport
    REQUIRED
)

# 源文件 - 使用 file(GLOB) 自动收集源文件
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_PLUGIN_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_PLUGIN_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

# 创建动态库 - 插件必须是动态库
add_library(${DA_PLUGIN_NAME} SHARED
    ${DA_PLUGIN_HEADER_FILES}
    ${DA_PLUGIN_SOURCE_FILES}
    ${DA_PLUGIN_QT_UI_FILES}
    ${DA_PLUGIN_QT_RC_FILES}
)

# 链接 Qt - 使用 Qt 版本变量
target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
)

# 导入第三方库（使用辅助宏）- 自动处理依赖路径
damacro_import_SARibbonBar(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_DALiteCtk(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtAdvancedDocking(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtPropertyBrowser(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_qwt(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})

# 导入 DA 模块 - 使用 find_package 查找主程序模块
find_package(DAWorkbench COMPONENTS
    DAUtils DAMessageHandler DAData DAWorkFlow
    DAFigure DACommonWidgets DAGui DAInterface DAPluginSupport
)

target_link_libraries(${DA_PLUGIN_NAME} PUBLIC
    DAWorkbench::DAUtils
    DAWorkbench::DAWorkFlow
    DAWorkbench::DAInterface
    DAWorkbench::DAPluginSupport
)

# 设置属性 - 启用 Qt 自动处理工具
set_target_properties(${DA_PLUGIN_NAME} PROPERTIES
    AUTOMOC ON                       # 自动处理 Q_OBJECT 宏
    AUTOUIC ON                       # 自动处理 .ui 文件
    AUTORCC ON                       # 自动处理 .qrc 文件
    RUNTIME_OUTPUT_DIRECTORY "${DAWorkbench_INSTALL_DIR}/bin/plugins"  # 输出到插件目录
)

# 安装插件 - 使用辅助宏
damacro_plugin_install()
```

---

## 实现插件主类

### MyPlugin.h

```cpp
#pragma once
#include <QObject>
#include "DAAbstractNodePlugin.h"

class MyNodeFactory;

/**
 * @brief My 插件主类
 */
class MyPlugin : public DA::DAAbstractNodePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "Plugin.MyPlugin")
    Q_INTERFACES(DA::DAAbstractNodePlugin)
public:
    MyPlugin();
    ~MyPlugin();

    // 插件信息
    QString pluginName() const override { return tr("My Plugin"); }
    QString pluginVersion() const override { return "0.0.1"; }
    QString pluginDescription() const override { return tr("My custom plugin for DAWorkbench"); }

    // 初始化
    bool initialize() override;

    // 语言变更
    void retranslate() override;

    // 获取节点工厂
    QList<DA::DAAbstractNodeFactory*> getFactories() const override;

private:
    MyNodeFactory* m_nodeFactory;
};
```

### MyPlugin.cpp

```cpp
#include "MyPlugin.h"
#include "MyNodeFactory.h"
#include <QTranslator>

MyPlugin::MyPlugin() : m_nodeFactory(nullptr)
{
}

MyPlugin::~MyPlugin()
{
    if (m_nodeFactory) {
        delete m_nodeFactory;
    }
}

bool MyPlugin::initialize()
{
    // 获取核心接口
    DA::DACoreInterface* core = this->core();
    if (!core) {
        return false;
    }

    // 创建节点工厂
    m_nodeFactory = new MyNodeFactory(core);
    if (!m_nodeFactory->initialize()) {
        delete m_nodeFactory;
        m_nodeFactory = nullptr;
        return false;
    }

    // 注册节点元数据
    registerNodeMetaData(m_nodeFactory);

    return true;
}

void MyPlugin::retranslate()
{
    // 多语言支持：重新加载翻译
}

QList<DA::DAAbstractNodeFactory*> MyPlugin::getFactories() const
{
    QList<DA::DAAbstractNodeFactory*> factories;
    if (m_nodeFactory) {
        factories.append(m_nodeFactory);
    }
    return factories;
}
```

---

## 实现节点工厂

### MyNodeFactory.h

```cpp
#pragma once
#include "DAAbstractNodeFactory.h"
#include <QMap>

class MyWorker;

/**
 * @brief My 插件的节点工厂
 */
class MyNodeFactory : public DA::DAAbstractNodeFactory
{
    Q_OBJECT
public:
    MyNodeFactory(DA::DACoreInterface* core);
    ~MyNodeFactory();

    // 初始化工厂
    bool initialize();

    // 创建节点
    DA::DAAbstractNode* create(const DA::DANodeMetaData& meta) override;

    // 获取节点元数据列表
    QList<DA::DANodeMetaData> getNodeMetaDataList() const override;

    // 获取工厂信息
    QString getFactoryName() const override { return tr("My Factory"); }
    QString getFactoryDescription() const override { return tr("My Plugin Node Factory"); }

private:
    void registerNodePrototypes();
    QMap<QString, DA::DANodeMetaData> m_nodePrototypes;
};
```

### MyNodeFactory.cpp

```cpp
#include "MyNodeFactory.h"
#include "MyWorker.h"

MyNodeFactory::MyNodeFactory(DA::DACoreInterface* core)
    : DA::DAAbstractNodeFactory(core)
{
}

MyNodeFactory::~MyNodeFactory()
{
}

bool MyNodeFactory::initialize()
{
    registerNodePrototypes();
    return true;
}

void MyNodeFactory::registerNodePrototypes()
{
    // 注册节点原型
    DA::DANodeMetaData meta;
    
    // 节点1：数据处理节点
    meta.setPrototype("My.Factory.DataProcess");
    meta.setName(tr("Data Process"));
    meta.setGroup(tr("My Nodes"));
    meta.setIcon(QIcon(":/icon/my-icon.png"));
    meta.setDescription(tr("Process data with custom algorithm"));
    
    // 定义输入输出
    meta.addInputKey("input_data", tr("Input DataFrame"));
    meta.addOutputKey("output_data", tr("Output DataFrame"));
    
    m_nodePrototypes[meta.prototype()] = meta;
}

QList<DA::DANodeMetaData> MyNodeFactory::getNodeMetaDataList() const
{
    return m_nodePrototypes.values();
}

DA::DAAbstractNode* MyNodeFactory::create(const DA::DANodeMetaData& meta)
{
    QString prototype = meta.prototype();
    
    if (prototype == "My.Factory.DataProcess") {
        return new MyWorker(meta, core());
    }
    
    return nullptr;
}
```

---

## 实现工作节点

### MyWorker.h

```cpp
#pragma once
#include "DAAbstractNode.h"

/**
 * @brief 数据处理工作节点
 */
class MyWorker : public DA::DAAbstractNode
{
    Q_OBJECT
public:
    MyWorker(const DA::DANodeMetaData& meta, DA::DACoreInterface* core);
    ~MyWorker();

    // 执行节点 - 核心逻辑
    bool exec() override;

    // 创建图元显示
    DA::DAAbstractNodeGraphicsItem* createGraphicsItem() override;

    // 获取节点信息
    QString getPrototype() const override { return "My.Factory.DataProcess"; }
};
```

### MyWorker.cpp

```cpp
#include "MyWorker.h"
#include "DAStandardNodeGraphicsItem.h"

MyWorker::MyWorker(const DA::DANodeMetaData& meta, DA::DACoreInterface* core)
    : DA::DAAbstractNode(meta, core)
{
}

MyWorker::~MyWorker()
{
}

bool MyWorker::exec()
{
    // 获取输入数据
    QVariant inputData = getInputData("input_data");
    
    if (!inputData.canConvert<DA::DADataPackage>()) {
        return false;
    }
    
    DA::DADataPackage pkg = inputData.value<DA::DADataPackage>();
    
    // 执行数据处理逻辑
    // ... 自定义数据处理代码 ...
    
    // 设置输出数据
    QVariant outputData;
    outputData.setValue(pkg);
    setOutputData("output_data", outputData);
    
    return true;
}

DA::DAAbstractNodeGraphicsItem* MyWorker::createGraphicsItem()
{
    // 使用标准图元显示
    DA::DAStandardNodeGraphicsItem* item = new DA::DAStandardNodeGraphicsItem(this);
    item->setBodySize(120, 60);
    return item;
}
```

---

## 使用内置服务

### 日志服务

```cpp
#include "DALog.h"

bool MyWorker::exec()
{
    DA_LOG_INFO("Starting data processing...");
    DA_LOG_DEBUG("Input data type: {}", inputData.typeName());
    
    // 处理逻辑
    
    if (error) {
        DA_LOG_ERROR("Processing failed: {}", errorMessage);
        return false;
    }
    
    DA_LOG_INFO("Processing completed successfully");
    return true;
}
```

### 配置服务

```cpp
#include "DAConfig.h"

bool MyPlugin::initialize()
{
    // 获取配置值
    QString dataPath = DA::getConfigValue("data.default_path").toString();
    int maxThreads = DA::getConfigValue("performance.max_threads").toInt();
    
    // 设置配置值
    DA::setConfigValue("my_plugin.custom_option", true);
    
    return true;
}
```

### 数据管理服务

```cpp
#include "DADataManagerInterface.h"

bool MyWorker::exec()
{
    DA::DACoreInterface* core = this->core();
    DA::DADataManagerInterface* dataMgr = core->getDataManagerInterface();
    
    // 获取数据对象
    DA::DADataObject* dataObj = dataMgr->getData("my_dataframe");
    
    // 操作数据
    // ...
    
    return true;
}
```

---

## 构建和安装

### 构建插件

```bash
# 配置
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build --config Release

# 安装（自动安装到 data-workbench bin 目录）
cmake --install build --config Release
```

### 验证插件

插件将安装到：
```
data-workbench/bin_Release_qtX.X_MSCV_x64/bin/plugins/MyPlugin.dll
```

运行主程序，插件将自动加载。

---

## 下一步

- [:material-sync: 插件生命周期](./plugin-lifecycle.md) - 生命周期管理详解
- [:material-database: 数据持久化](./plugin-persistence.md) - 数据存储方案
- [:material-puzzle: 功能扩展](./plugin-extension.md) - 界面和功能扩展