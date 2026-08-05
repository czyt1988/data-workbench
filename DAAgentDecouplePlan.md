# DAAgent 模块解耦重构方案

> **状态**:已通过 5 轮 grilling 讨论确认核心决策,待实施
> **日期**:2026-08-05
> **目标**:彻底解除 DAAgent 对 DAGui 的依赖,使 DAAgent 成为纯粹的"agent 框架库"

---

## 一、背景与问题陈述

### 1.1 问题一:DAAgent 不合理地依赖 DAGui

`src/DAAgent/AGENTS.md` 明确写道:

> **层级**:DAAgent 位于接口层(Layer 4,与 DAInterface/DAPluginSupport 同级)。它向上提供 `DAAgentInterface` 供插件注册工具/提示词、控制 UI 显隐与 LLM 配置;**向下依赖 DAGui(Layer 3)**等。

按项目五层架构的铁律——"上层可以依赖下层,下层绝不能依赖上层"——L4 依赖 L3 在方向上是**合规**的。但问题在于:**DAAgent 的职责是 agent 推理调度,与 GUI 无任何业务关联**。当前依赖纯粹是实现层面的耦合(工具代码写错位置 + Dock 持有方式不当),不是架构必然。

### 1.2 问题二:DAAgentSettingsWidget 用 QSettings 绕过 DAAppConfig

`src/DAAgent/AGENTS.md` 第九章写道:

> DAAgent 库**无法链接 APP 的 DAAppConfig**,因此用 QSettings 持久化(`DAAgentModule::getLLMConfig` 与 `DAAgentSettingsWidget` 共用同一存储源)。

代码注释(`DAAgentModule.cpp:294`、`DAAgentSettingsWidget.cpp:13`)也反复强调这一点。但 `DAAgentInterface` **已经**暴露了 `getLLMConfig()` / `setLLMConfig()` 接口——APP 层完全可以通过接口读写 agent 配置。问题只在于设置页(DAGui 里的 `DAAgentSettingsWidget`)直接写 QSettings 文件绕过了接口,这是实现缺陷,不是架构必然。

APP 层是最顶层,应该是 APP 访问 DAAgent 库的内容,而不是 DAAgent 去访问 APP 层。DAAgent 的设置参数应该通过接口暴露让 APP 层访问。

### 1.3 核心思想

> **DAAgent 应该是一个纯 agent 框架库**:提供工具注册机制、信号通知机制、LLM 配置接口、子进程通信管理。至于具体注册哪些工具、UI 怎么接线、设置页怎么持久化——全部由插件/APP 层决定。

---

## 二、代码调查关键发现

### 2.1 DAAgent 依赖 DAGui 的三条独立链路

| 链路 | 位置 | 性质 |
|------|------|------|
| **链路 1:工具层** | `src/DAAgent/tools/DAAgentToolBase.h` 直接 `#include "DAChartOperateWidget.h"` 等 DAGui 类 | 8 个绘图工具(create_chart / add_curve 等)访问 DAGui 的图表操作窗口 |
| **链路 2:Dock 持有层** | `DAAgentModule` 持有 `DAAgentDockWidget* m_dockWidget`,在 `connectSignals()` 中私连 9 个信号到 Dock 槽 | DAAgentModule 直接 `#include "DAAgentDockWidget.h"` |
| **链路 3:加解密** | `DAAgentModule.cpp:301,321` 调用 `DAAgentSettingsWidget::encryptApiKey/decryptApiKey`(DAGui 的静态方法) | DPAPI 加密方法定义在 DAGui 的设置页里,但 DAAgent 在调用 |

CMake 层面(`src/DAAgent/CMakeLists.txt:51-52`):
```cmake
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ...
    ${DA_PROJECT_NAME}::DAGui  # ← PUBLIC 依赖 DAGui
)
```

### 2.2 插件机制已就绪(绿地条件具备)

| 检查项 | 结果 | 位置 |
|--------|------|------|
| `DACoreInterface::getAgentInterface()` 已暴露? | ✅ | `src/DAInterface/DACoreInterface.h:41` |
| 插件能拿到 `DACoreInterface*`? | ✅ | `DAAbstractPlugin::core()`(`src/DAPluginSupport/DAAbstractPlugin.h:49`) |
| 插件加载在懒启动之前? | ✅ | 懒启动在首次 `sendMessage()` 才触发,插件 `initialize()` 必然先于此 |
| `DAAbstractAgentTool` 有导出宏? | ✅ | `DAAgent_API`(`src/DAAgent/DAAbstractAgentTool.h:24`),插件可安全跨 DLL 继承 |
| 当前有插件用 `registerTool`? | ❌ | 无(grep 0 命中),C 方案是绿地设计 |

### 2.3 设置页现状对比

| 设置页 | 位置 | 持久化方式 | 注入方式 |
|--------|------|-----------|---------|
| DASettingPagePython | APP (L5) | 通过 `DAAppConfig*` | `setAppConfig(DAAppConfig*)` |
| DASettingPageGeneral | APP (L5) | 通过 `DAAppConfig*` | `setAppConfig(DAAppConfig*)` |
| DASettingPageLog | APP (L5) | 通过 `DAAppConfig*` | `setAppConfig(DAAppConfig*)` |
| DASettingPageAdvanced | APP (L5) | 通过 `DAAppConfig*` | `setAppConfig(DAAppConfig*)` |
| **DAAgentSettingsWidget** | **DAGui (L3)** | **QSettings 绕过** | **无注入** |

`DAAgentSettingsWidget` 是唯一在 DAGui 里、且不走 `DAAppConfig` 注入的设置页。它在 `DAAppSettingDialog.cpp:44` 被 `new` 出来,但没有像其他设置页那样被注入任何配置指针。

### 2.4 其他关键事实

- **DAGui 不 link DAAgent**:grep 确认 DAGui 的 CMakeLists.txt 中无 DAAgent 引用
- **APP 已 link DAAgent**:`src/APP/CMakeLists.txt:98` 已有 `${DA_PROJECT_NAME}::DAAgent`
- **DAGui 引入了 Crypt32 仅为 DPAPI 加密**:Crypt32 将随 DAAgentSettingsWidget 搬走而可移除
- **DAAbstractPlugin 是通用插件基类**:IID `"org.da.abstract.plugin"`(`src/DAPluginSupport/DAAbstractPlugin.h:72`),与 DataAnalysis 用的 DAAbstractNodePlugin 不同
- **DAAgentBridge 有 9 个信号**:`agentToken` / `agentMessageComplete` / `agentToolCall` / `agentToolResult` / `agentQuestion` / `agentError` / `agentReady` / `agentBusy` / `agentDone`(`src/DAAgent/DAAgentBridge.h:105-152`)
- **DAAgentInterface 当前无 signals 段**:信号链靠 `DAAgentModule::connectSignals()` 私连,接口层无信号暴露

---

## 三、核心决策(经 grilling 讨论确认)

### 3.1 决策 C:16 个内置工具插件化

**问题**:8 个绘图工具依赖 DAGui 的 `DAChartOperateWidget` / `DAFigureWidget` / `DAChartWidget`,是 DAAgent 依赖 DAGui 的链路 1 根源。

| 选项 | 含义 | 选择 |
|------|------|------|
| A | 工具迁到 `src/DAGui/Agent/tools/`,DAGui 注册给 DAAgent | ❌ 依赖更混乱,被否决 |
| B | 工具留 DAAgent,在 DAInterface 定义抽象 `IChartOperate`,DAGui 实现 | 为单一调用点造抽象,代价高 |
| **C** | **工具整体搬到新插件 `plugins/DAAgentTools/`,通过 `DAAgentInterface::registerTool` 注册** | **✅ 最终选择** |

**核心理由**:
- "便于扩展"是 stated 的核心目标——插件是扩展的标准载体,DataAnalysis 插件已是成熟范式
- DAAgent 提供通用工具注册机制,具体哪些工具注册由插件决定
- 未来写领域工具插件(如企业版图表工具)无需改 DAAgent

### 3.2 决策 D1+D3b:接口信号转发 + APP 层 connect

**问题**:`DAAgentModule` 持有 `DAAgentDockWidget*` 并在 `connectSignals()` 私连信号,是链路 2 根源。

**子决策 D1**:`DAAgentInterface` 增加对应信号,`DAAgentModule` 转发 Bridge 的信号 emit 出去。

**子决策 D3b**(D3a 被否决,因 DAGui 无法访问 DACoreInterface):

| 方案 | 含义 | 问题 |
|------|------|------|
| D3a | DAGui 持有 `DAAgentInterface*` 指针,Dock 主动 connect | ❌ DAGui(L3)无法拿到 DAInterface 层的 `DACoreInterface`,走不通;且要求 DAGui link DAAgent(L4),违反层级铁律 |
| **D3b** | **DAGui 完全不感知 DAAgent,Dock 只暴露槽函数和信号,由 DAAppController 完成 connect** | **✅ 最终选择** |

**D3b 落地后信号流**:
```
用户输入 → DockWidget::sendMessageRequested  ──┐
                                                ├─ DAAppController::connect() ─→ DAAgentInterface::sendMessage
DAAgentInterface::agentToken ───────────────────┤
DAAgentInterface::agentMessageComplete ─────────┤
DAAgentInterface::agentToolCall ────────────────┤
... (9 个信号)                                  └─ DAAppController::connect() ─→ DockWidget::onAgentXxx 槽
```

**核心理由**:
- DAGui 与 DAAgent 完全解耦,两个 L3/L4 模块互不依赖,是兄弟模块
- 与现有架构一致——DAGui 本来就不 link DAAgent
- connect 工作量不大——~11 行 `connect()` 代码(原 `DAAgentModule::connectSignals()` 的逻辑搬过去)
- 如果将来有第二个 agent 实现,DAGui 的 Dock 完全不用改

### 3.3 决策 E1:设置页迁移到 APP

**问题**:`DAAgentSettingsWidget` 在 DAGui 里用 QSettings 绕过接口,是链路 3 根源,也是问题二的直接体现。

| 选项 | 含义 | 选择 |
|------|------|------|
| E1 | 设置页搬到 APP,通过 `setAgentInterface` 注入,`apply()` 调接口持久化 | ✅ 与现有 4 个 APP 设置页一致;信号少一点 |
| E2 | 设置页留 DAGui,用信号槽模式,APP 负责 connect | agent UI 分裂(Dock 在 DAGui,Settings 在 APP) |
| E3 | 保持现状 | ❌ 用户明确反对 |

**核心理由**:
- 与整个程序先统一(与 DASettingPagePython 等同层)
- 信号少一点(无需中间信号转发)
- 加解密方法归位——`encryptApiKey/decryptApiKey` 从 DAGui 搬到 DAAgent,DAAgentModule::setLLMConfig 内部加密后写 QSettings;设置页只管传明文 QJsonObject,不接触加密

### 3.4 决策 F1+G2:单插件 + 工具基类拆分

**问题**:`DAAgentToolBase.h` 是"胖基类",混合了数据方法(只依赖 DAData)和图表方法(依赖 DAGui)。

| 子决策 | 含义 | 选择 |
|--------|------|------|
| **F1** | 16 个工具全部放到**一个新插件** `plugins/DAAgentTools/` | ✅ 16 个工具都是平台默认,没必要拆多插件 |
| F3 | 16 个工具由 APP 直接注册(不建插件) | APP 已 190 文件,继续膨胀;且工具代码混在 APP 里不如独立插件清晰 |
| **G2** | DAAgentToolBase **拆分**——瘦身版(数据+响应)留 DAAgent,图表方法搬到插件的 `DAAgentChartToolBase` | ✅ 未来写数据工具插件继承瘦身版即可,不用拉 DAGui 依赖 |
| G1 | DAAgentToolBase 整体搬到插件 | 未来写数据工具的插件要自己造轮子 |

**落地后工具继承关系**:
```
DAAbstractAgentTool (DAAgent, 纯虚)
    └─ DAAgentToolBase (DAAgent, 瘦身版: dataMgr/findData/allDatas/errorResponse/successResponse)
        ├─ DAAgentToolListData     (插件, 直接继承瘦身版)
        ├─ DAAgentToolDataInfo     (插件)
        ├─ ... (8 个数据/文件工具)
        └─ DAAgentChartToolBase    (插件, 新增: chartOperateWidget/currentFigure/findChart/enableAutoScale 等)
            ├─ DAAgentToolCreateChart   (插件, 继承图表基类)
            ├─ DAAgentToolAddCurve      (插件)
            └─ ... (8 个绘图工具)
```

---

## 四、最终依赖关系

```
重构前:                              重构后:

DAAgent ──PUBLIC──→ DAGui            DAAgent (无 GUI 依赖,纯框架)
  │                                    ↑
  ├─ tools/ (16 个工具)                ├─ PUBLIC link ← APP (DAAppCore 持有)
  │   └─ #include DAChartOperateWidget │
  └─ #include DAAgentDockWidget        ├─ PUBLIC link ← plugins/DAAgentTools/ (16 个工具)
      └─ connectSignals 私连           │
                                       └─ DAAbstractAgentTool 导出宏供插件跨 DLL 继承

DAGui ──(不 link DAAgent)             DAGui ──(不 link DAAgent,无 Crypt32)
  └─ Agent/DAAgentDockWidget             └─ Agent/DAAgentDockWidget (保留,槽函数不变)
  └─ Agent/DAAgentSettingsWidget         └─ (DAAgentSettingsWidget 搬到 APP)
  └─ Crypt32 (DPAPI)
```

---

## 五、分阶段执行计划

### Phase 1:DAAgent 模块瘦身(纯框架化)

#### 1.1 DAAgentInterface.h — 增加 9 个信号(D1)

**文件**:`src/DAAgent/DAAgentInterface.h`

新增 signals 段(对照 `DAAgentBridge.h:105-152` 的签名):
```cpp
Q_SIGNALS:
    void agentToken(const QString& token);
    void agentMessageComplete(const QString& fullText);
    void agentToolCall(const QString& toolName, const QJsonObject& args);
    void agentToolResult(const QString& toolName, const QJsonObject& result);
    void agentQuestion(const QString& text, const QStringList& options, bool multiSelect);
    void agentError(const QString& message);
    void agentReady(const QString& model);
    void agentBusy(bool busy);
    void agentDone();
```

同时新增 `sendUserAnswer` 虚函数(当前接口缺失,对应 `DAAgentBridge::sendUserAnswer`):
```cpp
virtual void sendUserAnswer(const QString& answer) = 0;
```

#### 1.2 DAAgentModule — 删除 Dock 持有,转发 Bridge 信号(D1+D3b)

**文件**:`src/DAAgent/DAAgentModule.h` / `.cpp`

- **删除**:`DAAgentDockWidget* m_dockWidget` 成员
- **删除**:`setDockWidget()` 方法
- **删除**:`#include "DAAgentDockWidget.h"`
- **删除**:`#include "DAAgentSettingsWidget.h"`(加解密方法将内化,见 1.4)
- **修改** `connectSignals()`:只保留 `m_bridge` → `this`(DAAgentInterface 信号)的转发 connect,删除所有 → `m_dockWidget` 的 connect
- **新增**:在 `initialize()` 中,把 Bridge 的 9 个信号 connect 到 DAAgentInterface 的同名信号(转发 emit)
  ```cpp
  connect(m_bridge, &DAAgentBridge::agentToken, this, &DAAgentInterface::agentToken);
  // ... 其余 8 个同理
  ```
- **实现** `sendUserAnswer`:转发到 `m_bridge->sendUserAnswer(answer)`

#### 1.3 DAAgentToolBase — 拆分为瘦身版(G2)

**文件**:`src/DAAgent/tools/DAAgentToolBase.h` / `.cpp`

**保留**(只依赖 DAData + DAInterface,DAAgent 本来就依赖):
- `dataMgr()` / `findData()` / `allDatas()`
- `errorResponse()` / `successResponse()`
- `m_core` 成员

**删除**(依赖 DAGui,搬到插件的 `DAAgentChartToolBase`):
- `chartOperateWidget()` / `currentFigure()` / `currentChart()` / `findFigureByName()` / `createFigure()` / `findChart()` / `enableAutoScale()`
- `#include "DAChartOperateWidget.h"` / `DAFigureWidget.h` / `DAChartWidget.h` / `DAUIInterface.h` / `DADockingAreaInterface.h`

#### 1.4 加解密方法内化到 DAAgent

**从** `src/DAGui/Agent/DAAgentSettingsWidget.cpp:310-354` 的 `encryptApiKey`/`decryptApiKey` 静态方法
**搬到** `src/DAAgent/DAAgentModule.cpp`(改为私有静态方法或 DAAgentModule 内部 utility)

`DAAgentModule::getLLMConfig()`/`setLLMConfig()` 内部调用本模块的加解密方法,不再依赖 DAGui 的设置页类。

#### 1.5 DAAgent CMakeLists.txt — 移除 DAGui 依赖

**文件**:`src/DAAgent/CMakeLists.txt`

- **删除** `${DA_PROJECT_NAME}::DAGui` from PUBLIC link(第 51-52 行)
- **删除** `damacro_import_QtAdvancedDocking`(第 63-65 行,原本因 `DAAgentToolBase.h` include `DADockingAreaInterface.h` → `ads_globals.h` 而需要,拆分后不再需要)
- **删除** `damacro_import_qwt`(第 69-70 行,原本因 `DAAgentToolBase.h` include `DAChartWidget.h` → `qwt_samples.h` 而需要,拆分后不再需要)
- 保留 DAInterface/DAData/DAPyBindQt/DAPyScripts 的 PUBLIC link
- **待验证**:DAFigure 是否可移除——DAAgentModule 本身不直接用 DAFigure 类,实施时先保留,编译通过后再尝试移除

#### 1.6 删除 16 个工具文件

**删除整个目录**:`src/DAAgent/tools/`(16 个工具的 .h/.cpp 全部搬到插件,见 Phase 2)

**修改** `DAAgentModule.cpp`:
- 删除 16 个 `#include "tools/DAAgentToolXxx.h"`
- `registerBuiltinTools()` 改为空体(或删除该方法,从 `initialize()` 中移除调用)

---

### Phase 2:新插件 plugins/DAAgentTools/(F1)

#### 2.1 目录结构

```
plugins/DAAgentTools/
├── CMakeLists.txt          # 参照 plugins/DataAnalysis/CMakeLists.txt
├── DAAgentToolsPlugin.h    # 继承 DAAbstractPlugin
├── DAAgentToolsPlugin.cpp
├── DAAgentChartToolBase.h  # 图表工具基类(继承瘦身后的 DAAgentToolBase)
├── DAAgentChartToolBase.cpp
├── tools/                  # 16 个工具(从 src/DAAgent/tools/ 搬来)
│   ├── DAAgentToolListData.h/.cpp         # 继承 DAAgentToolBase(瘦身版)
│   ├── DAAgentToolDataInfo.h/.cpp
│   ├── DAAgentToolQueryData.h/.cpp
│   ├── DAAgentToolColumnStats.h/.cpp
│   ├── DAAgentToolExportData.h/.cpp
│   ├── DAAgentToolCreateChart.h/.cpp      # 继承 DAAgentChartToolBase
│   ├── DAAgentToolAddCurve.h/.cpp
│   ├── DAAgentToolSetChartStyle.h/.cpp
│   ├── DAAgentToolAddAnnotation.h/.cpp
│   ├── DAAgentToolAddRegion.h/.cpp
│   ├── DAAgentToolCreateSubplots.h/.cpp
│   ├── DAAgentToolSaveChartImage.h/.cpp
│   ├── DAAgentToolListFigures.h/.cpp
│   ├── DAAgentToolReadFile.h/.cpp
│   ├── DAAgentToolWriteFile.h/.cpp
│   └── DAAgentToolSaveReport.h/.cpp
└── DAAgentToolsGlobal.h    # 导出宏(若需要)
```

#### 2.2 DAAgentChartToolBase(图表工具基类)

**新文件**:`plugins/DAAgentTools/DAAgentChartToolBase.h` / `.cpp`

从原 `DAAgentToolBase.h` 搬出全部图表方法,继承瘦身后的 `DAAgentToolBase`:
```cpp
#include "DAAgentToolBase.h"  // DAAgent 模块的瘦身基类
#include "DAChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"

class DAAgentChartToolBase : public DAAgentToolBase
{
    Q_OBJECT
public:
    DAAgentChartToolBase(DACoreInterface* core, QObject* parent = nullptr)
        : DAAgentToolBase(core, parent) {}

protected:
    // 从原 DAAgentToolBase 搬来的图表方法(实现不变)
    DAChartOperateWidget* chartOperateWidget() const;
    DAFigureWidget* currentFigure() const;
    DAChartWidget* currentChart() const;
    DAFigureWidget* findFigureByName(const QString& name) const;
    DAFigureWidget* createFigure(const QString& name) const;
    DAChartWidget* findChart(const QString& chartId, const QString& figureName = QString()) const;
    void enableAutoScale(DAChartWidget* chart) const;
};
```

#### 2.3 DAAgentToolsPlugin(插件入口)

**新文件**:`plugins/DAAgentTools/DAAgentToolsPlugin.h` / `.cpp`

```cpp
#include "DAAbstractPlugin.h"

class DAAgentToolsPlugin : public QObject, public DA::DAAbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DAABSTRACTPLUGIN_IID)
    Q_INTERFACES(DA::DAAbstractPlugin)
public:
    bool initialize() override
    {
        auto* agent = core()->getAgentInterface();
        if (!agent) return false;
        // 注册 16 个工具(从原 DAAgentModule::registerBuiltinTools 搬来)
        agent->registerTool(new DAAgentToolListData(core(), this));
        agent->registerTool(new DAAgentToolDataInfo(core(), this));
        // ... 其余 14 个
        return true;
    }
    QString getIID() const override { return DAABSTRACTPLUGIN_IID; }
    QString getName() const override { return "DAAgentTools"; }
    QString getVersion() const override { return "0.0.1"; }
    QString getDescription() const override { return "Platform built-in agent tools"; }
};
```

#### 2.4 插件 CMakeLists.txt

参照 `plugins/DataAnalysis/CMakeLists.txt` 结构,关键差异:
- `damacro_plugin_setting("DAAgentTools" ... )`
- `find_package(DAWorkbench COMPONENTS DAAgent DAGui DAData DAInterface DAPluginSupport DAAxOfficeWrapper ...)`
- link `DAWorkbench::DAAgent`(PUBLIC,继承 DAAgentToolBase)
- link `DAWorkbench::DAGui`(PUBLIC,图表工具用 DAChartOperateWidget)
- link `DAWorkbench::DAAxOfficeWrapper`(Win,save_report 工具用)
- `damacro_import_qwt`(图表工具间接依赖 qwt)
- `damacro_import_QtAdvancedDocking`(图表工具间接依赖 ADS)

---

### Phase 3:Dock 信号链在 APP 层 connect(D3b)

#### 3.1 DAAgentDockWidget — 无需改动

**文件**:`src/DAGui/Agent/DAAgentDockWidget.h` / `.cpp`

Dock 的 8 个槽(`onAgentToken` 等)+ 3 个信号(`sendMessageRequested` / `stopRequested` / `userAnswerSelected`)保持不变。这些是纯 QWidget 级别的接口,不依赖 DAAgent。

#### 3.2 DAAppController — 替换 setDockWidget 为直接 connect

**文件**:`src/APP/DAAppController.cpp`(约第 242-248 行)

**删除**:
```cpp
if (auto* agentMod = qobject_cast< DAAgentModule* >(mCore->getAgentInterface())) {
    agentMod->setDockWidget(mDock->getAgentDockWidget());
}
```

**替换为**(把原 `DAAgentModule::connectSignals()` 中的 11 条 connect 搬到此处):
```cpp
auto* agent = mCore->getAgentInterface();
auto* dock = mDock->getAgentDockWidget();
if (agent && dock) {
    // DAAgentInterface 信号 → Dock 槽
    connect(agent, &DAAgentInterface::agentToken, dock, &DAAgentDockWidget::onAgentToken);
    connect(agent, &DAAgentInterface::agentMessageComplete, dock, &DAAgentDockWidget::onAgentMessageComplete);
    connect(agent, &DAAgentInterface::agentToolCall, dock, &DAAgentDockWidget::onAgentToolCall);
    connect(agent, &DAAgentInterface::agentToolResult, dock, &DAAgentDockWidget::onAgentToolResult);
    connect(agent, &DAAgentInterface::agentQuestion, dock, &DAAgentDockWidget::onAgentQuestion);
    connect(agent, &DAAgentInterface::agentError, dock, &DAAgentDockWidget::onAgentError);
    connect(agent, &DAAgentInterface::agentReady, dock, &DAAgentDockWidget::onAgentReady);
    connect(agent, &DAAgentInterface::agentBusy, dock, &DAAgentDockWidget::onAgentBusy);
    // Dock 信号 → DAAgentInterface
    connect(dock, &DAAgentDockWidget::sendMessageRequested, agent, &DAAgentInterface::sendMessage);
    connect(dock, &DAAgentDockWidget::stopRequested, agent, &DAAgentInterface::stop);
    connect(dock, &DAAgentDockWidget::userAnswerSelected, agent, &DAAgentInterface::sendUserAnswer);
}
```

#### 3.3 DAAppController 需 include DAAgentInterface

**文件**:`src/APP/DAAppController.cpp`

确保 `#include "DAAgentInterface.h"`(APP 已 link DAAgent,头文件可达)。

---

### Phase 4:DAAgentSettingsWidget 迁移到 APP(E1)

#### 4.1 文件移动

**从**:`src/DAGui/Agent/DAAgentSettingsWidget.h` / `.cpp`
**到**:`src/APP/SettingPages/DAAgentSettingsWidget.h` / `.cpp`

#### 4.2 修改设置页 — 通过接口持久化

**文件**:`src/APP/SettingPages/DAAgentSettingsWidget.h` / `.cpp`

- **新增成员**:`DAAgentInterface* m_agentInterface { nullptr };`
- **新增方法**:`void setAgentInterface(DAAgentInterface* p) { m_agentInterface = p; }`
- **删除**:`encryptApiKey` / `decryptApiKey` 静态方法(已搬到 DAAgent)
- **修改** `loadConfig()`:改为 `m_agentInterface->getLLMConfig()` 返回明文 QJsonObject,直接填入 UI 控件
- **修改** `saveConfig()` / `apply()`:从 UI 控件收集明文 QJsonObject,调 `m_agentInterface->setLLMConfig(config)`

#### 4.3 DAAppSettingDialog — 注入接口

**文件**:`src/APP/DAAppSettingDialog.cpp`(第 44-47 行)

**修改**:
```cpp
// Agent LLM 设置页(改为通过接口持久化,与其他设置页一致)
DAAgentSettingsWidget* agentPage = new DAAgentSettingsWidget();
agentPage->setAgentInterface(config->getCore()->getAgentInterface());
settingWidget()->addPage(agentPage);
```

**注意**:`DAAppConfig` 持有 `mCore`(DAAppCore*),通过 `getCore()` 可获取。需确认 `DAAppConfig::getCore()` 是否 public(从 `DAAppConfig.h:34` 看有 `setCore`,应有对应 getter,若无则需添加)。

#### 4.4 DAGui CMakeLists.txt — 清理

**文件**:`src/DAGui/CMakeLists.txt`

- 删除 `DAAgentSettingsWidget.h/.cpp` 的 GLOB 匹配(文件搬走后 GLOB 自动不匹配,无需显式删除)
- **删除** `Crypt32` 的 PRIVATE link(第 529-531 行,原本只为 DPAPI 加密,设置页搬走后 DAGui 不再需要)
- WebEngine/WebChannel/Network 保留(DAAgentDockWidget 仍需要)

#### 4.5 APP CMakeLists.txt — 已 link DAAgent,无需改动

`src/APP/CMakeLists.txt:98` 已有 `${DA_PROJECT_NAME}::DAAgent`,设置页的头文件可达。GLOB 会自动收集新位置的 `DAAgentSettingsWidget.cpp`。

---

### Phase 5:文档更新

#### 5.1 src/DAAgent/AGENTS.md

重写以下章节:
- **一、模块定位**:删除 DAGui PUBLIC 依赖;依赖列表改为 DAInterface/DAData/DAPyBindQt/DAPyScripts(待验证 DAFigure 是否可移除)
- **二、架构总览**:删除"DAAgentModule 持有 DockWidget"描述;改为"DAAgentInterface 暴露 9 个信号,由 APP 层 connect 到 Dock"
- **三、目录结构**:删除 `tools/` 章节(已搬到插件);删除 `DAAgentSettingsWidget` 描述(已搬到 APP)
- **七、工具系统**:改为"工具由插件 `plugins/DAAgentTools/` 提供,通过 `DAAgentInterface::registerTool` 注册"
- **八、DAGui 模块涉及的界面**:删除 8.3 DAAgentSettingsWidget 章节
- **九、配置持久化**:改为"通过 `DAAgentInterface::getLLMConfig/setLLMConfig` 接口持久化,APP 层设置页调用接口;加解密由 DAAgentModule 内部处理"
- **十、APP 层集成**:更新 setDockWidget → connect 信号链的描述
- **十一、铁律 T3**:更新——"Dock 不再由 DAAgentModule 持有,信号链在 DAAppController 中 connect"

#### 5.2 根 AGENTS.md

若 CODE MAP 或 WHERE TO LOOK 中提到 DAAgent 的依赖关系,同步更新。

---

## 六、实施顺序(降低风险)

采用双轨过渡策略,每个 Phase 是独立 commit,可逐 Phase 回退:

1. **Phase 1.1-1.2**:DAAgentInterface 加信号 + DAAgentModule 转发(先不改 Dock 接线,确保编译通过)
2. **Phase 3.2**:DAAppController 切换到直接 connect(此时 DAAgentModule 的 setDockWidget 还在,双轨过渡)
3. **Phase 1.2 续**:删除 DAAgentModule 的 m_dockWidget/setDockWidget/connectSignals dock 部分
4. **Phase 2**:创建插件,搬工具(此时 DAAgentModule 的 registerBuiltinTools 还在,双轨过渡)
5. **Phase 2 续**:插件完成注册后,清空 DAAgentModule::registerBuiltinTools
6. **Phase 1.3-1.6**:DAAgentToolBase 拆分 + CMake 清理 + 删除 tools/ 目录
7. **Phase 4**:设置页迁移
8. **Phase 5**:文档更新

---

## 七、验证方法

### 7.1 编译验证

```powershell
.\scripts\build.ps1 -Full
```

必须全量编译通过,无 unresolved external symbol 错误。

### 7.2 运行时验证(启动 DAWorkbench.exe)

1. **插件加载**:检查 `da_log.log` 有 `DAAgentTools` 插件加载成功日志,无 "plugin load failed"
2. **Agent Dock 显示**:Ribbon "Agent 助手" 按钮可切换 Dock 显隐
3. **信号链(D3b)**:发送一条消息,验证:
   - token 流式渲染正常(agentToken 信号链通)
   - 消息完成渲染正常(agentMessageComplete)
   - 工具调用显示在对话流(agentToolCall + agentToolResult)
4. **工具注册(F1)**:让 agent 调用 `list_data` / `create_chart`,验证 16 个工具都可用
5. **设置页(E1)**:打开设置对话框 → Agent LLM 设置页 → 修改 base_url → 应用 → 重启程序 → 验证配置持久化
6. **API Key 加密**:设置 api_key → 重启 → 验证 `agent-config.ini` 中 `agent/llm_api_key` 为加密 blob,非明文
7. **DAGui 无 Crypt32**:确认 DAGui.dll 不再依赖 Crypt32.dll(用 Dependency Walker 或 `dumpbin /dependents`)

### 7.3 日志检查

- `%APPDATA%\DAWorkBench\log\da_log.log` 无 `[error]` / `[critical]` 条目
- 无 "Agent 子进程启动后 X 毫秒内未就绪" 错误
- 无 "tool not found" 错误(工具注册失败的表现)

---

## 八、风险与回退

| 风险 | 缓解 |
|------|------|
| 信号链漏 connect | 对照原 `connectSignals()` 的 11 条 connect 逐一迁移;运行时验证 token 流式 |
| 插件加载时机晚于首次 sendMessage | 验证 `DAPluginManager::loadAllPlugins` 在 `DAAppCore::initialized` 之后、UI 交互之前完成;懒启动在用户首次发消息才触发,插件加载必然先于 |
| DAFigure 依赖误删 | Phase 1.5 中先保留 DAFigure link,编译通过后再尝试移除验证 |
| 跨 DLL 工具派生 RTTI | 确认 `DAAGENT_BUILD` 定义在 DAAgent 库编译时,`DAAgent_API` 在 `DAAbstractAgentTool` 上已正确导出(已确认) |
| DAAppConfig 无 getCore() getter | 若 `DAAppConfig::getCore()` 不存在,需在 Phase 4.3 前添加 public getter |

**回退**:每个 Phase 是独立 commit,可逐 Phase 回退。Phase 2 和 Phase 4 涉及文件移动,用 `git mv` 保留历史。

---

## 九、决策记录(grilling 讨论摘要)

| 轮次 | 问题 | 选项 | 最终选择 | 理由 |
|------|------|------|---------|------|
| 1 | 8 个绘图工具如何处理? | A(搬到 DAGui)/ B(抽象接口)/ C(插件化) | **C** | 便于扩展,插件是扩展的标准载体 |
| 2 | DAAgentDockWidget 归属? | D1(接口加信号)+ D3a(DAGui 持有接口)/ D1+D3b(APP connect) | **D1+D3b** | DAGui 无法访问 DACoreInterface,D3a 走不通;D3b 让 DAGui 与 DAAgent 互不依赖 |
| 3 | DAAgentSettingsWidget 如何处理? | E1(搬 APP)/ E2(留 DAGui 信号槽)/ E3(保持现状) | **E1** | 与整个程序先统一;信号少一点 |
| 4 | 16 个工具归属 + 工具基类处理? | F1+G2(单插件+拆基类)/ F1+G1(单插件+整体搬)/ F3+G2(APP 注册+拆基类) | **F1+G2** | 插件是扩展载体;瘦身基类保留价值,未来数据工具插件可复用 |

---

## 十、涉及文件清单

### 新增文件
- `plugins/DAAgentTools/CMakeLists.txt`
- `plugins/DAAgentTools/DAAgentToolsPlugin.h` / `.cpp`
- `plugins/DAAgentTools/DAAgentChartToolBase.h` / `.cpp`
- `plugins/DAAgentTools/DAAgentToolsGlobal.h`(若需要)
- `plugins/DAAgentTools/tools/DAAgentTool*.h` / `.cpp`(16 个工具,从 src/DAAgent/tools/ 搬来)
- `src/APP/SettingPages/DAAgentSettingsWidget.h` / `.cpp`(从 src/DAGui/Agent/ 搬来)

### 删除文件
- `src/DAAgent/tools/`(整个目录,16 个工具搬到插件)

### 修改文件
- `src/DAAgent/DAAgentInterface.h`(加 9 个信号 + sendUserAnswer 虚函数)
- `src/DAAgent/DAAgentModule.h` / `.cpp`(删除 Dock 持有,转发信号,内化加解密,清空 registerBuiltinTools)
- `src/DAAgent/tools/DAAgentToolBase.h` / `.cpp`(瘦身,删除图表方法)
- `src/DAAgent/CMakeLists.txt`(删除 DAGui/qwt/ADS 依赖)
- `src/DAGui/CMakeLists.txt`(删除 Crypt32 link)
- `src/APP/DAAppController.cpp`(替换 setDockWidget 为直接 connect)
- `src/APP/DAAppSettingDialog.cpp`(注入 DAAgentInterface 给设置页)
- `src/DAAgent/AGENTS.md`(文档更新)
- `AGENTS.md`(若涉及 DAAgent 依赖描述,同步更新)

### 无需改动文件
- `src/DAGui/Agent/DAAgentDockWidget.h` / `.cpp`(槽函数和信号不变)
- `src/DAGui/Agent/DAAgentWebChannel.h` / `.cpp`(JS 桥不变)
- `src/DAGui/Agent/resources/`(chat.html / chat.js / chat.css 不变)
- `src/DAAgent/DAAgentBridge.h` / `.cpp`(子进程管理不变)
- `src/DAAgent/DAAbstractAgentTool.h`(纯虚基类不变)
- `src/DAAgent/DAAgentAPI.h`(导出宏不变)
- `src/PyScripts/DAWorkbench/agent/agent_runner.py`(Python 侧不变)
- `src/DAAgent/system_prompt.md`(系统提示词不变)
