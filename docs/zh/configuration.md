# 配置文件说明

DAWorkBench 支持多种配置文件格式，用于存储程序设置、项目信息、工作流数据和插件配置。正确理解和使用配置文件是开发和运维的基础。

## 主要功能特性

**特性**

- ✅ **多格式支持**：支持 INI 和 JSON 两种配置文件格式
- ✅ **分层配置**：程序配置、项目配置、工作流配置、插件配置四个层次
- ✅ **跨平台路径**：自动适应 Windows、Linux、macOS 的配置目录规范
- ✅ **优先级机制**：项目级配置 > 命令行参数 > 全局配置 > 默认配置
- ✅ **热更新支持**：部分配置支持运行时变更，无需重启

## 配置文件类型

| 配置类型 | 文件格式 | 存储位置 | 说明 |
|----------|----------|----------|------|
| **程序配置** | INI/JSON | 用户目录 | 全局程序设置 |
| **项目配置** | JSON | 项目目录 | 项目信息和工作流关联 |
| **工作流配置** | JSON | 项目目录 | 工作流节点和连接信息 |
| **插件配置** | JSON/INI | 插件目录 | 插件特定设置 |

## 程序全局配置

### 配置文件位置

程序全局配置存储在用户目录，根据操作系统不同位置有所差异：

```text
Windows: C:\Users\[用户名]\AppData\Local\DAWorkbench\settings.ini
Linux: ~/.config/DAWorkbench/settings.ini
macOS: ~/Library/Application Support/DAWorkbench/settings.ini
```

上述路径使用 Qt 的 QSettings 自动管理，无需手动处理跨平台差异。

### 配置内容示例

全局配置使用 INI 格式，包含语言、主题、Python 环境等基本设置。

下面的 INI 示例展示了典型程序配置的内容：

```ini
[General]
language=zh_CN                    # 界面语言设置
theme=default                     # 主题样式
auto_save=true                    # 自动保存开关
save_interval=300                 # 自动保存间隔（秒）

[MainWindow]
geometry=@ByteArray(xxxx)         # 主窗口几何信息（序列化）
state=@ByteArray(xxxx)            # 主窗口状态（序列化）
last_project=/path/to/last/project.dawproj  # 最近打开的项目

[Python]
enabled=true                      # Python 支持开关
python_path=/path/to/python       # Python 可执行文件路径
python_env=/path/to/venv          # Python 虚拟环境路径

[Logging]
log_level=info                    # 日志级别
log_file=/path/to/log/DAWorkbench.log  # 日志文件路径
max_log_size=10MB                 # 最大日志文件大小

[Performance]
max_threads=4                     # 最大线程数
cache_size=100MB                  # 缓存大小限制
```

程序启动时自动加载此配置，关闭时自动保存修改的设置。

## 项目配置文件 (.dawproj)

项目配置文件使用 JSON 格式，存储项目的基本信息、工作流关联、数据文件列表和插件配置。

### 文件结构

下面的 JSON 示例展示了项目配置文件的完整结构：

```json
{
    "version": "1.0",
    "name": "MyDataAnalysisProject",
    "description": "数据分析项目",
    "created": "2024-03-10T10:30:00Z",
    "modified": "2024-03-10T15:45:00Z",
    
    "workflow": {
        "path": "workflow.daw",           // 工作流文件相对路径
        "start_node": "node_001"          // 工作流起始节点 ID
    },
    
    "data": {
        "manager_path": "data/manager.json",  // 数据管理配置路径
        "files": [
            {
                "name": "input_data",         // 数据对象名称
                "path": "data/input.csv",     // 数据文件路径
                "type": "csv"                 // 数据文件类型
            },
            {
                "name": "config",
                "path": "data/config.xlsx",
                "type": "xlsx"
            }
        ]
    },
    
    "plugins": {
        "MyPlugin": {
            "enabled": true,                 // 插件启用状态
            "config_path": "plugins/MyPlugin/config.json"  // 插件配置路径
        }
    },
    
    "settings": {
        "auto_run": false,                   // 项目特定设置
        "output_format": "xlsx",
        "output_path": "results/"
    }
}
```

项目配置文件包含项目元信息、工作流引用、数据文件列表和插件设置，是项目打开时的主要配置来源。

## 工作流配置文件 (.daw)

工作流配置文件存储节点列表、连接关系和分组信息，是工作流持久化的核心文件。

### 文件结构

下面的 JSON 示例展示了工作流配置文件的完整结构，包含元数据、节点定义和连接关系：

```json
{
    "version": "1.0",
    "metadata": {
        "name": "数据处理流程",               // 工作流名称
        "description": "CSV数据清洗和分析",   // 工作流描述
        "author": "user",                     // 作者信息
        "created": "2024-03-10T10:30:00Z"     // 创建时间
    },
    
    "nodes": [
        {
            "id": "node_001",                 // 节点唯一标识
            "prototype": "DataAnalysis.IO.CSVReader",  // 节点原型
            "name": "读取CSV",                // 节点显示名称
            "position": {
                "x": 100,
                "y": 100                      // 画布坐标位置
            },
            "data": {
                "file_path": "data/input.csv",  // 节点自定义数据
                "encoding": "UTF-8",
                "delimiter": ","
            }
        },
        {
            "id": "node_002",
            "prototype": "DataAnalysis.Process.Cleaner",
            "name": "数据清洗",
            "position": {"x": 300, "y": 100},
            "data": {
                "remove_null": true,          // 清洗配置参数
                "remove_duplicates": true,
                "trim_spaces": true
            }
        },
        {
            "id": "node_003",
            "prototype": "DataAnalysis.IO.ExcelWriter",
            "name": "导出Excel",
            "position": {"x": 500, "y": 100},
            "data": {
                "file_path": "results/output.xlsx",
                "sheet_name": "ProcessedData"
            }
        }
    ],
    
    "links": [
        {
            "id": "link_001",                 // 连接唯一标识
            "from_node": "node_001",          // 起始节点 ID
            "from_key": "output_dataframe",   // 起始节点输出端口
            "to_node": "node_002",            // 目标节点 ID
            "to_key": "input_dataframe"       // 目标节点输入端口
        },
        {
            "id": "link_002",
            "from_node": "node_002",
            "from_key": "output_dataframe",
            "to_node": "node_003",
            "to_key": "input_dataframe"
        }
    ],
    
    "groups": [
        {
            "id": "group_001",
            "name": "数据IO",
            "nodes": ["node_001", "node_003"]
        }
    ]
}
```

## 插件配置文件

### JSON 格式配置

```json
{
    "version": 2,
    "plugin_info": {
        "name": "MyPlugin",
        "version": "0.0.1"
    },
    
    "general": {
        "enabled": true,
        "auto_save": true,
        "log_level": "info"
    },
    
    "processing": {
        "default_algorithm": "standard",
        "threshold": 0.5,
        "max_iterations": 1000,
        "filters": [
            "low_pass",
            "noise_reduction"
        ]
    },
    
    "ui": {
        "dock_position": "right",
        "dock_visible": true,
        "toolbar_visible": true
    },
    
    "cache": {
        "enabled": true,
        "max_size_mb": 50,
        "expire_hours": 24
    }
}
```

### INI 格式配置

INI 格式配置适合简单键值对存储，使用分组方式组织配置项。

下面的 INI 示例展示了插件配置的典型格式：

```ini
[General]
enabled=true                     # 插件启用状态
auto_save=true                   # 自动保存开关
log_level=info                   # 日志级别

[Processing]
default_algorithm=standard       # 默认算法选择
threshold=0.5                    # 处理阈值
max_iterations=1000              # 最大迭代次数

[UI]
dock_position=right              # Dock 窗口位置
dock_visible=true                # Dock 窗口可见性
toolbar_visible=true             # 工具栏可见性

[Cache]
enabled=true                     # 缓存启用状态
max_size_mb=50                   # 最大缓存大小（MB）
expire_hours=24                  # 缓存过期时间（小时）
```

INI 格式配置便于手动编辑，适合简单的插件配置场景。

## 配置文件读写 API

### QSettings 使用示例

QSettings 是 Qt 提供的跨平台配置管理类，自动处理不同操作系统的配置存储位置。

下面的代码展示了 QSettings 的读写操作：

```cpp
// 读取全局配置 - 使用默认构造函数自动定位配置文件
QSettings settings;
settings.beginGroup("MyPlugin");  // 进入插件配置分组
bool enabled = settings.value("enabled", true).toBool();     // 读取配置，带默认值
QString algorithm = settings.value("algorithm", "default").toString();
settings.endGroup();             // 退出分组

// 写入全局配置 - 修改后调用 sync() 确保保存
QSettings settings;
settings.beginGroup("MyPlugin");
settings.setValue("enabled", true);       // 设置配置值
settings.setValue("algorithm", "advanced");
settings.endGroup();
settings.sync();                  // 立即写入文件
```

QSettings 支持分组管理，便于组织层级化的配置结构。

### JSON 配置读写示例

JSON 格式适合复杂嵌套结构的配置，支持数组和对象类型。

下面的代码展示了 JSON 配置的读写操作：

```cpp
// 读取 JSON 配置 - 使用 QJsonDocument 解析文件内容
QFile file(configPath);
if (file.open(QIODevice::ReadOnly)) {
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());  // 解析 JSON
    QJsonObject config = doc.object();  // 获取根对象
    
    bool enabled = config["enabled"].toBool(true);     // 读取配置项
    QString algorithm = config["algorithm"].toString("default");
    
    file.close();
}

// 写入 JSON 配置 - 构建 JSON 对象并序列化
QJsonObject config;
config["enabled"] = true;          // 设置配置项
config["algorithm"] = "advanced";
config["version"] = 2;             // 添加版本号

QJsonDocument doc(config);         // 创建 JSON 文档
QFile file(configPath);
if (file.open(QIODevice::WriteOnly)) {
    file.write(doc.toJson(QJsonDocument::Indented));  // 写入格式化 JSON
    file.close();
}
```

JSON 配置支持嵌套结构和数组类型，适合复杂的插件配置场景。

## 配置文件查找路径

### 主程序配置路径

使用 QStandardPaths 获取跨平台的配置目录路径，无需手动处理操作系统差异。

下面的代码展示了获取配置目录的方法：

```cpp
// 获取配置目录 - QStandardPaths 自动适应不同操作系统
QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
// Windows: C:/Users/[user]/AppData/Local/DAWorkbench
// Linux: ~/.config/DAWorkbench
// macOS: ~/Library/Application Support/DAWorkbench
```

### 插件配置路径

插件配置分为全局级和项目级两个层次，优先加载项目级配置。

下面的代码展示了获取插件配置路径的方法：

```cpp
// 全局插件配置 - 存储在用户目录，跨项目共享
QString globalConfigPath = configPath + "/plugins/MyPlugin/config.json";

// 项目级插件配置 - 存储在项目目录，项目特定
QString projectConfigPath = projectPath + "/plugins/MyPlugin/config.json";
```

## 配置优先级

配置加载优先级（从高到低）：

1. **项目级配置** - 项目目录中的配置
2. **命令行参数** - 启动时指定的参数
3. **全局用户配置** - 用户目录中的配置
4. **默认配置** - 程序内置默认值

## 配置热更新

部分配置支持热更新，无需重启程序：

```cpp
// 监听配置变更
connect(configManager, &ConfigManager::configChanged,
        this, &MyPlugin::onConfigChanged);

void MyPlugin::onConfigChanged(const QString& key, const QVariant& value)
{
    if (key == "MyPlugin.algorithm") {
        m_algorithm = value.toString();
        updateProcessingEngine();
    }
}
```

## 下一步

- [:material-book: 最佳实践](./best-practices.md) - 配置管理最佳实践
- [:material-database: 数据持久化](./plugin-persistence.md) - 数据存储方案
- [:material-help-circle: FAQ](./faq.md) - 常见问题解答