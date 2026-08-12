#include "DASettingContainerWidget.h"
#include "DAPyWorkFlowNodeItemSettingWidget.h"
#include "Chart/DAChartSettingWidget.h"
#include "Chart3DSetting/DAChart3DSettingWidget.h"
#include "ChartSetting/DAFigureWidgetSettingPanel.h"
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
	initWorkFlowSettingWidgets();
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

void DASettingContainerWidget::initWorkFlowSettingWidgets()
{
	mWorkFlowNodeItemSettingWidget = new DAPyWorkFlowNodeItemSettingWidget();
	addWidget(mWorkFlowNodeItemSettingWidget);
	mChartSettingWidget = new DAChartSettingWidget();
	addWidget(mChartSettingWidget);
	mChart3DSettingWidget = new DAChart3DSettingWidget();
	addWidget(mChart3DSettingWidget);
	mFigureWidgetSettingWidget = new DAFigureWidgetSettingPanel();
	addWidget(mFigureWidgetSettingWidget);
}

/**
 * @brief 获取工作量节点设置窗口
 * @return
 */
DAPyWorkFlowNodeItemSettingWidget* DASettingContainerWidget::getWorkFlowNodeItemSettingWidget()
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

/**
 * @brief 获取3D绘图设置窗口
 * @return
 */
DAChart3DSettingWidget* DASettingContainerWidget::getChart3DSettingWidget()
{
    return mChart3DSettingWidget;
}

/**
 * @brief 显示3D绘图设置窗口
 */
void DASettingContainerWidget::showChart3DSettingWidget()
{
    setCurrentWidget(mChart3DSettingWidget);
}

/**
 * @brief 获取 Figure 设置窗口
 * @return
 */
DAFigureWidgetSettingPanel* DASettingContainerWidget::getFigureWidgetSettingWidget()
{
    return mFigureWidgetSettingWidget;
}

/**
 * @brief 显示 Figure 设置窗口
 */
void DASettingContainerWidget::showFigureWidgetSettingWidget()
{
    setCurrentWidget(mFigureWidgetSettingWidget);
}
