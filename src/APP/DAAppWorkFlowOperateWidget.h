#ifndef DAAPPWORKFLOWOPERATEWIDGET_H
#define DAAPPWORKFLOWOPERATEWIDGET_H
#include "DAWorkFlowOperateWidget.h"
namespace DA
{
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
};
}

#endif  // DAAPPWORKFLOWOPERATEWIDGET_H
