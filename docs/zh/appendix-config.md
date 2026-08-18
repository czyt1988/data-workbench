# 配置文件格式

本页面详细说明 DAWorkBench 项目中各类配置文件的格式规范，包括项目配置、工作流配置、插件配置和应用配置。

## 主要功能特性

- ✅ **项目配置文件**：`.dawproj` 格式，JSON 结构存储项目元信息
- ✅ **工作流配置文件**：节点拓扑和连接关系的持久化格式
- ✅ **插件配置文件**：JSON/INI 双格式插件设置
- ✅ **应用配置文件**：程序运行时配置和 UI 状态

---

## 项目配置文件 (.dawproj)

项目配置文件使用 JSON 格式，存储项目的基本信息、工作流关联、数据文件列表和插件配置。

下面的 JSON 示例展示了典型项目配置文件的结构：

```json
{
    "version": "1.0",
    "name": "MyProject",
    "created": "2024-03-10T10:30:00",
    "workflow": {
        "path": "workflow.daw",
        "nodes": [
            {
                "id": "node_001",
                "prototype": "DataAnalysis.IO.CSVReader",
                "position": {"x": 100, "y": 100}
            }
        ]
    },
    "data": {
        "files": [
            "data/input.csv",
            "data/output.xlsx"
        ]
    },
    "plugins": {
        "MyPlugin": {
            "config": "plugins/MyPlugin/config.json"
        }
    }
}
```

上述配置文件包含项目名称、创建时间、工作流信息、数据文件和插件配置等核心内容。

### 字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `version` | string | 配置文件版本号 |
| `name` | string | 项目名称 |
| `created` | string | 创建时间（ISO 8601 格式） |
| `workflow.path` | string | 工作流文件相对路径 |
| `workflow.nodes[]` | array | 节点列表 |
| `data.files[]` | array | 数据文件列表 |
| `plugins` | object | 插件配置映射 |

---

## 工程文件 (.dapro)

工程文件是 DAWorkBench 的核心持久化格式，本质上是一个 ZIP 压缩包，包含完整的工作状态。

### 工程文件结构

```
project.dapro (ZIP)
├── system.xml               # 系统信息（根节点 type="system-info"，含版本、创建时间等）
├── workflow-data.xml        # Python 工作流逻辑数据（节点拓扑、参数值、连接关系）
├── workflow.xml             # 工作流视图数据（节点位置、图元属性、连线布局）
├── data-manager.xml         # 数据管理器状态
├── datas/                   # 数据文件目录（数据源持久化）
├── charts.xml                # 图表元信息（图表项清单）
├── chart-data/               # 图表项数据目录（每个图元的序列/属性）
├── table-styles.xml          # 表格样式数据
├── agent_sessions/           # Agent 会话持久化目录
│   └── <session-id>.jsonl    # 每个会话一个 JSONL 文件（DAZipArchiveTask_LoadAgentSessions）
└── plugins/                  # 插件自定义数据
    └── [plugin-name]/
```

!!! warning "加载顺序铁律"
    Python 逻辑数据（`workflow-data.xml`）必须先于 C++ 视图数据（`workflow.xml`）加载。
    加载时先恢复节点实例和参数，再创建视图图元，此顺序不可违反。

---

## 工作流配置文件

工作流配置文件存储节点列表、连接关系和分组信息，是工作流持久化的核心文件。在 `.dapro` 工程包内，工作流数据以 **XML** 格式存放（并非独立 JSON 文件），见上文工程文件结构中的 `workflow-data.xml` 与 `workflow.xml`。

`workflow-data.xml` 的根节点为 `<root type="workflow-data">`，其 `<workflows>` 子节点内以 CDATA 形式承载 `DAPyWorkFlowSerializer` 序列化出的节点拓扑、参数值与连接关系；`workflow.xml` 则由 `DAPyWorkFlowSceneSerializer` 写出节点位置、图元属性、连线布局等 C++ 视图数据。两者通过节点 `name` 匹配关联。

工作流文件通过节点名称和端口名称建立连接关系，支持节点的自定义参数存储。详细的序列化机制见 [序列化架构](./dev-guide/project-serialization-architecture.md) 与 [工作流生命周期](./dev-guide/workflow-lifecycle.md)。

### 序列化双轨制

工作流序列化分为两个独立轨道：

| 序列化类型 | 类 | 内容 | 文件 |
|-----------|-----|------|------|
| **逻辑数据** | `DAPyWorkFlowSerializer` | 节点拓扑、参数值、连接关系 | `workflow-data.xml` |
| **视图数据** | `DAPyWorkFlowSceneSerializer` | 节点位置、图元属性、连线布局 | `workflow.xml` |

---

## 插件配置文件格式

插件配置文件支持 JSON 和 INI 两种格式，用于存储插件的设置参数。

下面的 JSON 示例展示了插件配置文件的典型结构：

```json
{
    "version": 2,
    "general": {
        "auto_save": true,
        "max_cache_size": 100,
        "log_level": "info"
    },
    "processing": {
        "algorithm": "advanced",
        "threshold": 0.7,
        "filters": ["filter1", "filter2"]
    },
    "ui": {
        "dock_position": "right",
        "show_toolbar": true
    }
}
```

!!! tip "版本号建议"
    插件配置建议包含 `version` 字段，以便后续升级时的兼容性处理。配置文件通常存储在项目的 `plugins/[插件名]/` 目录下。

---

## 应用配置文件

### UI 状态配置

程序退出时会保存 UI 状态（Ribbon 布局、Dock 窗口位置等）到配置文件。

| 配置项 | 说明 |
|--------|------|
| UI 状态文件路径 | 通过 `AppMainWindow::getUIStateSettingFilePath()` 获取 |
| 保存时机 | 程序退出时（可配置 `setSaveUIStateOnClose`） |
| 恢复方式 | `restoreUIState()` 从默认路径恢复 |
| 重置方式 | `removeStateSettingFile()` 删除状态文件 |

### Python 环境配置

当 `DA_ENABLE_AUTO_INSTALL_PYTHON_ENV=ON` 时，程序会自动搜索 Python 环境并复制必要的 DLL 到 bin 目录。

### 运行时配置文件清单

以下文件均位于 `DA::DADir::getConfigPath()` 返回的 `config/` 目录下：

| 文件 | 格式 | 维护者 | 说明 |
|------|------|--------|------|
| `dawork-config.xml` | XML | `DAAppConfig` | 程序全局配置（日志/字体/语言/启动画面/Python 路径等，键见 `DA_CONFIG_KEY_*`） |
| `agent-config.ini` | INI | `DAAgentModule` | Agent LLM 配置：`[agent]` 段含 `llm_base_url`/`llm_model`/`llm_api_key`（DPAPI 加密）/`ready_timeout_sec`/`stop_timeout_sec`/`providers`（多供应商 JSON 数组） |
| `recent-files.ini` | INI | `DARecentFilesManager` | 最近打开文件列表（`RecentFiles` 键） |
| UI 状态文件 | INI | `AppMainWindow` | Ribbon 布局、Dock 窗口位置等 UI 状态（见上文） |

!!! info "注册表→INI 一次性迁移"
    首次以 INI 模式启动时，`main.cpp` 的 `migrateSettingsFromRegistry()` 会将旧注册表路径（`HKCU\Software\DA\DAWorkBench`）下的 agent 配置与最近文件列表迁移到 `agent-config.ini` / `recent-files.ini`，旧值保留作为备份。详见 [配置文件说明](./configuration.md#agent-配置ai-分析子系统)。

---

## CMake 构建选项

构建选项的完整说明见 [构建选项参考](./build/build-options.md)。

---

## 相关文档

- [附录](./appendix.md) — 术语表、缩略语、CMake 宏说明、社区支持
- [工程文件结构](./dev-guide/project-file-structure.md) — .dapro 文件格式详解
- [项目序列化架构](./dev-guide/project-serialization-architecture.md) — 序列化系统架构
- [配置文件说明](./configuration.md) — 运行时配置详解
