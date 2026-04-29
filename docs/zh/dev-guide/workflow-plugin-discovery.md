# 插件与节点发现机制

本文档详细说明 data-workbench 中插件和节点的发现机制。系统采用**双通道发现架构**，分别管理 C++ DLL 插件和 Python 节点的发现与加载，最终在应用层统一合并，供工作流编辑器的节点面板使用。

## 导航

本系列文档包含以下章节：

- [DAPyWorkFlow 模块概述](./workflow-overview.md)
- [插件与节点发现机制](./workflow-plugin-discovery.md) ← 当前页
- [Python 节点开发指南](./workflow-python-node-dev.md)
- [工作流生命周期](./workflow-lifecycle.md)
- [C++ 集成指南](./workflow-cpp-integration.md)
- [场景操作指南](./workflow-scene-operation.md)

## 概述

data-workbench 的插件系统支持两种类型的扩展：

| 类型 | 载体 | 发现机制 | 用途 |
|------|------|----------|------|
| **DLL 插件** | `.dll` / `.so` / `.dylib` 共享库 | Qt `QPluginLoader` 扫描目录 | 注册 Ribbon 界面、菜单、节点工厂、数据接口 |
| **Python 节点** | `.py` 脚本 / `entry_points` 注册 | Python `DANodeRegistry` 扫描模块 | 定义工作流节点类型，实现数据处理逻辑 |

两种插件各有独立的发现通道，最终在 `DAAppPluginManager` 中合并为统一的节点元数据列表，提供给工作流编辑器。

```
App Startup
    │
    ▼
DAAppPluginManager::loadAllPlugins()
    │
    ├── DAPluginManager::loadAllPlugins()  ──→  DLL 通道
    │       └── 扫描 appDir/plugins/*.dll → QPluginLoader → dynamic_cast → initialize
    │
    ├── 提取 C++ 节点工厂 → 获取节点元数据
    │
    └── initPyNodeFactory()  ──→  Python 通道
            ├── 添加 appDir/PyScripts 到 sys.path
            ├── 扫描 appDir/pyplugins/
            └── DAPyNodeFactory::discoverNodes() → DANodeRegistry.discover()
```

## DLL 插件发现

### 目录结构

所有 DLL 插件集中放置在应用目录下的 `plugins/` 子目录中：

```
<app-dir>/
├── DataAnalysis.dll       # 数据分析插件
├── OtherPlugin.dll        # 其他插件
└── .pluginignore          # 插件忽略列表（可选）
```

插件目录路径由 `DAPluginManager::getPluginDirPath()` 返回，其值为 `QCoreApplication::applicationDirPath() + "/plugins"`。也可以通过 `DAPluginManager::setPluginPath()` 覆盖。

### 发现流程

DLL 插件的发现由 `DAPluginManager::loadAllPlugins()` 驱动，包含以下步骤：

| 步骤 | 操作 | 实现位置 |
|------|------|----------|
| 1 | 扫描插件目录，获取所有文件列表 | `DAPluginManager.cpp:142` |
| 2 | 按扩展名过滤（Windows: `.dll`，macOS: `.dylib`/`.so`，Linux: `.so`） | `DAPluginManager.cpp:152-163` |
| 3 | 跳过忽略列表中的插件（读取 `.pluginignore` 文件） | `DAPluginManager.cpp:164-167` |
| 4 | 使用 `QLibrary::isLibrary()` 验证文件是否为有效库 | `DAPluginManager.cpp:169-172` |
| 5 | 通过 `DAPluginOption::load()` 加载每个插件 | `DAPluginManager.cpp:173-179` |
| 6 | `QPluginLoader::load()` 执行操作系统级加载（`dlopen`/`LoadLibrary`） | `DAPluginOption.cpp:88` |
| 7 | `QPluginLoader::instance()` 创建 Qt 插件实例 | `DAPluginOption.cpp:102` |
| 8 | `dynamic_cast<DAAbstractPlugin*>` 验证插件接口 | `DAPluginOption.cpp:112` |
| 9 | 注入核心接口：`plugin->setCore(c)` | `DAPluginOption.cpp:144` |
| 10 | 调用初始化：`plugin->initialize()`，失败则卸载 | `DAPluginOption.cpp:147-153` |

#### 插件加载验证链（9 步校验）

任何一个步骤失败，插件将被跳过并记录 `qWarning` 日志：

```
文件存在 → 扩展名过滤 → 忽略列表 → QLibrary 验证 → QPluginLoader 加载
    → 实例创建 → 接口转换 → 核心注入 → 初始化成功
```

### 插件必须具备的 3 个要素

一个 C++ DLL 插件要能够被发现和加载，必须满足以下条件：

**1. 放置位置正确** — `.dll` 文件必须位于 `<app-dir>/plugins/` 目录下。

**2. 声明 Qt 插件元数据** — 插件类必须使用 `Q_PLUGIN_METADATA` 和 `Q_INTERFACES` 宏：

```cpp
#include <QObject>
#include "DAAbstractNodePlugin.h"

class DataAnalysisPlugin : public QObject, public DA::DAAbstractNodePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)  // Qt 插件标识
    Q_INTERFACES(DA::DAAbstractNodePlugin)             // 声明实现的接口
public:
    // ... 实现纯虚函数
};
```

**3. 导出符号** — DLL 必须导出符号，通过自定义宏实现：

```cpp
// DataAnalysisGlobal.h
#if defined(DATAANALYSIS_PLUGIN_BUILD)
#define DATAANALYSIS_API Q_DECL_EXPORT
#else
#define DATAANALYSIS_API Q_DECL_IMPORT
#endif

class DATAANALYSIS_API DataAnalysisPlugin : ... { ... };
```

在 `CMakeLists.txt` 中定义构建宏：

```cmake
target_compile_definitions(${DA_PLUGIN_NAME} PRIVATE DATAANALYSIS_PLUGIN_BUILD)
```

### 插件接口层级

```
DAAbstractPlugin                  # 基础插件接口
  ├── getIID()                    # 唯一标识（如 "DA.Plugin.DataAnalysis"）
  ├── getName()                   # 显示名称
  ├── getVersion()                # 版本号
  ├── getDescription()            # 描述信息
  ├── initialize()                # 初始化回调
  └── finalize()                  # 终结回调
        │
DAAbstractNodePlugin              # 节点插件接口（继承自 DAAbstractPlugin）
  ├── createNodeFactory()         # 创建节点工厂
  ├── destroyNodeFactory()        # 销毁节点工厂
  └── afterLoadedNodes()          # 全部节点加载后的回调
```

IID 常量定义：

| 常量 | 值 | 定义位置 |
|------|-----|----------|
| `DAABSTRACTPLUGIN_IID` | `"org.da.abstract.plugin"` | `DAAbstractPlugin.h:97` |
| `DAABSTRACTNODEPLUGIN_IID` | `"org.da.abstract.nodePlugin"` | `DAAbstractNodePlugin.h:80` |

### .pluginignore 忽略文件

插件目录下可放置 `.pluginignore` 文件，列出需要跳过的插件基础名称（不含扩展名），每行一个。支持 `#` 开头的注释行。

```
# 以下插件在调试时跳过
OldPlugin
DebugTool
```

读取位置：`DAPluginManager.cpp:281-284`

### 生命周期回调

| 回调 | 调用时机 | 返回值含义 |
|------|----------|-----------|
| `initialize()` | 插件加载完成、核心接口注入后 | `true` 表示初始化成功，`false` 表示拒绝加载 |
| `finalize()` | 插件卸载前 | `true` 允许卸载，`false` 取消卸载 |
| `retranslate()` | 语言切换时 | — |
| `afterLoadedNodes()` | 所有节点加载完成后 | — |
| `createSettingPage()` | 用户打开设置界面时 | 返回设置页面指针，或 `nullptr` |

### 卸载流程

```cpp
bool DAPluginManager::unloadPlugin(const QString& pluginName)
```

1. 调用 `plugin->finalize()` — 如果返回 `false`，取消卸载
2. 调用 `QPluginLoader::unload()` 卸载共享库
3. 从 `mPluginOptions` 列表中移除

批量卸载使用 `unloadAllPlugins()`，按加载顺序的逆序卸载。

## Python 节点发现

Python 节点的发现与 C++ DLL 插件完全独立，通过 `DAPyNodeFactory` 和 Python 端的 `DANodeRegistry` 协作完成。

### 三种发现来源

Python 节点可从以下三个来源被发现：

| 来源 | 路径 | 说明 |
|------|------|------|
| **内置脚本** | `<app-dir>/PyScripts/` | 随应用发布的内置 Python 节点 |
| **Python 插件包** | `<app-dir>/pyplugins/<name>/<pkg>/` | 第三方 Python 插件，必须包含 `__init__.py` |
| **Entry Points** | `setup.py` 中注册的 `data_workbench.plugin` | 通过 pip 安装的 Python 包自动注册 |

### 目录扫描结构

内置 PyScripts 目录结构示例：

```
<app-dir>/PyScripts/
└── DAWorkbench/
    ├── __init__.py
    ├── DAWorkFlowPy/
    │   ├── __init__.py
    │   ├── node_def.py          # @NodeDef 装饰器
    │   ├── node_registry.py     # DANodeRegistry 发现引擎
    │   ├── node_descriptor.py   # DANodeDescriptor 描述符
    │   ├── types.py             # Input/Output/Parameter
    │   ├── workflow.py          # DAWorkflow 有向图模型
    │   └── executor.py          # DAWorkflowExecutor 执行引擎
    └── ...
```

Python 插件包（pyplugins）目录结构：

```
<app-dir>/pyplugins/
└── DADataAnalysisPy/          # Python 包（含 __init__.py）
    ├── __init__.py
    ├── data_source_node.py    # @NodeDef 节点定义
    ├── data_filter_node.py
    └── ...
 ... (其他资源文件)
```

### 节点声明方式（@NodeDef）

Python 节点通过 `@NodeDef` 装饰器声明，该装饰器在被装饰的类上设置 `_node_descriptor` 属性，作为发现标记：

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Source", category="Data Analysis", icon="data_source")
class DataSourceNode:
    """数据源节点 — 从 CSV 文件读取数据"""

    file_path = Parameter(str, default="", description="CSV 文件路径")

    class Outputs:
        data = Output("DataFrame", description="读取的数据")
        row_count = Output("int", description="数据行数")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        # ... 执行逻辑
        return True
```

`@NodeDef` 装饰器在类上设置的 `_node_descriptor` 字典包含以下字段：

| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | str | 节点显示名称 |
| `category` | str | 节点分类 |
| `icon` | str | 图标标识 |
| `qualified_name` | str | 唯一标识（`module.ClassName`） |
| `inputs` | list | 输入端口描述列表 |
| `outputs` | list | 输出端口描述列表 |
| `parameters` | list | 参数描述列表 |
| `render_template` | str | 渲染模板类型（`rect`/`svg`/`widget`） |

### Python 发现引擎（DANodeRegistry）

`DANodeRegistry` 是 Python 端的节点发现引擎，支持**双模式发现**：

#### 模式一：目录扫描（_scan_directory）

1. 遍历指定路径下的所有 `.py` 文件（排除 `__pycache__/` 和 `__init__.py`）
2. 使用 `importlib.util.spec_from_file_location` 动态导入每个模块
3. 扫描模块中的所有类属性，查找带有 `_node_descriptor` 属性的类
4. 调用 `register_node()` 注册发现的节点

```python
registry = DANodeRegistry()
descriptors = registry.discover(
    scan_paths=["/path/to/plugins/DataAnalysis"],
    use_entry_points=False
)
```

#### 模式二：Entry Points 发现（_discover_from_entry_points）

通过 `importlib.metadata.entry_points(group='data_workbench.plugin')` 发现通过 pip 安装的插件包：

```python
# setup.py 中的注册方式
setup(
    name="DADataAnalysisPy",
    entry_points={
        "data_workbench.plugin": [
            "DADataAnalysisPy = DADataAnalysisPy",
        ],
    },
)
```

#### discover() 方法

```python
def discover(self, scan_paths=None, use_entry_points=False):
    """
    双模式发现节点

    :param scan_paths: 扫描路径列表
    :param use_entry_points: 是否启用 entry_points 发现
    :return: DANodeDescriptor 列表
    """
```

两种模式发现的节点按 `qualified_name` 去重，确保同一节点不会重复注册。

### C++ 端发现编排（DAPyNodeFactory）

C++ 端的 `DAPyNodeFactory::discoverNodes()` 负责编排完整的 Python 节点发现流程：

```cpp
bool DAPyNodeFactory::discoverNodes(
    const QStringList& scanPaths = QStringList(),
    bool useEntryPoints = false
);
```

执行流程：

| 步骤 | 操作 | 代码位置 |
|------|------|----------|
| 1 | 将 `scanPaths` 添加到 Python `sys.path` | `DAPyNodeFactory.cpp:344-346` |
| 2 | 导入 Python 模块 `DAWorkbench.DAWorkFlowPy` | `DAPyNodeFactory.cpp:349-356` |
| 3 | 获取 `DANodeRegistry` 类并创建实例 | `DAPyNodeFactory.cpp:359-366` |
| 4 | 调用 `registry.discover(pyScanPaths, useEntryPoints)` | `DAPyNodeFactory.cpp:375` |
| 5 | 将返回的 `DANodeDescriptor` 转换为 `DAPyNodeMetaData` | `DAPyNodeFactory.cpp:379-411` |
| 6 | 构建 `prototype → qualified_name` 映射表 | `DAPyNodeFactory.cpp:409` |
| 7 | 发射 `nodeDiscovered` 信号通知 UI | `DAPyNodeFactory.cpp:419` |

### DAPyNodeMetaData 结构

```cpp
struct DAPyNodeMetaData {
    QString name;                    // 节点显示名称
    QString prototype;               // 唯一标识（qualified_name）
    QString group;                   // 节点分类
    QString iconPath;                // 图标路径
    QString tooltip;                 // 提示文本
    QList<QString> inputKeys;        // 输入端口键名列表
    QList<QString> outputKeys;       // 输出端口键名列表

    bool isValid() const;            // prototype 非空则有效
};
```

### 节点创建

当用户在编辑器中拖入一个 Python 节点时，`DAPyNodeFactory::createNodeProxy()` 根据 `qualified_name` 创建节点实例：

1. 将 `qualified_name` 拆分为 `module.ClassName`
2. 导入对应 Python 模块
3. 获取类引用，创建实例
4. 将 Python 实例包装为 `DAPyNodeProxy`（C++ 代理对象）

```cpp
DAPyNodeProxy* DAPyNodeFactory::createNodeProxy(const QString& qualifiedName);
```

## 应用层整合

### DAAppPluginManager 完整发现流程

应用的插件管理器 `DAAppPluginManager` 继承自 `DAPluginManager`，在其 `loadAllPlugins()` 方法中整合了两个通道：

```cpp
void DAAppPluginManager::loadAllPlugins(DACoreInterface* c)
{
    // 阶段一：加载 C++ DLL 插件
    if (!isLoaded()) {
        DAPluginManager::loadAllPlugins(c);  // 扫描 plugins/*.dll
    }

    // 阶段二：从 C++ 插件中提取节点元数据
    QList<DAAbstractPlugin*> plugins = ...;
    for (auto* p : plugins) {
        if (auto* np = dynamic_cast<DAAbstractNodePlugin*>(p)) {
            DAPyNodeFactory* fac = np->createNodeFactory();
            mNodeMetaDatas += fac->getNodeMetadataList();
        }
    }

    // 阶段三：发现 Python 节点
    initPyNodeFactory();

    // 阶段四：去重
    // 使用 QMap<DAPyNodeMetaData, int> 按 prototype 去重
}
```

### initPyNodeFactory

```cpp
void DAAppPluginManager::initPyNodeFactory()
```

| 步骤 | 操作 |
|------|------|
| 1 | 检查 `DAPyInterpreter::isPythonInitialized()` |
| 2 | 将 `<app-dir>/PyScripts` 添加到 Python `sys.path` |
| 3 | 调用 `scanPyPluginsDir(<app-dir>/pyplugins)` 扫描 Python 插件包 |
| 4 | 创建 `DAPyNodeFactory` 实例 |
| 5 | 调用 `factory->discoverNodes(scanPaths, true)` |
| 6 | 合并 Python 节点元数据到 `mNodeMetaDatas` |

### scanPyPluginsDir

`scanPyPluginsDir()` 是一个静态辅助函数，扫描 `pyplugins/` 目录下的 Python 插件包：

```cpp
static QStringList scanPyPluginsDir(const QString& pyPluginsDir)
```

算法：

1. 列出 `pyplugins/` 下的所有子目录
2. 对每个子目录，检查是否存在 `pyplugins/<subDir>/PyScripts/`
3. 对 `PyScripts` 下的每个子包，检查是否包含 `__init__.py`
4. 返回所有有效的 `PyScripts` 路径列表

### 元数据去重

来自 C++ 插件和 Python 发现的元数据可能包含相同 `prototype` 的节点。`DAAppPluginManager` 使用 `QMap<DAPyNodeMetaData, int>` 进行去重，保留首次出现的顺序。

### 初始化时序

完整的初始化由应用核心 `DAAppCore` 启动：

```
DAAppCore 构造函数
    │
    ├── DACoreInterface 构造函数
    │   ├── initializePythonScripts()
    │   │   ├── DAPyInterpreter::appendSysPath(PyScripts)
    │   │   └── DAPyScripts::initScripts()  ← 导入 DAWorkbench 基础模块
    │   └── ...
    │
    └── DAAppPluginManager::loadAllPlugins()
        ├── DAPluginManager::loadAllPlugins()  ← DLL 插件
        ├── 提取 C++ 节点元数据
        └── initPyNodeFactory()                 ← Python 节点
            ├── appendSysPath(PyScripts)
            ├── scanPyPluginsDir(pyplugins)
            └── discoverNodes(scanPaths, true)
```

## 完整流程图

```
┌─────────────────────────────────────────────────────────────┐
│                   应用启动                                   │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  Python 解释器初始化                                          │
│  · DAPyInterpreter 启动嵌入式 Python                          │
│  · DAPyScripts::initScripts() 导入基础模块                     │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  DLL 插件通道                                                │
│                                                              │
│  DAPluginManager::loadAllPlugins()                           │
│      │                                                       │
│      ├── 扫描 appDir/plugins/                                │
│      ├── 过滤扩展名 (.dll/.so/.dylib)                        │
│      ├── 跳过 .pluginignore 列表                              │
│      ├── QLibrary::isLibrary() 验证                          │
│      │                                                       │
│      └── 对每个 DLL：                                         │
│          ├── QPluginLoader::load() ← LoadLibrary/dlopen      │
│          ├── QPluginLoader::instance() ← 创建 QObject        │
│          ├── dynamic_cast<DAAbstractPlugin*> ← 接口验证      │
│          ├── setCore(c) ← 注入核心服务                        │
│          └── initialize() ← 插件初始化，失败则卸载             │
│                                                              │
│  结果: mPluginOptions ← 有效插件列表                           │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  C++ 节点元数据提取                                           │
│                                                              │
│  for each DAAbstractNodePlugin:                              │
│      createNodeFactory() → DAPyNodeFactory                    │
│      getNodeMetadataList() → 节点元数据列表                    │
│                                                              │
│  结果: mNodeMetaDatas (C++ 部分)                              │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  Python 节点通道                                             │
│                                                              │
│  initPyNodeFactory()                                         │
│      │                                                       │
│      ├── appendSysPath(appDir/PyScripts) ← 内置脚本路径      │
│      │                                                       │
│      ├── scanPyPluginsDir(appDir/pyplugins)                   │
│      │   ├── 枚举子目录                                       │
│      │   └── 检查 PyScripts/<pkg>/__init__.py                 │
│      │                                                       │
│      └── DAPyNodeFactory::discoverNodes(scanPaths, true)     │
│          ├── 添加 sys.path                                    │
│          ├── import DAWorkbench.DAWorkFlowPy                  │
│          ├── DANodeRegistry.discover()                        │
│          │   ├── _scan_directory() ← 扫描 .py 文件            │
│          │   └── _discover_from_entry_points() ← pip 包       │
│          ├── 转换 → DAPyNodeMetaData                          │
│          └── emit nodeDiscovered()                           │
│                                                              │
│  结果: mNodeMetaDatas (Python 部分)                           │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  元数据合并与去重                                             │
│                                                              │
│  mNodeMetaDatas = C++元数据 + Python元数据                     │
│  按 prototype (qualified_name) 去重，保留首次出现顺序           │
│                                                              │
│  最终结果: 统一节点元数据列表 → 工作流编辑器节点面板              │
└─────────────────────────────────────────────────────────────┘
```

## 关键类型速查

### C++ 核心类

| 类名 | 头文件 | 作用 |
|------|--------|------|
| `DAPluginManager` | `DAPluginSupport/DAPluginManager.h` | DLL 插件管理器，扫描与加载 |
| `DAAppPluginManager` | `APP/DAAppPluginManager.h` | 应用级插件管理器，整合 DLL + Python |
| `DAPluginOption` | `DAPluginSupport/DAPluginOption.h` | 单个 DLL 插件加载包装 |
| `DAAbstractPlugin` | `DAPluginSupport/DAAbstractPlugin.h` | 插件基类接口 |
| `DAAbstractNodePlugin` | `DAPluginSupport/DAAbstractNodePlugin.h` | 节点插件接口 |
| `DAPyNodeFactory` | `DAPyWorkFlow/DAPyNodeFactory.h` | Python 节点工厂，驱动发现与创建 |
| `DAPyNodeMetaData` | `DAPyWorkFlow/DAPyNodeFactory.h` | 节点元数据结构 |
| `DAPyNodeProxy` | `DAPyWorkFlow/DAPyNodeProxy.h` | Python 节点 C++ 代理 |
| `DAPyModuleWorkflow` | `DAPyWorkFlow/DAPyModuleWorkflow.h` | Python 模块导入单例 |

### Python 核心类

| 类名/装饰器 | 模块路径 | 作用 |
|-------------|----------|------|
| `@NodeDef` | `DAWorkbench.DAWorkFlowPy.node_def` | 节点声明装饰器，设置发现标记 |
| `DANodeRegistry` | `DAWorkbench.DAWorkFlowPy.node_registry` | 节点发现引擎（目录扫描 + entry_points） |
| `DANodeDescriptor` | `DAWorkbench.DAWorkFlowPy.node_descriptor` | 节点描述符，包含 `to_dict()` 序列化方法 |
| `Input` | `DAWorkbench.DAWorkFlowPy.types` | 输入端口声明 |
| `Output` | `DAWorkbench.DAWorkFlowPy.types` | 输出端口声明 |
| `Parameter` | `DAWorkbench.DAWorkFlowPy.types` | 节点参数声明 |

### 关键函数签名

| 函数 | 文件 | 签名 |
|------|------|------|
| `loadAllPlugins` | `DAPluginManager.cpp` | `void loadAllPlugins(DACoreInterface* c)` |
| `loadAllPlugins` | `DAAppPluginManager.cpp` | `void loadAllPlugins(DACoreInterface* c)` (override) |
| `initPyNodeFactory` | `DAAppPluginManager.cpp` | `void initPyNodeFactory()` (private) |
| `scanPyPluginsDir` | `DAAppPluginManager.cpp` | `static QStringList scanPyPluginsDir(const QString& pyPluginsDir)` |
| `discoverNodes` | `DAPyNodeFactory.cpp` | `bool discoverNodes(const QStringList& scanPaths, bool useEntryPoints)` |
| `createNodeProxy` | `DAPyNodeFactory.cpp` | `DAPyNodeProxy* createNodeProxy(const QString& qualifiedName)` |

## 调试与排错

### 发现失败的常见原因

| 问题 | DLL 插件 | Python 节点 |
|------|----------|-------------|
| **路径错误** | 插件不在 `appDir/plugins/` 目录下 | PyScripts 未部署到应用目录 |
| **缺少依赖** | 缺少运行时 DLL（如 Qt 库） | Python 包未安装（如 pandas） |
| **接口不匹配** | 未使用 `Q_PLUGIN_METADATA` 或 `Q_INTERFACES` | 类未使用 `@NodeDef` 装饰器 |
| **初始化失败** | `initialize()` 返回 `false` | Python 解释器未初始化 |
| **符号未导出** | 未定义 `Q_DECL_EXPORT` 宏 | — |
| **忽略列表** | 插件名在 `.pluginignore` 中 | — |
| **Python 错误** | — | 模块导入异常，节点定义语法错误 |

### 日志定位

- **DLL 插件加载日志**：带 `qWarning` 级别，包含文件路径和具体失败原因
- **DAPluginOption** 加载失败时可通过 `getErrorString()` 获取详细的 QPluginLoader 错误信息
- **Python 节点发现日志**：`DAPyNodeFactory::discoverNodes()` 中的异常会通过 `qWarning` 输出，可通过 `getLastErrorString()` 获取
- **Python 解释器状态**：`DAPyInterpreter::isPythonInitialized()` 检查 Python 是否可用

## 参考资料

### 源码参考

- `src/APP/DAAppPluginManager.cpp` — 应用级插件整合（DLL + Python）
- `src/DAPluginSupport/DAPluginManager.cpp` — DLL 插件发现与加载
- `src/DAPluginSupport/DAPluginOption.cpp` — QPluginLoader 包装
- `src/DAPluginSupport/DAAbstractPlugin.h` — 插件基类接口
- `src/DAPluginSupport/DAAbstractNodePlugin.h` — 节点插件接口
- `src/DAPyWorkFlow/DAPyNodeFactory.cpp` — Python 节点发现编排
- `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_registry.py` — Python 侧发现引擎
- `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_def.py` — `@NodeDef` 装饰器
- `src/DAPyWorkFlow/DAPyModuleWorkflow.cpp` — Python 模块导入

### 插件示例

- `plugins/DataAnalysis/` — 完整的 C++ DLL 插件示例（含界面、节点工厂、Python 脚本）
- `plugins/CrewAIAdapter/` — Python-only 插件（仅含 PyScripts，通过 entry_points 发现）
- `plugins/plugin-template/` — 插件脚手架生成工具

### 相关文档

- [Python 节点开发指南](./workflow-python-node-dev.md) — `@NodeDef` 装饰器和节点定义
- [工作流生命周期](./workflow-lifecycle.md) — 从发现到执行完成的完整流程
- [C++ 集成指南](./workflow-cpp-integration.md) — C++ 与 Python 绑定层
- [DAPluginSupport 模块](./plugin-module.md) — 插件框架核心模块
- [插件接口关系](./plugins-interfaces.md) — 插件接口层次与依赖
