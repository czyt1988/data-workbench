# 通过插件注册内置 Agent（以 DAPaperAgent 为例）

本文以 `plugins/DAPaperAgent/`（论文撰写 agent 插件）为参考实现，讲解如何通过插件机制向平台注入：

1. **一个内置 agent 提示词**（出现在 Ribbon「AI Agent」gallery 与管理对话框中，用户可运行、编辑、删除）
2. **若干领域工具**（agent 可调用的 C++ 工具，如文献检索）
3. **一个系统提示词片段**（教所有 agent 使用这些工具）

DAPaperAgent 是 `DAAgentInterface::registerBuiltinAgent` 机制的第一个使用者，可作为新插件的最佳模板。

---

## 机制总览

```
插件 initialize()（AppMainWindow::initPlugins 时机）
    │
    ├─ agent->registerBuiltinAgent(name, content)
    │      └─ DAAgentManager::registerBuiltin：
    │         仅当 <exe>/daAgent/<name>.md 不存在时写入（幂等，尊重用户编辑）
    │         → loadAgents() → emit agentListChanged
    │              └─ DAAppRibbonArea 兜底刷新 gallery（首次构建早于插件加载）
    │
    ├─ agent->registerTool(tool)
    │      └─ DAAgentModule::registerTool：校验 snake_case/重名 → 注册表
    │         → m_bridge->setTools（子进程 init 消息携带完整 schema）
    │
    └─ agent->registerSystemPrompt(name, content)
           └─ 追加到系统提示词（每次子进程启动时组装下发）
```

三个接口都定义在 `src/DAAgent/DAAgentInterface.h`，实现见 `src/DAAgent/DAAgentModule.cpp`。

---

## 插件文件清单

```
plugins/DAPaperAgent/
├── CMakeLists.txt                  # da_add_plugin 声明（见下）
├── DAPaperAgentPlugin.h/.cpp       # 插件类：QObject + DAAbstractPlugin
├── tools/
│   ├── DAPaperLiteratureApi.h/.cpp # 共享辅助：同步 HTTP + CrossRef/OpenAlex 解析
│   ├── DAPaperToolSearchLiterature.h/.cpp   # search_literature 工具
│   └── DAPaperToolVerifyDoi.h/.cpp          # verify_doi 工具
└── resources/
    ├── paper-agent.qrc             # 内嵌提示词（前缀 /da/paper）
    └── paper-agent.md              # 论文撰写助手提示词（核心资产）
```

### 插件类

```cpp
class DAPaperAgentPlugin : public QObject, public DAAbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DAABSTRACTPLUGIN_IID)
    Q_INTERFACES(DA::DAAbstractPlugin)
    // getIID/getName/getVersion/getDescription 略，见 DAAgentToolsPlugin
};
```

要点：

- **直接继承 `DAAbstractPlugin`**（不需要节点工厂）——与 `DAAgentToolsPlugin` 相同，这是 agent 工具类插件的形态；不要用 `plugin-template`（那是节点插件模板）
- **插件类持有工具 QObject 的 parent**，工具随插件释放

### initialize() 三连调用

```cpp
bool DAPaperAgentPlugin::initialize()
{
    auto* c = core();
    auto* agent = c->getAgentInterface();
    if (!c || !agent) return false;

    // 1. 内置 agent：提示词从 qrc 读取（随 dll 部署，无磁盘依赖）
    QFile promptFile(QStringLiteral(":/da/paper/paper-agent.md"));
    if (promptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        agent->registerBuiltinAgent(QStringLiteral("论文撰写助手"),
                                    QString::fromUtf8(promptFile.readAll()));
    }
    // 2. 领域工具
    agent->registerTool(new DAPaperToolSearchLiterature(c, this));
    agent->registerTool(new DAPaperToolVerifyDoi(c, this));
    // 3. 工具用法系统提示词片段
    agent->registerSystemPrompt(QStringLiteral("literature_tools"), QStringLiteral("..."));
    return DAAbstractPlugin::initialize();
}
```

**为什么提示词用 qrc 内嵌而非磁盘文件**：插件分发形态是单个 dll，qrc 保证提示词随插件部署到任何机器；`registerBuiltinAgent` 的"仅不存在时写入"语义保证用户编辑过的 `daAgent/论文撰写助手.md` 永远不会被插件覆盖。

### CMakeLists 要点

```cmake
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_plugin_utils.cmake)
    da_plugin_bootstrap("DAPaperAgent" "Paper writing agent plugin")
endif()
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp")
file(GLOB DA_PLUGIN_QRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.qrc")

da_add_plugin(
    NAME DAPaperAgent
    BUILD_DEFINE DAPAPERAGENT_PLUGIN_BUILD
    SOURCES ${DA_PLUGIN_HEADER_FILES} ${DA_PLUGIN_SOURCE_FILES} ${DA_PLUGIN_QRC_FILES}
    QT_PRIVATE Core Gui Widgets Network        # Network：文献工具的 QNetworkAccessManager
    LINK_PUBLIC DAAgent                        # DAAgentToolBase 跨 DLL moc
    LINK_PRIVATE DAPluginSupport               # 插件基类不经传递，必须显式
    THIRDPARTY python                          # DAAgentToolBase.h → DAData.h → pybind11 链
    INCLUDE_PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
```

顶层注册：`plugins/CMakeLists.txt` 加 `add_subdirectory(DAPaperAgent)`。

---

## 工具实现要点

工具继承 `DAAgentToolBase`（`src/DAAgent/DAAgentToolBase.h`），模式与 `plugins/DAAgentTools/tools/` 完全一致：

```cpp
class DAPaperToolSearchLiterature : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;   // 禁手写构造
    DAAgentToolSpec getToolSpec() const override;   // name 为 snake_case，description 英文
    QJsonObject execute(const QJsonObject& params) override;  // 同步执行
    QString getOwnerModule() const override { return QStringLiteral("DAPaperAgent"); }
};
```

DAPaperAgent 的两个工具特殊在需要**网络请求**，而工具 `execute` 是主线程同步调用。解决模式（见 `tools/DAPaperLiteratureApi.cpp`）：

```cpp
QNetworkAccessManager nam;
QNetworkReply* reply = nam.get(request);
QTimer::singleShot(10000, reply, [reply]() { if (reply->isRunning()) reply->abort(); });
QEventLoop loop;
QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
loop.exec(QEventLoop::ExcludeUserInputEvents);  // 防用户输入重入
```

该模式来自 `DAAppProject::waitArchiveSave` 的同步等待先例；`ExcludeUserInputEvents` 保证等待期间不处理用户输入（避免重入 `executeTool`），QProcess 协议读写仍可进行。**超时必须设置**（10s），否则断网时工具永久卡死。

工具的 `getToolSpec` 应在 description 中写明已知限制（如"需直连网络，不走代理"），LLM 会据此降级。

---

## 时序：为什么需要 agentListChanged 兜底

Ribbon 的 agent gallery 在 `AppMainWindow` 构造期（`createUi` → `buildRibbonAgentCategory`）首次填充，而插件 `initialize()` 在其后 `init()` → `initPlugins()` 才执行——插件注入的 agent 错过了首次填充。

因此 `DAAgentModule::initialize` 把 `DAAgentManager::agentListChanged` 透传为接口信号（与 `subagentListChanged` 同构），`DAAppRibbonArea::buildRibbonAgentCategory` 末尾连接该信号兜底刷新 gallery：

```cpp
connect(agent, &DA::DAAgentInterface::agentListChanged, this, [this]() {
    QString prevTitle = mSelectedAgentTitle;
    populateAgentGallery();          // 重新读取 daAgent/ 目录
    if (!prevTitle.isEmpty()) mSelectedAgentTitle = prevTitle;  // 保留选中
});
```

用户在管理对话框保存/删除 agent 同样触发此信号，gallery 始终与磁盘状态一致。

---

## 内置 agent 提示词的写法

`resources/paper-agent.md` 是约 200 行的中文流程编排型提示词（对比 `src/DAAgent/default-agent.md` 的 25 行简短风格），包含：

- **角色与核心原则**（数据的客观性绝不被叙事需求干扰）
- **启动流程**（跨会话恢复：读 progress.md 续作）
- **六阶段工作流**（数据理解 → 建模与锁定 → 图表 → 文献与大纲 → 写作 → 评审定稿），每阶段以 `ask_user` 确认为门禁
- **铁律**（防"叙事污染数据"：结果包锁定后只读、统计量逐字转述、禁止编造引用等）

设计要点：

1. **状态持久化靠文件约定**，不需要 C++ 支持：`daPaper/<论文名>/progress.md` + `.approved/<阶段>.md` 审批标记，全部由 agent 经 `write_file`/`read_file` 自管理
2. **门禁用 `ask_user`**：这是平台内置的人机交互工具，每阶段结束向用户呈现摘要并等待确认
3. **降级路径写进提示词**：statsmodels 缺失时降级 scipy、文献 API 不可达时切离线模式，agent 不会因环境缺失而卡死

---

## 新建同类插件的最短路径

1. 复制 `plugins/DAPaperAgent/` 为模板，改名插件类/目录/`BUILD_DEFINE`
2. 替换 `resources/paper-agent.md` 为你的领域提示词，qrc 前缀自定（避免与其它插件冲突）
3. 工具按需增删（继承 `DAAgentToolBase`；数据访问用 `dataMgr()/findData()/allDatas()`）
4. `plugins/CMakeLists.txt` 加 `add_subdirectory`
5. 构建：`.\scripts\build.ps1 -Target <插件名>`
6. 验证：启动后检查 Ribbon gallery 出现你的 agent；`da_log.log` 中 `setTools` 收录你的工具名
