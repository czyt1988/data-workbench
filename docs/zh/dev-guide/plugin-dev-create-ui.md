# 插件界面开发

插件的入口函数是`Plugin::initialize`，在调用此函数时，主程序的界面已经创建完成，此时可以进行插件界面的初始化。

## 获取界面的主要组件

插件的核心函数是`core()`函数，此函数返回核心对象，`data-workbench`的所有可开放对象都可以通过`core()`获取

其中ui相关的接口对象通过`DACoreInterface::getUiInterface`函数获取，此函数返回`DAUIInterface`对象

`DAUIInterface`对象主要有4个关键函数：

```cpp
//获取界面的docking区域
virtual DADockingAreaInterface* getDockingArea() = 0;
//获取界面的ribbon区域
virtual DARibbonAreaInterface* getRibbonArea() = 0;
//获取action管理器
DAActionsInterface* getActionInterface() const;
//获取命令接口，如果没有注册命令接口，返回nullptr，当前设计为一个命令接口
DACommandInterface* getCommandInterface() const;
```

这四个函数获取各自对应的管理对象，其中：
1. `getDockingArea()` 获取DockingArea，这个区域是界面的docking区域，里面可以放置和隐藏docking控件。
2. `getRibbonArea()` 获取界面的ribbon区域，这个区域是界面的ribbon区域，里面可以放置ribbon控件。
3. `getActionInterface()` 获取action管理器，这个管理器管理界面中的所有action，你可以通过这个管理器来创建action，并添加到界面中。
4. `getCommandInterface()` 获取命令接口，这个接口管理界面中的所有命令，你可以通过这个接口来创建命令，并添加到界面中。

## 插件操作界面

### 隐藏窗口

下面例子演示如何隐藏工作流相关的界面

```cpp
void MyPlugin::initialize()
{
    DA::DAUIInterface* ui   = core()->getUiInterface();

    // 通过这里可以隐藏掉不要的dock
    DA::DADockingAreaInterface* dockArea    = ui->getDockingArea();
    DA::DAActionsInterface* actionInterface = ui->getActionInterface();
    DA::DARibbonAreaInterface* ribbonArea   = ui->getRibbonArea();
    //! dock界面的隐藏
    //! 隐藏workflow相关的界面
    dockArea->hideDockWidget(dockArea->getWorkFlowOperateWidget());
    dockArea->hideDockWidget(dockArea->getWorkflowNodeListWidget());
    // 通过这里隐藏掉不需要的ribbon
    // 1、隐藏主页的工作流相关内容
    // 隐藏主页workflow相关
    safeHideRibbonPanel(ribbonArea, "da-pannel-main.workflow");
}
```