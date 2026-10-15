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

## dawork-config.xml

应用设置由 `DAAppConfig` 管理，文件名 `dawork-config.xml`（`DAAppConfig::getConfigFileName`）。它继承 `DAProperties` 和 `DAXMLFileInterface`，启动时 `loadConfig()` 读取，退出时写回。

文件是一个扁平的表，根节点 `<configs>`，每个设置项是一个 `<prop key="...">`，值放在 `<value>` 子节点里。

```xml
<?xml version="1.0" encoding="UTF-8"?>
<configs ver="0.1.1" name="da-app">
  <prop key="language"><value>zh_CN</value></prop>
  <prop key="ribbon-style"><value>0</value></prop>
  <prop key="show-splash"><value>true</value></prop>
  <prop key="log-level"><value>0</value></prop>
  <prop key="log-max-size"><value>10485760</value></prop>
</configs>
```

`ver` 属性是配置版本号，`name` 是配置名（`DAAppConfig::cConfigName`）。上面的例子里 `language` 是界面语言代码，空值表示跟随系统；`ribbon-style` 是 Ribbon 的四种布局之一；`log-level` 是枚举序号，`0` 对应 Trace；`log-max-size` 的单位是字节。设置项在源码里由 `DA_CONFIG_KEY_*` 宏定义，默认值在 `DAAppConfig` 的构造函数里赋。

### 设置项与默认值

| 键名 | 含义 | 默认值 |
|------|------|--------|
| `ribbon-style` | Ribbon 布局样式 | `CompactTwoRow` |
| `ribbon-theme` | Ribbon 主题序号 | `-1` |
| `app-font-family` / `app-font-size` | 界面字体族与字号，空值与 0 表示跟随系统 | 空、`0` |
| `language` | 界面语言代码，空表示跟随系统 | 空 |
| `dock-tab-position` | 内层停靠标签页的位置 | `bottom` |
| `save-ui-state-on-close` | 关闭时是否保存界面几何与状态 | `false` |
| `show-splash` | 启动时是否显示启动画面 | `true` |
| `show-log-num` | 消息面板保留的日志条数 | `5000` |
| `python-interpreter-path` | Python 解释器路径 | 空（自动探测） |
| `python-extra-paths` | 额外的 Python 模块搜索路径 | 空 |
| `log-level` | 文件日志级别 | Trace |
| `log-queue-level` | 进消息面板的日志级别 | Info |
| `log-output-stdout` | 是否把日志输出到标准输出 | `true` |
| `log-rotation-mode` | 轮转方式，0 按大小、1 按天、2 只输出控制台 | `0` |
| `log-max-size` | 单个日志文件最大字节数 | `10485760` |
| `log-max-files` | 保留的日志文件份数 | `5` |
| `autosave-interval` | 自动保存间隔，0 表示关闭 | `0` |
| `recent-files-max` | 最近文件列表上限 | `10` |
| `dump-retention-days` | 崩溃转储保留天数 | `7` |
| `workspace-dir` | 脚本工作区根目录，空表示系统临时目录 | 空 |
| `workspace-overwrite-policy` | 工作区冲突策略，`ask`、`always-local` 或 `always-zip` | `ask` |
| `script-timeout` | 脚本执行超时秒数 | `300` |
| `script-result-max-chars` | 脚本结果最大字符数 | `10000` |
| `plugin-extra-paths` | 插件搜索路径 | 空 |
| `node-script-paths` | 节点脚本搜索路径 | 空 |
| `workflow-timeout` | 工作流执行超时秒数，−1 表示不限 | `-1` |

### 有界面但没有消费方的三个键

`plugin-extra-paths`、`node-script-paths` 和 `workflow-timeout` 只被设置页写入和读回，程序里没有任何地方读它们去改变行为。唯一能搜到的用法是 `DAAppConfig.h` 里的宏定义和设置页的读写代码。填了这些值不会报错，也不会有任何效果。

真正决定插件搜索位置的常量在 `DAPluginManager::getPluginDirPath()` 里，返回可执行文件目录下的 `plugins`；Python 节点包由 `DAAppPluginManager::getPyPluginDirPath()` 固定在可执行文件目录下的 `pyplugins`。

## python-config.json

放在可执行文件目录下，记录解释器位置。查找顺序由 `DAPyInterpreter::getPythonInterpreterPath()` 决定：先读这个文件，读不到再退回在系统路径里找 `python`。

设置页的「Python」页写入的就是这个文件（`DASettingPagePython::writePythonConfigJson`）。解释器路径变更需要重启程序，因为解释器在启动早期就初始化好了。

## agent-config.json 与 agent-permissions.json

智能体的配置单独放，不走 `dawork-config.xml`。原因是 `DAAgent` 模块不依赖应用层的 `DAAppConfig`，它由自己的领域模型 `DAAgentConfig` 读写，`load()` 和 `save()` 是唯一碰存储格式的代码。

- `agent-config.json` 放模型供应商列表、模型清单和「Agent 设置」页上的那些数值项。密钥在内存里是明文，落盘时用 Windows 的数据保护接口加密。
- `agent-permissions.json` 放权限引擎的配置：文件路径规则、代码危险模式、工具分级覆盖，以及默认权限模式等标量。

两者都在配置目录下。保存时会向 Python 子进程发一次重新配置，所以改完立即生效，不需要重启。

历史上这两项配置存在 `agent-config.ini` 里，加载时会自动迁移，迁移后留下 `.bak` 文件。

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

`main` 启动时调用 `QSettings::setDefaultFormat(QSettings::IniFormat)`，并用 `QSettings::setPath` 把用户作用域重定向到配置目录。这是给第三方库默认构造的 `QSettings` 用的，比如随项目维护的部件库里那份最近文件管理器的副本。程序自身不用 `QSettings` 读写设置，它显式读写 `dawork-config.xml`、`agent-config.json` 和 `recent-files.ini`。

早先版本把一部分设置写在 Windows 注册表里，启动时会做一次迁移：智能体相关的键迁到 `agent-config.ini`，最近文件迁到 `recent-files.ini`。迁移函数是 `migrateSettingsFromRegistry`。
