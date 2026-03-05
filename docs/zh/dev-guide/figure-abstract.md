# 绘图模块概述

绘图模块`DAFigure`基于`Qwt`绘图库，封装了绘图功能，主要在Qwt基础上，集成扩展了`redo/undo`功能、区域选择功能、缩放平移等交互功能

## 命名约定

绘图模块的命名约定如下：

- `Figure`代表画布，它是顶层的绘图窗口容器，能容纳多个绘图（`Chart`）
- `Chart`代表绘图，它是一个具体的绘图窗口，能容纳多个绘图元素(`Item`)
- `Item`代表绘图元素，比如曲线、网格等，实际是Qwt的`QwtPlotItem`的实例

绘图模块`DAFigure`的类以`DAChart*`或者`DAFigure*`开头

!!! tip "说明"
    绘图模块的属性设置窗口位于`DAGui`模块，主要也是以`DAChart*`开头

## 核心类结构

绘图模块的核心类层次如下：

```
DAFigureWidget (画布容器)
├── DAChartWidget (图表控件，基于QwtPlot)
│   ├── QwtPlotCurve (曲线)
│   ├── QwtPlotGrid (网格)
│   ├── QwtPlotMarker (标记)
│   └── ... (其他QwtPlotItem)
└── Undo/Redo 命令栈
```

### DAFigureWidget

`DAFigureWidget` 是绘图模块的顶层容器，继承自 `QScrollArea`，主要功能：

- 管理多个 `DAChartWidget` 图表实例
- 提供 undo/redo 命令栈支持
- 管理图表的布局和显示

### DAChartWidget

`DAChartWidget` 是具体的图表控件，在 `QwtPlot` 基础上扩展了交互功能，支持以下编辑模式：

| 编辑模式 | 编号 | 说明 |
|---------|------|------|
| SubChartEditor | 0 | 子图表编辑 |
| RectSelectEditor | 1 | 矩形区域选择 |
| EllipseSelectEditor | 2 | 椭圆区域选择 |
| PolygonSelectEditor | 3 | 多边形区域选择 |
| UserDefineEditor | 1000+ | 用户自定义编辑器 |

## 支持的数据系列类型

绘图模块支持以下数据系列类型：

| 系列类型 | 说明 |
|---------|------|
| XY曲线 | 基础的二维数据曲线 |
| XYE系列 | 带误差条的数据曲线 |
| OHLC系列 | K线图（开高低收） |
| 柱状图 | 柱状数据可视化 |
| 区间曲线 | 带上下界的区间显示 |
| 网格栅格数据 | 热力图等二维分布 |

## 关键文件说明

| 文件 | 功能 |
|------|------|
| `DAFigureWidget.h/cpp` | 画布容器，管理多个图表 |
| `DAChartWidget.h/cpp` | 图表控件，核心绘图组件 |
| `DAChartUtil.h/cpp` | 图表工具函数 |
| `DAChartSerialize.h/cpp` | 图表序列化/反序列化 |
| `DAChartAxisRangeBinder.h/cpp` | 多图表轴范围同步绑定 |

## 与其他模块的关系

绘图模块的依赖关系：

- 依赖 `DAUtils` 模块提供的基础工具类
- 依赖第三方库 `Qwt` 提供绑图基础
- 依赖 `Qt::Concurrent` 实现异步绘图
- 依赖 `Qt::PrintSupport` 实现图表打印和导出
- 绘图模块的属性设置窗口（如轴设置、曲线样式等）位于 `DAGui` 模块中
