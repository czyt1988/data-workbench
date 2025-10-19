# 开发规范

## 命名规则

### 总体原则

- **全局变量、全局函数、宏、静态变量**：使用下划线分隔的命名风格。
- **其余所有标识符**（类、函数、局部变量、成员变量、参数等）：统一采用**驼峰命名法**（CamelCase）。
- 命名应遵循 **IDE 最优搜索原则**，便于开发者在输入时快速联想并定位目标符号（如类、函数、枚举等）。

### 具体规范

| 类型 | 命名格式 | 示例 |
|------|--------|------|
| 全局变量 | `g_` + 下划线命名 | `g_total_count` |
| 静态变量（函数内或类内） | `s_` + 下划线命名 | `s_exec_count` |
| 全局函数 | 全小写 + 下划线分隔 | `this_is_global_function()` |
| 宏 | 全大写 + 下划线分隔 | `MAX_BUFFER_SIZE` |
| 类名 | 首字母大写的驼峰命名 | `ExampleClass` |
| 枚举 | 首字母大写的驼峰命名，枚举应以统一前缀开头 | `DockingAreaWorkFlowOperate` |
| 公有成员变量 | 小写开头的驼峰命名，**无前缀** | `publicMemberValue` |
| 私有成员变量 | `m` + 小写开头的驼峰命名（**不加下划线**） | `mResultValue` |
| 成员函数 / 局部变量 / 参数 | 小写开头的驼峰命名 | `functionOne`, `argOne`, `loopCount` |
| 命名空间 | 简洁、全大写缩写（避免过长） | `DA`（而非 `DataAnalysis`） |

!!! tips "说明"
    谷歌规范推荐私有成员变量使用后缀下划线（如 `value_`），虽可避免与局部变量混淆，但不利于 IDE 搜索。而传统 `m_` 前缀在 Pimpl 模式下显得冗余（如 `d_ptr->m_value`）
    本规范采用 **`m` 前缀 + 驼峰**（如 `mValue`），兼顾简洁性与 IDE 提示体验：在 Pimpl 中写作 `d_ptr->mValue` 更美观，且输入 `d_ptr->m` 即可触发 IDE 自动补全


示例代码：

```cpp
int g_total_count;//全局变量以g开头，下划线命名

class ExampleClass{//类名以驼峰命名，且首字母大写
public:
    ExampleClass();
    void functionOne(int argOne,double argTwo);//类函数首字母小写，驼峰命名 ;变量参数驼峰命名，首字母小写
    virtual void virtualFunction();
public:
    double publicMemberValue;//类公有成员变量不做前缀限定
private:
    double mResultValue;//类私有成员变量以m开头，驼峰命名
};

void ExampleClass::functionOne(int argOne,double argTwo){
    static int s_exec_count = 0;//静态参数s_开头，下划线命名
    int loopCount = argOne;// 局部变量小写开头，驼峰命名
}

void this_is_global_fucntion();//全局函数以下划线分隔
void this_is_global_fucntion(){
    int loopCount = 0;//局部变量小写开头，驼峰命名
}

/**
 * @brief 定义了固定的dock窗口
 * 
 * 下面枚举的命名，使用了以IDE最优搜索原则，开发者输入Dock，即可联想到所有的枚举，
 * 如果命名为WorkFlowOperateDockingArea，DataOperateDockingArea这样不利于IDE列举出枚举
 */
enum DockingArea
{
    DockingAreaWorkFlowOperate,
    DockingAreaDataOperate,
    DockingAreaChartOperate,
    DockingAreaWorkFlowManager,
    DockingAreaDataManager,
    DockingAreaChartManager,
    DockingAreaSetting,
    DockingAreaMessageLog
};
```

!!! tips "说明"
    `QStyleOption*`系列就是比较好的例子，所有相似功能的类都是以`QStyleOption`打头，例如`QStyleOptionToolButton`，而不是命名为`QToolButtonStyleOption`

## 函数命名规范

### 属性命名规

- **普通属性**：使用 `setXxx()` / `getXxx()` 配对。**不要省略 `get`**，以利于 IDE 搜索。
- **返回引用的属性**：**不加 `get`**，直接命名为 `xxx()`。
- **布尔属性**：
  - 可设置性（capability）：`setXxxable(bool)`，如 `setSelectable(bool)`；
  - 当前状态（state）：`isSelected()` / `setSelected(bool)`；
  - 读取函数统一以 `is` 开头。
- **统一使用 `set` 前缀**，使 IDE 能通过输入 `set` 联想所有可配置项。

总之尽量使用`set`，让编译器能通过`set`联想出所有可设置属性，用`get`和`is`能联想出所有可读属性

```cpp
class ExampleClass{
public:
    void setText(const std::string& v);//设置值以set开头
    std::string getText() const;//获取值以get开头，且返回值
    //以下是返回引用
    std::string& text();//返回引用的不写get
    const std::string& text() const;//返回引用的不写get
    //能否被选中
    void enableSelect(bool on);//可以但不推荐，某些场合下enable更好理解可以使用
    void setSelectable(bool on);//推荐
    bool isSelectable() const;
    //是否选中了
    void setSelect(bool on);
    bool isSelected() const;
}
```

### 虚函数重写

- 重写虚函数时，**必须**使用 `override`；
- **建议保留 `virtual` 关键字**，以增强可读性，明确其为虚函数。

```cpp
class ChildClass : public ExampleClass{
public:
    ChildClass();
    virtual void virtualFunction() override;//虚函数后面接override，为了更好发现，前面的virtual关键字不省略
};
```

## 注释规范

遵循 **Doxygen** 注释风格

- **头文件中**：
  - 类成员函数**不写完整 Doxygen 注释**（避免因文档变更触发全量构建）；
  - 但需保留**简短普通注释**，内容等同于 `@brief`；
  - `get`/`set` 配对函数可共用一条注释；
  - 针对Qt的**信号（signals）** 没有 `.cpp` 实现，可以在头文件中写详细的 Doxygen 注释。
- **源文件（.cpp）中**：
  - 所有函数实现前写完整 Doxygen 注释；
  - 注释前**空一行**；
  - 使用 `/** ... */` 格式；
  - 成员变量注释使用 `///<`。

示例：（`//!`为示例的说明）

h文件注释示范

```cpp
class DAAbstractNode{
    /////////////示例的说明////////////////
    //! 头文件中不写doxygen注释
    //! get/set配对的可以写一个注释
    //////////////////////////////////////


    //获取图标，图标是节点对应的图标
    QIcon getIcon() const;
    void setIcon(const QIcon& icon);
    //获取节点的元数据
    const DANodeMetaData& metaData() const;
    DANodeMetaData& metaData();
    //说明
    QString getNodeTooltip() const;
    void setNodeTooltip(const QString& tp);
    //设置元数据
    void setMetaData(const DANodeMetaData& metadata);
    //返回自身智能指针
    SharedPointer pointer();
    // id操作
    IdType getID() const;
    void setID(const IdType& d);
};

class DAWorkFlow{
    /////////////示例的说明////////////////
    //! 信号没有cpp的实现，可以把doxygen注释写在头文件中
    //////////////////////////////////////
signals:
    /**
     * @brief 节点创建产生的信号
     * @param node
     */
    void nodeCreated(DAAbstractNode::SharedPointer node);
    /**
     * @brief 节点添加的信号
     * @param node
     */
    void nodeAdded(DAAbstractNode::SharedPointer node);
};
```

cpp文件注释示范

```cpp

/**
 * @brief 获取节点的说明
 * @return 返回说明字符串
 * @sa setNodeTooltip
 */
QString DAAbstractNode::getNodeTooltip() const
{
    return (d_ptr->_meta.getNodeTooltip());
}
            <--注意doxygen注释前空一行
/**
 * @brief 设置节点的说明
 * @param tp 说明文本
 * @sa getNodeTooltip
 */
void DAAbstractNode::setNodeTooltip(const QString& tp)
{
    d_ptr->_meta.setNodeTooltip(tp);
}
```

## 调试与功能开关

- 对于大段功能的开关，**禁止使用注释屏蔽代码**
- 应使用 **宏开关** 控制调试输出或功能启用。

示例：

```cpp
/**
 * @def 此为1将开启打印辅助信息
 */
#define DA_DAABSTRACTNODE_DEBUG_PRINT 0

int DAAbstractNode::getInputNodesCount() const
{
    int res = 0;
#if DA_DAABSTRACTNODE_DEBUG_PRINT
    qDebug() << getNodeName()
             << "-> getInputNodesCount()\n"
                "    _linksInfo:";
#endif
    for (const DAAbstractNodePrivate::LinkData& d : qAsConst(d_ptr->_linksInfo)) {
#if DA_DAABSTRACTNODE_DEBUG_PRINT
        qDebug() << "    outputKey=" << d.outputKey << "(" << d.outputNode.lock()->getNodeName()
                 << ")--->inputKey=" << d.inputKey << "(" << d.inputNode.lock()->getNodeName() << ")";
#endif
        SharedPointer toNode = d.inputNode.lock();
        if (toNode.get() == this) {
            //说明这个是其他节点连接到自身的item
            ++res;
        }
    }
    return res;
}
```

包括功能的调整，禁止使用注释而应该使用宏

## 编码禁止列表

- ❌ 禁止头文件中使用`using namespace`
- ❌ 基类若用于继承，**析构函数必须为 `virtual`**
- ❌ **库导出类禁止继承非模板的 STL 或 Qt 模板类**（如 `class DLLEXPORT X : public QList<QColor>`），因可能导致符号冲突或析构不完整。若需继承模板类，自身也应为模板类。


## 语法规范性条例

下面这些内容是涉及一些语法相关的规范和条例，通用规范条例常见effective cpp和effective modern cpp

### 用异常而不是用std::optional

早期c++规范都不推荐使用异常，例如谷歌的条款，而是推荐使用错误码，现代C++中，更为推荐使用异常，主要有如下原因：

- 异常可携带更多的信息，甚至结合Qt的多语言框架，能直接进行多语言的异常信息携带
- `std::optional`在某些场合能替代异常，但是有很大的缺陷，首先无法携带信息，除非用一个类似`getLastErrorString`的函数来获取，实际工程上大规模使用并不友好
- 宁愿用错误码也不应该用`std::optional`，在和界面相关的编程中，错误码至少能进行更详细的错误信息描述
- 如果使用了异常，开发过程要注意处理，尤其和一些不使用异常的库混用的时候，例如Qt
- 一些科学计算类、io操作类的库建议使用异常

### 操作符重载

操作符重载函数**必须定义在所属类的命名空间内**，以支持 ADL（Argument-Dependent Lookup）。

✅ 如下是**正确**的做法：

```cpp
namespace DA
{
class DANodeLinkPoint
{
}

bool operator==(const DANodeLinkPoint& a, const DANodeLinkPoint& b);
}// end namespace DA
```

❌ 如下是**错误**的做法：

```cpp
namespace DA
{
class DANodeLinkPoint
{
}
}// end namespace DA
//操作符重载不应该定义在命名空间以外
bool operator==(const DA::DANodeLinkPoint& a, const DA::DANodeLinkPoint& b);
```

## Qt编程注意事项

### 不同屏幕缩放比例下QPixmap绘制问题

在高 DPI 或缩放比例 ≠ 100% 的环境下，`QPixmap` 的物理尺寸与逻辑尺寸不一致。  
**必须除以 `devicePixelRatio()` 获取逻辑尺寸**，否则绘制位置会偏移。

```cpp
QPixmap mPixmap;
QIcon mIcon;

...

mPixmap = mIcon.pixmap(getBodySize().toSize());
```

在paintEvent里绘制,这里演示是要把pixmap绘制到bodyRect的中心，下面是错误的绘制方法

❌ 如下是**错误**的做法：

```cpp
void xxx::paintBody(QPainter* painter,
                    const QStyleOptionGraphicsItem* option,
                    QWidget* widget,
                    const QRectF& bodyRect)
{
    painter->save();
    QRect pixmapRect = mPixmap.rect();
    // 计算位图绘制的起始点
    int xoffset = bodyRect.x() + (bodyRect.width() - pixmapRect.width()) / 2;
    int yoffset = bodyRect.y() + (bodyRect.height() - pixmapRect.height()) / 2;
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawPixmap(QPoint(xoffset, yoffset), mPixmap);
    painter->restore();
    ...
}
```

上面方法之所以错误，是因为没有考虑devicePixelRatio，在缩放比例为100%时，上面代码绘制的图片没有问题，但缩放比例不是100%，上面的代码绘制出来是不正确的位置，主要异常在xoffset和yoffset，因为pixmapRect.width()/height()并非QPixmap在当前绘图坐标系下的宽高，这时两个坐标系是不一致的，你要获取当前绘图坐标系下的宽高，需要除以devicePixelRatio

✅ 如下是**正确**的做法：

```cpp
void xxx::paintBody(QPainter* painter,
                    const QStyleOptionGraphicsItem* option,
                    QWidget* widget,
                    const QRectF& bodyRect)
{
    painter->save();
    QRect pixmapRect = mPixmap.rect();
    qreal dr         = mPixmap.devicePixelRatio();
    // 注意在不同屏幕缩放比例下，pixmap的实际尺寸要除以devicePixelRatio
    int xoffset = bodyRect.x() + (bodyRect.width() - pixmapRect.width() / dr) / 2;
    int yoffset = bodyRect.y() + (bodyRect.height() - pixmapRect.height() / dr) / 2;
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawPixmap(QPoint(xoffset, yoffset), mPixmap);
    painter->restore();
    ...
}
```

### 永远不要在线程中直接操作GUI控件

- **严禁在线程中直接操作 GUI 控件**；
- 必须通过 **信号槽** 或 `QMetaObject::invokeMethod()` 切回主线程执行。

例如一个窗口，用QtConcurrent来执行一个耗时操作，操作结束后要进行界面刷新，那么可以如下执行（QtConcurrent::run就在当前界面的函数中）

```cpp
QtConcurrent::run([this] {//this是当前窗口的指正，为了最后进行刷新
        // 执行耗时数据加载

        // 回到主线程更新模型
        QMetaObject::invokeMethod(this, [=] {
            // 更新数据
            widgetUpdate();
        });
    });
```

### 继承QObject的类，不要忘记加上Q_OBJECT宏

如果你的类继承了QObject，不要忘记加上Q_OBJECT宏，即使你没有使用信号和槽，你也要加上这个宏，否则你会遇到一些奇怪的问题

例如你需要进行多语言翻译，在需要翻译的文字使用了tr，但如果你所在的类没加上Q_OBJECT宏，你的翻译是不会生效的

### 自定义数据类型的信号和槽要写全命名空间

自定义数据类型，例如`MyClass`要作为参数在信号和槽中使用，那么必须写全命名空间，否则信号和槽无法识别

例如：

```cpp hl_lines="5 14"
namespace DA{
    class MyClass{
    };
}
Q_DECLARE_METATYPE(std::shared_ptr< DA::MyClass >)

//在cpp中注册：
qRegisterMetaType< std::shared_ptr< DA::MyClass > >();

//应用此类型作为信号参数
namespace DA{
    class OtherClass{
    Q_SIGNALS:
        void mySignal(std::shared_ptr< DA::MyClass > val);
    }
}
```

上面代码中，由于`Q_DECLARE_METATYPE`必须在命名空间外定义，所以`std::shared_ptr< DA::MyClass >`必须写全命名空间

而作为使用这个参数的类，信号定义时，虽然在命名空间内，但信号/槽的参数**必须写全命名空间**，否则Qt会识别不了

!!! tips "注意"
    qRegisterMetaType建议不要传入参数，让qt自动识别
    建议使用自动注册机制，参考`DAGlobals.h`的`DA_AUTO_REGISTER_META_TYPE`宏

### 信号和槽的参数要匹配

所谓匹配是不要信号定义值传递，槽定义了一个常引用来接收，例如

```cpp hl_lines="11 19"
namespace DA{
    class MyClass{
    };
}
Q_DECLARE_METATYPE(std::shared_ptr< DA::MyClass >)

//信号定义
namespace DA{
    class OtherClass{
    Q_SIGNALS:
        void mySignal(std::shared_ptr< DA::MyClass > val);
    }
}

//槽函数
namespace DA{
    class OtherClass2{
    Q_SLOTS:
        void mySlot(const std::shared_ptr< DA::MyClass >& val);
    }
}
```

上面代码中，信号和槽的参数就没有匹配，一个值传递，一个常引接收，这是**不匹配**的，Qt在某些情况会识别不了，这个调试起来非常困难