#ifndef DAAPPWORKFLOWOPERATEWIDGET_H
#define DAAPPWORKFLOWOPERATEWIDGET_H
#include "DAPyWorkFlowOperateWidget.h"
#include <QPointer>
namespace DA
{
class DAAppPluginManager;
class DAAppCommand;
/**
 * @brief DAPyWorkFlowOperateWidget的app实例化
 */
class DAAppWorkFlowOperateWidget : public DAPyWorkFlowOperateWidget
{
    Q_OBJECT
public:
    DAAppWorkFlowOperateWidget(QWidget* parent = nullptr);
    ~DAAppWorkFlowOperateWidget();
    virtual DAPyWorkFlow* createWorkflow() override;
    // 设置插件管理器，工作流工厂通过插件管理器生成
    void setPluginManager(DAAppPluginManager* pluginMgr);
private slots:
    // 工作流创建
    void onWorkflowCreated(DA::DAPyWorkFlowEditWidget* wfw);
    // 当前工作流的界面发生变化
    void onCurrentWorkFlowWidgetChanged(DA::DAPyWorkFlowEditWidget* w);
    // 工作流删除
    void onWorkflowRemoving(DA::DAPyWorkFlowEditWidget* w);
    // 工作流清空
    void onWorkflowClearing();

private:
    DAAppCommand* cmd() const;
    QPointer< DAAppPluginManager > mPluginMgr;
};
}

#endif  // DAAPPWORKFLOWOPERATEWIDGET_H
