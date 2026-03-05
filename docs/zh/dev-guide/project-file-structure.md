# DA 工程文件结构说明

## 概述

DAWorkBench 的工程文件采用 **ZIP压缩包** 格式，文件扩展名为 `.dapro`。打开工程文件时，程序会在临时目录解压该压缩包；保存文件时，将临时文件压缩并移动到指定位置。

ZIP 压缩包的管理基于开源库 `QuaZip` 实现，具体实现类为 `DAZipArchive`。

## 工程文件内部结构

`.dapro` 文件解压后的目录结构如下：

```txt
project.dapro (ZIP Archive)
├── system-info.xml           # 本地系统信息
├── workflow.xml              # 工作流信息（节点、连接、图元等）
├── charts.xml                # 绘图信息（图表配置、数据绑定等）
├── data-manager.xml          # 数据管理器配置
├── datas/                    # 数据文件目录
│   └── [各种数据文件]
└── chart-data/               # 图表数据目录
    └── [图表数据文件]
```

## XML 总体结构

xml的总体结构在`DAGui.DAXmlHelper`类中，当前 XML 版本号为 `1.4.0`。

### 顶层节点`<root>`

工程文件以xml形式保存顶层节点为`<root>`,节点属性`type`用于标记此xml的类型，目前可选项为：

- project 代表这是工程文件

## 工程文件保存和加载顺序

工程文件保存过程通过`DAUtils.DAXMLFileInterface`类的`saveToXml`和`loadFromXml`接口，在工程文件保存过程中，各个部件的接口调用顺序决定了工程文件xml的标签层次

> 具体工程的保存业务代码实现在`DAGui.DAXmlHelper`类中

### 保存流程

工程保存时会依次创建以下任务：

1. 保存系统信息（`system-info.xml`）
2. 保存工作流（`workflow.xml`）
3. 保存数据管理器（`data-manager.xml` 及 `datas/` 目录）
4. 保存图表（`charts.xml` 及 `chart-data/` 目录）

工作流的保存过程顺序为：

- workflow信息：`DAWorkFlow::saveExternInfoToXml`
- 节点信息：`DAAbstractNode::saveExternInfoToXml`,`DAAbstractNodeGraphicsItem::saveToXml`
- 连接信息：`DAAbstractNodeLinkGraphicsItem::saveToXml`
- 通用图元信息:`DAGraphicsItem::saveToXml`
- 工厂信息:`DAAbstractNodeFactory::saveExternInfoToXml`
- 视图场景信息

### 加载流程

同理，工作流的加载过程顺序为：

- workflow信息：`DAWorkFlow::loadExternInfoFromXml`
- 节点信息：`DAAbstractNode::loadExternInfoFromXml`,`DAAbstractNodeGraphicsItem::loadFromXml`
- 连接信息：`DAAbstractNodeLinkGraphicsItem::loadFromXml`
- 通用图元信息:`DAGraphicsItem::loadFromXml`
- 工厂信息:`DAAbstractNodeFactory::loadExternInfoFromXml`
- 视图场景信息

工作流加载完成后，会调用工厂的`DAAbstractNodeFactory::workflowReady`回调函数，可以重载这个回调函数执行一些刷新动作

!!! tip "提示"
    工厂的额外信息加载是在节点之后，因此，可以把节点的全局性信息存入工厂的额外信息中，例如有些场景有需求保存一些和节点无关的额外的变量或者参数，而这些参数要在节点都加载完成后再赋值给节点，把这些信息在`DAAbstractNodeFactory::saveExternInfoToXml`中保存，加载时会在`DAAbstractNodeFactory::loadExternInfoFromXml`中加载，这时已经加载完所有节点，可以对节点进一步操作

## Workflow XML 结构示例

```xml
<?xml version="1.0" encoding="utf-8"?>
<root>
  <workflows ver="1.4.0" currentIndex="0">
    <workflow name="Workflow1">
      <extern>
        <!-- 工作流扩展信息 -->
      </extern>
      <nodes>
        <node id="123456" name="Node1" protoType="NodeType">
          <inputs>
            <li name="input1">
              <value>...</value>
            </li>
          </inputs>
          <outputs>
            <li name="output1">
              <data>...</data>
            </li>
          </outputs>
          <props>
            <prop>
              <key>propName</key>
              <value>propValue</value>
            </prop>
          </props>
          <item className="NodeGraphicsItem" tid="12345">
            <!-- 图形项信息 -->
          </item>
        </node>
      </nodes>
      <links>
        <link>
          <from id="123" name="outputKey"/>
          <to id="456" name="inputKey"/>
          <item>
            <!-- 连接线图形信息 -->
          </item>
        </link>
      </links>
      <items>
        <!-- 其他图形元素 -->
      </items>
      <factorys>
        <factory prototypes="FactoryType">
          <extern><!-- 工厂扩展信息 --></extern>
        </factory>
      </factorys>
      <scene x="0" y="0" width="800" height="600">
        <background><!-- 背景图信息 --></background>
      </scene>
    </workflow>
  </workflows>
</root>
```

## Charts XML 结构示例

```xml
<?xml version="1.0" encoding="utf-8"?>
<root>
  <project version="1.0.0">
    <charts>
      <figure id="fig1" pickgroup="1" figure-name="Figure1">
        <background><!-- 背景色 --></background>
        <colortheme style="UserDefine">
          <li>#FF0000</li>
          <li>#00FF00</li>
        </colortheme>
        <charts>
          <chart x="0" y="0" w="400" h="300">
            <!-- 图表配置 -->
          </chart>
        </charts>
      </figure>
    </charts>
  </project>
</root>
```

## 本地信息记录`<local-info>`

`<local-info>`位于`<root>`节点下，用于保存保存文件的本地计算机信息

- `<machineHostName>` 计算机名
- `<cpuArch>` cpu信息
- `<kernelType>` 操作系统类型
- `<kernelVersion>` 操作系统版本
- `<prettyProductName>` 操作系统全称

## 版本兼容性

XML 文件中通过 `ver` 属性标识版本号，当前版本为 `1.4.0`。加载旧版本文件时，程序会根据版本号选择对应的解析逻辑：

| 版本 | 说明 |
|------|------|
| v1.1.0 | 旧版本节点输入输出解析 |
| v1.3.0 | 中间版本 |
| v1.4.0 | 当前版本 |

## 核心类关系

```
DAProjectInterface (接口定义)
    ↓
DAAppProject (具体实现)
    ├── DAZipArchive (ZIP压缩包处理)
    ├── DAXmlHelper (XML序列化/反序列化)
    ├── DAWorkFlowOperateWidget (工作流操作)
    ├── DADataOperateWidget (数据操作)
    └── DAChartOperateWidget (绘图操作)
```

# DA 剪切板数据结构

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
