#ifndef DAPYWORKFLOWRIBBONGROUP_H
#define DAPYWORKFLOWRIBBONGROUP_H
#include "DAGuiAPI.h"
#include "DAPyWorkflowActions.h"
#include <QObject>
#include <QList>

class QAction;
class SARibbonPanel;
class SARibbonCategory;

namespace DA
{

/**
 * @brief Python工作流Ribbon组构建辅助类
 * 
 * 此类负责在Ribbon中构建Python工作流相关的操作面板，
 * 包括新建、打开、执行和终止Python工作流的Action按钮。
 * 通过DAPyWorkflowActions结构体接收Action指针，避免依赖APP模块。
 * 
 * @see DAPyWorkflowActions DAPyWorkFlowScene DAPyWorkFlowExecuter
 */
class DAGUI_API DAPyWorkFlowRibbonGroup : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyWorkFlowRibbonGroup)
public:
    DAPyWorkFlowRibbonGroup(QObject* parent = nullptr);
    ~DAPyWorkFlowRibbonGroup();

    // 构建Python工作流Ribbon面板，将Action添加到指定的panel中
    void buildPyWorkflowPanel(SARibbonPanel* panel, const DAPyWorkflowActions& actions);

    // 构建Python工作流Ribbon category页（独立的标签页）
    SARibbonCategory* buildPyWorkflowCategory(SARibbonCategory* category, const DAPyWorkflowActions& actions);

    // 更新Action的启用状态（执行时禁用新建/打开/执行，启用终止）
    void setExecutingState(bool isExecuting);

    // 语言变更时更新文本
    void retranslateUi();

private:
    // 初始化信号槽连接
    void initConnections(const DAPyWorkflowActions& actions);
};

}  // namespace DA

#endif  // DAPYWORKFLOWRIBBONGROUP_H