#ifndef DASETTINGCONTAINERWIDGET_H
#define DASETTINGCONTAINERWIDGET_H
#include <QStackedWidget>
#include <QList>
#include "DAGuiAPI.h"

namespace DA
{
class DAWorkFlowNodeItemSettingWidget;

/**
 * @brief 这是一个类似QStackedWidget的窗体，只内部有一个scallview
 */
class DAGUI_API DASettingContainerWidget : public QStackedWidget
{
    Q_OBJECT

public:
    DASettingContainerWidget(QWidget* parent = nullptr);
    ~DASettingContainerWidget();
    //判断当前是否已经有这个窗口
    bool isContainWidget(QWidget* w) const;
    //特化
    DAWorkFlowNodeItemSettingWidget* getWorkFlowNodeItemSettingWidget();
    //显示默认的工作流节点设置窗口
    void showWorkFlowNodeItemSettingWidget();

protected:
    void initWorkFlowSettingWidgets();

private:
    DAWorkFlowNodeItemSettingWidget* mWorkFlowNodeItemSettingWidget;
};
}  // namespace DA
#endif  // DASETTINGCONTAINERWIDGET_H
