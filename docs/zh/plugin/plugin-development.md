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
    "plugin-iid": "DAABSTRACTNODEPLUGIN_IID",           # 插件唯一标识符（节点插件用此宏，值为 "org.da.abstract.nodePlugin"）
    "factory-prototypes": "My.Factory",                 # 节点工厂原型前缀
    "factory-name": "My Factory",                       # 节点工厂名称
    "factory-description": "My Plugin Node Factory"     # 节点工厂描述
}
```

上述配置定义了插件的基本信息，脚本将根据此配置生成完整的插件项目。各字段说明：

- `plugin-base-name`：插件基础名称，用于生成类名和文件名
- `plugin-display-name`：插件显示名称，用于界面展示
- `plugin-description`：插件功能描述
- `plugin-iid`：插件接口标识符，节点插件用 `DAABSTRACTNODEPLUGIN_IID` 宏（值为 `"org.da.abstract.nodePlugin"`，定义于 `DAAbstractNodePlugin.h`），通用插件用 `DAABSTRACTPLUGIN_IID`
- `factory-prototypes`：节点工厂原型标识
- `factory-name`：工厂显示名称
- `factory-description`：工厂功能描述

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

如果你用 `git` 来管理插件项目，`data-workbench` 应该作为插件项目的子模块，这是最推荐的依赖管理方式。在插件项目根目录执行：

```bash
git submodule add https://gitee.com/czyt1988/data-workbench.git ./data-workbench
```

上述命令的效果：

- 在当前项目根目录下创建 `data-workbench` 子目录
- 将 data-workbench 仓库作为子模块添加到项目中
- 自动更新 `.gitmodules` 文件记录子模块信息

首次拉取插件项目（或首次使用 `git worktree`）时，需要初始化并更新所有子模块：

```bash
git submodule update --init --recursive
```

该命令会递归拉取 `data-workbench` 及其所有第三方依赖库。添加主项目子模块确保插件能正确引用主程序提供的接口和库。

### 方式三：Python-first 节点插件（纯 Python，无需 C++ NodeFactory）

!!! tip "新节点插件推荐此模式"
    新增工作流节点插件时，**无需编写 C++ NodeFactory**。参考 `plugins/DASystemNodes/`：C++ 插件入口只负责注册 Python 脚本路径，节点用 `@NodeDef` 装饰器声明，`DAPyNodeFactory::discoverNodes()` 启动时扫描 Python 包自动发现节点。详见 `plugins/DASystemNodes/AGENTS.md`。

DASystemNodes 风格的纯 Python 节点插件结构（作为 src/ C++ 布局的替代方案）：

```text
MyPlugin/
├── CMakeLists.txt                 # CMake 安装规则（复制 PyScripts 到 pyplugins/）
├── MyPluginPlugin.cpp/h           # C++ 插件入口（仅注册 Python 路径，不定义节点）
└── PyScripts/
    └── MyPlugin/                  # Python 包（pip-installable，entry_points 声明）
        ├── __init__.py            # 导出所有节点类；顶部先调 setup_i18n() 再导入节点
        ├── setup.py               # entry_points 注册（data_workbench.plugin）
        └── nodes/
            └── my_node.py         # @NodeDef 装饰器定义节点
```

C++ 插件入口只需继承 `DAAbstractNodePlugin` 并实现 `createNodeFactory()`/`destroyNodeFactory()`，把工厂交由框架的 Python 自动发现机制接管，无需手写节点创建逻辑。节点执行入口为 `execute(self, inputs, params)`，输出通过 `self._output_data` 写入。

---

## CMake 配置

### 根目录 CMakeLists.txt

根目录 CMake 文件负责查找 Qt、指定 data-workbench 安装目录、引入辅助工具等基础配置。

下面的 CMake 示例展示了根目录配置文件的完整内容：

```cmake
cmake_minimum_required(VERSION 3.16)

# standalone 构建时引导（顶层构建时由主工程根 CMakeLists 提供环境）
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_plugin_utils.cmake)
    da_plugin_bootstrap("MyPlugin" "My Plugin for DAWorkbench")
endif()

# 顶层构建时 DAWorkbench::* targets 已由 add_subdirectory(src) 创建，无需 find_package；
# standalone 构建时按组件清单查找（DAWorkbench_DIR 由 da_plugin_bootstrap 指向安装目录）
if(NOT TARGET DAWorkbench::DAUtils)
    find_package(DAWorkbench COMPONENTS
        DAUtils DAMessageHandler DAData DAPyWorkFlow DAPyBindQt
        DAPyScripts DAPyCommonWidgets DAGraphicsView
        DAFigure DAGui DAInterface DAPluginSupport
    )
endif()

# 源文件 - 使用 file(GLOB) 自动收集源文件（!!!!** 注意按实际文件变更 **!!!!）
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_PLUGIN_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_PLUGIN_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

# 一次调用完成插件 target 创建、属性设置（输出 bin/plugins）、第三方导入与安装
da_add_plugin(
    NAME MyPlugin
    BUILD_DEFINE MYPLUGIN_PLUGIN_BUILD
    SOURCES ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
            ${DA_PLUGIN_QT_UI_FILES}
            ${DA_PLUGIN_QT_RC_FILES}
    QT_PRIVATE Core Gui Widgets Xml Svg PrintSupport
    LINK_PUBLIC DAUtils DAPyWorkFlow DAPyBindQt DAPyScripts
                DAPyCommonWidgets DAInterface DAPluginSupport
    THIRDPARTY SARibbonBar DALiteCtk ads qwt orderedmap
)
```

上述 src 目录 CMake 配置的关键点：

- standalone 构建时在顶部调用 `da_plugin_bootstrap` 完成 project/安装目录/工具文件引导
- 使用 `file(GLOB)` 收集源文件、头文件、UI 文件和资源文件
- `da_add_plugin` 一次完成 target 创建、`_PLUGIN_BUILD` 宏定义、Qt/DA 模块/第三方库链接、
  属性设置与安装（VERSION 缺省继承根 `DA_VERSION`）
- Windows 平台的 `AxContainer` / `DAAxOfficeWrapper` 依赖用 `QT_WIN32_PUBLIC` /
  `LINK_WIN32_PUBLIC` 参数表达，否则 Linux 下无法构建
- 第三方库经 `THIRDPARTY` 参数导入（内部走 `da_link_3rdparty`，自动处理安装目录查找）

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
 * 注意：继承列表中 QObject 必须位于第一位（见 DAAbstractNodePlugin.h 注释）
 */
class MyPlugin : public QObject, public DA::DAAbstractNodePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)
    Q_INTERFACES(DA::DAAbstractNodePlugin)
public:
    MyPlugin();
    ~MyPlugin() override;

    // 插件元信息（DAAbstractPlugin 纯虚接口，必须实现）
    QString getIID() const override { return DAABSTRACTNODEPLUGIN_IID; }
    QString getName() const override { return tr("My Plugin"); }
    QString getVersion() const override { return "0.0.1"; }
    QString getDescription() const override { return tr("My custom plugin for DAWorkbench"); }

    // 初始化（非纯虚，默认返回 true；此处重载以创建节点工厂）
    bool initialize() override;

    // 语言变更
    void retranslate() override;

    // 创建节点工厂（DAAbstractNodePlugin 纯虚接口，必须实现）
    DA::DAPyNodeFactory* createNodeFactory() override;
    // 销毁节点工厂（谁创建谁删除）
    void destroyNodeFactory(DA::DAPyNodeFactory* p) override;

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
    // 插件卸载时由 destroyNodeFactory 负责销毁，这里不要重复 delete
}

bool MyPlugin::initialize()
{
    // 获取核心接口
    DA::DACoreInterface* core = this->core();
    if (!core) {
        return false;
    }

    // 创建节点工厂（具体工厂对象在 createNodeFactory() 时返回给框架）
    m_nodeFactory = new MyNodeFactory(core);
    if (!m_nodeFactory->initialize()) {
        delete m_nodeFactory;
        m_nodeFactory = nullptr;
        return false;
    }

    // 不再调用 registerNodeMetaData —— 旧 API 已废弃
    // 节点元数据由 DAPyNodeFactory::discoverNodes() 扫描 Python 包自动发现
    return true;
}

void MyPlugin::retranslate()
{
    // 多语言支持：重新加载翻译
}

DA::DAPyNodeFactory* MyPlugin::createNodeFactory()
{
    // 把工厂指针交给框架；销毁由 destroyNodeFactory() 负责
    return m_nodeFactory;
}

void MyPlugin::destroyNodeFactory(DA::DAPyNodeFactory* p)
{
    // 谁创建谁删除
    if (p == m_nodeFactory) {
        delete m_nodeFactory;
        m_nodeFactory = nullptr;
    } else {
        delete p;
    }
}
```

---

## 实现节点工厂（旧 C++ 架构，仅参考）

!!! warning "遗留架构 —— 当前推荐 Python-first `@NodeDef`"
    下面 `MyNodeFactory` 继承 `DA::DAPyNodeFactory` 并重写 `create` / `getNodeMetaDataList` / `getFactoryName` 的写法属于**已废弃的 C++ 节点工厂架构**，当前 `DAPyNodeFactory`（`src/DAPyWorkFlow/DAPyNodeFactory.h`）不再暴露这些虚函数，上述代码无法编译。

    **新插件应采用 Python-first 模型**：节点用 `@NodeDef` 装饰器声明，C++ 插件入口只负责注册 Python 脚本路径，`DAPyNodeFactory::discoverNodes()` 启动时扫描 Python 包自动发现节点，无需手写 C++ 工厂。参考 `plugins/DASystemNodes/`（纯 Python 节点插件，C++ 入口 `DASystemNodesPlugin` 仅注册 Python 路径，不定义任何节点）。本节仅作历史背景保留。

### MyNodeFactory.h

```cpp
#pragma once
#include "DAPyNodeFactory.h"
#include <QMap>

class MyWorker;

/**
 * @brief My 插件的节点工厂
 */
class MyNodeFactory : public DA::DAPyNodeFactory
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
    : DA::DAPyNodeFactory(core)
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

## 实现工作节点（旧 C++ 节点，仅参考）

!!! warning "遗留架构 —— 当前推荐 Python-first `@NodeDef`"
    下面 `MyWorker` 继承 `DA::DAAbstractNode` 并重写 `exec()` 的写法属于**已废弃的 C++ 节点架构**。当前工作流节点统一用 Python `@NodeDef` 装饰器声明，执行入口为 `execute(self, inputs, params)`，输出通过 `self._output_data` 写入，数据包装类为 `DAData`（`src/DAData/DAData.h`，**不是** `DADataPackage`）。本节仅作历史背景保留。

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
    QVariant inputData = getInputData("input_data");  // cn:数据包装类为 DAData，非 DADataPackage

    if (!inputData.canConvert<DA::DAData>()) {  // cn:类型校验用 DAData
        return false;
    }

    DA::DAData data = inputData.value<DA::DAData>();  // cn:取出 DAData
    if (!data.isDataFrame()) {
        return false;
    }
    DA::DAPyDataFrame df = data.toDataFrame();
    
    // 执行数据处理逻辑
    // ... 自定义数据处理代码 ...
    
    // 设置输出数据 —— 输出也是 DAData
    QVariant outputData;
    DA::DAData outData(df);  // cn:用处理后的 DataFrame 构造 DAData
    outputData.setValue(outData);
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

### Python-first `@NodeDef` 等价实现（推荐）

上面 C++ 节点的功能，用 Python-first 模型可以这样实现（推荐写法，参考 `plugins/DASystemNodes/`）：

```python
# -*- coding: utf-8 -*-
"""Data Process node."""  # docstring 改英文（作为 tooltip）

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(
    name="Data Process",            # 显示名称（保持英文不翻译，参与序列化）
    category=_("My Nodes"),          # cn:我的节点  分类路径，翻译
)
class DataProcessNode:
    """Data Process node."""

    class Inputs:
        input_data = Input("DataFrame", required=True, description=_("Input DataFrame"))  # cn:输入 DataFrame

    class Outputs:
        output_data = Output("DataFrame", description=_("Output DataFrame"))  # cn:输出 DataFrame

    def __init__(self):
        super().__init__()  # 必须调用，激活 MRO 链，初始化 _output_data
        self._cache = None

    def execute(self, inputs=None, params=None):
        # 签名必须是 (self, inputs=None, params=None)
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        df = inputs.get("input_data")
        if df is None:
            return False  # 必填输入缺失

        # ... 自定义数据处理代码 ...
        result = df  # 示例

        # 输出通过 self._output_data 写入，不要 return 结果
        self._output_data["output_data"] = result
        return True
```

---

## 使用内置服务

### 日志服务

```cpp
#include "DALogCategory.h"

bool MyWorker::exec()
{
    daInfo << "Starting data processing...";
    daDebug << "Input data type:" << inputData.typeName();

    // 处理逻辑

    if (error) {
        daCritical << "Processing failed:" << errorMessage;
        return false;
    }

    daInfo << "Processing completed successfully";
    return true;
}
```

> 详见 [日志系统文档](../dev-guide/general/logging.md)。

### 配置服务

DAWorkBench 的配置由 `DAAppConfig`（`src/APP/SettingPages/DAAppConfig.h`）统一管理，配置键通过 `DA_CONFIG_KEY_*` 宏定义，生成的总头文件为 `DAConfigs.h`（注意是复数）。不要使用 `DA::getConfigValue` / `DA::setConfigValue`，也不要 `#include "DAConfig.h"`（无此文件）。

```cpp
#include "DAConfigs.h"  // 引入 DAAppConfig + 所有 DA_CONFIG_KEY_* 宏（生成的总头文件）

bool MyPlugin::initialize()
{
    // 通过 DAAppConfig 读写配置；DA_CONFIG_KEY_* 宏见 DAAppConfig.h
    // 已有键示例：DA_CONFIG_KEY_LOG_LEVEL / DA_CONFIG_KEY_WORKFLOW_TIMEOUT 等
    QString logLevel = DA::DAAppConfig::instance()->value(DA_CONFIG_KEY_LOG_LEVEL).toString();

    // 读取自定义配置可基于已注册键名，写入后需调用 saveConfig() 持久化
    DA::DAAppConfig::instance()->setValue(DA_CONFIG_KEY_WORKFLOW_TIMEOUT, 60);

    return true;
}
```

> 插件自有的设置页应通过重载 `DAAbstractPlugin::createSettingPage()` 返回 `DAAbstractSettingPage*`，而不是直接读写全局配置。

### 数据管理服务

```cpp
#include "DADataManagerInterface.h"

bool MyWorker::exec()
{
    DA::DACoreInterface* core = this->core();
    DA::DADataManagerInterface* dataMgr = core->getDataManagerInterface();

    // 获取当前选中的数据列表（返回 QList<DAData>，不是 DADataObject*）
    QList<DA::DAData> selected = dataMgr->getSelectDatas();

    // 获取当前正在操作的数据（返回 DAData）
    DA::DAData operateData = dataMgr->getOperateData();

    // 按名称查找数据（返回 DAData）
    DA::DAData data = dataMgr->findData("my_dataframe");
    if (data.isDataFrame()) {
        DA::DAPyDataFrame df = data.toDataFrame();  // cn:转换为 DataFrame 操作
        // ...
    }

    return true;
}
```

---

## Python 脚本组织

对于涉及 Python 数据处理的插件，推荐使用**三层架构**组织 PyScripts 目录。以 DataAnalysis 插件为例：

```text
PyScripts/
├── DADataAnalysisCore/      # 纯 pandas 核心算法（无 Qt 依赖）
│   ├── __init__.py
│   ├── cleaning.py          # 数据清洗算法
│   ├── io.py                # 文件读写算法
│   └── operations.py        # DataFrame 操作算法
├── DADataAnalysisGui/       # GUI 交互逻辑（可引用 da_app/da_interface）
│   ├── __init__.py
│   ├── dataframe_cleaner.py # 数据清洗 UI 交互
│   ├── dataframe_io.py      # 数据导入导出 UI 交互
│   ├── utils.py             # 工具函数
│   └── i18n/                # 国际化翻译
├── DADataAnalysisNodes/     # 工作流节点插件（@NodeDef 装饰器定义）
│   ├── __init__.py
│   ├── *_node.py            # 各节点定义（20+ 节点）
│   └── setup.py             # entry_points 注册
└── DADataAnalysis/          # 兼容旧包（可选，保持向后兼容）
```

**三层架构原则：**

| 层 | 职责 | 依赖限制 |
|----|------|----------|
| **Core** | 纯算法（pandas/numpy），不含任何 Qt 或 UI 代码 | 仅 pandas, numpy 等科学计算库 |
| **Gui** | UI 交互逻辑，调用 Core 层算法并与主程序接口交互 | Core 层 + da_app, da_interface, da_data |
| **Nodes** | 工作流节点定义（使用 `@NodeDef` 装饰器），自动被主程序发现 | Core 层 + DAWorkbench.DAWorkFlowPy |

### CMake 安装 Python 脚本

Python 脚本需要在 CMakeLists.txt 中配置安装规则，将不同层安装到不同目标目录：

```cmake
# Core 和 Gui 层安装到 PyScripts 目录（供普通脚本导入）
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PyScripts/DADataAnalysisCore
    DESTINATION bin/PyScripts
    FILES_MATCHING PATTERN "*.py"
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PyScripts/DADataAnalysisGui
    DESTINATION bin/PyScripts
    FILES_MATCHING PATTERN "*.py"
)

# Nodes 层安装到 pyplugins 目录（供工作流引擎自动发现）
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PyScripts/DADataAnalysisNodes
    DESTINATION bin/pyplugins
    FILES_MATCHING PATTERN "*.py"
)
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

### 启用/禁用与热插拔

主程序 Ribbon 主页 → 配置 → **插件设置** 打开插件管理对话框，可勾选启用/禁用插件：

- C++ 插件（plugins 目录）勾选变更点「应用」后**立即热加载/热卸载**，状态持久化到 `plugins/.pluginignore`
- Python 节点包（pyplugins 目录）禁用后**下次启动生效**，状态持久化到 `pyplugins/.pluginignore`

热卸载前宿主会调用插件的 `finalize()` 钩子，插件必须在此清理加入宿主的 ribbon panel/action 等资源；
agent 工具须以插件对象为 parent、系统提示词注册须传 provider，宿主才能在卸载前自动注销。
完整流程、降级停用状态与 finalize 契约详见 [插件生命周期管理 · 运行期热插拔](./plugin-lifecycle.md#运行期热插拔启用禁用管理)。

---

## 现有插件参考

DAWorkBench 自带的三个插件（`plugins/CMakeLists.txt` 构建）是开发新插件的最佳参考：

| 插件 | 路径 | 类型 | 说明 |
|------|------|------|------|
| **DataAnalysis** | `plugins/DataAnalysis/` | C++ + Python | 数据分析节点插件，三层 Python 包架构（Core/Gui/Nodes）参考实现 |
| **DASystemNodes** | `plugins/DASystemNodes/` | 纯 Python | 系统级工作流节点（Start/End/If/Else/Delay/Print/TextViewer/DataToManager 等），C++ 入口仅注册 Python 路径，是 **Python-first `@NodeDef` 节点插件的最佳模板** |
| **DAAgentTools** | `plugins/DAAgentTools/` | C++ | 平台内置 agent 工具插件（19 个工具，通过 `DAAgentInterface::registerTool()` 注册），供 LLM agent 调用 |

> 新增 Python-first 节点插件优先参考 `plugins/DASystemNodes/`，无需编写 C++ NodeFactory。详见 `plugins/DASystemNodes/AGENTS.md`。

---

## 下一步

- [:material-sync: 插件生命周期](./plugin-lifecycle.md) - 生命周期管理详解
- [:material-database: 数据持久化](./plugin-persistence.md) - 数据存储方案
- [:material-puzzle: 功能扩展](./plugin-extension.md) - 界面和功能扩展
