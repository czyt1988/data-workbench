#include "DASettingContainerWidget.h"
#include "DAWorkFlowNodeItemSettingWidget.h"
#include "DAFigureSettingWidget.h"
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
    mWorkFlowNodeItemSettingWidget = new DAWorkFlowNodeItemSettingWidget();
    addWidget(mWorkFlowNodeItemSettingWidget);
    mFigureSettingWidget = new DAFigureSettingWidget();
    addWidget(mFigureSettingWidget);
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

/**
 * @brief 获取绘图设置窗口
 * @return
 */
DAFigureSettingWidget* DASettingContainerWidget::getFigureSettingWidget()
{
    return mFigureSettingWidget;
}

/**
 * @brief 显示绘图设置窗口
 * @return
 */
void DASettingContainerWidget::showFigureSettingWidget()
{
    setCurrentWidget(mFigureSettingWidget);
}
