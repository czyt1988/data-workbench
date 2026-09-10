#include "DAAppDockingArea.h"
#include <QApplication>
#include <QScreen>
#include <QLabel>
#include <QFileInfo>
#include "AppMainWindow.h"
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include "DockAreaWidget.h"
// API相关
#include "DAAppCore.h"
#include "DAAppProject.h"
#include "DALogCategory.h"
#include "DAUIInterface.h"
#include "DAUiObjectNames.h"
#include "DACommandInterface.h"
#include "DAAppCommand.h"
#include "DAAppDataManager.h"
// chart相关
#include "Chart/DAChartOperateWidget.h"
#include "DAAppFigureFactory.h"
#include "DAAppChartOperateWidget.h"
#include "DAAppChartManageWidget.h"
#include "Chart/DAChartSettingWidget.h"
// Data相关
#include "DADataOperateWidget.h"
#include "DADataManageWidget.h"
// message相关
#include "DAMessageLogViewWidget.h"
// workflow相关
#include "DAPyNodeGraphicsItem.h"
#include "DAPyWorkFlowNodeListWidget.h"
#include "DAPyWorkFlowEditWidget.h"
#include "DASettingContainerWidget.h"
#include "DAPyWorkFlowNodeItemSettingWidget.h"
#include "DAAppWorkFlowOperateWidget.h"
// Agent 相关
#include "DAAgentDockWidget.h"
// Markdown 查看相关
#include "DAMarkdownView.h"
// 数据联动表
#include "Chart/DADataLinkTableWidget.h"

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppDockingArea
//===================================================
DAAppDockingArea::DAAppDockingArea(DAUIInterface* u) : DADockingAreaInterface(u)
{
    mApp     = qobject_cast< AppMainWindow* >(u->mainWindow());
    mAppCmd  = qobject_cast< DAAppCommand* >(u->getCommandInterface());
    mDataMgr = qobject_cast< DAAppDataManager* >(core()->getDataManagerInterface());
    buildDockingArea();
}

DAAppDockingArea::~DAAppDockingArea()
{
}

void DAAppDockingArea::retranslateUi()
{
    resetText();
}

void DAAppDockingArea::resetText()
{
    mWorkflowNodeListDock->setWindowTitle(tr("Workflow Node"));    // cn:节点
    mChartManageDock->setWindowTitle(tr("Charts Manager"));        // cn:绘图管理
    mDataManageDock->setWindowTitle(tr("Data Manager"));          // cn:数据管理
    mWorkFlowOperateDock->setWindowTitle(tr("Workflow Operate"));  // cn:工作流操作
    mChartOperateDock->setWindowTitle(tr("Chart Operate"));        // cn:绘图操作
    mDataOperateDock->setWindowTitle(tr("Data Operate"));          // cn:数据操作
    mSettingContainerDock->setWindowTitle(tr("Setting"));          // cn:设置
    mMessageLogDock->setWindowTitle(tr("Log"));                    // cn:消息
    mAgentDock->setWindowTitle(tr("Agent Assistant"));  // cn:Agent 助手
    mDataLinkTableDock->setWindowTitle(tr("Data Link Table"));  // cn:数据联动表
}

/**
 * @brief 获取工作流操作窗口
 * @return
 */
DAPyWorkFlowNodeListWidget* DAAppDockingArea::getWorkflowNodeListWidget() const
{
    return mWorkflowNodeListWidget;
}

/**
 * @brief 获取工作流操作窗口
 * @return
 */
DAPyWorkFlowOperateWidget* DAAppDockingArea::getWorkFlowOperateWidget() const
{
    return mWorkFlowOperateWidget;
}

/**
 * @brief 获取绘图管理窗口
 * @return
 */
DAChartManageWidget* DAAppDockingArea::getChartManageWidget() const
{
    return mChartManageWidget;
}

/**
 * @brief 获取绘图操作窗口
 * @return
 */
DAChartOperateWidget* DAAppDockingArea::getChartOperateWidget() const
{
    return mChartOperateWidget;
}

/**
 * @brief 获取数据操作窗口
 * @return
 */
DADataManageWidget* DAAppDockingArea::getDataManageWidget() const
{
    return mDataManageWidget;
}

/**
 * @brief 获取数据操作窗口
 * @return
 */
DADataOperateWidget* DAAppDockingArea::getDataOperateWidget() const
{
    return mDataOperateWidget;
}

/**
 * @brief 获取日志显示窗口
 * @return
 */
DAMessageLogViewWidget* DAAppDockingArea::getMessageLogViewWidget() const
{
    return mMessageLogViewWidget;
}

/**
 * @brief 获取设置窗口
 * @return
 */
DASettingContainerWidget* DAAppDockingArea::getSettingContainerWidget() const
{
    return mSettingContainerWidget;
}

/**
 * @brief 显示数据
 * @param data
 */
void DAAppDockingArea::showDataOperateWidget(const DA::DAData& data, const QString& name)
{
    mDataOperateWidget->showData(data);
    mDataOperateWidget->ensureCurrentTableColumnVisible(name);
    // 把表格窗口唤起
    raiseDockByWidget((QWidget*)getDataOperateWidget());
}

/**
 * @brief 构建dockwidget
 *
 * 可以通过dockManager()->findDockWidget(objname:QString)函数来找到对应的dockwidget并进行操作
 *
 * 所有本APP相关的关键object name都以da打头:
 *
 * da_workflowNodeListWidgetDock
 * da_chartManageWidgetDock
 * da_dataManageWidgetDock
 * da_workFlowOperateWidgetDock
 * da_chartOperateWidgetDock
 * da_dataOperateWidgetDock
 * da_settingDock
 * da_messageLogViewWidgetDock
 *
 */
void DAAppDockingArea::buildDockingArea()
{
    // 中央操作区
    buildWorkflowAboutWidgets();
    buildChartAboutWidgets();
    buildDataAboutWidgets();
    buildOtherWidgets();

    // QLabel* centerLabel = new QLabel(mApp);
    // auto center         = createCenterDockWidget(centerLabel, QStringLiteral("centerLabel"));

    mWorkFlowOperateDock = createCenterDockWidget(mWorkFlowOperateWidget, QString::fromUtf8(UiNames::Dock::WorkFlowOperateWidgetDock));
    mWorkFlowOperateDock->setIcon(QIcon(":/app/bright/Icon/showWorkFlow.svg"));
    mWorkFlowOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    mChartOperateDock =
        createDockWidgetTabAtCenterDockArea(mChartOperateWidget, QString::fromUtf8(UiNames::Dock::ChartOperateWidgetDock));
    mChartOperateDock->setIcon(QIcon(":/app/bright/Icon/showChart.svg"));
    mChartOperateDock->setToggleViewActionMode(ads::CDockWidget::ActionModeToggle);
    mChartOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    mDataOperateDock = createDockWidgetTabAtCenterDockArea(mDataOperateWidget, QString::fromUtf8(UiNames::Dock::DataOperateWidgetDock));
    mDataOperateDock->setIcon(QIcon(":/app/bright/Icon/showTable.svg"));
    mDataOperateDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    mDataOperateDock->raise();

    // 左侧管理区 - 工作流节点窗口
    mWorkflowNodeListDock = createDockWidget(
        mWorkflowNodeListWidget, ads::LeftDockWidgetArea, QString::fromUtf8(UiNames::Dock::WorkflowNodeListWidgetDock));
    mWorkflowNodeListDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    mChartManageDock = createDockWidgetAsTab(
        mChartManageWidget, QString::fromUtf8(UiNames::Dock::ChartManageWidgetDock), mWorkflowNodeListDock->dockAreaWidget());
    mChartManageDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

    mDataManageDock = createDockWidgetAsTab(
        mDataManageWidget, QString::fromUtf8(UiNames::Dock::DataManageWidgetDock), mWorkflowNodeListDock->dockAreaWidget());
    mDataManageDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    mDataManageDock->raise();

    // 右侧附属区 - 添加设置视图
    mSettingContainerDock =
        createDockWidget(mSettingContainerWidget, ads::RightDockWidgetArea, QString::fromUtf8(UiNames::Dock::SettingDock));
    mSettingContainerDock->setIcon(QIcon(":/app/bright/Icon/showSettingWidget.svg"));
    // Agent 助手 Dock —— 作为右侧附属区的标签页，与"设置"同组
    mAgentDockWidget = new DAAgentDockWidget(mApp);
    mAgentDockWidget->setObjectName(QString::fromUtf8(UiNames::Dock::AgentDockWidget));
    mAgentDock = createDockWidgetAsTab(
        mAgentDockWidget,
        QString::fromUtf8(UiNames::Dock::AgentDockWidgetDock),
        mSettingContainerDock->dockAreaWidget());
    mAgentDock->setIcon(QIcon(":/app/bright/Icon/showAgent.svg"));

    // 数据联动表 Dock —— 作为右侧附属区的标签页，与"设置"/"Agent"同组
    mDataLinkTableWidget = new DADataLinkTableWidget(mApp);
    mDataLinkTableWidget->setObjectName(QString::fromUtf8(UiNames::Dock::DataLinkTableWidget));
    mDataLinkTableWidget->setChartOperateWidget(mChartOperateWidget);
    mDataLinkTableDock = createDockWidgetAsTab(
        mDataLinkTableWidget,
        QString::fromUtf8(UiNames::Dock::DataLinkTableWidgetDock),
        mSettingContainerDock->dockAreaWidget());
    mDataLinkTableDock->setIcon(QIcon(":/app/bright/Icon/plot-probe.svg"));
    // 探针创建（探针模式点击）-> 联动表 dock 提到前台
    connect(mDataLinkTableWidget, &DADataLinkTableWidget::requestRaise, this, [this]() {
        raiseDockByWidget(mDataLinkTableWidget);
    });

    // 日志窗口 —— pin to right，auto-hide 到右侧边栏
    mMessageLogDock = new ads::CDockWidget(dockManager(), QString::fromUtf8(UiNames::Dock::MessageLogViewWidgetDock));
    mMessageLogDock->setWidget(mMessageLogViewWidget);
    mMessageLogDock->setIcon(QIcon(":/app/bright/Icon/showInfomation.svg"));
    dockManager()->addAutoHideDockWidget(ads::SideBarRight, mMessageLogDock);

    // 设置dock的区域大小,默认为左1：中间4：右：1
    resetDefaultSplitterSizes();
#if 0
	QScreen* screen = QApplication::primaryScreen();
	int leftwidth   = screen->size().width() / 6;
	int rightwidth  = leftwidth;
	int centerwidth = screen->size().width() - leftwidth - rightwidth;
	dockManager()->setSplitterSizes(mWorkflowNodeListDock->dockAreaWidget(), { leftwidth, centerwidth, rightwidth });
#endif
    //

    initConnection();
    resetText();
}

/**
 * @brief 创建workflow相关的窗口
 */
void DAAppDockingArea::buildWorkflowAboutWidgets()
{
    mWorkFlowOperateWidget = new DAAppWorkFlowOperateWidget(mApp);
    mWorkFlowOperateWidget->setObjectName(QString::fromUtf8(UiNames::Dock::WorkFlowOperateWidget));
    mWorkflowNodeListWidget = new DAPyWorkFlowNodeListWidget(mApp);
    mWorkflowNodeListWidget->setObjectName(QString::fromUtf8(UiNames::Dock::WorkflowNodeListWidget));
    // 把工作流操作窗口设置到工程中
    DAAppProject* project = DA_APP_CORE.getAppProject();
    project->setDockingAreaInterface(this);
}

/**
 * @brief 创建绘图相关窗口（绘图操作窗口、绘图管理窗口）
 */
void DAAppDockingArea::buildChartAboutWidgets()
{
    DADataManager* dmgr = mDataMgr->dataManager();

    DAAppChartOperateWidget* appChartOptWidget = new DAAppChartOperateWidget(mApp);
    appChartOptWidget->setObjectName(QString::fromUtf8(UiNames::Dock::ChartOperateWidget));
    appChartOptWidget->setDataManager(dmgr);
    appChartOptWidget->setupFigureFactory(new DAAppFigureFactory());
    mChartOperateWidget = appChartOptWidget;

    mChartManageWidget = new DAAppChartManageWidget(mApp);
    mChartManageWidget->setObjectName(QString::fromUtf8(UiNames::Dock::ChartManageWidget));

    mChartManageWidget->setChartOperateWidget(mChartOperateWidget);
}

/**
 * @brief 创建数据相关窗口（数据操作窗口、数据管理窗口）
 */
void DAAppDockingArea::buildDataAboutWidgets()
{
    DADataManager* dmgr = mDataMgr->dataManager();
    mDataOperateWidget  = new DADataOperateWidget(dmgr, mApp);
    mDataOperateWidget->setObjectName(QString::fromUtf8(UiNames::Dock::DataOperateWidget));

    mDataManageWidget = new DADataManageWidget(mApp);
    mDataManageWidget->setObjectName(QString::fromUtf8(UiNames::Dock::DataManageWidget));
    mDataManageWidget->setDataManager(dmgr);
}

/**
 * @brief 创建其他相关窗口（设置窗口、日志窗口）
 */
void DAAppDockingArea::buildOtherWidgets()
{
    // 右侧附属区 - 添加设置视图
    mSettingContainerWidget = new DASettingContainerWidget(mApp);
    mSettingContainerWidget->setObjectName(QString::fromUtf8(UiNames::Dock::SettingContainerWidget));
    // 日志窗口
    mMessageLogViewWidget = new DAMessageLogViewWidget(mApp);
    mMessageLogViewWidget->setObjectName(QString::fromUtf8(UiNames::Dock::MessageLogViewWidget));
}

/**
 * @brief 初始化信号槽连接
 */
void DAAppDockingArea::initConnection()
{
    // DADataManageWidget的数据双击，在DADataOperateWidget中显示
    connect(mDataManageWidget, &DADataManageWidget::dataDbClicked, this, &DAAppDockingArea::onDataManageWidgetDataDbClicked);
    connect(mDataManageWidget,
            &DADataManageWidget::dataSeriesDbClicked,
            this,
            &DAAppDockingArea::onDataManageWidgetDataSeriesDbClicked);
    // 设置窗口的绑定
    mSettingContainerWidget->getWorkFlowNodeItemSettingWidget()->setWorkFlowOperateWidget(mWorkFlowOperateWidget);
}

/**
 * @brief 数据管理窗口数据双击回调
 * @param data 数据
 */
void DAAppDockingArea::onDataManageWidgetDataDbClicked(const DA::DAData& data)
{
    showDataOperateWidget(data);
}

/**
 * @brief 数据管理窗口数据系列双击回调
 * @param data 数据
 * @param name 名称
 */
void DAAppDockingArea::onDataManageWidgetDataSeriesDbClicked(const DAData& data, const QString& name)
{
    showDataOperateWidget(data, name);
}

/**
 * @brief 获取图表管理窗口 dock
 * @return
 */
ads::CDockWidget* DAAppDockingArea::getChartManageDock() const
{
    return mChartManageDock;
}

/**
 * @brief 切换左侧边栏（工作流节点、图表管理、数据管理）的显示/隐藏
 * @param show 是否显示
 */
void DAAppDockingArea::toggleLeftSidebar(bool show)
{
    // 左侧边栏包含的dock widgets
    QList< ads::CDockWidget* > leftSidebarDocks = { mWorkflowNodeListDock, mChartManageDock, mDataManageDock };

    for (ads::CDockWidget* dock : leftSidebarDocks) {
        if (dock) {
            if (show) {
                // 显示：如果当前是关闭状态，则打开
                if (dock->isClosed()) {
                    dock->toggleView(true);  // true表示打开
                }
            } else {
                // 隐藏：如果当前是打开状态，则关闭
                if (!dock->isClosed()) {
                    dock->toggleView(false);  // false表示关闭
                }
            }
        }
    }
}

/**
 * @brief 切换右侧边栏（设置、日志）的显示/隐藏
 * @param show 是否显示
 */
void DAAppDockingArea::toggleRightSidebar(bool show)
{
    // 右侧边栏包含的dock widgets
    QList< ads::CDockWidget* > rightSidebarDocks = { mSettingContainerDock, mMessageLogDock };

    for (ads::CDockWidget* dock : rightSidebarDocks) {
        if (dock) {
            if (show) {
                // 显示
                if (dock->isClosed()) {
                    dock->toggleView(true);
                }
            } else {
                // 隐藏
                if (!dock->isClosed()) {
                    dock->toggleView(false);
                }
            }
        }
    }
}

/**
 * @brief 检查左侧边栏是否至少有一个dock可见
 * @return
 */
bool DAAppDockingArea::isLeftSidebarVisible() const
{
    // 检查左侧边栏是否至少有一个dock是可见的
    return (mWorkflowNodeListDock && !mWorkflowNodeListDock->isClosed())
           || (mChartManageDock && !mChartManageDock->isClosed()) || (mDataManageDock && !mDataManageDock->isClosed());
}

/**
 * @brief 检查右侧边栏是否至少有一个dock可见
 * @return
 */
bool DAAppDockingArea::isRightSidebarVisible() const
{
    // 检查右侧边栏是否至少有一个dock是可见的
    return (mSettingContainerDock && !mSettingContainerDock->isClosed())
           || (mMessageLogDock && !mMessageLogDock->isClosed());
}

/**
 * @brief 获取数据管理窗口 dock
 * @return
 */
ads::CDockWidget* DAAppDockingArea::getDataManageDock() const
{
    return mDataManageDock;
}

/**
 * @brief 获取工作流操作窗口 dock
 * @return
 */
ads::CDockWidget* DAAppDockingArea::getWorkFlowOperateDock() const
{
    return mWorkFlowOperateDock;
}

/**
 * @brief 获取绘图操作窗口 dock
 * @return
 */
ads::CDockWidget* DAAppDockingArea::getChartOperateDock() const
{
    return mChartOperateDock;
}

/**
 * @brief 获取数据操作窗口 dock
 * @return
 */
ads::CDockWidget* DAAppDockingArea::getDataOperateDock() const
{
    return mDataOperateDock;
}

/**
 * @brief 获取设置窗口 dock
 * @return
 */
ads::CDockWidget* DAAppDockingArea::getSettingContainerDock() const
{
    return mSettingContainerDock;
}

/**
 * @brief 获取信息窗口 dock
 * @return
 */
ads::CDockWidget* DAAppDockingArea::getMessageLogDock() const
{
    return mMessageLogDock;
}

/**
 * @brief 获取工作流节点 dock
 * @return
 */
ads::CDockWidget* DAAppDockingArea::getWorkflowNodeListDock() const
{
    return mWorkflowNodeListDock;
}

/**
 * @brief 获取 Agent 助手 dock
 * @return
 */
ads::CDockWidget* DAAppDockingArea::getAgentDock() const
{
    return mAgentDock;
}

/**
 * @brief 获取 Agent 助手 Dock Widget（原始 QWidget）
 * @return
 */
DAAgentDockWidget* DAAppDockingArea::getAgentDockWidget() const
{
    return mAgentDockWidget;
}

/**
 * @brief 获取数据联动表 dock
 * @return
 */
ads::CDockWidget* DAAppDockingArea::getDataLinkTableDock() const
{
    return mDataLinkTableDock;
}

/**
 * @brief 获取数据联动表窗口
 * @return
 */
DADataLinkTableWidget* DAAppDockingArea::getDataLinkTableWidget() const
{
    return mDataLinkTableWidget;
}

/**
 * @brief 在中央区按需创建 Markdown 查看 dock 并加载文件
 *
 * 首次调用创建 DAMarkdownView 并作为标签页挂到中央区；后续调用复用同一 dock，
 * 重新载入文件、更新标题为文件名并 raise。
 * @param filePath markdown 文件路径
 * @return 加载成功返回 true
 */
bool DAAppDockingArea::showMarkdownFile(const QString& filePath)
{
    // 首次：创建 markdown 控件 + dock 标签页，挂到中央区
    if (nullptr == mMarkdownView) {
        mMarkdownView = new DAMarkdownView(mApp);
        mMarkdownView->setObjectName(QString::fromUtf8(UiNames::Dock::MarkdownView));
        mMarkdownDock = createDockWidgetTabAtCenterDockArea(mMarkdownView, QString::fromUtf8(UiNames::Dock::MarkdownViewDock));
        mMarkdownDock->setIcon(QIcon(":/app/bright/Icon/markdown.svg"));
        // 默认标题（无文件时）
        mMarkdownDock->setWindowTitle(tr("Markdown Viewer"));  // cn:Markdown 查看器
    }
    // 载入文件
    if (!mMarkdownView->loadFile(filePath)) {
        daWarning << tr("Failed to open markdown file: %1").arg(filePath);  // cn:打开 Markdown 文件失败:%1
        return false;
    }
    // 标题用文件名，便于多文件区分
    mMarkdownDock->setWindowTitle(QFileInfo(filePath).fileName());
    mMarkdownDock->raise();
    mMarkdownView->scrollToTop();
    return true;
}
