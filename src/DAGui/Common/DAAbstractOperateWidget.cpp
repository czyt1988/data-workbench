#include "Common/DAAbstractOperateWidget.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include "Common/DANoDataGuideWidget.h"
namespace DA {

DAAbstractOperateWidget::DAAbstractOperateWidget(QWidget* par):QWidget(par)
{
}

DAAbstractOperateWidget::~DAAbstractOperateWidget()
{

}

/**
 * @brief 安装「无数据引导页 + 内容页」堆叠容器
 *
 * 子类在创建完自己的主内容（如嵌套停靠区）后调用一次；本函数负责根布局、
 * 引导页与内容页的堆叠，并把引导页的入口信号转成基类信号供宿主接线。
 * @param content 主内容控件（由堆叠容器接管所有权）
 * @param guide 无数据引导页，通常由子类按角色创建，nullptr 时忽略
 */
void DAAbstractOperateWidget::setupNoDataGuide(QWidget* content, DANoDataGuideWidget* guide)
{
	if (nullptr == content || nullptr == guide || nullptr != mOperateStack) {
		return;
	}
	mOperateContent = content;
	mNoDataGuide   = guide;
	mOperateStack  = new QStackedWidget(this);
	mOperateStack->addWidget(mNoDataGuide);
	mOperateStack->addWidget(mOperateContent);
	QVBoxLayout* lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->setSpacing(0);
	lay->addWidget(mOperateStack);
	connect(mNoDataGuide, &DANoDataGuideWidget::openProjectRequested, this, &DAAbstractOperateWidget::openProjectRequested);
	connect(mNoDataGuide, &DANoDataGuideWidget::importDataRequested, this, &DAAbstractOperateWidget::importDataRequested);
	// 初始没有内容，先显示引导页（子类创建出首个内容后再切走）
	mOperateStack->setCurrentWidget(mNoDataGuide);
}

/**
 * @brief 无数据引导页是否可见
 * @return 引导页当前是堆叠容器当前页返回 true
 */
bool DAAbstractOperateWidget::isNoDataGuideVisible() const
{
	return mNoDataGuide && mOperateStack && mOperateStack->currentWidget() == mNoDataGuide;
}

/**
 * @brief 设置无数据引导页显隐
 *
 * 只切堆叠容器当前页，不创建/销毁内容，供子类在内容数量变化时调用。
 * @param visible true 显示引导页，false 显示主内容
 */
void DAAbstractOperateWidget::setNoDataGuideVisible(bool visible)
{
	if (nullptr == mOperateStack || nullptr == mNoDataGuide || nullptr == mOperateContent) {
		return;
	}
	mOperateStack->setCurrentWidget(visible ? static_cast< QWidget* >(mNoDataGuide) : mOperateContent);
}

/**
 * @brief 操作窗口对应的UndoStack
 *
 * @note 操作窗口有可能是tab窗口，包含了很多文档，每个文档会有对应的UndoStack，也有可能当前没有UndoStack，因此此函数可能返回nullptr
 *
 * @ref DAPyWorkFlowOperateWidget 的getUndoStack实现如下
 *
 * @code
 * QUndoStack* DAPyWorkFlowOperateWidget::getUndoStack()
 * {
 *     DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
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
