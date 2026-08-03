#ifndef DAAPPDOCKINGAREA_H
#define DAAPPDOCKINGAREA_H
#include "DADockingAreaInterface.h"
//
#include "DAData.h"
// CDockArea
namespace ads
{
class CDockManager;
class CDockAreaWidget;
}
// SARibbon
class SARibbonMainWindow;

namespace DA
{
class AppMainWindow;
class DACoreInterface;
class DAAppDataManager;
class DAAppCommand;
// 管理窗口
class DAPyWorkFlowNodeListWidget;
class DAChartManageWidget;
class DADataManageWidget;
// 操作窗口
class DAPyWorkFlowOperateWidget;
class DAPyWorkFlowEditWidget;
class DAChartOperateWidget;
class DADataOperateWidget;
// 设置窗口
class DASettingContainerWidget;
// 日志窗口
class DAMessageLogViewWidget;
// Agent 窗口
class DAAgentDockWidget;

/**
 * @brief 负责docking窗口区域的管理，APP分两大区域-RibbonArea和DockArea
 * DockArea包含所有的窗口
 */
class DAAppDockingArea : public DADockingAreaInterface
{
    Q_OBJECT
public:
public:
    DAAppDockingArea(DAUIInterface* u);
    ~DAAppDockingArea();

    // 翻译
    void retranslateUi() override;
    // 设置文本
    void resetText();

public:
    // 获取工作节点管理窗口
    virtual DAPyWorkFlowNodeListWidget* getWorkflowNodeListWidget() const override;

    // 工作流操作窗口
    virtual DAPyWorkFlowOperateWidget* getWorkFlowOperateWidget() const override;

    // 绘图管理窗口
    virtual DAChartManageWidget* getChartManageWidget() const override;

    // 绘图操作窗口
    virtual DAChartOperateWidget* getChartOperateWidget() const override;

    // 数据管理窗口
    virtual DADataManageWidget* getDataManageWidget() const override;

    // 数据操作窗口
    virtual DADataOperateWidget* getDataOperateWidget() const override;

    // 获取日志显示窗口
    virtual DAMessageLogViewWidget* getMessageLogViewWidget() const override;

    // 获取设置窗口,设置容器可以放置多个设置窗口
    virtual DASettingContainerWidget* getSettingContainerWidget() const override;

    /**
     * @brief 工作流节点dock
     * @return
     */
    ads::CDockWidget* getWorkflowNodeListDock() const override;

    /**
     * @brief 信息窗口dock
     * @return
     */
    ads::CDockWidget* getMessageLogDock() const override;

    /**
     * @brief 设置窗口dock
     * @return
     */
    ads::CDockWidget* getSettingContainerDock() const override;

    /**
     * @brief 数据操作窗口dock
     * @return
     */
    ads::CDockWidget* getDataOperateDock() const override;

    /**
     * @brief 绘图操作窗口dock
     * @return
     */
    ads::CDockWidget* getChartOperateDock() const override;

    /**
     * @brief 工作流操作窗口dock
     * @return
     */
    ads::CDockWidget* getWorkFlowOperateDock() const override;

    /**
     * @brief 数据管理窗口dock
     * @return
     */
    ads::CDockWidget* getDataManageDock() const override;

    /**
     * @brief 图表管理窗口dock
     * @return
     */
    ads::CDockWidget* getChartManageDock() const override;

    /**
     * @brief Agent 助手 dock
     * @note base DADockingAreaInterface 无 getAgentDock() 虚函数，故不可 override
     * @return
     */
    ads::CDockWidget* getAgentDock() const;

    /**
     * @brief 获取 Agent 助手 Dock Widget（原始 QWidget）
     * @return
     */
    DAAgentDockWidget* getAgentDockWidget() const;
    // 切换左侧边栏（工作流节点、图表管理、数据管理）的显示/隐藏
    void toggleLeftSidebar(bool show);
    // 切换右侧边栏（设置、日志）的显示/隐藏
    void toggleRightSidebar(bool show);
    // 获取左右侧边栏的当前状态
    bool isLeftSidebarVisible() const;
    bool isRightSidebarVisible() const;
public Q_SLOTS:
    // 显示数据
    void showDataOperateWidget(const DA::DAData& data, const QString& name = QString());

private:
    // 构建界面
    void buildDockingArea();
    // 创建各个相关的窗口
    void buildWorkflowAboutWidgets();
    void buildChartAboutWidgets();
    void buildDataAboutWidgets();
    void buildOtherWidgets();
    // 初始化信号槽
    void initConnection();
private Q_SLOTS:
    void onDataManageWidgetDataDbClicked(const DA::DAData& data);
    void onDataManageWidgetDataSeriesDbClicked(const DA::DAData& data, const QString& name);

private:
    AppMainWindow* mApp { nullptr };
    DAAppCommand* mAppCmd { nullptr };  ///< cmd
    DAAppDataManager* mDataMgr { nullptr };

    // 管理窗口不允许关闭
    //  管理窗口
    DAPyWorkFlowNodeListWidget* mWorkflowNodeListWidget { nullptr };  ///< 工作流节点窗口
    ads::CDockWidget* mWorkflowNodeListDock { nullptr };            ///< m_workflowNodeListWidget对应的dock
    DAChartManageWidget* mChartManageWidget { nullptr };            ///< 绘图管理窗口
    ads::CDockWidget* mChartManageDock { nullptr };                 ///< m_chartManageWidget对应的dock
    DADataManageWidget* mDataManageWidget { nullptr };              ///< 数据窗口
    ads::CDockWidget* mDataManageDock { nullptr };                  ///< m_dataManageWidget对应的dock
    // 操作窗口不允许关闭
    //  操作窗口
    DAPyWorkFlowOperateWidget* mWorkFlowOperateWidget { nullptr };  ///< 工作流操作窗口
    ads::CDockWidget* mWorkFlowOperateDock { nullptr };           ///< m_workFlowOperateWidget对应的dock
    DAChartOperateWidget* mChartOperateWidget { nullptr };        ///< 绘图操作窗口
    ads::CDockWidget* mChartOperateDock { nullptr };              ///< m_chartOperateWidget对应的dock
    DADataOperateWidget* mDataOperateWidget { nullptr };          ///< 数据操作窗口
    ads::CDockWidget* mDataOperateDock { nullptr };               ///< m_dataOperateWidget对应的dock

    // 设置窗口
    DASettingContainerWidget* mSettingContainerWidget { nullptr };  ///< 设置窗口容器
    ads::CDockWidget* mSettingContainerDock { nullptr };
    // 日志窗口
    DAMessageLogViewWidget* mMessageLogViewWidget { nullptr };  ///< 日志窗口
    ads::CDockWidget* mMessageLogDock { nullptr };

    // Agent 窗口
    DAAgentDockWidget* mAgentDockWidget { nullptr };  ///< Agent 助手窗口
    ads::CDockWidget* mAgentDock { nullptr };         ///< m_agentDockWidget 对应的 dock
};
}  // namespace DA
#endif  // DAAPPDOCKINGAREA_H
