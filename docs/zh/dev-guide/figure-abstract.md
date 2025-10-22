# 绘图模块概述

绘图模块`DAFigure`基于`Qwt`绘图库，封装了绘图功能，主要在Qwt基础上，集成扩展了`redo/undo`功能、区域选择功能、缩放平移等交互功能

## 命名约定

绘图模块的命名约定如下：

- `Figure`代表画布，它是顶层的绘图窗口容器，能容纳多个绘图（`Chart`）
- `Chart`代表绘图，它是一个具体的绘图窗口，能容纳多个绘图元素(`Item`)
- `Item`代表绘图元素，比如曲线、网格等，实际是Qwt的`QwtPlotItem`的实例

绘图模块`DAFigure`的类以`DAChart*`或者`DAFigure*`开头

!!! tips "说明"
    绘图模块的属性设置窗口位于`DAGui`模块，主要也是以`DAChart*`开头