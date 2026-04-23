#include "DAPyWorkFlowRibbonGroup.h"
#include "DAGlobals.h"
// SARibbon
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
// APP
#include "DAAppActions.h"

namespace DA
{

class DAPyWorkFlowRibbonGroup::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlowRibbonGroup)
public:
    PrivateData(DAPyWorkFlowRibbonGroup* p);
    bool mIsExecuting { false };  // 是否正在执行Python工作流
};

DAPyWorkFlowRibbonGroup::PrivateData::PrivateData(DAPyWorkFlowRibbonGroup* p) : q_ptr(p)
{
}

/**
 * @brief DAPyWorkFlowRibbonGroup构造函数
 *
 * @param[in] parent 父对象
 */
DAPyWorkFlowRibbonGroup::DAPyWorkFlowRibbonGroup(QObject* parent)
    : QObject(parent), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief DAPyWorkFlowRibbonGroup析构函数
 */
DAPyWorkFlowRibbonGroup::~DAPyWorkFlowRibbonGroup()
{
}

/**
 * @brief 构建Python工作流Ribbon面板
 *
 * 将Python工作流相关的Action按钮添加到指定的Ribbon面板中：
 * - New Python Workflow (新建Python工作流)
 * - Open Python Workflow (打开Python工作流)
 * - Execute Python Workflow (执行Python工作流)
 * - Terminate Python Workflow (终止Python工作流)
 *
 * @param[in] panel 目标Ribbon面板
 * @param[in] actions Action管理器，用于获取Python workflow相关Action
 */
void DAPyWorkFlowRibbonGroup::buildPyWorkflowPanel(SARibbonPanel* panel, DAAppActions* actions)
{
#if DA_ENABLE_PYTHON
    panel->addLargeAction(actions->actionPyWorkflowNew);
    panel->addLargeAction(actions->actionPyWorkflowOpen);
    panel->addSeparator();
    panel->addLargeAction(actions->actionPyWorkflowExecute);
    panel->addLargeAction(actions->actionPyWorkflowTerminate);
    initConnections(actions);
#endif
}

/**
 * @brief 构建Python工作流Ribbon category页
 *
 * 在指定的category中创建"Python Workflow"面板，
 * 添加新建、打开、执行和终止操作按钮。
 *
 * @param[in] category 目标Ribbon category
 * @param[in] actions Action管理器
 * @return 传入的category指针（便于链式调用）
 */
SARibbonCategory* DAPyWorkFlowRibbonGroup::buildPyWorkflowCategory(SARibbonCategory* category, DAAppActions* actions)
{
#if DA_ENABLE_PYTHON
    SARibbonPanel* pyWorkflowPanel = category->addPanel(QObject::tr("Python Workflow"));  // cn:Python工作流
    pyWorkflowPanel->setObjectName(QStringLiteral("da-pannel-context.workflow.pyworkflow"));
    buildPyWorkflowPanel(pyWorkflowPanel, actions);
#endif
    return category;
}

/**
 * @brief 更新Action的启用状态
 *
 * 执行Python工作流时，禁用新建、打开和执行按钮，启用终止按钮；
 * 非执行状态下，启用新建、打开和执行按钮，禁用终止按钮。
 *
 * @param[in] isExecuting 是否正在执行Python工作流
 */
void DAPyWorkFlowRibbonGroup::setExecutingState(bool isExecuting)
{
    DA_D(d);
    d->mIsExecuting = isExecuting;
    // 状态由DAAppController统一管理，此处仅记录状态
}

/**
 * @brief 语言变更时更新文本
 *
 * 更新Python工作流面板中各Action的显示文本和提示信息。
 * 此函数由DAAppActions::retranslateUi()统一管理，
 * 此处仅提供接口供外部调用。
 */
void DAPyWorkFlowRibbonGroup::retranslateUi()
{
    // 文本翻译由DAAppActions::retranslateUi()统一管理
    // 此处仅预留接口，面板标题由buildPyWorkflowCategory中的tr()处理
}

/**
 * @brief 初始化信号槽连接
 *
 * 将Python工作流Action的triggered信号连接到DAAppController的对应处理槽。
 * 注意：实际的信号槽连接在DAAppController::initialize()中完成，
 * 此处仅预留扩展接口。
 *
 * @param[in] actions Action管理器
 */
void DAPyWorkFlowRibbonGroup::initConnections(DAAppActions* actions)
{
    // 信号槽连接由DAAppController::initConnection()统一管理
    // 此处仅预留接口，无需在此处额外连接
}

}  // namespace DA