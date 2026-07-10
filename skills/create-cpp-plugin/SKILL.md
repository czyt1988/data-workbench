---
name: create-cpp-plugin
description: Use when creating a new C++ plugin DLL for the data-workbench platform, or modifying an existing plugin's structure, CMake, UI integration, or data access. Triggers: create plugin, new plugin, DAAbstractNodePlugin, plugin CMake, ribbon category, dock widget, DAWorkbench plugin, plugin skeleton.
---

# 创建 data-workbench C++ 插件

本技能指导如何基于 DAWorkbench 平台框架创建 C++ 插件 DLL，涵盖插件骨架、CMake 配置、UI 集成（Ribbon/Dock）、数据访问、工程存档等完整流程。

## 决策树

```
你要做什么？
├── 从零创建一个新插件 → 场景 A：完整插件骨架
├── 为已有插件添加 Ribbon 界面 → 场景 B：UI 集成
├── 插件需要访问数据管理器 → 场景 C：数据访问
├── 插件需要工程存档（保存/加载） → 场景 D：工程存档
└── 插件需要调用 Python 脚本 → 参见 skill: python-script-integration
```

## 架构概览

```
┌────────────────── DAWorkbench 主程序 ──────────────────┐
│                                                         │
│  DAAppPluginManager                                     │
│    └ QPluginLoader::instance() 加载 bin/plugins/*.dll   │
│        ↓                                                │
│  DAAbstractNodePlugin (接口)                             │
│    ├ initialize()       ← 插件入口                       │
│    ├ createNodeFactory() ← 提供工作流节点（可返回 nullptr）│
│    ├ createSettingPage() ← 提供设置页（可返回 nullptr）   │
│    └ createArchiveTask() ← 提供存档任务                   │
│        ↓                                                │
│  DACoreInterface (核心接口)                              │
│    ├ getUiInterface()       → DAUIInterface              │
│    │    ├ getDockingArea()  → DADockingAreaInterface     │
│    │    ├ getRibbonArea()   → DARibbonAreaInterface      │
│    │    ├ getStatusBar()    → DAStatusBarInterface       │
│    │    └ getMainWindow()   → QMainWindow                │
│    ├ getDataManagerInterface() → DADataManagerInterface  │
│    ├ getProjectInterface()  → DAProjectInterface         │
│    └ getPythonSignalHandler() → DAPythonSignalHandler    │
└─────────────────────────────────────────────────────────┘
```

## 场景 A：完整插件骨架

### A1. 目录结构

```
MyPlugin/
├── CMakeLists.txt           ← 顶层 CMake（定位 DAWorkbench 安装目录）
├── src/
│   ├── CMakeLists.txt       ← 插件 CMake（构建配置）
│   ├── MyPluginGlobal.h     ← 导出宏定义
│   ├── MyPlugin.h           ← 插件类声明
│   ├── MyPlugin.cpp         ← 插件类实现
│   ├── MyUI.h/.cpp          ← UI 管理类
│   ├── MyWorker.h/.cpp      ← 业务逻辑类
│   ├── my_resource.qrc      ← Qt 资源文件
│   └── PyScripts/           ← Python 脚本目录
│       └── MyPackage/
│           └── __init__.py
└── data-workbench/          ← 平台子模块（git submodule）
    └── bin_Release_qtX.Y.Z_MSVC_x64/  ← 构建安装后生成
```

### A2. 顶层 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyPlugin DESCRIPTION "My Plugin For DAWorkbench")

# --- 定位 DAWorkbench 安装目录 ---
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

# 自动计算安装目录名: bin_<BuildType>_qt<QtVer>_<Compiler>_<Arch>
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(_platform_name "x86")
else()
    set(_platform_name "x64")
endif()
set(DAWorkbench_INSTALL_FOLDER_NAME bin_${CMAKE_BUILD_TYPE}_qt${QT_VERSION}_${CMAKE_CXX_COMPILER_ID}_${_platform_name})
set(DAWorkbench_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/data-workbench/${DAWorkbench_INSTALL_FOLDER_NAME}")
set(DAWorkbench_DIR "${DAWorkbench_INSTALL_DIR}/lib/cmake/DAWorkbench")

# 加载平台提供的 CMake 宏
list(APPEND CMAKE_MODULE_PATH ${DAWorkbench_INSTALL_DIR})
list(APPEND CMAKE_MODULE_PATH ${DAWorkbench_DIR})
include(${DAWorkbench_DIR}/daworkbench_plugin_utils.cmake)

set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_DIR})

add_subdirectory(src)
```

### A3. src/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)

# --- 插件元信息宏（名称、描述、版本号、安装目录）---
damacro_plugin_setting(
    "MyPlugin"
    "My Plugin Description"
    0   # ver major
    1   # ver minor
    0   # ver patch
    ${DAWorkbench_INSTALL_DIR}
)

# --- Qt 依赖 ---
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core Gui Widgets Xml Svg PrintSupport Concurrent OpenGL
    REQUIRED
)
if(${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS OpenGLWidgets Core5Compat REQUIRED)
endif()
if(WIN32)
    find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS AxContainer REQUIRED)
endif()

# --- 文件加载 ---
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_PLUGIN_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_PLUGIN_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

add_library(${DA_PLUGIN_NAME} SHARED
    ${DA_PLUGIN_HEADER_FILES}
    ${DA_PLUGIN_SOURCE_FILES}
    ${DA_PLUGIN_QT_UI_FILES}
    ${DA_PLUGIN_QT_RC_FILES}
)

target_compile_definitions(${DA_PLUGIN_NAME} PRIVATE MyPlugin_BUILDLIB)

# --- 第三方库导入（平台提供的宏） ---
damacro_import_SARibbonBar(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_DALiteCtk(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtAdvancedDocking(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_qwt(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_orderedmap(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_Python(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_pybind11(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})

# --- DAWorkbench 组件依赖 ---
# 注意：DAWorkFlow 已废弃（用 DAPyWorkFlow），DACommonWidgets 已合并到 DAGui
find_package(DAWorkbench COMPONENTS
    DAUtils
    DAMessageHandler
    DAAxOfficeWrapper        # 仅 Windows 需要
    DAPyBindQt
    DAPyScripts
    DAData
    DAGraphicsView
    DAFigure
    DAPyCommonWidgets
    DAGui
    DAInterface
    DAPluginSupport
)

# DAWidgets 是独立第三方库（不是 DAWorkbench::DAWidgets），需显式 find_package
find_package(DAWidgets PATHS ${DAWorkbench_INSTALL_DIR})

target_link_libraries(${DA_PLUGIN_NAME} PUBLIC
    DAWorkbench::DAUtils
    DAWorkbench::DAMessageHandler
    DAWorkbench::DAAxOfficeWrapper
    DAWorkbench::DAPyBindQt
    DAWorkbench::DAPyScripts
    DAWorkbench::DAData
    DAWorkbench::DAGraphicsView
    DAWorkbench::DAFigure
    DAWorkbench::DAPyCommonWidgets
    DAWorkbench::DAGui
    DAWorkbench::DAInterface
    DAWorkbench::DAPluginSupport
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

# --- 构建属性 ---
set_target_properties(${DA_PLUGIN_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${DA_PLUGIN_VERSION}
    EXPORT_NAME ${DA_PLUGIN_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    # DLL 直接输出到平台的 plugins 目录
    RUNTIME_OUTPUT_DIRECTORY "${DAWorkbench_INSTALL_DIR}/bin/plugins"
)

# --- 安装 ---
damacro_plugin_install()

# --- Python 脚本复制（配置阶段自动执行）---
file(COPY "${CMAKE_CURRENT_LIST_DIR}/PyScripts/MyPackage"
     DESTINATION ${DAWorkbench_INSTALL_DIR}/bin/PyScripts)
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/PyScripts/MyPackage"
        DESTINATION ${CMAKE_INSTALL_BINDIR}/PyScripts)
```

### A4. 导出宏头文件

```cpp
// MyPluginGlobal.h
#ifndef MYPLUGIN_GLOBAL_H
#define MYPLUGIN_GLOBAL_H
#include <QtCore/QtGlobal>
#if defined(MYPLUGIN_BUILDLIB)
#define MYPLUGIN_API Q_DECL_EXPORT
#else
#define MYPLUGIN_API Q_DECL_IMPORT
#endif
#endif
```

### A5. 插件类

```cpp
// MyPlugin.h
#include <QObject>
#include "DAAbstractNodePlugin.h"
#include "DAAbstractSettingPage.h"
#include "DAAbstractArchiveTask.h"

class MyUI;
class MyWorker;

class MyPlugin : public QObject, public DA::DAAbstractNodePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)
    Q_INTERFACES(DA::DAAbstractNodePlugin)
public:
    MyPlugin();
    ~MyPlugin() override;

    // === 必须实现的纯虚函数 ===
    bool initialize() override;
    QString getIID() const override;
    QString getName() const override;
    QString getVersion() const override;
    QString getDescription() const override;

    // === 节点工厂（如不提供自定义节点，返回 nullptr）===
    DA::DAPyNodeFactory* createNodeFactory() override;
    void destroyNodeFactory(DA::DAPyNodeFactory* p) override;

    // === 可选重写 ===
    DA::DAAbstractSettingPage* createSettingPage() override;
    std::shared_ptr<DA::DAAbstractArchiveTask> createArchiveTask(bool isSave) override;

private:
    MyUI* mUI { nullptr };
    MyWorker* mWorker { nullptr };
};
```

```cpp
// MyPlugin.cpp
#include "MyPlugin.h"
#include "MyUI.h"
#include "MyWorker.h"
#include "DAZipArchiveTask_Xml.h"
#include "DALog.h"

MyPlugin::MyPlugin() : DA::DAAbstractNodePlugin() {}
MyPlugin::~MyPlugin() {}

bool MyPlugin::initialize()
{
    // 1. 初始化 UI
    mUI = new MyUI(this);
    mUI->initialize(core());  // core() 来自 DAAbstractPlugin 基类

    // 2. 初始化业务逻辑
    mWorker = new MyWorker(this);
    mWorker->initialize(core(), mUI);

    // 3. 连接 UI 和 Worker
    mUI->bindWorker(mWorker);

    // 4. 初始化 Python 环境（如有）
    if (!mWorker->initializePythonEnv()) {
        daCritical << QString(u8"初始化 Python 脚本失败");
    }

    // 5. 调用基类 initialize
    return DA::DAAbstractNodePlugin::initialize();
}

QString MyPlugin::getIID() const { return "Plugin.MyPlugin"; }
QString MyPlugin::getName() const { return u8"My Plugin"; }
QString MyPlugin::getVersion() const { return "1.0.0"; }
QString MyPlugin::getDescription() const { return u8"插件描述"; }

DA::DAPyNodeFactory* MyPlugin::createNodeFactory()
{
    // 不提供工作流节点时返回 nullptr
    return nullptr;
}

void MyPlugin::destroyNodeFactory(DA::DAPyNodeFactory* p)
{
    delete p;  // DAPyNodeFactory 继承自 DAPyObjectWrapper（非 QObject），直接 delete
}

DA::DAAbstractSettingPage* MyPlugin::createSettingPage()
{
    return nullptr;  // 无设置页
}

// === 工程存档 ===
static const char* cs_xmlFilePath = "myplugin/data.xml";

std::shared_ptr<DA::DAAbstractArchiveTask> MyPlugin::createArchiveTask(bool isSave)
{
    if (isSave) {
        QDomDocument doc;
        doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\""));
        QDomElement root = doc.createElement("root");
        doc.appendChild(root);
        mWorker->saveToXml(root, doc);
        mUI->saveToXml(root, doc);
        return std::make_shared<DA::DAZipArchiveTask_Xml>(cs_xmlFilePath, doc);
    } else {
        auto task = std::make_shared<DA::DAZipArchiveTask_Xml>(cs_xmlFilePath);
        task->setLoadedCallBack([this](std::shared_ptr<DA::DAAbstractArchiveTask> t) {
            auto xmltask = std::static_pointer_cast<DA::DAZipArchiveTask_Xml>(t);
            QDomDocument doc = xmltask->getDomDocument();
            QDomElement root = doc.firstChildElement("root");
            if (!root.isNull()) {
                mWorker->loadFromXml(root);
                mUI->loadFromXml(root);
            }
        });
        return task;
    }
}
```

### A6. 推荐的 UI/Worker 分层模式

DAWorkbench 插件推荐将代码分为 **UI 层**和**业务层**：

```
MyPlugin (入口)
  ├── MyUI (UI 层)
  │     ├── 管理 Ribbon 面板、Action、Dock 窗口
  │     ├── 转发用户操作到 Worker
  │     └── 从 Worker 接收数据更新界面
  └── MyWorker (业务层)
        ├── 数据处理逻辑
        ├── Python 脚本调用
        └── 绘图模板管理
```

**UI 层**不直接处理数据，**Worker 层**不直接操作界面。两者通过信号槽通信。

```cpp
// MyUI.h
#include <QObject>
#include "DAData.h"

namespace DA { class DACoreInterface; class DAUIInterface; }
class MyWorker;

class MyUI : public QObject
{
    Q_OBJECT
public:
    explicit MyUI(QObject* par = nullptr);
    void initialize(DA::DACoreInterface* core);
    void bindWorker(MyWorker* worker);
    void saveToXml(QDomElement& parentElement, QDomDocument& doc);
    void loadFromXml(const QDomElement& parentElement);
private:
    DA::DACoreInterface* m_core { nullptr };
    DA::DAUIInterface* m_ui { nullptr };
    QMainWindow* m_mainWindow { nullptr };
    MyWorker* m_worker { nullptr };
    QAction* actionMyAction { nullptr };
    // ... 其他 UI 成员
};
```

## 场景 B：UI 集成

### B1. 获取 UI 接口

```cpp
void MyUI::initialize(DA::DACoreInterface* core)
{
    m_core = core;
    m_ui = core->getUiInterface();
    m_mainWindow = m_ui->getMainWindow();

    // 获取各 UI 区域
    DA::DADockingAreaInterface* dockArea = m_ui->getDockingArea();
    DA::DARibbonAreaInterface* ribbonArea = m_ui->getRibbonArea();
    DA::DAActionsInterface* actionInterface = m_ui->getActionInterface();
    DA::DAStatusBarInterface* statusBar = m_ui->getStatusBar();
}
```

### B2. 添加 Ribbon 工具栏

```cpp
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"

void MyUI::initialize(DA::DACoreInterface* core)
{
    // ... 获取接口 ...

    SARibbonBar* ribbon = ribbonArea->ribbonBar();

    // 方式1：在已有主页面板中添加 Action
    SARibbonPanel* dataOptPanel = ribbonArea->getPannelByObjectName(
        QStringLiteral("da-pannel-main.data-opt"));
    if (dataOptPanel) {
        actionMyAction = new QAction(QIcon(":/myicon.svg"), QString(u8"我的功能"));
        dataOptPanel->addLargeAction(actionMyAction);
    }

    // 方式2：创建新的 Ribbon 标签页
    SARibbonCategory* myCategory = new SARibbonCategory(QString(u8"我的功能"), ribbon);
    ribbon->addCategoryPage(myCategory);

    SARibbonPanel* panel1 = myCategory->addPanel(QString(u8"操作"));
    QAction* action1 = new QAction(QIcon(":/icon1.svg"), QString(u8"操作1"));
    panel1->addLargeAction(action1);

    // 添加可选中的 Action（如设备选择）
    QAction* actionSelectable = new QAction(this);
    actionSelectable->setCheckable(true);
    actionSelectable->setChecked(true);
    panel1->addLargeAction(actionSelectable);
}
```

### B3. 隐藏不需要的 UI 元素

```cpp
void MyUI::initialize(DA::DACoreInterface* core)
{
    DA::DADockingAreaInterface* dockArea = m_ui->getDockingArea();
    DA::DARibbonAreaInterface* ribbonArea = m_ui->getRibbonArea();
    DA::DAActionsInterface* actionInterface = m_ui->getActionInterface();

    // 隐藏不需要的 Dock 窗口（如工作流相关）
    dockArea->hideDockWidget(dockArea->getWorkFlowOperateWidget());
    dockArea->hideDockWidget(dockArea->getWorkflowNodeListWidget());

    // 隐藏不需要的 Ribbon 面板
    SARibbonPanel* panel = ribbonArea->getPannelByObjectName("da-pannel-main.workflow");
    if (panel) panel->hide();

    // 隐藏不需要的 Action
    if (QAction* a = actionInterface->findAction("actionShowWorkFlowArea")) {
        a->setVisible(false);
    }
}
```

### B4. 创建 Dock 停靠窗口

```cpp
#include "DockWidget.h"
#include "DockManager.h"

void MyUI::initialize(DA::DACoreInterface* core)
{
    DA::DADockingAreaInterface* dockArea = m_ui->getDockingArea();

    // 方式1：在中心停靠区创建标签页式 Dock
    QWidget* myWidget = new QWidget();
    ads::CDockWidget* myDock = dockArea->createDockWidgetTabAtCenterDockArea(
        myWidget, QString(u8"我的窗口"));

    // 方式2：作为已有 Dock 的标签页
    ads::CDockWidget* settingDock = dockArea->getSettingContainerDock();
    ads::CDockWidget* myTabDock = dockArea->createDockWidgetAsTab(
        myWidget, QStringLiteral("my_widget_dock"),
        settingDock->dockAreaWidget());
    myTabDock->setWindowTitle(QString::fromUtf8("我的标签页"));

    // 切换到特定功能区域
    dockArea->raiseFeatureArea(DA::DAWorkbenchFeatureType::Chart);
}
```

### B5. 使用 Gallery 控件（模板选择器）

```cpp
#include "SARibbonGallery.h"
#include "SARibbonGalleryGroup.h"

void MyUI::buildTemplateGallery()
{
    SARibbonPanel* panel = m_category->addPanel(QString(u8"模板"));
    SARibbonGallery* gallery = panel->addGallery();

    QList<QAction*> actions;
    for (const auto& tmpl : m_templates) {
        QAction* act = new QAction(QIcon(":/icon.svg"), tmpl->name(), this);
        act->setData(QVariant::fromValue(tmpl));
        actions.append(act);
    }

    SARibbonGalleryGroup* group = gallery->addCategoryActions(
        QString(u8"模板组"), actions);
    group->setGalleryGroupStyle(SARibbonGalleryGroup::IconWithWordWrapText);
    group->setGridMinimumWidth(80);

    connect(group, &SARibbonGalleryGroup::triggered,
            this, &MyUI::onTemplateSelected);
}
```

### B6. 状态栏交互

```cpp
void MyWorker::doLongTask()
{
    DA::DAStatusBarInterface* statusBar = m_ui->getStatusBar();
    statusBar->showProgressBar();
    statusBar->setBusy(true);
    statusBar->setProgressText(QString(u8"正在处理..."));
    statusBar->setProgress(50.0);

    // ... 处理逻辑 ...

    statusBar->hideProgressBar();
    statusBar->setBusy(false);
    statusBar->showMessage(QString(u8"完成!"));
}
```

## 场景 C：数据访问

### C1. 通过 DataManager 查找数据

```cpp
#include "DADataManagerInterface.h"
#include "DAData.h"

void MyWorker::loadData()
{
    DA::DADataManagerInterface* datamgr = m_core->getDataManagerInterface();

    // 按名称通配符查找数据表
    QList<DA::DAData> moduleDatas = datamgr->findDatas("*module*");
    QList<DA::DAData> indoorDatas = datamgr->findDatas("*indoor*");
    QList<DA::DAData> systemDatas = datamgr->findDatas("*system*");

    // 遍历数据
    for (const DA::DAData& d : moduleDatas) {
        QString name = d.getName();
        // 从名称提取信息
        auto parts = name.split("_");
        int ip = parts.last().toInt();
        // ...
    }
}
```

### C2. DAData 转 DataFrame

```cpp
#include "DAPyDataFrame.h"

void MyWorker::processData(const DA::DAData& data)
{
    // DAData 转 DataFrame（Python pandas 对象）
    DA::DAPyDataFrame df = data.toDataFrame();
    auto shape = df.shape();
    int rows = static_cast<int>(shape.first);
    int cols = static_cast<int>(shape.second);

    // 逐列读取数据
    QVector<double> values;
    df[0].castTo<double>(std::back_inserter(values));
}
```

### C3. 创建绘图（Figure）

```cpp
#include "DAChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAChartManageWidget.h"
#include "DADockingAreaInterface.h"

bool MyWorker::createFigure(const QString& name)
{
    DA::DADockingAreaInterface* docking = m_ui->getDockingArea();
    DA::DAChartOperateWidget* chartOpt = docking->getChartOperateWidget();

    // 创建 Figure
    DA::DAFigureWidget* figureWidget = chartOpt->createFigure(name);

    // 获取 Figure 管理器
    QwtFigure* figure = figureWidget->figure();

    // 获取所有 Plot
    QList<QwtPlot*> plots = figure->allAxes();

    // 配置绘图...
    for (QwtPlot* plot : plots) {
        plot->rescaleAxes();
    }

    // 显示 Dock
    m_ui->getDockingArea()->raiseDockByWidget(chartOpt);
    return true;
}
```

## 场景 D：工程存档

### D1. 保存到 XML

```cpp
void MyWorker::saveToXml(QDomElement& parentElement, QDomDocument& doc)
{
    QDomElement workerElement = doc.createElement("worker");
    parentElement.appendChild(workerElement);

    // 保存状态
    workerElement.setAttribute("value", QString::number(m_someValue));

    // 保存列表
    for (int i : m_someList) {
        QDomElement item = doc.createElement("item");
        item.setAttribute("ip", QString::number(i));
        workerElement.appendChild(item);
    }
}
```

### D2. 从 XML 加载

```cpp
bool MyWorker::loadFromXml(const QDomElement& parentElement)
{
    QDomElement workerElement = parentElement.firstChildElement("worker");
    if (workerElement.isNull()) return false;

    m_someValue = workerElement.attribute("value").toInt();

    m_someList.clear();
    for (QDomElement item = workerElement.firstChildElement("item");
         !item.isNull();
         item = item.nextSiblingElement("item")) {
        m_someList.append(item.attribute("ip").toInt());
    }
    return true;
}
```

### D3. 存档机制说明

`createArchiveTask(isSave)` 返回 `DAAbstractArchiveTask` 的子类。常用 `DAZipArchiveTask_Xml`：
- **保存**：传入 XML 路径和 `QDomDocument`，框架将其写入工程压缩包
- **加载**：传入 XML 路径，框架从工程压缩包读取，通过 `setLoadedCallBack` 回调通知

## 关键接口速查

| 接口 | 获取方式 | 用途 |
|------|---------|------|
| `DACoreInterface` | `core()` (基类提供) | 核心入口，获取所有其他接口 |
| `DAUIInterface` | `core()->getUiInterface()` | 界面接口 |
| `DADockingAreaInterface` | `ui->getDockingArea()` | 停靠窗口管理 |
| `DARibbonAreaInterface` | `ui->getRibbonArea()` | Ribbon 工具栏管理 |
| `DAStatusBarInterface` | `ui->getStatusBar()` | 状态栏管理 |
| `DAActionsInterface` | `ui->getActionInterface()` | Action 管理 |
| `DADataManagerInterface` | `core()->getDataManagerInterface()` | 数据管理 |
| `DAProjectInterface` | `core()->getProjectInterface()` | 工程管理 |
| `DAPythonSignalHandler` | `core()->getPythonSignalHandler()` | Python 跨线程通信 |

## DAWorkbench 组件速查

| 组件 | 用途 |
|------|------|
| DAUtils | 通用工具函数 |
| DAMessageHandler | 全局消息处理 |
| DAAxOfficeWrapper | Office COM 自动化（仅 Windows） |
| DAPyBindQt | pybind11 与 Qt 集成 |
| DAPyScripts | Python 脚本资源管理 |
| DAData | 数据模型管理 |
| DAGraphicsView | 图形视图框架 |
| DAFigure | 科学绘图（基于 qwt） |
| DAPyCommonWidgets | Python 相关窗口部件 |
| DAGui | 主界面 GUI 组件 |
| DAInterface | 接口层 |
| DAPluginSupport | 插件加载支持 |

> **已废弃**：`DAWorkFlow` → 用 `DAPyWorkFlow`；`DACommonWidgets` → 已合并到 `DAGui`。

## 第三方库导入宏

| 宏 | 导入库 |
|----|--------|
| `damacro_import_SARibbonBar` | SARibbonBar（Ribbon 工具栏） |
| `damacro_import_DALiteCtk` | DALiteCtk |
| `damacro_import_QtAdvancedDocking` | ADS（停靠面板） |
| `damacro_import_qwt` | Qwt（科学绘图） |
| `damacro_import_orderedmap` | tsl::ordered_map |
| `damacro_import_Python` | Python3 开发库 |
| `damacro_import_pybind11` | pybind11 头文件 |
| `damacro_import_spdlog` | spdlog（日志） |

## 常见陷阱

1. **必须先构建 data-workbench 子模块** — 插件依赖 `daworkbench_plugin_utils.cmake` 和所有 DAWorkbench 库
2. **使用 Visual Studio 生成器** — 不要用 Ninja，PowerShell 中 MSVC 环境无法正确注入
3. **构建类型必须一致** — 插件的 `--config` 参数必须与 data-workbench 一致（Release/Debug）
4. **DAWidgets 需显式 find_package** — DAGui 依赖 DAWidgets，但 DAWorkbenchConfig 不会自动加载它
5. **Q_PLUGIN_METADATA 必须正确** — IID 必须是 `DAABSTRACTNODEPLUGIN_IID`
6. **插件类必须同时继承 QObject 和 DAAbstractNodePlugin** — QObject 必须是第一个基类
7. **DLL 直接输出到 plugins 目录** — 通过 `RUNTIME_OUTPUT_DIRECTORY` 设置，非 install 阶段
8. **Python 脚本通过 file(COPY) 部署** — CMake 配置阶段自动复制到 `bin/PyScripts/`，修改 .py 后需重新 CMake configure 或手动复制

## 参考文件

| 文件 | 说明 |
|------|------|
| `src/MyPlugin.h/.cpp` | 完整插件实现示例 |
| `src/MyUI.h/.cpp` | UI 管理类示例（Ribbon/Dock/Action） |
| `src/MyWorker.h/.cpp` | 业务逻辑类示例（数据/绘图/Python） |
| `src/CMakeLists.txt` | 插件 CMake 完整配置 |
| `data-workbench/cmake/daworkbench_plugin_utils.cmake` | 平台 CMake 宏定义 |
| `data-workbench/src/DAPluginSupport/DAAbstractNodePlugin.h` | 插件接口定义 |
| `data-workbench/src/DAPluginSupport/DAAbstractPlugin.h` | 插件基类定义 |
| `data-workbench/src/DAInterface/DACoreInterface.h` | 核心接口定义 |
| `data-workbench/src/DAInterface/DAUIInterface.h` | UI 接口定义 |
