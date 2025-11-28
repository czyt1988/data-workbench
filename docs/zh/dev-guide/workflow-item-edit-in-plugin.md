# 节点操作

工作流的对象是节点(`DAAbstractNode`)，但节点(`DAAbstractNode`)是抽象的，而要在画布上呈现，则通过节点图元(`DAAbstractNodeGraphicsItem`)来呈现

`DAWorkFlow`是用来管理节点的工作流，它管理所有的节点(`DAAbstractNode`)，`DAWorkFlow`不负责显示，只用于维护节点图结构

`DANodeGraphicsScene`是渲染节点的场景，`DANodeGraphicsScene`负责管理节点图元(`DAAbstractNodeGraphicsItem`)的显示工作

也就是说，`DAWorkFlow`和`DAAbstractNode`属于`model`层，`DANodeGraphicsScene`属于`view`层

节点都是由插件提供，下面将介绍如何在插件中操作节点，同时能保证正常的功能，例如redo/undo等功能

## 插件操作节点

插件里可以直接操作主程序的场景和工作流，执行自定义的动作，下面介绍如何在插件开发过程中操作节点和场景

### 获取场景

所有插件都会有`core()`接口，此接口返回`DACoreInterface`，通过`DACoreInterface::getUiInterface`可获取`DAUIInterface`对象

`DAUIInterface`负责界面的所有工作，通过`DAUIInterface::getDockingArea`可获取`DADockingAreaInterface`，场景窗口位于Docking区域

`DADockingAreaInterface`提供了`getCurrentScene`函数，用于获取当前的场景，通过此函数即可得到当前`DAWorkFlowGraphicsScene`指针

在插件获取场景的代码如下：

```cpp
DAWorkFlowGraphicsScene* sc = core()->getUiInterface()->getDockingArea()->getCurrentScene();
```

场景是用于显示的基础，后续节点元素的添加都离不开此`DAWorkFlowGraphicsScene`

### 节点的创建和显示

#### 节点的创建

节点的创建可以直接在插件里`new`出来或者通过插件工厂的`create`方法

!!! tips "建议"
    这里推荐使用插件工厂的`create`方法创建节点，原因是一个合理的`create`方法里面会调用`initializNode`,此函数会让节点记录创建它的工厂

创建节点后，需要把节点添加到工作流中，否则节点会处于孤立状态，通过`DAWorkFlow::addNode`函数把节点添加到工作流中。

!!! tips "说明"
    你也可以先不添加到工作流中，通过节点生成图元后，调用`DAWorkFlowGraphicsScene`的`addNodeItem/addNodeItem_`同步添加图元和节点

#### 图元的创建

此时节点即可创建完成，但此时不会在场景中显示，节点的显示需要调用`DAAbstractNode::createGraphicsItem`方法，这个是一个接口函数，实际会调用具体实例化节点的`createGraphicsItem`函数，此函数会创建节点对应的`GraphicsItem`它的基类是`DAAbstractNodeGraphicsItem`，负责节点的渲染

图元创建后，需要把它添加到场景中，这里需要注意的是，`DAWorkFlowGraphicsScene`提供了带`redo/undo`的图元添加方法，如果你需要支持回退功能，你应该使用带`redo/undo`的图元添加方法，在`data-workflow`的命名规则中，函数后带`_`代表此方法支持`redo/undo`功能

例如，`DAWorkFlowGraphicsScene`提供了`addItem`和`addItem_`两个方法，以及`addNodeItem`和`addNodeItem_`两个方法，其中`addItem_`和`addNodeItem_`是带`redo/undo`功能支持的，`addItem`和`addNodeItem`是不支持`redo/undo`功能的

这里使用`DAWorkFlowGraphicsScene::addItem_`则可以把图元添加到场景中，并支持`redo/undo`功能

!!! wanrning "注意"
    `addItem/addItem_`和`addNodeItem/addNodeItem_`的区别是，`addItem/addItem_`只添加图元，而`addNodeItem/addNodeItem_`添加的是节点和图元，如果节点创建后，没有调用`DAWorkFlow::addNode`把节点添加到工作流中，可以使用`addNodeItem/addNodeItem_`添加图元同步把节点添加到工作流中

#### 创建连接

节点的连接可通过`DAAbstractNode::linkTo`函数执行节点的连接，但此函数只有逻辑层的连接，要连接后能渲染，你应该直接操作`view`层更适合

在获取`DAAbstractNodeGraphicsItem`后，调用`DAAbstractNodeGraphicsItem::linkTo`或`DAAbstractNodeGraphicsItem::linkToByName`都可以连接到对应图元，且工作流中的节点也会调用`linkTo`建立好连接

!!! tips "注意"
    在大部分情况下，你都不需要直接调用`DAAbstractNode::linkTo`函数

`DAAbstractNodeGraphicsItem::linkTo`/`DAAbstractNodeGraphicsItem::linkToByName`会返回`DAAbstractNodeLinkGraphicsItem`指针，`DAAbstractNodeLinkGraphicsItem`主要负责连接线的渲染，此时`DAAbstractNodeLinkGraphicsItem`还没添加到场景中，还需执行`DAWorkFlowGraphicsScene`的`addItem/addItem_`来添加连接线

连接线不会立即刷新，需要调用`DAAbstractNodeGraphicsItem`的`updateLinkItems`函数来刷新连接线的位置

#### 命令打包

有些情况，一个过程可能涉及多个命令，例如先创建多个节点，把节点连接，这个过程在程序中是多个步骤，但对用户应该执行一次回退操作，这种情况可以使用`beginMacro`和`endMacro`来创建一个命令组，这样当用户点击回退按钮时，就会执行宏中的所有命令。

`beginMacro`和`endMacro`属于QUndoStack函数，通过`DAWorkFlowGraphicsScene::undoStack`即可获取此场景的回退栈

```cpp
DAWorkFlowGraphicsScene* sc = core()->getUiInterface()->getDockingArea()->getCurrentScene();
...
sc->undoStack()->beginMacro("create workflow");
// 创建节点
// 连接节点
...
sc->undoStack()->endMacro();
```

#### 示例

下面是一个具体的示例

```cpp
//这里演示了把当前选中的一个连接线打断，中间插入5个节点的例子
void MyNodePlugin::createWorkflow()
{
    DA::DAWorkFlowGraphicsScene* sc = core()->getUiInterface()->getDockingArea()->getCurrentScene();
    if (!sc) {
        return;
    }
     // 找到当前选中的item
    const QList< QGraphicsItem* > selItems = sc->selectedItems();
    MyLinkGraphicsItem* mylinkItem = nullptr;
    for (auto item : selItems) {
        if (MyLinkGraphicsItem* p = dynamic_cast< MyLinkGraphicsItem* >(item)) {
            if (mylinkItem) {
                // 说明之前已经找到，这是第二个
                QMessageBox::warning(getMainWindow(), QString(u8"警告"), QString(u8"请选中一条连接线，不支持多个连接线"));
                return;
            }
            mylinkItem = p;
        }
    }
    if (!mylinkItem) {
        QMessageBox::warning(getMainWindow(), QString(u8"警告"), QString(u8"请选中一条连接线"));
        return;
    }
    // 找到原来直线连接的前后两个节点
    auto fromNode = std::dynamic_pointer_cast< MyNode >(mylinkItem->fromNode());
    auto toNode   = std::dynamic_pointer_cast< MyNode >(mylinkItem->toNode());
    auto fromItem = dynamic_cast< MyNodeGraphicsItem* >(mylinkItem->fromNodeItem());
    auto toItem   = dynamic_cast< MyNodeGraphicsItem* >(mylinkItem->toNodeItem());
    if (!fromNode || !toNode || !fromItem || !toItem) {
        qCritical() << QString(u8"异常");
        return;
    }
    //用于记录要最后调用updateLinkItems更新的节点item
    QList< DA::DAAbstractNodeGraphicsItem* > needUpdateLinkItems;
    needUpdateLinkItems << fromItem;

    sc->undoStack().beginMacro("break link line");
    // 删除直线
    sc->removeItem_(mylinkItem);
    // 计算要添加激励的位置
    MyNodeGraphicsItem* lastAddItem { nullptr };  // 记录上次添加的item
    
    for (int i = 0; i < 5; ++i) {
        QPointF pos = 这里是你要添加的图元的位置;
        // 创建节点
        std::shared_ptr< MyNode > newNode = std::static_pointer_cast< MyNode >(
            create(DA::DANodeMetaData(QString("My.Node"), QString(u8"My Group"))));
        if (!newNode) {
            qCritical() << QString(u8"无法创建节点");
            sc->undoStack().endMacro();
            // 恢复之前直线的删除
            sc->undoStack().redo();
            return;
        }
        // 创建节点
        auto newNodeItem = static_cast< MyNodeGraphicsItem* >(newNode->createGraphicsItem());
        //addNodeItem_会同步添加到workflow
        sc->addNodeItem_(newNodeItem);
        newNodeItem->setPos(pos);
        needUpdateLinkItems << newNodeItem;
        // 进行连线
        if (i == 0) {
            // 第一个要让from连接到当前
            // Mynode,添加了两个通用的输入输出
            auto linkItem = fromItem->linkToByName("out", newNodeItem, "in");
            sc->addItem_(linkItem);
        }
        // 如果有存在lastAddItem，则要让lastAddItem的输出连接到当前的输入
        if (lastAddItem) {
            auto linkItem = lastAddItem->linkToByName("out", newNodeItem, "in");
            sc->addItem_(linkItem);
        }
        // 更新last
        lastAddItem = newNodeItem;
    }
    // 最后，要让lastAddItem连接到toItem
    if (lastAddItem) {
        auto linkItem = lastAddItem->linkToByName("out", toItem, "in");
        sc->addItem_(linkItem);
    }
    needUpdateLinkItems << toItem;
    //一次全部刷新所有连接线的位置和渲染
    for (DA::DAAbstractNodeGraphicsItem* willUpdateItem : qAsConst(needUpdateLinkItems)) {
        willUpdateItem->updateLinkItems();
    }
    sc->undoStack().endMacro();
}
```