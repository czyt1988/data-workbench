#include "DAAbstractOperateWidget.h"
namespace DA {


DAAbstractOperateWidget::DAAbstractOperateWidget(QWidget* par):QWidget(par)
{
}

DAAbstractOperateWidget::~DAAbstractOperateWidget()
{

}

/**
 * @brief 操作窗口对应的UndoStack
 *
 * @note 操作窗口有可能是tab窗口，包含了很多文档，每个文档会有对应的UndoStack，也有可能当前没有UndoStack，因此此函数可能返回nullptr
 *
 * @example @ref DAWorkFlowOperateWidget 的getUndoStack实现如下
 * @code
 * QUndoStack* DAWorkFlowOperateWidget::getUndoStack()
 * {
 *     DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
 *     if (w) {
 *         return w->getUndoStack();
 *     }
 *     return nullptr;
 * }
 * @endcode
 *
 * @return
 */
QUndoStack* DAAbstractOperateWidget::getUndoStack()
{
    return nullptr;
}
}
