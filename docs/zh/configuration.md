# 配置文件说明

DAWorkBench 支持多种配置文件格式，用于存储程序设置、项目信息、插件配置等。

## 配置文件类型

| 配置类型 | 文件格式 | 存储位置 | 说明 |
|----------|----------|----------|------|
| **程序配置** | INI/JSON | 用户目录 | 全局程序设置 |
| **项目配置** | JSON | 项目目录 | 项目信息和工作流关联 |
| **工作流配置** | JSON | 项目目录 | 工作流节点和连接信息 |
| **插件配置** | JSON/INI | 插件目录 | 插件特定设置 |

## 程序全局配置

### 配置文件位置

```
Windows: C:\Users\[用户名]\AppData\Local\DAWorkbench\settings.ini
Linux: ~/.config/DAWorkbench/settings.ini
macOS: ~/Library/Application Support/DAWorkbench/settings.ini
```

### 配置内容示例

```ini
[General]
language=zh_CN
theme=default
auto_save=true
save_interval=300

[MainWindow]
geometry=@ByteArray(xxxx)
state=@ByteArray(xxxx)
last_project=/path/to/last/project.dawproj

[Python]
enabled=true
python_path=/path/to/python
python_env=/path/to/venv

[Logging]
log_level=info
log_file=/path/to/log/DAWorkbench.log
max_log_size=10MB

[Performance]
max_threads=4
cache_size=100MB
```

## 项目配置文件 (.dawproj)

### 文件结构

```json
{
    "version": "1.0",
    "name": "MyDataAnalysisProject",
    "description": "数据分析项目",
    "created": "2024-03-10T10:30:00Z",
    "modified": "2024-03-10T15:45:00Z",
    
    "workflow": {
        "path": "workflow.daw",
        "start_node": "node_001"
    },
    
    "data": {
        "manager_path": "data/manager.json",
        "files": [
            {
                "name": "input_data",
                "path": "data/input.csv",
                "type": "csv"
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
            "enabled": true,
            "config_path": "plugins/MyPlugin/config.json"
        }
    },
    
    "settings": {
        "auto_run": false,
        "output_format": "xlsx",
        "output_path": "results/"
    }
}
```

## 工作流配置文件 (.daw)

### 文件结构

```json
{
    "version": "1.0",
    "metadata": {
        "name": "数据处理流程",
        "description": "CSV数据清洗和分析",
        "author": "user",
        "created": "2024-03-10T10:30:00Z"
    },
    
    "nodes": [
        {
            "id": "node_001",
            "prototype": "DataAnalysis.IO.CSVReader",
            "name": "读取CSV",
            "position": {
                "x": 100,
                "y": 100
            },
            "data": {
                "file_path": "data/input.csv",
                "encoding": "UTF-8",
                "delimiter": ","
            }
        },
        {
            "id": "node_002",
            "prototype": "DataAnalysis.Process.Cleaner",
            "name": "数据清洗",
            "position": {
                "x": 300,
                "y": 100
            },
            "data": {
                "remove_null": true,
                "remove_duplicates": true,
                "trim_spaces": true
            }
        },
        {
            "id": "node_003",
            "prototype": "DataAnalysis.IO.ExcelWriter",
            "name": "导出Excel",
            "position": {
                "x": 500,
                "y": 100
            },
            "data": {
                "file_path": "results/output.xlsx",
                "sheet_name": "ProcessedData"
            }
        }
    ],
    
    "links": [
        {
            "id": "link_001",
            "from_node": "node_001",
            "from_key": "output_dataframe",
            "to_node": "node_002",
            "to_key": "input_dataframe"
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

```ini
[General]
enabled=true
auto_save=true
log_level=info

[Processing]
default_algorithm=standard
threshold=0.5
max_iterations=1000

[UI]
dock_position=right
dock_visible=true
toolbar_visible=true

[Cache]
enabled=true
max_size_mb=50
expire_hours=24
```

## 配置文件读写 API

### QSettings 使用示例

```cpp
// 读取全局配置
QSettings settings;
settings.beginGroup("MyPlugin");
bool enabled = settings.value("enabled", true).toBool();
QString algorithm = settings.value("algorithm", "default").toString();
settings.endGroup();

// 写入全局配置
QSettings settings;
settings.beginGroup("MyPlugin");
settings.setValue("enabled", true);
settings.setValue("algorithm", "advanced");
settings.endGroup();
settings.sync();
```

### JSON 配置读写示例

```cpp
// 读取 JSON 配置
QFile file(configPath);
if (file.open(QIODevice::ReadOnly)) {
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject config = doc.object();
    
    bool enabled = config["enabled"].toBool(true);
    QString algorithm = config["algorithm"].toString("default");
    
    file.close();
}

// 写入 JSON 配置
QJsonObject config;
config["enabled"] = true;
config["algorithm"] = "advanced";
config["version"] = 2;

QJsonDocument doc(config);
QFile file(configPath);
if (file.open(QIODevice::WriteOnly)) {
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}
```

## 配置文件查找路径

### 主程序配置路径

```cpp
// 获取配置目录
QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
// Windows: C:/Users/[user]/AppData/Local/DAWorkbench
// Linux: ~/.config/DAWorkbench
```

### 插件配置路径

```cpp
// 全局插件配置
QString globalConfigPath = configPath + "/plugins/MyPlugin/config.json";

// 项目级插件配置
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