#ifndef DAPYWORKFLOWEDITWIDGETDOCKWIDGET_H
#define DAPYWORKFLOWEDITWIDGETDOCKWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "DAGlobals.h"
namespace DA
{
class DAPyWorkFlowEditWidget;
DA_IMPL_FORWARD_DECL(DAPyWorkFlowEditWidgetDockWidget)
/**
 * @brief 工作流编辑窗口的停靠封装部件
 *
 * DAPyWorkFlowEditWidget 的停靠封装层，继承 QWidget（非 QDockWidget / ads::CDockWidget）。
 * 本项目使用 Qt-Advanced-Docking-System (ADS)，DAPyWorkFlowOperateWidget 内部持有一个
 * 嵌套的 ads::CDockManager，把本 QWidget 包装进 ads::CDockWidget 后加入停靠区。
 * 若继承 QDockWidget / ads::CDockWidget 再交给停靠管理器，会出现双标题栏、
 * float/拖拽冲突。
 *
 * @note 本类继承 QWidget，作为工作流编辑窗口在停靠区的容器层
 */
class DAGUI_API DAPyWorkFlowEditWidgetDockWidget : public QWidget
{
    Q_OBJECT
    DA_IMPL(DAPyWorkFlowEditWidgetDockWidget)
public:
    // 构造，wfe 为被封装的工作流编辑窗口，构造后由本对象接管其布局父级（reparent）
    explicit DAPyWorkFlowEditWidgetDockWidget(DAPyWorkFlowEditWidget* wfe, QWidget* parent = nullptr);
    ~DAPyWorkFlowEditWidgetDockWidget();
    // 获取封装的工作流编辑窗口
    DAPyWorkFlowEditWidget* getEditWidget() const;
};
}  // namespace DA
#endif  // DAPYWORKFLOWEDITWIDGETDOCKWIDGET_H
