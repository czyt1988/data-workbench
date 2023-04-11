#ifndef DAAPPDOCKINGAREA_H
#define DAAPPDOCKINGAREA_H
#include "DAAppDockingAreaInterface.h"
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
class DAAppDockingArea : public DAAppDockingAreaInterface
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
    DAAppDockingArea(DAAppUIInterface* u);
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
    DAMessageLogViewWidget* getMessageLogViewWidget() const;

    //获取设置窗口
    DASettingContainerWidget* getSettingContainerWidget() const;

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
    AppMainWindow* m_app;
    DAAppCommand* m_appCmd;  ///< cmd
    DAAppDataManager* m_dataMgr;
    ads::CDockManager* m_dockmgr;

    //管理窗口不允许关闭
    // 管理窗口
    DAWorkFlowNodeListWidget* m_workflowNodeListWidget;  ///< 工作流节点窗口
    ads::CDockWidget* m_workflowNodeListDock;            ///< m_workflowNodeListWidget对应的dock
    DAChartManageWidget* m_chartManageWidget;            ///< 绘图管理窗口
    ads::CDockWidget* m_chartManageDock;                 ///< m_chartManageWidget对应的dock
    DADataManageWidget* m_dataManageWidget;              ///< 数据窗口
    ads::CDockWidget* m_dataManageDock;                  ///< m_dataManageWidget对应的dock
    //操作窗口不允许关闭
    // 操作窗口
    DAWorkFlowOperateWidget* m_workFlowOperateWidget;  ///< 工作流操作窗口
    ads::CDockWidget* m_workFlowOperateDock;           ///< m_workFlowOperateWidget对应的dock
    DAChartOperateWidget* m_chartOperateWidget;        ///< 绘图操作窗口
    ads::CDockWidget* m_chartOperateDock;              ///< m_chartOperateWidget对应的dock
    DADataOperateWidget* m_dataOperateWidget;          ///< 数据操作窗口
    ads::CDockWidget* m_dataOperateDock;               ///< m_dataOperateWidget对应的dock

    //设置窗口
    DASettingContainerWidget* m_settingContainerWidget;  ///< 设置窗口容器
    ads::CDockWidget* m_settingContainerDock;
    //日志窗口
    DAMessageLogViewWidget* m_messageLogViewWidget;  ///< 日志窗口
    ads::CDockWidget* m_messageLogDock;
    //
    DAAbstractNodeWidget* m_lastSetNodeWidget;
};
}  // namespace DA
#endif  // DAAPPDOCKINGAREA_H
