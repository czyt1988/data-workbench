# 功能定制与扩展能力

本指南说明如何通过插件扩展 DAWorkBench 的界面和功能，包括添加菜单项、工具栏按钮、Dock 窗口、自定义节点等。

## 主要功能特性

**特性**

- ✅ **界面扩展点**：Ribbon 区域、Docking 区域、Actions 管理、Commands 管理
- ✅ **Ribbon 界面扩展**：添加 Category、Panel、Action 按钮
- ✅ **Dock 窗口扩展**：创建自定义 Dock 窗口、布局管理
- ✅ **菜单扩展**：添加上下文菜单、自定义触发逻辑
- ✅ **快捷键扩展**：注册全局快捷键
- ✅ **自定义节点类型**：注册节点元数据、创建自定义图元
- ✅ **数据类型扩展**：注册自定义数据类型、创建数据对象
- ✅ **事件监听与响应**：监听项目、数据、工作流事件

---

## 扩展点概览

下面的 mermaid 图表展示了插件可用的各类扩展点：

图表展示了插件可以扩展的界面、数据和功能三个维度的扩展点，以及它们之间的关系：

```mermaid
graph TB
    subgraph "界面扩展点"
        RA[Ribbon 区域]      # 工具栏按钮扩展
        DA[Docking 区域]     # 停靠窗口扩展
        AC[Actions 管理]     # 动作对象管理
        CM[Commands 管理]    # 命令管理
    end
    
    subgraph "数据扩展点"
        DM[数据管理器]       # 数据对象管理
        DT[数据类型]         # 自定义数据类型
        DP[数据处理]         # 数据处理扩展
    end
    
    subgraph "工作流扩展点"
        NF[节点工厂]         # 节点创建工厂
        NM[节点元数据]       # 节点信息注册
        NE[节点执行]         # 节点执行逻辑
    end
    
    Plugin --> RA           # 插件可扩展 Ribbon
    Plugin --> DA           # 插件可扩展 Dock
    Plugin --> AC           # 插件可注册 Action
    Plugin --> CM           # 插件可管理命令
    Plugin --> NF           # 插件提供节点工厂
    
    NF --> NM               # 工厂注册节点元数据
    NF --> NE               # 工厂创建执行节点
    
    style Plugin fill:#fff3e0
    style RA fill:#e1f5fe
    style DA fill:#e1f5fe
    style NF fill:#f3e5f5
```

上述扩展点覆盖了界面、数据和功能三个维度，插件可以根据需求选择合适的扩展点进行集成。

---

## Ribbon 界面扩展

### 获取 Ribbon 接口

```cpp
bool MyPlugin::initialize()
{
    DA::DACoreInterface* core = this->core();
    DA::DAUIInterface* ui = core->getUiInterface();
    DA::DARibbonAreaInterface* ribbon = ui->getRibbonArea();

    return true;
}
```

### 添加 Ribbon Category

`DARibbonAreaInterface` 只暴露 `ribbonBar()` / `getCategories()` / `getCategoryByObjectName()` / `getPanelByObjectName()` 四个方法，添加 Category/Panel 要通过 `ribbonBar()` 返回的 `SARibbonBar*` 操作：

```cpp
void MyPlugin::setupRibbon()
{
    DA::DARibbonAreaInterface* ribbon = getRibbonInterface();

    // 通过 ribbonBar() 拿到 SARibbonBar*，再调用 SARibbonBar::addCategory
    SARibbonCategory* category = ribbon->ribbonBar()->addCategory(tr("My Tools"));
    category->setObjectName("myplugin.main.category");

    // 在 Category 上创建 Panel（SARibbonCategory::addPanel）
    SARibbonPanel* panel1 = category->addPanel(tr("Data Processing"));
    panel1->setObjectName("myplugin.panel.processing");

    // 创建 Panel 2
    SARibbonPanel* panel2 = category->addPanel(tr("Visualization"));
    panel2->setObjectName("myplugin.panel.visualization");
}
```

### 添加 Action 按钮

`DAActionsInterface` 暴露 `createAction(const char* objname)` / `recordAction(QAction*)` / `findAction(objname)`，没有 `registerAction` / `addContextMenuAction`。获取已存在的 Category/Panel 要用 `getCategoryByObjectName()` / `getPanelByObjectName()`，不是 `category()` / `panel()`：

```cpp
void MyPlugin::setupActions()
{
    DA::DARibbonAreaInterface* ribbon = getRibbonInterface();
    DA::DAActionsInterface* actions = getActionsInterface();

    // 通过 objectName 获取已创建的 Category / Panel（不是 category()/panel()）
    SARibbonCategory* category = ribbon->getCategoryByObjectName("myplugin.main.category");
    SARibbonPanel* panel = ribbon->getPanelByObjectName("myplugin.panel.processing");

    // 用 createAction(objname) 创建并交由管理器托管的 Action
    QAction* actionProcess = actions->createAction("myplugin.action.process");
    actionProcess->setIcon(QIcon(":/icon/process.png"));
    actionProcess->setText(tr("Process Data"));  // cn:文本
    actionProcess->setToolTip(tr("Process selected data with custom algorithm"));

    // 连接信号
    connect(actionProcess, &QAction::triggered, this, &MyPlugin::onProcessData);

    // 如果用 new QAction 自建 Action，需 recordAction() 记录到管理器（不是 registerAction）
    // QAction* myAct = new QAction(...);
    // actions->recordAction(myAct);

    // 添加到 Ribbon Panel
    if (panel) {
        panel->addLargeAction(actionProcess);
    }
}
```

### 隐藏默认界面元素

`DARibbonAreaInterface` 没有 `hidePanel()`；隐藏 Dock 窗口用 `DADockingAreaInterface::hideDockWidget(QWidget*)`：

```cpp
void MyPlugin::hideDefaultUI()
{
    DA::DADockingAreaInterface* dock = getDockingInterface();

    // 隐藏不需要的 Dock 窗口（hideDockWidget(QWidget*)）
    dock->hideDockWidget(dock->getWorkFlowOperateWidget());
    dock->hideDockWidget(dock->getWorkflowNodeListWidget());

    // Ribbon 没有提供 hidePanel 接口；如需隐藏 Panel，通过 getCategoryByObjectName()
    // 取到 SARibbonCategory* 后操作其 SARibbonPanel 的可见性
}
```

---

## Dock 窗口扩展

### 创建自定义 Dock 窗口

```cpp
// MyDockWidget.h
class MyDockWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MyDockWidget(QWidget* parent = nullptr);
    
    void retranslate();  // 多语言支持
    
private:
    QTableView* m_dataView;
    QPushButton* m_refreshBtn;
};

// MyDockWidget.cpp
MyDockWidget::MyDockWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    m_dataView = new QTableView(this);
    m_refreshBtn = new QPushButton(tr("Refresh"), this);
    
    layout->addWidget(m_dataView);
    layout->addWidget(m_refreshBtn);
    
    connect(m_refreshBtn, &QPushButton::clicked, this, &MyDockWidget::onRefresh);
}
```

### 注册 Dock 窗口

`DADockingAreaInterface` 用 `createDockWidget(QWidget* w, ads::DockWidgetArea area, const QString& widgetName, ads::CDockAreaWidget* dockAreaWidget = nullptr)` 创建停靠窗体，不是 `addDockWidget`。停靠区域用 `ads::DockWidgetArea`（来自 Qt Advanced Docking System），**不是** `Qt::RightDockWidgetArea`。也可用固定的 `DADockingAreaInterface::DockingArea` 枚举（8 个固定区域，如 `DockingAreaWorkFlowOperate`）定位内置 dock：

```cpp
#include "ads_globals.h"  // ads::DockWidgetArea

bool MyPlugin::initialize()
{
    DA::DADockingAreaInterface* dock = getDockingInterface();

    // 创建 Dock 窗口内容
    m_dockWidget = new MyDockWidget();

    // 创建停靠窗体 —— 注意是 createDockWidget，参数顺序：
    //   QWidget*, ads::DockWidgetArea, const QString& widgetName, ads::CDockAreaWidget* = nullptr
    ads::CDockWidget* dockWidget = dock->createDockWidget(
        m_dockWidget,
        ads::DockWidgetArea::RightDockWidgetArea,  // cn:ads 枚举，非 Qt::RightDockWidgetArea
        tr("My Data View"));

    // createDockWidget 不提供 setDockWidgetFeatures；停靠特性由 ads::CDockWidget 自身管理

    return true;
}
```

> 也可用 `dock->createDockWidgetAsTab(w, name, dockAreaWidget)` 把窗体作为标签页添加，或 `dock->createFloatingDockWidget(w, name, pos)` 创建浮动窗体。

### Dock 窗口布局管理

`DADockingAreaInterface` 没有 `stackDockWidgets` / `tabifyDockWidgets`。把多个窗体堆叠成标签页用 `createDockWidgetAsTab(QWidget* w, const QString& name, ads::CDockAreaWidget* dockAreaWidget)`：

```cpp
void MyPlugin::setupDockLayout()
{
    DA::DADockingAreaInterface* dock = getDockingInterface();

    // 先创建第一个 dock，得到它的 dockAreaWidget
    ads::CDockWidget* firstDock = dock->createDockWidget(
        m_dataDock, ads::DockWidgetArea::LeftDockWidgetArea, tr("Data"));
    ads::CDockAreaWidget* area = firstDock->dockAreaWidget();

    // 把第二个窗体作为标签页加入同一个 dockArea（堆叠/Tab 化）
    dock->createDockWidgetAsTab(m_resultDock, tr("Result"), area);
}
```

---

## 菜单扩展

### 添加上下文菜单

`DAActionsInterface` 没有 `addContextMenuAction`；它只暴露 `createAction(objname)` / `recordAction(QAction*)` / `findAction(objname)`。上下文菜单需由具体控件的右键事件触发，Action 先用 `createAction` 创建并 `recordAction` 记录，再在目标控件的 `contextMenuEvent` 中通过 `findAction` 取出放入菜单：

```cpp
void MyPlugin::setupContextMenu()
{
    DA::DAActionsInterface* actions = getActionsInterface();

    // 用 createAction(objname) 创建并由管理器托管
    QAction* actionExport = actions->createAction("myplugin.action.export");
    actionExport->setText(tr("Export to MyFormat"));  // cn:导出
    connect(actionExport, &QAction::triggered, this, &MyPlugin::onExportData);

    // 没有针对 data.table / chart.figure 的内置上下文菜单注册接口
    // 如需在数据表/图表右键菜单注入，需在对应控件的 contextMenuEvent 中
    // 调用 actions->findAction("myplugin.action.export") 取出并加入 QMenu
}
```

### 自定义菜单触发逻辑

`DADataManagerInterface` 的 `getSelectDatas()` 返回 `QList<DAData>`，`getOperateData()` 返回 `DAData`；没有返回 `DADataObject*` 的 `getSelectedData()`：

```cpp
void MyPlugin::onExportData()
{
    // 获取当前选中的数据
    DA::DACoreInterface* core = this->core();
    DA::DADataManagerInterface* dataMgr = core->getDataManagerInterface();

    // getSelectDatas() 返回 QList<DAData>，不是 DADataObject*
    QList<DA::DAData> selected = dataMgr->getSelectDatas();
    if (selected.isEmpty()) {
        QMessageBox::warning(nullptr, tr("Warning"), tr("No data selected"));  // cn:未选中数据
        return;
    }

    // 取第一个选中的 DAData 导出
    DA::DAData data = selected.first();
    exportToMyFormat(data);
}
```

---

## 快捷键扩展

### 注册快捷键

`DAActionsInterface` 没有 `registerAction`；用 `createAction(objname)` 创建托管 Action 并设快捷键：

```cpp
bool MyPlugin::initialize()
{
    DA::DAActionsInterface* actions = getActionsInterface();

    // 用 createAction(objname) 创建并由管理器托管
    QAction* actionQuickProcess = actions->createAction("myplugin.action.quick_process");
    actionQuickProcess->setText(tr("Quick Process"));  // cn:快速处理
    actionQuickProcess->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
    connect(actionQuickProcess, &QAction::triggered, this, &MyPlugin::onQuickProcess);

    // createAction 已记录该 Action，无需再调用 recordAction
    return true;
}
```

---

## 自定义节点类型（旧 C++ 架构，仅参考）

!!! warning "遗留架构 —— 当前推荐 Python-first `@NodeDef`"
    下面 `registerNodePrototypes()` 用 `DANodeMetaData` 注册节点、`MyWorker` 继承 `DAAbstractNode` 重写 `createGraphicsItem()` 的写法属于**已废弃的 C++ 节点架构**。当前工作流节点统一用 Python `@NodeDef` 装饰器声明，节点外观通过 `NodeDisplay` 或 Python `paint()` 自定义，参考 `plugins/DASystemNodes/AGENTS.md`。本节仅作历史背景保留。

### 注册节点元数据（旧 C++）

```cpp
void MyNodeFactory::registerNodePrototypes()
{
    // 创建节点元数据
    DA::DANodeMetaData meta;
    
    meta.setPrototype("My.Factory.CustomProcess");
    meta.setName(tr("Custom Processor"));
    meta.setGroup(tr("My Tools"));
    meta.setIcon(QIcon(":/icon/node-process.png"));
    meta.setDescription(tr("Process data with custom algorithm"));
    
    // 定义输入输出连接点
    meta.addInputKey("data_in", tr("Input Data"));
    meta.addInputKey("config", tr("Configuration"));
    meta.addOutputKey("data_out", tr("Output Data"));
    meta.addOutputKey("report", tr("Report"));
    
    // 设置节点属性
    meta.setAttribute("category", "processing");
    meta.setAttribute("complexity", "medium");
    
    m_nodePrototypes[meta.prototype()] = meta;
}
```

### 创建自定义节点图元

```cpp
DA::DAAbstractNodeGraphicsItem* MyWorker::createGraphicsItem()
{
    // 使用自定义图元类
    MyCustomNodeItem* item = new MyCustomNodeItem(this);
    
    // 设置外观
    item->setBodySize(150, 80);
    item->setHeaderColor(QColor(100, 150, 200));
    item->setBodyColor(QColor(240, 240, 240));
    
    return item;
}

// 自定义图元类
class MyCustomNodeItem : public DA::DAStandardNodeGraphicsItem
{
public:
    MyCustomNodeItem(DA::DAAbstractNode* node);
    
    void paint(QPainter* painter, 
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;
               
protected:
    void changeLinkPointPos(QList<DA::DANodeLinkPoint>& lps, 
                            const QRectF& bodyRect) const override;
};
```

---

## 数据类型扩展

### 注册自定义数据类型

`DADataManagerInterface` 没有 `registerDataType`。数据通过 `addData(DAData&)` 加入管理器，数据类型由 `DAData` 包装的对象本身决定（DataFrame / Series / DataPackage，见 `src/DAData/DAData.h` 的 `getDataType()`）。插件如需自定义数据，可构造 `DAData` 并 `addData` 注册：

```cpp
bool MyPlugin::initialize()
{
    DA::DADataManagerInterface* dataMgr = core->getDataManagerInterface();

    // 没有 registerDataType；构造 DAData 并 addData 注册
    DA::DAData myData(myDataFrame);  // cn:从 DAPyDataFrame 构造 DAData
    myData.setName("my_custom_data");
    myData.setDescribe(tr("My custom data"));  // cn:我的自定义数据
    dataMgr->addData(myData);

    return true;
}
```

### 创建自定义数据对象

数据包装类是 `DAData`（`src/DAData/DAData.h`），**不是** `DADataObject`。`DAData` 是值类型封装，底层持 `DAAbstractData::Pointer` 智能指针，支持 DataFrame / Series / DataPackage 三种类型：

```cpp
// DAData 是值类型，不是 DADataObject 继承关系
DA::DAData data;
data.setName("my_data");
data.setDescribe(tr("My data"));  // cn:我的数据
// data.getDataType() 返回类型；data.isDataFrame() / isSeries() / isDataPackage() 判断类型
```

---

## 事件监听与响应

### 监听项目事件

`DAProjectInterface` 的真实信号是 `projectBeginLoad` / `projectLoaded` / `projectBeginSave` / `projectSaved` / `dirtyStateChanged` / `projectIsCleaned`，**没有** `projectOpened` / `projectClosed`：

```cpp
bool MyPlugin::initialize()
{
    DA::DAProjectInterface* project = core->getProjectInterface();

    // 监听项目加载完成（不是 projectOpened）
    connect(project, &DA::DAProjectInterface::projectLoaded,
            this, &MyPlugin::onProjectLoaded);

    // 监听项目保存完成
    connect(project, &DA::DAProjectInterface::projectSaved,
            this, &MyPlugin::onProjectSaved);

    // 监听脏状态变化 / 工程被清空（替代 projectClosed 的语义）
    connect(project, &DA::DAProjectInterface::dirtyStateChanged,
            this, &MyPlugin::onDirtyChanged);
    connect(project, &DA::DAProjectInterface::projectIsCleaned,
            this, &MyPlugin::onProjectCleaned);

    return true;
}

void MyPlugin::onProjectLoaded(const QString& path)
{
    // 加载项目相关配置
    loadProjectConfig(path);

    // 初始化项目数据
    initializeProjectData();
}
```

### 监听数据事件

```cpp
bool MyPlugin::initialize()
{
    DA::DADataManagerInterface* dataMgr = core->getDataManagerInterface();
    
    // 监听数据添加
    connect(dataMgr, &DA::DADataManagerInterface::dataAdded,
            this, &MyPlugin::onDataAdded);
    
    // 监听数据删除
    connect(dataMgr, &DA::DADataManagerInterface::dataRemoved,
            this, &MyPlugin::onDataRemoved);
    
    // 监听数据变更
    connect(dataMgr, &DA::DADataManagerInterface::dataChanged,
            this, &MyPlugin::onDataChanged);
    
    return true;
}
```

### 监听工作流事件

!!! note "工作流事件"
    新的 Python-first 工作流架构中，工作流事件由 `DAPyWorkFlowManager` 通过信号驱动。
    插件可通过 `DAPyWorkFlowManager` 的 `executionStarted()`、`executionFinished(bool)`、`nodeExecuted(QString, bool)` 等信号监听工作流执行状态。
    具体用法参见 [工作流生命周期](../dev-guide/workflow/workflow-lifecycle.md)。

---

## 配置项扩展

### 添加插件配置面板

设置页通过重载 `DAAbstractPlugin::createSettingPage()` 返回一个 `DAAbstractSettingPage*` 注册，**不是** `ui->addSettingsPage(...)`。`DAAbstractSettingPage`（`src/DAGui/DAAbstractSettingPage.h`）是抽象基类，需实现 `apply()` / `getSettingPageTitle()` / `getSettingPageIcon()`：

```cpp
// MySettingPage.h —— 继承 DAAbstractSettingPage
class MySettingPage : public DA::DAAbstractSettingPage
{
    Q_OBJECT
public:
    explicit MySettingPage(QWidget* parent = nullptr);
    void apply() override;                       // 用户点确定/应用时调用
    QString getSettingPageTitle() const override;
    QIcon getSettingPageIcon() const override;
private:
    QCheckBox* m_autoSaveCheck;
    QSpinBox* m_cacheSizeSpin;
    QComboBox* m_algorithmCombo;
};

// MyPlugin.h —— 重载 createSettingPage() 注册
class MyPlugin : public DA::DAAbstractNodePlugin
{
public:
    DA::DAAbstractSettingPage* createSettingPage() override;
};

// MyPlugin.cpp
DA::DAAbstractSettingPage* MyPlugin::createSettingPage()
{
    // 返回设置页实例；主程序在打开设置窗口时调用此函数并把页面加入设置容器
    // 默认返回 nullptr 代表没有设置页
    return new MySettingPage();
}
```

> `DAAbstractSettingPage` 提供 `settingChanged()` 信号，页面参数改变后必须发射此信号，否则设置窗口不会标记为 dirty，`apply()` 不会被调用。

---

## 完整扩展示例

### 创建一个完整的插件界面

```cpp
bool MyPlugin::initialize()
{
    // 1. 获取接口
    DA::DACoreInterface* core = this->core();
    DA::DAUIInterface* ui = core->getUiInterface();
    DA::DARibbonAreaInterface* ribbon = ui->getRibbonArea();
    DA::DADockingAreaInterface* dock = ui->getDockingArea();
    DA::DAActionsInterface* actions = ui->getActionInterface();

    // 2. 通过 ribbonBar()->addCategory 创建 Ribbon Category（不是 ribbon->addCategory）
    SARibbonCategory* category = ribbon->ribbonBar()->addCategory(tr("My Tools"));
    SARibbonPanel* panel = category->addPanel(tr("Main"));

    // 3. 用 actions->createAction(objname) 创建托管 Action（不是 registerAction）
    QAction* actionProcess = actions->createAction("myplugin.action.process");
    actionProcess->setIcon(QIcon(":/icon/process.png"));
    actionProcess->setText(tr("Process"));  // cn:处理
    QAction* actionConfig = actions->createAction("myplugin.action.config");
    actionConfig->setIcon(QIcon(":/icon/config.png"));
    actionConfig->setText(tr("Config"));  // cn:配置

    panel->addLargeAction(actionProcess);
    panel->addSmallAction(actionConfig);

    // 4. 用 dock->createDockWidget(QWidget*, ads::DockWidgetArea, name) 创建 Dock
    m_dockWidget = new MyDockWidget();
    dock->createDockWidget(m_dockWidget, ads::DockWidgetArea::RightDockWidgetArea,
                           tr("My View"));  // cn:非 Qt::RightDockWidgetArea

    // 5. 监听事件
    connectToEvents(core);

    // 6. Python-first 节点：C++ 入口只注册 Python 路径，无需 new MyNodeFactory
    // 参考 plugins/DASystemNodes/ 的 DASystemNodesPlugin 实现
    return true;
}
```

---

## 下一步

- [:material-book: 最佳实践](../reference/best-practices.md) - 开发最佳实践
- [:material-help-circle: 常见问题](../reference/faq.md) - 常见问题解答
- [:material-file-document: 贡献指南](../reference/contribution-guide.md) - 参与贡献