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
    //特化
    DAWorkFlowNodeItemSettingWidget* getWorkFlowNodeItemSettingWidget();

protected:
    void initWorkFlowSettingWidgets();

private:
    DAWorkFlowNodeItemSettingWidget* mWorkFlowNodeItemSettingWidget;
};
}  // namespace DA
#endif  // DASETTINGCONTAINERWIDGET_H
