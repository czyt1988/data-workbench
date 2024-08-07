# 命名规则

原则上，全局属性变量以下划线命名，其他都以驼峰命名

- 命名以IDE最优搜索原则，方便开发过程让编译器快速找到对应变量、枚举、函数等
- 统一以驼峰命名（除全局函数全局变量以外）
- 局部变量和成员函数**首字母小写**
- 类命名**首字母大写**
- 类私有成员变量以`m`为开头(不加下横线,如`mValue`)，类公有成员变量不做前缀，驼峰命名，小写字母开头
- 全局变量以下划线分隔，以g_作为开头，如`g_gloabl_value`
- 静态变量以下划线分隔，以s_作为开头，如`s_static_value`
- 全局函数使用下划线小写的命名规则
- 命名空间不宜过长，所有都大写，例如DataAnalysis应写为DA
- 宏都大写，以下划线进行分隔

> 谷歌规范里私有变量用结尾_来标记，如`value_`，这样虽然能避免和局部变量混淆，但不利于IDE的搜索，而下斜杠前置又和C++的保留关键字冲突（C++有很多下横杠开头的全局函数，以及保留下横杠开头作为关键字的可能），而传统的`m_`开头法实际上是对IDE最优的，但使用Pimpl模式时的“颜值”不好看，因此，这里使用m开头，不要下横杠的模式，这个模式IDE的搜索并不是最优，一般IDE要输入2~3个才开始展示推荐列表，单独输入m很有可能并不会触发推荐列表，当然可以使用this->m来进行触发，或者输入m后后面随便输入几个字母再删除来获取推荐列表，使用m不要下横杠的好处是更为简洁，尤其在Pimpl模式下，`d_ptr->m_value`就没有`d_ptr->mValue`好看，Pimpl模式下通过pimpl指针是能触发IDE的提示列表的，输入d_ptr->m，这时IDE的提示就会弹出来了
>
>下面展示了几种成员变量的表示方法
>
> ```cpp
> d_ptr->m_value = 1;
> d_ptr->mValue = 1;
> d_ptr->_value = 1;
> d_ptr->value_ = 1;
>```

以下为命名示例：

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

> `QStyleOption*`系列就是比较好的例子，所有相似功能的类都是以`QStyleOption`打头，例如`QStyleOptionToolButton`，而不是命名为`QToolButtonStyleOption`

另外在函数命名上因遵守如下原则：

- 属性前面都带get/set（布尔变量除外），不省略get以便查IDE能方便联想
- get函数应返回值而不是引用
- 若返回引用，不带get以和返回值的属性做区分
- 布尔变量的属性命名为 set(enable)/is为主，布尔变量的动作可以以set开头，但所有布尔变量的读函数都以is开头
- 如果布尔属性有形容词xxxxble，可使用set替代enable，如setVisible,setCheckable,但为了统一，更推荐用setXXable的形式，也可以用setEnableXX形式
- 布尔变量的动作应该以set开头，如setSelect和enableSelect是表达不一样的概念，具体可通过下列例子的selectable和selected来区分

总之尽量使用set，让编译器能通过set联想出所有可设置属性，用get和is能联想出所有可读属性

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

针对虚函数的重写，按照C++11写法，后面必须接`override`,但为了更直观发现虚函数，重写的虚函数前面也加上`virtual`关键字

```cpp
class ChildClass : public ExampleClass{
public:
    ChildClass();
    virtual void virtualFunction() override;//虚函数后面接override，为了更好发现，前面的virtual关键字不省略
};
```

# 注释规范

注释遵循`doxygen`注释规则

- 头文件的类成员函数不使用`doxygen`注释，这样是为了避免大型项目构建时，因为更改了文档而导致长时间的构建
- 头文件的类成员函数虽然不使用`doxygen`注释，但要求有简短的普通注释，其内容和`doxygen`的`brief`标签内容一致
- 头文件中，get/set配对的函数注释可以写一个
- 类成员函数的`doxygen`注释写在cpp文件中，除非此函数不在cpp中实现，如qt的信号函数
- 函数的`doxygen`注释前空一行
- 函数以`/**`开启`doxygen`注释，变量以`///<` 进行`doxygen`注释

示例：（`//!`为示例的说明）

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

对应内容的屏蔽不应该使用注释而应该使用宏，如下示例，在屏蔽打印不允许使用注释，而是定义一个宏来进行开关

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

# 编码禁止列表

- 禁止头文件中使用`using namespace`
- 基类如果用于继承，析构函数必须使用`virtual`修饰符
- 库导出类禁止继承于库的模板类，例如`dllexport class xx : public QList<QColor>`，这种情况在一些编译器会导致符号冲突，且如果出现虚函数，基类析构函数没有虚函数会导致内存泄漏，如果要继承模板类，此类也应该为模板类


# 语法规范性条例

下面这些内容是涉及一些语法相关的规范和条例，通用规范条例常见effective cpp和effective modern cpp

## 用异常而不是用std::optional

早期c++规范都不推荐使用异常，例如谷歌的条款，而是推荐使用错误码，现代C++中，更为推荐使用异常，主要有如下原因：

- 异常可携带更多的信息，甚至结合Qt的多语言框架，能直接进行多语言的异常信息携带
- `std::optional`在某些场合能替代异常，但是有很大的缺陷，首先无法携带信息，除非用一个类似`getLastErrorString`的函数来获取，实际工程上大规模使用并不友好
- 宁愿用错误码也不应该用`std::optional`，在和界面相关的编程中，错误码至少能进行更详细的错误信息描述
- 如果使用了异常，开发过程要注意处理，尤其和一些不使用异常的库混用的时候，例如Qt
- 一些科学计算类、io操作类的库建议使用异常

## 在命名空间中的类进行操作符重载时，重载函数也应该定义在该命名空间中

提供使用ADL发现操作符的能力,编译器首先在当前作用域以及包含该函数或操作符的作用域中查找

如下是**正确**的做法：

```cpp
namespace DA
{
class DANodeLinkPoint
{
}

bool operator==(const DANodeLinkPoint& a, const DANodeLinkPoint& b);
}// end namespace DA
```

如下是**错误**的做法：

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

# Qt编程注意事项

## 不同屏幕缩放比例下QPixmap绘制问题

在不同屏幕缩放比例下，QPixmap的长宽高尺寸是不一样的

有如下程序，先从QIcon获取QPixmap

```cpp
QPixmap mPixmap;
QIcon mIcon;

...

mPixmap = mIcon.pixmap(getBodySize().toSize());
```

在paintEvent里绘制,这里演示是要把pixmap绘制到bodyRect的中心，下面是错误的绘制方法

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

上面方法之所以错误，是因为没有考虑devicePixelRatio，在缩放比例为100%时，上面代码绘制的图片没有问题，但缩放比例不是100%，上面的代码绘制出来是不正确的位置，主要异常在xoffset和yoffset，因为pixmapRect.width()/height()并非QPixmap在当前绘图坐标系下的宽高，这时两个坐标系是不一致的，你要获取当前绘图坐标系下的宽高，需要除以devicePixelRatio，正确的操作如下

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