# DAWorkBench的action添加过程

这里介绍DAWorkBench的主程序中添加action的过程

> [!NOTE]这里不是针对插件的action添加，是主程序的action添加过程，如果参与主程序的开发，请阅读此文

DAWorkBench的所有内置action位于`APP/DAAppActions`类中，`DAAppActions`负责整个程序所有action的管理

界面构建位于`APP/DAAppRibbonArea`类中，action如果要呈现到界面上，你需要在此类中操作

信号槽动作通过`APP/DAAppController`类实现，action的槽函数都在这里绑定

因此，一个action的添加，你需要在这三个类中操作：

1. 添加action到`APP/DAAppActions`类中
2. 在`APP/DAAppRibbonArea`类中构建action对应的界面
3. 在`APP/DAAppController`类中绑定action的槽函数

## 例子

这里以添加一个action为例，例如我要实现打开这个功能

1. `APP/DAAppActions`类的头文件中定义action

```cpp
QAction* actionOpen; ///< 打开
```

2. `APP/DAAppActions`类的源文件中初始化action

```cpp
void DAAppActions::buildMainAction()
{
    ...
    actionOpen  = createAction("actionOpen", ":/app/bright/Icon/file.svg");
    ...
}

void DAAppActions::retranslateUi()
{
    ...
	actionOpen->setText(tr("Open"));                     // cn:打开
	actionOpen->setToolTip(tr("Open file or project"));  // cn:打开文件或项目
    ...
}
```

> [!NOTE] action所有文字相关的操作都放到retranslateUi()函数中，这样可以根据语言进行翻译

3. 在`APP/DAAppRibbonArea`类的源文件中构建action对应的界面

```cpp
/**
 * @brief 构建主页标签
 * 主页的category objname = da-ribbon-category-main
 */
void DAAppRibbonArea::buildRibbonMainCategory(){
    ...
    m_pannelMainFileOpt = new SARibbonPannel(m_categoryMain);
	m_pannelMainFileOpt->setObjectName(QStringLiteral("da-ribbon-pannel-main.common"));
	m_pannelMainFileOpt->addLargeAction(m_actions->actionOpen);
    ...
}
```

4. 在`APP/DAAppController`类中绑定action的槽函数

APP/DAAppController.h:

```cpp
/**
 * @brief 控制层负责逻辑的对接
 */
class DAAppController : public QObject
{
	Q_OBJECT
    ...
public Q_SLOTS:
	// 打开文件
	void open();
};
```

APP/DAAppController.cpp:

```cpp
/**
 * @brief 基本绑定
 * @note 在setDockAreaInterface函数中还有很多绑定操作
 */
void DAAppController::initConnection()
{
	...
    connect(mActions->actionOpen, &QAction::triggered, this, &DAAppController::open)
    ...
}
```