# 开发规范

本文档定义 data-workbench 项目的代码编写规范，包括命名规则、注释规范、调试开关和 Qt 编程注意事项。

## 主要功能特性

- **统一命名风格**：驼峰命名法用于类和函数，下划线命名用于全局变量
- **IDE 搜索优化**：命名遵循 IDE 最优搜索原则，便于快速定位符号
- **Doxygen 注释**：头文件简短注释，源文件完整 Doxygen 注释
- **宏开关控制**：调试功能和代码开关一律用宏控制，不用注释屏蔽
- **Qt 兼容性**：涵盖 Qt5/Qt6 差异处理和多线程 UI 操作规范

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

> **💡 说明**
>
> 谷歌规范推荐私有成员变量使用后缀下划线（如 `value_`），虽可避免与局部变量混淆，但不利于 IDE 搜索。而传统 `m_` 前缀在 Pimpl 模式下显得冗余（如 `d_ptr->m_value`）。
> 本规范采用 **`m` 前缀 + 驼峰**（如 `mValue`），兼顾简洁性与 IDE 提示体验：在 Pimpl 中写作 `d_ptr->mValue` 更美观，且输入 `d_ptr->m` 即可触发 IDE 自动补全。


示例代码：

以下代码展示各类标识符的命名方式：

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

> **💡 说明**
>
> `QStyleOption*` 系列就是比较好的例子：相似功能的类都以 `QStyleOption` 打头，比如 `QStyleOptionToolButton`，而 `QToolButtonStyleOption` 这样的命名不利于在 IDE 里列举

## 函数命名规范

### 属性命名规

- **普通属性**：使用 `setXxx()` / `getXxx()` 配对。**不要省略 `get`**，以利于 IDE 搜索。
- **返回引用的属性**：**不加 `get`**，直接命名为 `xxx()`。
- **布尔属性**：
  - 可设置性（capability）：`setXxxable(bool)`，如 `setSelectable(bool)`；
  - 当前状态（state）：`isSelected()` / `setSelected(bool)`；
  - 读取函数统一以 `is` 开头。
- **统一使用 `set` 前缀**，使 IDE 能通过输入 `set` 联想所有可配置项。

尽量使用 `set`，让编译器能通过 `set` 联想出所有可设置属性，用 `get` 和 `is` 能联想出所有可读属性

以下代码展示普通属性、引用属性和布尔属性的命名：

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

以下代码展示虚函数重写的正确方式：

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
  - 针对 Qt 的**信号（signals）** 没有 `.cpp` 实现，可以在头文件中写详细的 Doxygen 注释。
- **源文件（.cpp）中**：
  - 所有函数实现前写完整 Doxygen 注释；
  - 注释前**空一行**；
  - 使用 `/** ... */` 格式；
  - 成员变量注释使用 `///<`。

示例：（`//!` 为示例的说明）

头文件注释示范，只写简短注释：

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

class DAPyWorkFlowManager{
    /////////////示例的说明////////////////
    //! 信号没有cpp的实现，可以把doxygen注释写在头文件中
    //////////////////////////////////////
signals:
    /**
     * @brief 节点创建产生的信号
     * @param node
     */
    void nodeCreated(DAPyNode* node);
    /**
     * @brief 节点添加的信号
     * @param node
     */
    void nodeAdded(DAPyNode* node);
};
```

cpp 文件注释示范，源文件里写完整 Doxygen 注释：

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

Doxygen 注释里用 `@brief`、`@param`、`@return`、`@sa` 等标签，正文写中文。

## 调试与功能开关

- 对于大段功能的开关，**禁止使用注释屏蔽代码**
- 应使用 **宏开关** 控制调试输出或功能启用。

宏开关控制调试输出的示例：

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

`DA_DAABSTRACTNODE_DEBUG_PRINT` 为 0 时关闭调试输出，为 1 时开启。

功能的启用和调整同样用宏控制，不要用注释屏蔽代码。

## 编码禁止列表

- 禁止头文件中使用 `using namespace`
- 基类若用于继承，**析构函数必须为 `virtual`**
- **库导出类禁止继承非模板的 STL 或 Qt 模板类**（如 `class DLLEXPORT X : public QList<QColor>`），因可能导致符号冲突或析构不完整。若需继承模板类，自身也应为模板类。


## 语法规范性条例

下面这些内容涉及一些语法相关的规范和条例，通用条例常见于 effective cpp 和 effective modern cpp

### 用异常而不是用std::optional

早期 C++ 规范不推荐使用异常，比如谷歌的条款推荐用错误码；现代 C++ 更推荐异常，原因如下：

- 异常能携带更多信息，结合 Qt 的多语言框架还能带上多语言的异常文本
- `std::optional` 在某些场合能替代异常，但缺陷明显：它无法携带信息，除非再用一个类似 `getLastErrorString` 的函数去取，实际工程里大规模用并不友好
- 在错误码和 `std::optional` 之间宁可选错误码，和界面相关的编程中，错误码至少能描述更详细的错误信息
- 用了异常就要注意处理流程，尤其在和一些不使用异常的库（比如 Qt）混用的时候
- 科学计算类、io 操作类的库建议使用异常

### 操作符重载

操作符重载函数**必须定义在所属类的命名空间内**，以支持 ADL（Argument-Dependent Lookup）。

如下是**正确**的做法，操作符定义在命名空间内：

```cpp
namespace DA
{
class DANodeLinkPoint
{
}

bool operator==(const DANodeLinkPoint& a, const DANodeLinkPoint& b);
}// end namespace DA
```

如下是**错误**的做法，操作符定义在命名空间外会导致 ADL 失效：

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

以下代码展示了 QPixmap 的基本设置：

```cpp
QPixmap mPixmap;
QIcon mIcon;

...

mPixmap = mIcon.pixmap(getBodySize().toSize());
```

在 paintEvent 里绘制时，本例要把 pixmap 画到 bodyRect 的中心，下面是错误的做法

如下是**错误**的做法，未考虑 devicePixelRatio 导致位置偏移：

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

上面这段代码的问题在于没考虑 devicePixelRatio：缩放比例为 100% 时看不出异常，缩放比例不是 100% 时就会画到错误的位置，问题主要出在 xoffset 和 yoffset 上。pixmapRect.width()/height() 不是 QPixmap 在当前绘图坐标系下的宽高，两个坐标系并不一致，要拿到当前绘图坐标系下的宽高，需要除以 devicePixelRatio。

如下是**正确**的做法，用 devicePixelRatio 计算绘制位置：

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

比如一个窗口用 QtConcurrent 执行耗时操作，操作结束后要刷新界面，可以这样写（QtConcurrent::run 就在当前界面的函数中）

以下代码展示了如何在后台线程中安全地更新 UI，使用 `QMetaObject::invokeMethod` 切回主线程：

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

继承了 QObject 的类一定要加上 Q_OBJECT 宏，即使没有用到信号和槽，否则会遇到一些奇怪的问题

比如要做多语言翻译，文字上写了 tr，但类里没加 Q_OBJECT 宏，翻译就不会生效

### 自定义数据类型的信号和槽要写全命名空间

自定义数据类型（比如 `MyClass`）作为参数在信号和槽中使用时，必须写全命名空间，否则信号和槽无法识别

比如：

以下代码展示了自定义类型在信号槽中使用的正确声明方式，必须写全命名空间：

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

上面代码中，`Q_DECLARE_METATYPE` 必须在命名空间外定义，所以 `std::shared_ptr< DA::MyClass >` 必须写全命名空间

信号定义虽然写在命名空间内，但信号/槽的参数**必须写全命名空间**，否则 Qt 会识别不了

> **💡 注意**
>
> qRegisterMetaType 建议不要传入参数，让 Qt 自动识别
> 建议使用自动注册机制，参考 `DAGlobals.h` 的 `DA_AUTO_REGISTER_META_TYPE` 宏

### 信号和槽的参数要匹配

所谓匹配，是指信号和槽的参数形式一致。

以下代码是参数不匹配的示例，信号按值传递、槽用常引用接收：

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

上面代码里信号和槽的参数不匹配：一个值传递，一个常引接收。Qt 在某些情况下会识别不了，这类问题调试起来非常困难

## 相关规范

> **ℹ️ 图标与 UI 设计规范**
>
> 本规范约束**代码**的命名、注释与风格；项目同时有 [图标与 UI 设计规范](./icon-ui-design-guide.md) 约束**视觉表现**，包括 SVG 图标画布规格、语义化色板、色彩组合规则以及 UI 控件（按钮/面板/状态指示）配色映射。
>
> **涉及 SVG 图标设计（新增、替换、修改 `src/DAGui/icon/` 或 `src/APP/Icon/` 下任意 `.svg` 文件）时，必须先阅读 [图标与 UI 设计规范](./icon-ui-design-guide.md)** 并按其 Checklist 逐项核对，确保图标与界面控件配色同源、语义一致。
