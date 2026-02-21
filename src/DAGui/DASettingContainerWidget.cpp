#include "DASettingContainerWidget.h"
#include "DAWorkFlowNodeItemSettingWidget.h"
#if DA_USE_QIM
#else
#include "DAChartSettingWidget.h"
#endif
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DASettingContainerWidget
//===================================================
DASettingContainerWidget::DASettingContainerWidget(QWidget* parent) : QStackedWidget(parent)
{
	// 初始化工作流相关的配置窗口
	initSettingWidgets();
}

DASettingContainerWidget::~DASettingContainerWidget()
{
}

/**
 * @brief 判断当前是否已经有这个窗口
 * @param w
 * @return
 */
bool DASettingContainerWidget::isContainWidget(QWidget* w) const
{
	for (int i = 0; i < count(); ++i) {
		if (w == widget(i)) {
			return true;
		}
	}
	return false;
}

void DASettingContainerWidget::initSettingWidgets()
{
	mWorkFlowNodeItemSettingWidget = new DAWorkFlowNodeItemSettingWidget();
	addWidget(mWorkFlowNodeItemSettingWidget);
#if DA_USE_QIM
#else
	mChartSettingWidget = new DAChartSettingWidget();
	addWidget(mChartSettingWidget);
#endif
}

/**
 * @brief 获取工作量节点设置窗口
 * @return
 */
DAWorkFlowNodeItemSettingWidget* DASettingContainerWidget::getWorkFlowNodeItemSettingWidget()
{
    return mWorkFlowNodeItemSettingWidget;
}

/**
 * @brief 显示工作流节点设置窗口
 */
void DASettingContainerWidget::showWorkFlowNodeItemSettingWidget()
{
    setCurrentWidget(mWorkFlowNodeItemSettingWidget);
}
#if DA_USE_QIM
#else
/**
 * @brief 获取绘图设置窗口
 * @return
 */
DAChartSettingWidget* DASettingContainerWidget::getChartSettingWidget()
{
    return mChartSettingWidget;
}
/**
 * @brief 显示绘图设置窗口
 * @return
 */
void DASettingContainerWidget::showChartSettingWidget()
{
    setCurrentWidget(mChartSettingWidget);
}
#endif
