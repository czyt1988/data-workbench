#ifndef DAAPPAPPLICATIONWIDGET_H
#define DAAPPAPPLICATIONWIDGET_H
#include "DAGlobals.h"
#include "SARibbonApplicationWidget.h"
class QShowEvent;
class SARibbonMainWindow;
namespace DA
{
class DAAppActions;

/**
 * @brief 应用按钮弹出的应用面板（Office Backstage 式）
 *
 * 左栏为导航（上部内容页签 + 下部立即执行的动作），右侧内容区随页签切换；
 * 宽度占主窗口 2/3、全高贴左伸出，遮罩盖住主窗口剩余区域，
 * 点击遮罩、按 Esc 或再次点击应用按钮均可关闭
 */
class DAAppApplicationWidget : public SARibbonApplicationWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAppApplicationWidget)
public:
    // 构造：parent 为主窗口，actions 提供面板内各入口触发的动作
    DAAppApplicationWidget(SARibbonMainWindow* parent, DAAppActions* actions);
    ~DAAppApplicationWidget() override;
    // 语言变化时刷新全部界面文本
    void retranslateUi();

protected:
    void showEvent(QShowEvent* e) override;
};
}  // namespace DA
#endif  // DAAPPAPPLICATIONWIDGET_H
