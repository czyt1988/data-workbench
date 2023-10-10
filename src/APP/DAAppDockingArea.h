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
//管理窗口
class DAWorkFlowNodeListWidget;
class DAChartManageWidget;
class DADataManageWidget;
//操作窗口
class DAWorkFlowOperateWidget;
class DAWorkFlowEditWidget;
class DAChartOperateWidget;
class DADataOperateWidget;
//设置窗口
class DASettingContainerWidget;
//日志窗口
class DAMessageLogViewWidget;
//
class DAAbstractNodeGraphicsItem;
class DAAbstractNodeWidget;

/**
 * @brief 负责docking窗口区域的管理，APP分两大区域-RibbonArea和DockArea
 * DockArea包含所有的窗口
 */
class DAAppDockingArea : public DADockingAreaInterface
{
    Q_OBJECT
public:
    /**
     * @brief 定义了固定的dock窗口
     */
    enum DockingArea
    {
        DockingAreaWorkFlowOperate,
        DockingAreaDataOperate,
        DockingAreaChartOperate,
        DockingAreaWorkFlowManager,
        DockingAreaDataManager,
        DockingAreaChartManager,
        DockingAreaSetting,
        DockingAreaMessageLog
    };

public:
    DAAppDockingArea(DAUIInterface* u);
    ~DAAppDockingArea();

    //翻译
    void retranslateUi() override;
    //设置文本
    void resetText();

public:
    //获取工作节点管理窗口
    virtual DAWorkFlowNodeListWidget* getWorkflowNodeListWidget() const override;

    //工作流操作窗口
    virtual DAWorkFlowOperateWidget* getWorkFlowOperateWidget() const override;

    //绘图管理窗口
    virtual DAChartManageWidget* getChartManageWidget() const override;

    //绘图操作窗口
    virtual DAChartOperateWidget* getChartOperateWidget() const override;

    //数据管理窗口
    virtual DADataManageWidget* getDataManageWidget() const override;

    //数据操作窗口
    virtual DADataOperateWidget* getDataOperateWidget() const override;

    //获取日志显示窗口
    virtual DAMessageLogViewWidget* getMessageLogViewWidget() const override;

    //获取设置窗口,设置容器可以放置多个设置窗口
    virtual DASettingContainerWidget* getSettingContainerWidget() const override;

    // DockingArea对应的dock窗口指针
    ads::CDockWidget* dockingAreaToDockWidget(DockingArea area) const;

    //提升显示工作流操作页面
    void raiseDockingArea(DockingArea area);

    //判断是否处于焦点
    bool isDockingAreaFocused(DockingArea area) const;

public slots:
    //显示数据
    void showDataOperateWidget(const DA::DAData& data);

private:
    //构建界面
    void buildDockingArea();
    //创建各个相关的窗口
    void buildWorkflowAboutWidgets();
    void buildChartAboutWidgets();
    void buildDataAboutWidgets();
    void buildOtherWidgets();
    //初始化信号槽
    void initConnection();
private slots:
    //
    void onSelectNodeItemChanged(DA::DAAbstractNodeGraphicsItem* i);
    void onDataManageWidgetDataDbClicked(const DA::DAData& data);
    // workflow窗口创建信号对应槽
    void onWorkFlowOperateWidgetWorkflowCreated(DA::DAWorkFlowEditWidget* wfw);

private:
    AppMainWindow* mApp;
    DAAppCommand* mAppCmd;  ///< cmd
    DAAppDataManager* mDataMgr;
    ads::CDockManager* mDockmgr;

    //管理窗口不允许关闭
    // 管理窗口
    DAWorkFlowNodeListWidget* mWorkflowNodeListWidget;  ///< 工作流节点窗口
    ads::CDockWidget* mWorkflowNodeListDock;            ///< m_workflowNodeListWidget对应的dock
    DAChartManageWidget* mChartManageWidget;            ///< 绘图管理窗口
    ads::CDockWidget* mChartManageDock;                 ///< m_chartManageWidget对应的dock
    DADataManageWidget* mDataManageWidget;              ///< 数据窗口
    ads::CDockWidget* mDataManageDock;                  ///< m_dataManageWidget对应的dock
    //操作窗口不允许关闭
    // 操作窗口
    DAWorkFlowOperateWidget* mWorkFlowOperateWidget;  ///< 工作流操作窗口
    ads::CDockWidget* mWorkFlowOperateDock;           ///< m_workFlowOperateWidget对应的dock
    DAChartOperateWidget* mChartOperateWidget;        ///< 绘图操作窗口
    ads::CDockWidget* mChartOperateDock;              ///< m_chartOperateWidget对应的dock
    DADataOperateWidget* mDataOperateWidget;          ///< 数据操作窗口
    ads::CDockWidget* mDataOperateDock;               ///< m_dataOperateWidget对应的dock

    //设置窗口
    DASettingContainerWidget* mSettingContainerWidget;  ///< 设置窗口容器
    ads::CDockWidget* mSettingContainerDock;
    //日志窗口
    DAMessageLogViewWidget* mMessageLogViewWidget;  ///< 日志窗口
    ads::CDockWidget* mMessageLogDock;
    //
    DAAbstractNodeWidget* mLastSetNodeWidget;
};
}  // namespace DA
#endif  // DAAPPDOCKINGAREA_H
