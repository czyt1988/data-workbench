#ifndef DAMETHODEDITORMAINWINDOW_H
#define DAMETHODEDITORMAINWINDOW_H
#include "SARibbonMainWindow.h"
#include "DANodeListWidget.h"
#include <QScopedPointer>
#include "DAAbstractNodeGraphicsItem.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class AppMainWindow;
}
QT_END_NAMESPACE

namespace DA
{
class DAAppCore;
class DAAppRibbonArea;
class DAAppUI;
class DAAppDockingArea;
class DAAppController;
class DAAppConfig;
class DAConfigsManager;
class DAAppSettingDialog;
class DAAppRibbonApplicationMenu;
class AppMainWindow : public SARibbonMainWindow
{
    Q_OBJECT
    friend class DAAppRibbonArea;

public:
    AppMainWindow(QWidget* parent = nullptr);
    ~AppMainWindow();

    //多语言翻译
    void retranslateUi();

    DAAppConfig* getAppConfig() const;
    //显示设置对话框
    void showSettingDialog();

protected:
    void changeEvent(QEvent* e);

private:
    //初始化
    void init();

    //初始化插件
    void initPlugins();

    //初始化工作流的节点
    void initWorkflowNodes();

    //初始化设置
    void initConfig();

    //创建ribbon app menu
    void createApplicationMenu();
private slots:
    //
    void onWorkflowFinished(bool success);
    //配置文件需要保存
    void onConfigNeedSave();

private:
    DAAppCore* mCore { nullptr };
    DAAppUI* mUI { nullptr };
    DAAppDockingArea* mDockArea { nullptr };
    DAAppController* mController { nullptr };
    std::unique_ptr< DAAppConfig > mConfig;
    DAAppSettingDialog* mSettingDialog { nullptr };  ///< 设置窗口,避免过多的中间传递
    DAAppRibbonApplicationMenu* mApplicationMenu;    ///< ribbon-app menu
};
}  // namespace DA
#endif  // METHODMAINWINDOW_H
