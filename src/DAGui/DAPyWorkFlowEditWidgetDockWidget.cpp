#include "DAPyWorkFlowEditWidgetDockWidget.h"
// Qt
#include <QVBoxLayout>
// workflow
#include "DAPyWorkFlowEditWidget.h"
namespace DA
{
//==============================================================
// DAPyWorkFlowEditWidgetDockWidgetPrivate
//==============================================================
class DAPyWorkFlowEditWidgetDockWidgetPrivate
{
    DA_IMPL_PUBLIC(DAPyWorkFlowEditWidgetDockWidget)
public:
    DAPyWorkFlowEditWidgetDockWidgetPrivate(DAPyWorkFlowEditWidgetDockWidget* p);

public:
    DAPyWorkFlowEditWidget* mEditWidget { nullptr };  ///< 被封装的工作流编辑窗口
};
DAPyWorkFlowEditWidgetDockWidgetPrivate::DAPyWorkFlowEditWidgetDockWidgetPrivate(DAPyWorkFlowEditWidgetDockWidget* p)
    : q_ptr(p)
{
}

//===================================================
// DAPyWorkFlowEditWidgetDockWidget
//===================================================
/**
 * @brief 构造
 *
 * 把传入的 DAPyWorkFlowEditWidget 通过布局接管其父级（reparent 到本部件），
 * 供后续 ads::CDockWidget 包装
 * @param wfe 被封装的工作流编辑窗口，允许为空（此时不放置任何内容）
 * @param parent 父部件
 */
DAPyWorkFlowEditWidgetDockWidget::DAPyWorkFlowEditWidgetDockWidget(DAPyWorkFlowEditWidget* wfe, QWidget* parent)
    : QWidget(parent), d_ptr(new DAPyWorkFlowEditWidgetDockWidgetPrivate(this))
{
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    if (wfe) {
        d_ptr->mEditWidget = wfe;
        lay->addWidget(wfe);  // reparent wfe to this widget
    }
}

DAPyWorkFlowEditWidgetDockWidget::~DAPyWorkFlowEditWidgetDockWidget()
{
}

/**
 * @brief 获取封装的工作流编辑窗口
 * @return
 */
DAPyWorkFlowEditWidget* DAPyWorkFlowEditWidgetDockWidget::getEditWidget() const
{
    return d_ptr->mEditWidget;
}

}  // namespace DA
