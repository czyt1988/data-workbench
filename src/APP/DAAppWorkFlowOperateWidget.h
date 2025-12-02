#ifndef DAAPPWORKFLOWOPERATEWIDGET_H
#define DAAPPWORKFLOWOPERATEWIDGET_H
#include "DAWorkFlowOperateWidget.h"
#include <QPointer>
namespace DA
{
class DAAppPluginManager;
class DAAppCommand;
/**
 * @brief DAWorkFlowOperateWidget的app实例化
 */
class DAAppWorkFlowOperateWidget : public DAWorkFlowOperateWidget
{
    Q_OBJECT
public:
    DAAppWorkFlowOperateWidget(QWidget* parent = nullptr);
    ~DAAppWorkFlowOperateWidget();
    virtual DAWorkFlow* createWorkflow() override;
    // 设置插件管理器，工作流工厂通过插件管理器生成
    void setPluginManager(DAAppPluginManager* pluginMgr);
private slots:
    // 工作流创建
    void onWorkflowCreated(DA::DAWorkFlowEditWidget* wfw);
    // 当前工作流的界面发生变化
    void onCurrentWorkFlowWidgetChanged(DA::DAWorkFlowEditWidget* w);
    // 工作流删除
    void onWorkflowRemoving(DA::DAWorkFlowEditWidget* w);
    // 工作流清空
    void onWorkflowClearing();

private:
    DAAppCommand* cmd() const;
    QPointer< DAAppPluginManager > mPluginMgr;
};
}

#endif  // DAAPPWORKFLOWOPERATEWIDGET_H
