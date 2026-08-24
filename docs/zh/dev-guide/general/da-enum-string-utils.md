# DAEnumStringUtils 使用指南

本文档介绍 `DAEnumStringUtils.hpp` 中的枚举与字符串双向转换宏系统，面向 AI Agent 和人类开发者。

## 概述

`DAEnumStringUtils.hpp` 提供了一套基于宏的枚举字符串转换机制，用于在 C++ 枚举值和 `QString` 之间进行双向映射。该系统解决了手动编写枚举转换函数的繁琐问题，支持大小写敏感和不敏感两种匹配模式，并包含跨模块导出支持（DLL/共享库）。

核心功能：

- `enumToString(value)` — 枚举值转字符串
- `stringToEnum<Type>(str, default)` — 字符串转枚举值
- 大小写敏感 / 不敏感两种匹配策略
- 共享库导出支持（`DA_ENUM_STRING_DECLARE_EXPORT`）

头文件位置：`src/DAShared/DAEnumStringUtils.hpp`

## 核心概念

### DAEnumTraits 模板

`DAEnumTraits<T>` 是一个模板结构体，用于持有每个枚举类型的映射表。用户通过特化此模板来定义具体的枚举映射关系。

```cpp
template< typename T >
struct DAEnumTraits;
```

系统通过特化此模板，为每个枚举类型存储以下静态成员：

| 成员 | 类型 | 作用 |
|------|------|------|
| `enumToStringMap` | `QHash<EnumType, QString>` | 枚举到字符串的映射表 |
| `stringToEnumMap` | `QHash<QString, EnumType>` | 字符串到枚举的映射表 |
| `caseSensitive` | `bool` | 是否大小写敏感 |
| `defaultValue` | `EnumType` | 转换失败时的默认枚举值 |
| `defaultValueStr` | `QString` | `defaultValue` 对应的字符串 |

### 通用转换函数

在 `DA` 命名空间中提供两个模板函数：

```cpp
// 枚举转字符串
template< typename EnumType >
QString enumToString(EnumType value);

// 字符串转枚举
template< typename EnumType >
EnumType stringToEnum(const QString& s, EnumType defaultValue = DAEnumTraits<EnumType>::defaultValue);
```

### DAEnumEntry 辅助类型

```cpp
template< typename EnumType >
using DAEnumEntry = std::pair< EnumType, const char* >;
```

用于定义枚举条目，格式为 `{枚举值, "字符串"}`。

### 声明与定义分离

系统采用 C++ 模板分离编译模式：

- **头文件 (.h)** — 使用声明宏告知编译器需要为该枚举生成转换函数
- **源文件 (.cpp)** — 使用定义宏实际构建映射表

这种分离确保了一个枚举的映射只在翻译单元中定义一次，避免 ODR 违反。

## 使用步骤

以下是在项目中使用枚举字符串转换的标准流程。

### 第一步：在头文件中声明

在枚举定义所在的头文件末尾，使用声明宏告知编译器需要为该枚举提供转换功能。根据是否需要跨模块导出，选择对应的宏。

**不需要导出（同一模块内部使用）：**

```cpp
// MyModule/MyModuleEnum.h
enum class MyStatus
{
    Ok,
    Error,
    Pending
};

// 在命名空间外使用声明宏
DA_ENUM_STRING_DECLARE(MyStatus)
```

**需要导出（共享库场景）：**

```cpp
// MyModule/MyModuleEnum.h
#include "MyModuleAPI.h"  // 包含 DAMYMODULE_API 导出宏

enum class MyStatus
{
    Ok,
    Error,
    Pending
};

// 在命名空间外使用导出声明宏
DA_ENUM_STRING_DECLARE_EXPORT(DAMYMODULE_API, MyStatus)
```

### 第二步：在源文件中定义

在对应的 `.cpp` 文件中，使用定义宏构建实际的映射表。根据是否需要大小写敏感选择对应宏。

**大小写不敏感（推荐，用户输入场景）：**

```cpp
// MyModule/MyModuleEnumStringUtils.cpp
#include "MyModuleEnumStringUtils.h"
#include "MyModuleEnum.h"

DA_ENUM_STRING_INSENSITIVE_DEFINE(MyStatus,
                                  MyStatus::Ok,  // 默认值
                                  { MyStatus::Ok, "ok" },
                                  { MyStatus::Error, "error" },
                                  { MyStatus::Pending, "pending" });
```

**大小写敏感（精确匹配场景）：**

```cpp
DA_ENUM_STRING_SENSITIVE_DEFINE(MyStatus,
                                MyStatus::Ok,
                                { MyStatus::Ok, "Ok" },
                                { MyStatus::Error, "Error" },
                                { MyStatus::Pending, "Pending" });
```

### 第三步：在应用代码中使用

完成声明和定义后，即可在任意包含对应头文件的地方使用转换函数。

```cpp
#include "MyModuleEnumStringUtils.h"

void processData()
{
    // 枚举转字符串
    QString str = enumToString(MyStatus::Error);  // 返回 "error"

    // 字符串转枚举
    MyStatus status = stringToEnum<MyStatus>("pending");  // 返回 MyStatus::Pending

    // 字符串转枚举（指定默认值）
    MyStatus safe = stringToEnum<MyStatus>("unknown", MyStatus::Error);  // 返回 MyStatus::Error
}
```

## 宏参考

本节列出全部 4 个可用宏的签名、参数说明和使用示例。

### 1. DA_ENUM_STRING_DECLARE

**签名：**

```cpp
DA_ENUM_STRING_DECLARE(EnumType)
```

**参数：**

| 参数 | 说明 |
|------|------|
| `EnumType` | 枚举类型名。如果使用 `DA` 命名空间内的枚举，需使用完整限定名如 `DA::BodyShape` |

**示例：**

```cpp
enum class Color { Red, Green, Blue };
DA_ENUM_STRING_DECLARE(Color)

// 命名空间内的枚举
namespace DA {
enum class Direction { North, South };
}
DA_ENUM_STRING_DECLARE(DA::Direction)
```

**适用场景：** 枚举转换函数仅在定义它的模块内部使用，不需要跨 DLL/共享库导出。

### 2. DA_ENUM_STRING_DECLARE_EXPORT

**签名：**

```cpp
DA_ENUM_STRING_DECLARE_EXPORT(EXPORT_API, EnumType)
```

**参数：**

| 参数 | 说明 |
|------|------|
| `EXPORT_API` | 模块导出宏，如 `DAGUI_API`、`DAPYWORKFLOW_API` |
| `EnumType` | 枚举类型名，需完整限定名 |

**示例：**

```cpp
#include "DAPyWorkFlowAPI.h"
#include "DAEnumStringUtils.hpp"

// DAPyNodeStyleDefine.h 中的用法
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::BodyShape)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::PortShape)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::RenderTemplate)
```

**适用场景：** 枚举转换函数需要被其他模块（DLL/共享库）调用时使用。该宏会在 `DAEnumTraits` 特化前添加导出符号标注。

### 3. DA_ENUM_STRING_SENSITIVE_DEFINE

**签名：**

```cpp
DA_ENUM_STRING_SENSITIVE_DEFINE(EnumType, DefaultValue, {枚举值, "字符串"}, ...)
```

**参数：**

| 参数 | 说明 |
|------|------|
| `EnumType` | 枚举类型名，需与声明宏一致 |
| `DefaultValue` | 转换失败时返回的默认枚举值 |
| `...` | 枚举条目列表，每个条目格式为 `{枚举值, "字符串"}` |

**示例：**

```cpp
// 大小写敏感匹配：输入必须精确匹配字符串内容
DA_ENUM_STRING_SENSITIVE_DEFINE(DA::BodyShape,
                                DA::BodyShape::RoundedRect,
                                { DA::BodyShape::RoundedRect, "RoundedRect" },
                                { DA::BodyShape::Ellipse, "Ellipse" });

// 调用结果
stringToEnum<DA::BodyShape>("RoundedRect");  // 返回 DA::BodyShape::RoundedRect
stringToEnum<DA::BodyShape>("roundedrect");  // 返回默认值 RoundedRect（不匹配）
```

**适用场景：** 需要严格区分大小写的场景，比如解析格式固定的配置文件、序列化数据等。

### 4. DA_ENUM_STRING_INSENSITIVE_DEFINE

**签名：**

```cpp
DA_ENUM_STRING_INSENSITIVE_DEFINE(EnumType, DefaultValue, {枚举值, "字符串"}, ...)
```

**参数：**

| 参数 | 说明 |
|------|------|
| `EnumType` | 枚举类型名，需与声明宏一致 |
| `DefaultValue` | 转换失败时返回的默认枚举值 |
| `...` | 枚举条目列表，每个条目格式为 `{枚举值, "字符串"}` |

**示例：**

```cpp
// 大小写不敏感匹配：输入统一转换为小写后匹配
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::BodyShape,
                                  DA::BodyShape::RoundedRect,
                                  { DA::BodyShape::RoundedRect, "rounded_rect" },
                                  { DA::BodyShape::Ellipse, "ellipse" });

// 调用结果
stringToEnum<DA::BodyShape>("ROUNDED_RECT");  // 返回 DA::BodyShape::RoundedRect
stringToEnum<DA::BodyShape>("rounded_rect");  // 返回 DA::BodyShape::RoundedRect
stringToEnum<DA::BodyShape>("Ellipse");       // 返回 DA::BodyShape::Ellipse
```

**适用场景：** 处理用户输入、JSON 配置文件、网络传输数据等场景。这是项目中最常用的模式。

## 大小写处理

系统通过 `stringToEnum` 函数内部的逻辑决定匹配策略：

```cpp
QString key = DAEnumTraits< EnumType >::caseSensitive ? s : s.toLower();
```

当 `caseSensitive` 为 `true` 时，输入字符串直接查找。当 `caseSensitive` 为 `false` 时，输入字符串被转为小写后在映射表中查找。

### SENSITIVE_DEFINE 与 INSENSITIVE_DEFINE 的区别

两个定义宏的核心区别在于 **构建映射表时** 是否对键字符串做 `toLower()` 处理：

- `DA_ENUM_STRING_SENSITIVE_DEFINE`：插入映射表时保留原始字符串，但 `caseSensitive` 仍设为 `false`
- `DA_ENUM_STRING_INSENSITIVE_DEFINE`：插入映射表时对字符串调用 `toLower()`，`caseSensitive` 设为 `false`

由于两个宏都将 `caseSensitive` 设为 `false`，`stringToEnum` 调用时始终对输入做小写转换。**为了确保查询键和存储键一致，映射表中的字符串也应使用小写**，这正是 `INSENSITIVE_DEFINE` 自动完成的工作。

### 实践建议

项目中的所有现有枚举映射均使用 `DA_ENUM_STRING_INSENSITIVE_DEFINE`。推荐遵循这一约定，映射字符串值统一使用小写加下划线的格式，例如：

- `rounded_rect`、`ellipse`（节点形状）
- `left_of_text`、`above_text`（图标位置）
- `idle`、`running`、`success`（节点状态）

`DA_ENUM_STRING_SENSITIVE_DEFINE` 在项目中暂无实际使用场景。

## 导出模式

当枚举转换函数需要跨模块使用时，必须采用导出模式。

### 使用步骤

1. 头文件中包含模块的 API 导出宏头文件，如 `#include "DAGuiAPI.h"`

2. 使用 `DA_ENUM_STRING_DECLARE_EXPORT` 替代 `DA_ENUM_STRING_DECLARE`：

```cpp
#include "MyModuleAPI.h"
#include "DAEnumStringUtils.hpp"

DA_ENUM_STRING_DECLARE_EXPORT(DAMYMODULE_API, MyEnum)
```

3. `.cpp` 文件中的定义宏保持不变：

```cpp
DA_ENUM_STRING_INSENSITIVE_DEFINE(MyEnum,
                                  MyEnum::Default,
                                  { MyEnum::Default, "default" },
                                  { MyEnum::Other, "other" });
```

### 显式模板实例化

如果导出模块中的枚举转换函数需要被其他动态库使用，可能需要在 `.cpp` 末尾添加显式模板实例化：

```cpp
template class EXPORT_API DAEnumTraits<MyEnum>;
template EXPORT_API QString enumToString<MyEnum>(MyEnum);
template EXPORT_API MyEnum stringToEnum<MyEnum>(const QString&, MyEnum);
```

### 项目中的实际案例

`DAPyWorkFlow` 模块导出了大量枚举转换函数：

```cpp
// DAPyNodeStyleDefine.h
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::NamePosition)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::PortShape)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::IconPosition)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::BodyIconType)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::BodyShape)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::RenderTemplate)
```

`DAUtils` 模块导出了 Qt 类型的转换函数：

```cpp
// DAQtEnumTypeStringUtils.h
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::AlignmentFlag)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::Alignment)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::PenStyle)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::BrushStyle)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::AspectRatioMode)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::TransformationMode)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::TimeSpec)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, QFont::Weight)
```

## 项目约定

本项目对枚举字符串转换有一系列约定，遵循这些约定可以保持代码库的一致性。

### 文件命名

每个模块维护独立的 `EnumStringUtils` 文件，命名格式为：

```
{ModuleName}EnumStringUtils.h
{ModuleName}EnumStringUtils.cpp
```

**示例：**

| 文件 | 所属模块 |
|------|---------|
| `DAQtEnumTypeStringUtils.h/.cpp` | DAUtils |
| `DAGuiEnumStringUtils.h/.cpp` | DAGui |
| `DAGraphicsViewEnumStringUtils.h/.cpp` | DAGraphicsView |
| `DADataEnumStringUtils.h/.cpp` | DAData |
| `DAPyWorkFlowEnumStringUtils.h/.cpp` | DAPyWorkFlow |

### 声明与定义分离

- 声明在 `.h` 文件中，位于枚举定义之后
- 定义在 `.cpp` 文件中
- 不允许在同一个文件中混用声明和定义

### 枚举类型的命名规范

所有枚举转换中的枚举类型必须使用完全限定名：

- `DA` 命名空间内的枚举：使用 `DA::` 前缀，如 `DA::BodyShape`、`DA::DAPyNodeState`
- Qt 命名空间的枚举：使用 `Qt::` 前缀，如 `Qt::AlignmentFlag`、`Qt::PenStyle`
- Qwt 命名空间的枚举：使用 `Qwt` 前缀，如 `QwtPlot::LegendPosition`、`QwtText::TextFormat`

### 类型别名的处理

当某个枚举存在类型别名时，不需要重复声明和定义。直接复用原枚举的映射即可。

例如 `PortSide` 是 `AspectDirection` 的类型别名：

```cpp
// DAPyNodeStyleDefine.h
using PortSide = AspectDirection;

// PortSide 是 AspectDirection 的类型别名，其 DAEnumTraits 已在
// DAGraphicsViewEnumStringUtils.h 中声明
// 不重复声明，避免模板特化重复错误
// enumToString(PortSide::West) 和 stringToEnum<PortSide>() 自动复用 AspectDirection 的转换
```

### Doxygen 注释

在 `EnumStringUtils.h` 文件的头部添加文件级 Doxygen 注释，包含简要说明和示例用法。参考现有文件的注释风格。

### 映射值的格式

字符串映射值推荐使用小写字母加下划线的格式：

- `rounded_rect`、`ellipse`（节点形状）
- `left_of_text`、`above_text`（图标位置）
- `idle`、`running`、`success`（节点状态）

避免使用驼峰命名或混合格式。

## 已有映射参考

以下列出当前项目中所有已有的枚举字符串映射文件及其覆盖的枚举类型。新增映射时参考这些文件，避免重复定义。

### DAUtils 模块

**文件：** `src/DAUtils/DAQtEnumTypeStringUtils.h/.cpp`

覆盖的枚举类型：

| 枚举类型 | 说明 |
|---------|------|
| `Qt::AlignmentFlag` | Qt 对齐标志 |
| `Qt::Alignment` | Qt 对齐组合标志 |
| `Qt::PenStyle` | Qt 画笔样式 |
| `Qt::BrushStyle` | Qt 画刷样式 |
| `Qt::AspectRatioMode` | Qt 纵横比模式 |
| `Qt::TransformationMode` | Qt 变换模式 |
| `Qt::TimeSpec` | Qt 时间规范 |
| `QFont::Weight` | Qt 字体粗细 |

### DAGui 模块

**文件：** `src/DAGui/DAGuiEnumStringUtils.h/.cpp`

覆盖的枚举类型：

| 枚举类型 | 说明 |
|---------|------|
| `DA::DAColorTheme::ColorThemeStyle` | 颜色主题样式 |
| `QwtPlotItem::RttiValues` | Qwt 绘图项 RTTI 值 |
| `QwtPlot::LegendPosition` | Qwt 图例位置 |
| `QwtText::TextFormat` | Qwt 文本格式 |
| `QwtAxis::Position` | Qwt 坐标轴位置 |
| `QwtScaleDiv::TickType` | Qwt 刻度类型 |
| `QwtScaleDraw::Alignment` | Qwt 刻度绘制对齐方式 |
| `QwtDate::Week0Type` | Qwt 日期周起始类型 |
| `QwtDate::IntervalType` | Qwt 日期间隔类型 |

### DAGraphicsView 模块

**文件：** `src/DAGraphicsView/DAGraphicsViewEnumStringUtils.h/.cpp`

覆盖的枚举类型：

| 枚举类型 | 说明 |
|---------|------|
| `DA::DAGraphicsLinkItem::EndPointType` | 连线端点类型 |
| `DA::DAShapeKeyPoint::KeyPoint` | 形状关键点 |
| `DA::DAGraphicsLinkItem::LinkLineStyle` | 连线样式 |
| `DA::AspectDirection` | 方位方向 |

### DAData 模块

**文件：** `src/DAData/DADataEnumStringUtils.h/.cpp`

覆盖的枚举类型：

| 枚举类型 | 说明 |
|---------|------|
| `DA::DAAbstractData::DataType` | 数据类型 |

### DAPyWorkFlow 模块

**文件：** `src/DAPyWorkFlow/DAPyWorkFlowEnumStringUtils.h/.cpp`

**声明位置分散在：** `DAPyNodeStyleDefine.h` 和 `DAPyWorkFlowEnumStringUtils.h`

覆盖的枚举类型：

| 枚举类型 | 说明 |
|---------|------|
| `DA::DAPyNodeState` | Python 工作流节点状态 |
| `DA::BodyShape` | 节点体形状 |
| `DA::PortShape` | 端口形状 |
| `DA::NamePosition` | 节点名称位置 |
| `DA::IconPosition` | 图标位置 |
| `DA::BodyIconType` | 节点体图标类型 |
| `DA::RenderTemplate` | 渲染模板类型 |
| `DA::LinkPointLayoutStrategy` | 连接点布局策略 |

## 完整示例

以下提供从零到一的完整示例，涵盖头文件中定义枚举、声明映射、源文件中定义映射、以及应用代码中使用的全过程。

### 示例一：模块内部使用

假设在 `DAModule` 模块中定义了一组枚举，仅需在本模块内部使用转换功能。

**头文件 — `DAModule/DAModuleEnum.h`：**

```cpp
#ifndef DAMODULEENUM_H
#define DAMODULEENUM_H

#include <QString>
#include "DAEnumStringUtils.hpp"

namespace DA
{

/**
 * @brief 数据处理阶段枚举
 */
enum class DataPhase
{
    Loading,     ///< 加载阶段
    Processing,  ///< 处理阶段
    Exporting,   ///< 导出阶段
    Complete     ///< 完成阶段
};

}  // namespace DA

// 在命名空间外声明（不需要导出）
DA_ENUM_STRING_DECLARE(DA::DataPhase)

#endif  // DAMODULEENUM_H
```

**源文件 — `DAModule/DAModuleEnumStringUtils.cpp`：**

```cpp
#include "DAModuleEnum.h"

// 定义数据阶段枚举映射，使用大小写不敏感模式
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DataPhase,
                                  DA::DataPhase::Loading,
                                  { DA::DataPhase::Loading, "loading" },
                                  { DA::DataPhase::Processing, "processing" },
                                  { DA::DataPhase::Exporting, "exporting" },
                                  { DA::DataPhase::Complete, "complete" });
```

**应用代码：**

```cpp
#include "DAModuleEnum.h"

void runPhase(DA::DataPhase phase)
{
    // 枚举转字符串，用于日志输出
    QString phaseStr = enumToString(phase);
    qDebug() << "Current phase:" << phaseStr;

    // 字符串转枚举，用于解析配置
    DA::DataPhase next = stringToEnum<DA::DataPhase>("complete");
    if (next == DA::DataPhase::Complete) {
        qDebug() << "Task finished";
    }
}
```

### 示例二：导出给其他模块使用

如果枚举所在的模块是共享库，需要让其他模块调用转换函数，则使用导出模式。

**头文件 — `DALib/DALibEnum.h`：**

```cpp
#ifndef DALIBENUM_H
#define DALIBENUM_H

#include "DALibAPI.h"   // 包含 DAlIB_API 导出宏
#include "DAEnumStringUtils.hpp"

namespace DA
{

/**
 * @brief 日志级别枚举
 */
enum class LogLevel
{
    Debug,   ///< 调试级别
    Info,    ///< 信息级别
    Warning, ///< 警告级别
    Error    ///< 错误级别
};

}  // namespace DA

// 使用导出声明以便其他模块链接时使用
DA_ENUM_STRING_DECLARE_EXPORT(DALIB_API, DA::LogLevel)

#endif  // DALIBENUM_H
```

**源文件 — `DALib/DALibEnumStringUtils.cpp`：**

```cpp
#include "DALibEnum.h"

DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::LogLevel,
                                  DA::LogLevel::Info,
                                  { DA::LogLevel::Debug, "debug" },
                                  { DA::LogLevel::Info, "info" },
                                  { DA::LogLevel::Warning, "warning" },
                                  { DA::LogLevel::Error, "error" });

// 显式实例化模板特化，确保导出符号在其他模块中可见
template class DALIB_API DA::DAEnumTraits<DA::LogLevel>;
template DALIB_API QString DA::enumToString<DA::LogLevel>(DA::LogLevel);
template DALIB_API DA::LogLevel DA::stringToEnum<DA::LogLevel>(const QString&, DA::LogLevel);
```

### 示例三：非 DA 命名空间枚举

项目中大量使用了 Qt 和 Qwt 的枚举，这些枚举不在 `DA` 命名空间中，但仍可以使用本系统。

**头文件 — `DAUtils/DAQtEnumTypeStringUtils.h`：**

```cpp
#ifndef DAQTENUMTYPESTRINGUTILS_H
#define DAQTENUMTYPESTRINGUTILS_H

#include "DAEnumStringUtils.hpp"
#include "DAUtilsAPI.h"
#include <Qt>

// Qt 命名空间中的枚举，同样使用完全限定名声明
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::PenStyle)
DA_ENUM_STRING_DECLARE_EXPORT(DAUTILS_API, Qt::BrushStyle)

#endif  // DAQTENUMTYPESTRINGUTILS_H
```

**源文件 — `DAUtils/DAQtEnumTypeStringUtils.cpp`：**

```cpp
#include "DAQtEnumTypeStringUtils.h"

// 注意使用 Qt:: 前缀
DA_ENUM_STRING_INSENSITIVE_DEFINE(Qt::PenStyle,
                                  Qt::SolidLine,
                                  { Qt::NoPen, "NoPen" },
                                  { Qt::SolidLine, "SolidLine" },
                                  { Qt::DashLine, "DashLine" },
                                  { Qt::DotLine, "DotLine" },
                                  { Qt::DashDotLine, "DashDotLine" },
                                  { Qt::DashDotDotLine, "DashDotDotLine" },
                                  { Qt::CustomDashLine, "CustomDashLine" });
```

## 常见问题

### 为什么声明和定义要分开？

C++ 模板的特化需要遵循单一定义规则（ODR）。声明宏在头文件中告知编译器存在该枚举的特化，而定义宏在唯一的 `.cpp` 文件中实例化具体的映射表数据。如果将定义放在头文件中并被多个 `.cpp` 包含，会导致链接时多重定义错误。

### 同一个枚举可以声明两次吗？

不可以。每个枚举类型只能使用一次声明宏和一次定义宏。重复声明会导致模板特化重复错误（C2766）。

如果两个模块都需要转换同一个枚举，应在枚举所在的模块中声明和定义，其他模块通过包含该模块的 `EnumStringUtils.h` 来使用。对于类型别名（如 `using PortSide = AspectDirection`），直接复用原枚举的映射，不需要重复定义。

### 同一个字符串可以映射到两个枚举值吗？

不可以。在 `stringToEnumMap` 中字符串作为键，重复的键会导致后定义的覆盖前一个。确保每个映射字符串都是唯一的。

### 两个字符串可以映射到同一个枚举值吗？

可以。多个不同的字符串映射到同一个枚举值是合法的。`stringToEnum` 匹配第一个找到的值，`enumToString` 返回最先定义的字符串映射（`QHash` 中第一个插入的值）。

### 应该选择 SENSITIVE 还是 INSENSITIVE？

项目中推荐统一使用 `INSENSITIVE`（大小写不敏感）。用户输入、配置文件解析、网络传输等场景都不应依赖精确的大小写匹配。仅当需要严格区分大小写的序列化场景时才使用 `SENSITIVE`。

### 跨模块使用时需要注意什么？

1. 确保使用了正确的导出宏（`DA_ENUM_STRING_DECLARE_EXPORT` 而非 `DA_ENUM_STRING_DECLARE`）
2. 确保包含了对应模块的 API 头文件（如 `DAGuiAPI.h`）
3. 必要时在 `.cpp` 末尾添加显式模板实例化
4. 调用方需包含提供映射的 `EnumStringUtils.h` 头文件

### 每个模块管理自己的映射文件吗？

是的。每个模块在自身的 `EnumStringUtils.h/.cpp` 中声明和定义属于该模块的枚举映射。不要跨模块修改其他模块的映射文件。如果需要用到其他模块的枚举，直接包含对应的头文件即可。
