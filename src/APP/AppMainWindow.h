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
class DAAppRibbonArea;
class DAAppUI;
class DAAppDockingArea;
class DAAppController;
class AppMainWindow : public SARibbonMainWindow
{
    Q_OBJECT
    friend class DAAppRibbonArea;

public:
    AppMainWindow(QWidget* parent = nullptr);
    ~AppMainWindow();

    //多语言翻译
    void retranslateUi();

protected:
    void changeEvent(QEvent* e);

private:
    //根据模板生成nodewidget
    void setupNodeListWidget();
    //初始化
    void init();

    //初始化插件
    void initPlugins();

    //初始化工作流的节点
    void initWorkflowNodes();

private slots:
    //
    void onWorkflowFinished(bool success);

private:
    DAAppUI* m_ui;
    DAAppDockingArea* m_dockArea;
    DAAppController* _controller;
};
}  // namespace DA
#endif  // METHODMAINWINDOW_H
