# DA 工程文件结构说明

xml的总体结构在`DAGui.DAXmlHelper`类中

## 顶层节点`<root>`

工程文件以xml形式保存顶层节点为<root>,节点属性`type`用于标记此xml的类型，目前可选项为：

- project 代表这是工程文件

## 本地信息记录`<local-info>`

`<local-info>`位于`<root>`节点下，用于保存保存文件的本地计算机信息

- `<machineHostName>` 计算机名
- `<cpuArch>` cpu信息
- `<kernelType>` 操作系统类型
- `<kernelVersion>` 操作系统版本
- `<prettyProductName>` 操作系统全称

# DA 截切板数据结构

DA在进行复制粘贴时，通过xml传递复制粘贴内容，这里对不同板块的数据进行说明

图元复制粘贴例子见`DAGui.DAWorkFlowGraphicsView::copyItems`方法

## 图元编辑区的复制粘贴

### 传递信息结构da-xml

图元编辑区进行复制操作，形成一个以`<da-clip>`为顶层节点的xml描述，包含属性`type`,属性`type`分为`copy`和`cut`，以表证此动作是复制还是剪切，完整标签如下：

```xml
<da-clip type="copy">
```

一个简化后的xml文件如下所示，这里有两个节点和一个链接

```xml
<da-clip type="copy">
    <workflow>
        <nodes>
            <node protoType="AirSimulate.DynamicStaticConversion.DynamicToStatic" id="4660777498249756380" name="Dynamic To Static">
                <item className="DA::DAStandardNodeSvgGraphicsItem" tid="67539">
                </item>
            </node>
            <node protoType="AirSimulate.DynamicStaticConversion.StaticToDynamic" id="4661108451249719569" name="Static To Dynamic">
                <item className="DA::DAStandardNodeSvgGraphicsItem" tid="67539">
                </item>
            </node>
        </nodes>
        <links>
            <link>
                <from id="4660777498249756380" name="out"/>
                <to id="4661108451249719569" name="in"/>
                <item className="DA::DAStandardNodeLinkGraphicsItem" tid="66039">
                </item>
            </link>
        </links>
        <items/>
    </workflow>
</da-clip>
```

对于复制，其流程会相对复杂，因为每个节点在xml中是以一个id进行标记，复制过程，id是不能重复，需要重新生成，这样导致链接过程需要新的id进行识别，因此复制过程，旧的节点id和新的节点id的对应关系要记录下来，这样链接就知道要从哪个节点链接向哪个节点

### mime-data

生成的xml通过mimeData传递，类型定义为：`text/da-xml`