# 数据探针功能开发方案计划

## 1. 功能概述

### 1.1 功能目标

实现数据探针功能，允许用户在绘图区域点击添加探针，记录点击位置数据并在表格界面显示。探针功能为用户提供精确的数据标注和捕获能力，适用于数据分析、实验记录等场景。

### 1.2 功能特性

- **探针类型**：支持垂直探针和水平探针两种类型
- **命名机制**：默认从字母"A"开始递增命名，支持自定义命名
- **可视化标记**：探针在绘图区域显示标记（顶部或底部）以区分不同探针
- **数据捕获**：探针提供数据捕获能力，可获取对应位置的x值或y值
- **管理功能**：支持探针的创建、删除、修改、查询等操作

### 1.3 用户场景

1. **数据标注**：在关键数据点添加标记，便于后续分析
2. **数据比较**：通过多个探针比较不同位置的数据差异
3. **阈值标记**：使用水平探针标记阈值线或参考线
4. **时间点标记**：使用垂直探针标记特定时间点的数据

## 2. 现有代码分析

### 2.1 相关类结构

#### 2.1.1 QwtPlotSeriesDataPicker（数据拾取参考）

位置：`src/3rdparty/qwt/src/plot/qwt_plot_series_data_picker.h`

这是Qwt新增的数据拾取类，用于跟随鼠标显示数据值。探针可以复用其数据拾取逻辑。

**核心功能**：

- `pickYValue()`：在指定屏幕位置拾取所有曲线的Y值
- `pickNearestPoint()`：拾取最近的点
- `FeaturePoint` 结构：存储拾取到的特征点信息

```cpp
struct FeaturePoint {
    QwtPlotItem* item;      // 对应的曲线项
    QPointF feature;        // 特征点坐标
    size_t index;           // 在曲线数据中的索引
};

enum PickSeriesMode {
    PickYValue,         // 拾取Y值模式（垂直线）
    PickNearestPoint    // 拾取最近点模式
};

enum InterpolationMode {
    NoInterpolation,     // 不插值，使用最近数据点
    LinearInterpolation  // 线性插值
};
```

**探针与Picker的区别**：

| 特性 | QwtPlotSeriesDataPicker | 数据探针 |
|------|-------------------------|----------|
| 位置 | 跟随鼠标移动 | 固定位置 |
| 生命周期 | 临时显示 | 持久化存储 |
| 数据存储 | 不存储 | 存储到表格 |
| 命名 | 无 | 自动命名(A, B, C...) |

#### 2.1.2 QwtPlotSeriesDataPickerGroup（同步组）

位置：`src/3rdparty/qwt/src/plot/qwt_plot_series_data_picker_group.h`

用于多个绘图间picker位置的同步。探针可以参考其同步机制实现跨图同步。

#### 2.1.3 DAChartItemCreatInteractor

位置：`src/DAFigure/DAChartItemCreatInteractor.h`

创建图表项的交互器，通过工厂函数在点击位置创建 `QwtPlotItem`：

```cpp
class DAFIGURE_API DAChartItemCreatInteractor : public DAAbstractChartEditor
{
public:
    using FpCreatePlotItem = std::function<QwtPlotItem*(QwtPlot*, const QPointF&)>;
    
    void setPlotItemInteractorFactory(FpCreatePlotItem fun);
    virtual QwtPlotItem* takeItem() override;
    
protected:
    virtual bool mousePressEvent(const QMouseEvent* e) override;
};
```

现有的标记创建工厂函数：

- `createHLineMarkerPlotItem` - 创建水平线标记
- `createVLineMarkerPlotItem` - 创建垂直线标记
- `createCrossLineMarkerPlotItem` - 创建十字线标记

#### 2.1.4 DAFigureWidget

位置：`src/DAFigure/DAFigureWidget.h`

绘图窗口主类，管理图表和编辑器：

```cpp
class DAFIGURE_API DAFigureWidget : public QScrollArea
{
public:
    enum ChartEditorType {
        SubChartEditor = 0,
        RectSelectEditor,
        EllipseSelectEditor,
        PolygonSelectEditor,
        HLineMarker,
        VLineMarker,
        CrossMarker,
        ArrowMarker,
        BuilinEditorCount,
        UserDefineEditor = 1000
    };
    
    void beginChartEditor(ChartEditorType type);
    void endChartEditor();
};
```

### 2.2 编辑器创建流程

现有编辑器创建流程：

1. 调用 `beginChartEditor(ChartEditorType type)`
2. 根据类型调用对应的 `beginXxxEditor()` 方法
3. 使用 `beginSelectEditor<EditorType>(args...)` 模板函数创建编辑器
4. 编辑器完成交互后发出 `finished` 信号
5. `DAFigureWidget` 接收信号并处理结果

### 2.3 QwtPlotMarker 特性

`QwtPlotMarker` 支持的线条样式：

```cpp
enum LineStyle {
    NoLine,    // 无线
    HLine,     // 水平线
    VLine,     // 垂直线
    Cross      // 十字线
};
```

支持的标签对齐方式：

- `Qt::AlignLeft`, `Qt::AlignRight`, `Qt::AlignHCenter`
- `Qt::AlignTop`, `Qt::AlignBottom`, `Qt::AlignVCenter`

## 3. 技术方案设计

### 3.1 类设计

#### 3.1.1 DADataProbeMarker 类

**文件位置**：`src/DAFigure/DADataProbeMarker.h` / `src/DAFigure/DADataProbeMarker.cpp`

**继承关系**：`DADataProbeMarker` → `QwtPlotMarker`（直接继承，不继承已弃用的 `DAAbstractChartMarker`）

**设计说明**：

- 直接继承 `QwtPlotMarker`，复用其线条绘制和标签显示功能
- 参考 `QwtPlotSeriesDataPicker` 的数据拾取逻辑
- 探针是固定位置的数据拾取器，而非跟随鼠标的临时显示
- 要为探针设计单独的rtti，以便识别和处理

**类定义**：

```cpp
/**
 * \if ENGLISH
 * @brief Data probe marker class for capturing and displaying data points
 * @details This class provides vertical and horizontal probe markers that can
 *          capture data values at specific positions on the plot. Unlike 
 *          QwtPlotSeriesDataPicker which follows the mouse, probes are fixed
 *          position data pickers with persistent storage.
 * \endif
 *
 * \if CHINESE
 * @brief 数据探针标记类，用于捕获和显示数据点
 * @details 该类提供垂直和水平探针标记，可以捕获绘图上特定位置的数据值。
 *          与跟随鼠标的QwtPlotSeriesDataPicker不同，探针是固定位置的数据拾取器，
 *          支持持久化存储。
 * \endif
 */
class DAFIGURE_API DADataProbeMarker : public QwtPlotMarker
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DADataProbeMarker)
public:
    /**
     * \if ENGLISH
     * @brief Probe type enumeration
     * \endif
     * \if CHINESE
     * @brief 探针类型枚举
     * \endif
     */
    enum ProbeType {
        VerticalProbe = 0,   ///< 垂直探针，拾取指定X位置的Y值
        HorizontalProbe      ///< 水平探针，拾取指定Y位置的X值
    };
    Q_ENUM(ProbeType)

    /**
     * \if ENGLISH
     * @brief Label position enumeration
     * \endif
     * \if CHINESE
     * @brief 标签位置枚举
     * \endif
     */
    enum LabelPosition {
        LabelAtTop,          ///< 标签在顶部（垂直探针）或左侧（水平探针）
        LabelAtBottom        ///< 标签在底部（垂直探针）或右侧（水平探针）
    };
    Q_ENUM(LabelPosition)

    /**
     * \if ENGLISH
     * @brief Captured data point structure
     * \endif
     * \if CHINESE
     * @brief 捕获的数据点结构
     * \endif
     */
    struct CapturedData {
        QwtPlotItem* item;       ///< 对应的曲线项
        QPointF point;           ///< 捕获的点坐标
        size_t index;            ///< 在曲线数据中的索引
    };

public:
    // Constructor with probe type
    explicit DADataProbeMarker(ProbeType type, const QString& name = QString());
    // Constructor with QwtText title
    DADataProbeMarker(ProbeType type, const QwtText& title);
    // Destructor
    virtual ~DADataProbeMarker();

    // Get probe type
    ProbeType probeType() const;

    // Get/set probe name
    QString probeName() const;
    void setProbeName(const QString& name);

    // Get probe value (x for vertical, y for horizontal)
    double probeValue() const;
    void setProbeValue(double val);

    // Get position as QPointF
    QPointF probePosition() const;

    // Label position
    void setLabelPosition(LabelPosition pos);
    LabelPosition labelPosition() const;

    // Label visibility
    void setLabelVisible(bool visible);
    bool isLabelVisible() const;

    // Set label format (e.g., "%.2f")
    void setLabelFormat(const QString& format);
    QString labelFormat() const;

    // ========== 数据拾取功能（参考QwtPlotSeriesDataPicker） ==========

    /**
     * \if ENGLISH
     * @brief Capture data at current probe position
     * @param interpolate Whether to use interpolation
     * @return Number of captured data points
     * \endif
     *
     * \if CHINESE
     * @brief 在当前探针位置捕获数据
     * @param interpolate 是否使用插值
     * @return 捕获的数据点数量
     * \endif
     */
    int captureData(bool interpolate = true);

    /**
     * \if ENGLISH
     * @brief Get captured data points
     * @return List of captured data points
     * \endif
     *
     * \if CHINESE
     * @brief 获取捕获的数据点
     * @return 捕获的数据点列表
     * \endif
     */
    QList<CapturedData> capturedData() const;

    /**
     * \if ENGLISH
     * @brief Clear captured data
     * \endif
     *
     * \if CHINESE
     * @brief 清除捕获的数据
     * \endif
     */
    void clearCapturedData();

    // Override draw method to customize label rendering
    virtual void draw(QPainter* painter,
                      const QwtScaleMap& xMap,
                      const QwtScaleMap& yMap,
                      const QRectF& canvasRect) const override;

    // 要为探针设置一个单独的rtti，以便识别
    virtual int rtti() const override;
Q_SIGNALS:
    void probeNameChanged(const QString& name);
    void probeValueChanged(double value);
    void probePositionChanged(const QPointF& pos);
    void dataCaptured(const QList<CapturedData>& data);

private:
    // 内部数据拾取方法（参考QwtPlotSeriesDataPicker::pickYValue）
    int pickYValueAtPosition(double xValue, bool interpolate);
    int pickXValueAtPosition(double yValue, bool interpolate);

    void updateLabel();
    void updateLineStyle();
};
```

### 3.2 DAFigureWidget 扩展设计

#### 3.2.1 新增枚举值

在 `DAFigureWidget::ChartEditorType` 中添加：

```cpp
enum ChartEditorType {
    // ... existing values ...
    ArrowMarker,
    VerticalDataProbe,      ///< 垂直数据探针 (新增)
    HorizontalDataProbe,    ///< 水平数据探针 (新增)
    BuilinEditorCount,
    UserDefineEditor = 1000
};
```

#### 3.2.2 新增接口

QwtPlot可以获取所有的QwtPlotItem，因此DAFigureWidget无需保存探针的指针，仅需提供一些便捷创建的接口即可

```cpp
class DAFIGURE_API DAFigureWidget : public QScrollArea
{
    // ... existing code ...

public:
    // ========== 数据探针相关接口 ==========

    /**
     * \if ENGLISH
     * @brief Create a vertical data probe at specified x value
     * @param xValue The x-axis value for the probe
     * @param name Optional custom name for the probe
     * @return Pointer to the created probe
     * \endif
     *
     * \if CHINESE
     * @brief 在指定x值位置创建垂直数据探针
     * @param xValue 探针的x轴值
     * @param name 探针的自定义名称（可选）
     * @return 创建的探针指针
     * \endif
     */
    DADataProbeMarker* createVerticalProbe(double xValue, const QString& name = QString());

    /**
     * \if ENGLISH
     * @brief Create a horizontal data probe at specified y value
     * @param yValue The y-axis value for the probe
     * @param name Optional custom name for the probe
     * @return Pointer to the created probe
     * \endif
     *
     * \if CHINESE
     * @brief 在指定y值位置创建水平数据探针
     * @param yValue 探针的y轴值
     * @param name 探针的自定义名称（可选）
     * @return 创建的探针指针
     * \endif
     */
    DADataProbeMarker* createHorizontalProbe(double yValue, const QString& name = QString());

    /**
     * \if ENGLISH
     * @brief Remove all data probes from the current chart
     * \endif
     *
     * \if CHINESE
     * @brief 删除当前图表中的所有数据探针
     * \endif
     */
    void removeAllProbes();

    /**
     * \if ENGLISH
     * @brief Get all data probes
     * @return List of all data probes
     * \endif
     *
     * \if CHINESE
     * @brief 获取所有数据探针
     * @return 所有数据探针列表
     * \endif
     */
    QList<DADataProbeMarker*> getProbes() const;

    /**
     * \if ENGLISH
     * @brief Get data probe by name
     * @param name The probe name to search for
     * @return Pointer to the probe, or nullptr if not found
     * \endif
     *
     * \if CHINESE
     * @brief 通过名称获取数据探针
     * @param name 要搜索的探针名称
     * @return 探针指针，如果未找到则返回nullptr
     * \endif
     */
    DADataProbeMarker* getProbeByName(const QString& name) const;

    /**
     * \if ENGLISH
     * @brief Rename a data probe
     * @param probe The probe to rename
     * @param newName The new name
     * @return true if successful, false if name already exists
     * \endif
     *
     * \if CHINESE
     * @brief 重命名数据探针
     * @param probe 要重命名的探针
     * @param newName 新名称
     * @return 成功返回true，名称已存在返回false
     * \endif
     */
    bool renameProbe(DADataProbeMarker* probe, const QString& newName);

    /**
     * \if ENGLISH
     * @brief Start vertical probe creation interaction mode
     * \endif
     *
     * \if CHINESE
     * @brief 开始垂直探针创建交互模式
     * \endif
     */
    void beginVerticalProbeEditor();

    /**
     * \if ENGLISH
     * @brief Start horizontal probe creation interaction mode
     * \endif
     *
     * \if CHINESE
     * @brief 开始水平探针创建交互模式
     * \endif
     */
    void beginHorizontalProbeEditor();


private:
    // 探针命名生成（内部使用）
    QString generateProbeName();
    bool isProbeNameExists(const QString& name) const;
};
```

### 3.3 探针创建工厂函数

在 `DAChartItemCreatInteractor.h` 中添加声明：

```cpp
/**
 * \if ENGLISH
 * @brief Create a vertical data probe at the specified position
 * @param plot The plot to attach the probe to
 * @param pos The position in plot coordinates
 * @return The created probe marker
 * \endif
 *
 * \if CHINESE
 * @brief 在指定位置创建垂直数据探针
 * @param plot 要附加探针的绘图
 * @param pos 绘图坐标中的位置
 * @return 创建的探针标记
 * \endif
 */
DAFIGURE_API DADataProbeMarker* createVerticalDataProbePlotItem(QwtPlot* plot, const QPointF& pos);

/**
 * \if ENGLISH
 * @brief Create a horizontal data probe at the specified position
 * @param plot The plot to attach the probe to
 * @param pos The position in plot coordinates
 * @return The created probe marker
 * \endif
 *
 * \if CHINESE
 * @brief 在指定位置创建水平数据探针
 * @param plot 要附加探针的绘图
 * @param pos 绘图坐标中的位置
 * @return 创建的探针标记
 * \endif
 */
DAFIGURE_API DADataProbeMarker* createHorizontalDataProbePlotItem(QwtPlot* plot, const QPointF& pos);
```

### 3.4 探针命名算法

```cpp
QString DAFigureWidget::generateProbeName()
{
    DA_D(d);
    int counter = d->m_probeNameCounter++;
    
    if (counter < 26) {
        return QChar('A' + counter);
    }
    
    // For names beyond Z: AA, AB, AC, ...
    QString name;
    counter++;  // Adjust for 0-indexing
    
    while (counter > 0) {
        counter--;
        name.prepend(QChar('A' + (counter % 26)));
        counter /= 26;
    }
    
    return name;
}
```

### 3.5 数据拾取实现（参考 QwtPlotSeriesDataPicker）

探针的数据拾取功能参考 `QwtPlotSeriesDataPicker::pickYValue()` 实现：

```cpp
int DADataProbeMarker::pickYValueAtPosition(double xValue, bool interpolate)
{
    QwtPlot* plot = this->plot();
    if (!plot) {
        return 0;
    }
    
    d_ptr->m_capturedData.clear();
    
    // 获取所有曲线（参考QwtPlotSeriesDataPicker的实现）
    const QList<QwtPlot*> plotList = plot->plotList();
    
    for (QwtPlot* oneplot : plotList) {
        const QwtPlotItemList& items = oneplot->itemList();
        for (QwtPlotItem* item : items) {
            if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
                QwtPlotCurve* curve = static_cast<QwtPlotCurve*>(item);
                if (!curve->isVisible() || curve->dataSize() == 0) {
                    continue;
                }
                
                // 使用二分查找定位xValue位置
                // 参考 QwtPlotSeriesDataPicker::pickYValue 的实现
                // ...
                
                // 插值计算（如果启用）
                // ...
                
                // 存储捕获的数据
                CapturedData cd;
                cd.item = curve;
                cd.point = QPointF(xValue, yValue);
                cd.index = index;
                d_ptr->m_capturedData.append(cd);
            }
        }
    }
    
    return d_ptr->m_capturedData.size();
}
```

### 3.6 探针绘制实现

探针需要在绘制时显示名称标签和捕获的数据。通过重写 `draw` 方法实现：

```cpp
void DADataProbeMarker::draw(QPainter* painter,
                              const QwtScaleMap& xMap,
                              const QwtScaleMap& yMap,
                              const QRectF& canvasRect) const
{
    // Call base class to draw the line
    QwtPlotMarker::draw(painter, xMap, yMap, canvasRect);
    
    if (!isLabelVisible()) {
        return;
    }
    
    // Draw probe name label
    QPointF pos = value();
    double x = xMap.transform(pos.x());
    double y = yMap.transform(pos.y());
    
    QString labelText = QString("%1: %2").arg(probeName()).arg(probeValue(), 0, 'f', 2);
    
    // Calculate label position based on probe type and label position setting
    QPointF labelPos = calculateLabelPosition(x, y, canvasRect);
    
    // Draw label background
    // Draw label text
}
```

## 4. 文件结构

### 4.1 新增文件

```
src/DAFigure/
├── DADataProbeMarker.h          # 数据探针标记类头文件
└── DADataProbeMarker.cpp        # 数据探针标记类实现文件
```

### 4.2 修改文件

```
src/DAFigure/
├── DAFigureWidget.h             # 添加探针相关接口声明
├── DAFigureWidget.cpp           # 添加探针相关接口实现
├── DAChartItemCreatInteractor.h # 添加探针创建工厂函数声明
├── DAChartItemCreatInteractor.cpp # 添加探针创建工厂函数实现
└── CMakeLists.txt               # 添加新文件到构建系统
```

### 4.3 文档文件

```
docs/zh/use-guide/plot/
└── data-probe.md                # 数据探针功能说明文档
```

## 5. 开发步骤

### 步骤1：创建探针类基础框架

1. 创建 `DADataProbeMarker.h` 和 `DADataProbeMarker.cpp`
2. 实现基本的探针类框架（继承 `QwtPlotMarker`）
3. 实现探针类型区分（垂直/水平）
4. 实现探针命名机制

### 步骤2：实现数据拾取功能

1. 参考 `QwtPlotSeriesDataPicker::pickYValue()` 实现数据拾取
2. 实现插值计算
3. 实现捕获数据的存储和查询
4. 测试数据拾取准确性
5. 注意，探针针对一个曲线有可能拾取多个点，需要处理多个点的情况

### 步骤3：实现探针绘制

1. 重写 `draw` 方法
2. 实现探针标签绘制
3. 实现标签位置控制
4. 测试探针显示效果

### 步骤4：集成到 DAFigureWidget

1. 修改 `DAFigureWidget.h`，添加探针相关接口
2. 修改 `DAFigureWidget.cpp`，实现探针相关接口
3. 添加 `ChartEditorType` 枚举值
4. 实现探针创建交互模式

### 步骤5：创建探针工厂函数

1. 在 `DAChartItemCreatInteractor` 中添加探针创建工厂函数
2. 集成到现有的编辑器创建流程

具体过程，可由Agent根据需求实现。
