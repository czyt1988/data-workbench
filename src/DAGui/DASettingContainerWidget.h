#ifndef DASETTINGCONTAINERWIDGET_H
#define DASETTINGCONTAINERWIDGET_H
#include <QStackedWidget>
#include <QList>
#include "DAGuiAPI.h"

namespace DA
{
class DAWorkFlowNodeItemSettingWidget;
#if DA_USE_QIM
#else
class DAChartSettingWidget;
#endif
/**
 * @brief 这是一个类似QStackedWidget的窗体，只内部有一个scallview
 */
class DAGUI_API DASettingContainerWidget : public QStackedWidget
{
    Q_OBJECT

public:
    DASettingContainerWidget(QWidget* parent = nullptr);
    ~DASettingContainerWidget();
    // 判断当前是否已经有这个窗口
    bool isContainWidget(QWidget* w) const;
    // 工作流设置
    DAWorkFlowNodeItemSettingWidget* getWorkFlowNodeItemSettingWidget();
    // 显示默认的工作流节点设置窗口
    void showWorkFlowNodeItemSettingWidget();
#if DA_USE_QIM
#else
    // 绘图设置
    DAChartSettingWidget* getChartSettingWidget();
    // 显示默认的工作流节点设置窗口
    void showChartSettingWidget();
#endif

protected:
    void initSettingWidgets();

private:
    DAWorkFlowNodeItemSettingWidget* mWorkFlowNodeItemSettingWidget { nullptr };
#if DA_USE_QIM
#else
    DAChartSettingWidget* mChartSettingWidget { nullptr };
#endif
};
}  // namespace DA
#endif  // DASETTINGCONTAINERWIDGET_H
