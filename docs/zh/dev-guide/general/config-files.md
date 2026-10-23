# 配置文件

这里说明 Data Workbench 在磁盘上留下了哪些文件，每个文件里放什么，以及哪些配置项改了之后确实有作用。

## 配置文件都放在哪里

应用级配置集中在用户配置目录下的 `config` 子目录。路径由 `DADir::getConfigPath()` 给出，也就是 `QStandardPaths::AppDataLocation` 加上 `/DAWorkBench/config`，在 Windows 上是：

```text
%LOCALAPPDATA%\DAWorkBench\config\
```

同一层还有日志目录 `log`、崩溃转储目录 `dumps` 和会话目录 `sessions`。这三处与配置分开，删掉配置目录不会连带删掉日志和历史会话。

可执行文件目录下另有一个 `python-config.json`，记录 Python 解释器路径。它不在配置目录里，因为程序启动早期、配置系统还没准备好之前就要用到它。

工程级的配置不放在这里，全部装在 `.dapro` 工程压缩包里。两边的分工是：应用级配置跨工程生效，工程级配置跟着工程走。

## dawork-config.json

应用设置由 `DAAppConfig` 管理，文件名 `dawork-config.json`（`DAAppConfig::getConfigFileName`）。它是强类型配置对象：`main()` 创建唯一实例后，通过构造参数注入主窗口、再经 `setConfig()` / `setAppConfig()` 注入控制器与设置页，不使用全局单例。`load()` 和 `save()` 是唯一接触存储格式的代码。

文件按设置页分成五组，组内是键值对：

```json
{
  "version": 1,
  "ui": {
    "ribbon-style": 34,
    "ribbon-theme": -1,
    "language": "",
    "dock-tab-position": "bottom",
    "font-family": "",
    "font-point-size": -1.0,
    "save-ui-state-on-close": false,
    "undo-stack-limit": 20
  },
  "log": {
    "level": "trace",
    "queue-level": "info",
    "output-stdout": true,
    "rotation-mode": "rotating",
    "max-size-bytes": 10485760,
    "max-files": 5,
    "show-log-num": 5000
  },
  "python": { "interpreter-path": "", "extra-paths": [] },
  "advanced": {
    "recent-files-max": 10,
    "dump-retention-days": 7,
    "show-splash": true,
    "autosave-interval-min": 0
  },
  "script": {
    "workspace-dir": "",
    "workspace-overwrite-policy": "ask",
    "timeout-sec": 300,
    "result-max-chars": 10000
  }
}
```

取值约定：项目自定义的枚举写成可读字符串（`rotating`/`daily`/`console`、`bottom`/`top`、`ask`/`always-local`/`always-zip`、日志级别 `trace` 到 `off`），Ribbon 样式与主题沿用第三方库的整数枚举。读取时字符串和整数都接受。`version` 供将来升级迁移使用，当前是 `1`。

### 设置项与默认值

| JSON 路径 | 含义 | 默认值 |
|-----------|------|--------|
| `ui.ribbon-style` | Ribbon 布局样式（`SARibbonBar::RibbonStyles` 整数） | `34`（紧凑两行） |
| `ui.ribbon-theme` | Ribbon 主题序号，`-1` 表示不覆盖框架默认主题 | `-1` |
| `ui.language` | 界面语言代码，空表示跟随系统 | 空 |
| `ui.dock-tab-position` | 操作窗口内部标签页方位 | `bottom` |
| `ui.font-family` | 界面字体族，空表示系统默认 | 空 |
| `ui.font-point-size` | 界面字号，`<=0` 表示系统默认 | `-1.0` |
| `ui.save-ui-state-on-close` | 关闭时是否保存界面几何与状态 | `false` |
| `ui.undo-stack-limit` | 数据页撤销栈深度，`0` 表示无限制 | `20` |
| `log.level` | 文件日志级别 | `trace` |
| `log.queue-level` | 进消息面板的日志级别 | `info` |
| `log.output-stdout` | 是否把日志输出到标准输出 | `true` |
| `log.rotation-mode` | 轮转方式 | `rotating` |
| `log.max-size-bytes` | 单个日志文件最大字节数 | `10485760` |
| `log.max-files` | 保留的日志文件份数 | `5` |
| `log.show-log-num` | 消息面板保留的日志条数 | `5000` |
| `python.interpreter-path` | Python 解释器路径，空表示自动探测 | 空 |
| `python.extra-paths` | 额外的 Python 模块搜索路径 | 空 |
| `advanced.recent-files-max` | 最近文件列表上限 | `10` |
| `advanced.dump-retention-days` | 崩溃转储保留天数 | `7` |
| `advanced.show-splash` | 启动时是否显示启动画面 | `true` |
| `advanced.autosave-interval-min` | 自动保存间隔（分钟），`0` 表示关闭 | `0` |
| `script.workspace-dir` | 脚本工作区根目录，空表示系统临时目录 | 空 |
| `script.workspace-overwrite-policy` | 工作区冲突策略 | `ask` |
| `script.timeout-sec` | 脚本执行超时秒数，`0` 表示禁用 | `300` |
| `script.result-max-chars` | 脚本结果序列化后的最大字符数 | `10000` |

### 取值校验

默认值写在 `DAAppConfig.h` 的字段上；取值范围校验集中在 `DAAppConfig::normalize()`（加载后）和各 `setXxx()`（设置页写回时），例如日志文件大小 `<=0` 回落到 10MB、撤销深度 `<0` 回落到 20、枚举取值越界回落到默认值。消费方拿到的值已经合法，不需要再重复判断。

### 从旧版 XML 迁移

旧版本用 `dawork-config.xml` 存储同样的设置。`load()` 在找不到 JSON 文件时会读取同目录的旧 XML，按新旧路径映射填好字段，写出 JSON，再把旧文件改名为 `dawork-config.xml.bak`。迁移只发生一次；JSON 已存在时不再读 XML。

旧版里 `workflow-timeout`、`plugin-extra-paths`、`node-script-paths` 三个键已经删除（程序从未读取它们），迁移时直接忽略。插件搜索路径固定为可执行文件目录下的 `plugins`（`DAPluginManager::getPluginDirPath()`），Python 节点包固定为 `pyplugins`（`DAAppPluginManager::getPyPluginDirPath()`）。

### 未知键与版本兼容

保存时以加载到的原始 JSON 对象为底，只覆盖已知路径。因此将来版本新增的键在旧版本保存时不会被抹掉；文件里的 `version` 比当前程序新时会在日志里给出提示，但仍继续读取。

### 变更通知

`DAAppConfig` 是 `QObject`，任一 `setXxx()` 写回后发出 `sectionChanged(Section)` 和 `changed()`。`AppMainWindow` 订阅分节信号，交给 `applyAppSettings()`（`src/APP/DAAppConfigApplier.cpp`）按节热应用；启动阶段则用 `applyAllAppSettings()` 一次性应用全部。写配置时不要直接改字段，必须走 `setXxx()`，否则订阅方收不到通知。

`applyAppSettings()` 只读配置、只改运行时状态，**不得反向写配置**，否则会与信号形成回环。

语言、字体、Python 解释器路径、日志轮转参数、启动画面这几项仍在启动早期读取，修改后需要重启程序才生效。

### 新增一个配置项的步骤

1. 在 `DAAppConfig.h` 对应的结构体里加字段并给出默认值。
2. 需要约束范围时，在 `DAAppConfig.cpp` 的 `normalizeXxx()` 里加规则（`setXxx()` 会复用）。
3. 在 `toJson()` 和 `loadFromJsonObject()` 各加一行读写。
4. 设置页改用 `xxx()` 读取、`setXxx()` 写回。
5. 需要立即生效时，在 `DAAppConfigApplier.cpp` 的对应分节里施加。
6. 更新本文档的键表。

## python-config.json

放在可执行文件目录下，记录解释器位置。查找顺序由 `DAPyInterpreter::getPythonInterpreterPath()` 决定：先读这个文件，读不到再退回在系统路径里找 `python`。

设置页的「Python」页写入的就是这个文件（`DASettingPagePython::writePythonConfigJson`），同时也会把解释器路径写进 `dawork-config.json` 的 `python.interpreter-path`（用于界面回显）。两处是双写关系，实际生效的是 `python-config.json`，因为解释器在启动早期就要初始化。解释器路径变更需要重启程序。

## agent-config.json 与 agent-permissions.json

智能体的配置单独放，不走 `dawork-config.json`。原因是 `DAAgent` 模块不依赖应用层的 `DAAppConfig`，它由自己的领域模型 `DAAgentConfig` 读写，`load()` 和 `save()` 是唯一碰存储格式的代码。

- `agent-config.json` 放模型供应商列表、模型清单和「Agent 设置」页上的那些数值项。密钥在内存里是明文，落盘时用 Windows 的数据保护接口加密。
- `agent-permissions.json` 放权限引擎的配置：文件路径规则、代码危险模式、工具分级覆盖，以及默认权限模式等标量。

两者都在配置目录下。保存时会向 Python 子进程发一次重新配置，所以改完立即生效，不需要重启。

历史上这两项配置存在 `agent-config.ini` 里，加载时会自动迁移，迁移后留下 `.bak` 文件。

## shortcuts.json

快捷键的用户覆盖单独放一个文件，不走 `dawork-config.json`。命令的默认按键写在代码里，也就是各 action 创建处就地设置的快捷键；文件里只记用户改过的那些，键是命令的 objectName，值是按键序列的文本，空字符串表示用户把这个命令的按键清掉了。没出现在文件里的命令一律用默认按键，所以版本升级新增的默认按键不会被旧文件盖掉。

读写都经 `DAShortcutManager`：启动时 `load()` 读覆盖表，设置页点应用时 `save()` 落盘，保存用临时文件加改名的方式原子写入。文件在配置目录下，删掉它等于把全部按键恢复成出厂值。命令的注册与分类见 `DAAppShortcutSetup`，设置页是 `DASettingPageShortcuts`。

## 其余文件

| 文件 | 位置 | 内容 |
|------|------|------|
| `dawork-layouts.json` | 配置目录 | 布局管理器里保存的界面布局方案，含两个不可删除的预设 |
| `.dawork-ui-state` | 配置目录 | 主窗口几何、Qt 窗口状态和停靠管理器状态，在「通用」页勾选记录时于关闭时写入 |
| `recent-files.ini` | 配置目录 | 最近打开的文件列表 |
| `da_log.log` | 配置目录上一层的 `log` 目录 | 程序日志 |
| `da_pyscript.log` | 同上 | Python 侧日志，由 loguru 写入 |
| `sessions/` | 配置目录上一层 | 智能体会话，每个会话一个 `.jsonl`，另有索引和最后活跃记录 |
| `dumps/` | 配置目录上一层 | 崩溃转储，按保留天数清理 |
| `plugins/.pluginignore` | 可执行文件目录 | 被停用的 C++ 插件名单，一行一个 |
| `pyplugins/.pluginignore` | 可执行文件目录 | 被停用的 Python 节点包名单 |

`.pluginignore` 不存在时会自动生成并写入注释模板。`pyplugins` 目录本身不会自动创建，需要有节点包才会存在。

## 关于 QSettings

`main` 启动时调用 `QSettings::setDefaultFormat(QSettings::IniFormat)`，并用 `QSettings::setPath` 把用户作用域重定向到配置目录。这是给第三方库默认构造的 `QSettings` 用的，比如随项目维护的部件库里那份最近文件管理器的副本。程序自身不用 `QSettings` 读写设置，它显式读写 `dawork-config.json`、`agent-config.json`、`recent-files.ini` 和 `shortcuts.json`。

早先版本把一部分设置写在 Windows 注册表里，启动时会做一次迁移：智能体相关的键迁到 `agent-config.ini`，最近文件迁到 `recent-files.ini`。迁移函数是 `migrateSettingsFromRegistry`。
