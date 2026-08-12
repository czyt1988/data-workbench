#include "DAChart3DWidget.h"
// Qt includes
#include <QWidget>
#include <QDebug>
#include <QHash>
// Qwt includes
#include "qwt_figure.h"
// DA includes
#include "DAFigureWidget.h"

namespace DA
{

// ==================== 私有实现类 ====================

class DAChart3DWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAChart3DWidget)
public:
    explicit PrivateData(DAChart3DWidget* q);

    void initialize();

    // Qwt3DPlot::coordinates() 无 const 重载，无法在 const getter 中调用，
    // 因此缓存轴标签以保持 get3DAxisLabel() const。
    // chartTitle 和 colorLegendEnabled 不需要缓存——plan-01 已为 Qwt3DPlot
    // 添加了 title() 和 isColorLegendShown() const getter。
    QHash< AXIS, QString > axisLabels;
};

/**
 * @brief 构造PrivateData
 */
DAChart3DWidget::PrivateData::PrivateData(DAChart3DWidget* q) : q_ptr(q)
{
}

/**
 * @brief 初始化3D图表部件
 */
void DAChart3DWidget::PrivateData::initialize()
{
    // 设置白色背景
    q_ptr->setBackgroundColor(Qt2GL(Qt::white));
    // 默认启用鼠标交互
    q_ptr->enableMouse(true);
    // 默认禁用键盘交互
    q_ptr->disableKeyboard(true);
    // 默认关闭光照（用户可按需开启）
    q_ptr->disableLighting(true);
}

// ==================== DAChart3DWidget 实现 ====================

/**
 * @brief 构造DAChart3DWidget
 */
DAChart3DWidget::DAChart3DWidget(QWidget* parent) : Qwt3DPlot(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->initialize();
}

/**
 * @brief 析构DAChart3DWidget
 */
DAChart3DWidget::~DAChart3DWidget()
{
    // unique_ptr 自动清理 PrivateData
    // Qwt3DPlot 析构时会 detach 所有 item
}

// ==================== attach/detach override（总纲§2.10） ====================

/**
 * @brief override attach，在 item 挂载后发出 plot3DItemAttached 信号
 *
 * Qwt3DPlot 没有 QwtPlot::itemAttached 信号，3D item 的 attach/detach
 * 不会自动通知树模型和设置面板。通过 override 拦截并发出信号，
 * 镜像 2D QwtPlot::itemAttached 的通知机制。
 *
 * 调用链：item->attach(plot) → Qwt3DPlotItem::attach(plot) → plot->attach(this)
 * （qwt3d_plotitem.cpp:61）→ Qwt3DPlot::attach 是 virtual，分派到
 * DAChart3DWidget::attach override → 发出信号。
 *
 * 前置条件：计划 01 已将 Qwt3DPlot::attach/detach（qwt3d_plot.h:203-204，plot 侧）改为 virtual。
 * 注意：override 的是 plot 侧方法（Qwt3DPlot::attach/detach），不是 item 侧
 * （Qwt3DPlotItem::attach/detach）。DAChart3DWidget 继承 Qwt3DPlot，只能 override plot 侧。
 */
void DAChart3DWidget::attach(Qwt3DPlotItem* item)
{
    Qwt3DPlot::attach(item);
    Q_EMIT plot3DItemAttached(item, true);
}

/**
 * @brief override detach，在 item 卸载后发出 plot3DItemAttached 信号
 */
void DAChart3DWidget::detach(Qwt3DPlotItem* item)
{
    Qwt3DPlot::detach(item);
    Q_EMIT plot3DItemAttached(item, false);
}

// ==================== 3D数据管理 ====================

/**
 * @brief 获取所有数据相关的3D RTTI
 *
 * 参考 DAChartWidget::dataRttis()（DAChartWidget.cpp:126-135）。
 * 返回 01-qwt3d-enhancement.md 中定义的 RTTI 枚举值。
 * 此函数影响 clearAllData3D() 的行为。
 * @return 涉及数据绘图的 RTTI 列表
 */
QList< int > DAChart3DWidget::data3DRttis() const
{
    QList< int > rttis;
    rttis << Rtti_Plot3DSurface << Rtti_Plot3DBar << Rtti_Plot3DLine;
    return rttis;
}

/**
 * @brief 添加曲面图
 *
 * 参考 DAChartWidget::addCurve()（DAChartWidget.cpp:137-149）：
 * 创建 item → 设置数据 → 设置默认样式 → attach → 返回指针。
 * @param data z 值矩阵（data[col][row]）
 * @param columns 列数
 * @param rows 行数
 * @param minx/maxx/miny/maxy x/y 域范围
 * @param title 标题
 * @return 创建的 Qwt3DSurface 指针
 */
Qwt3DSurface* DAChart3DWidget::addSurface(double** data, unsigned int columns, unsigned int rows,
                                          double minx, double maxx, double miny, double maxy,
                                          const QString& title)
{
    if (!data || columns == 0 || rows == 0) {
        qWarning() << "Invalid data for 3D surface:" << title;
        return nullptr;
    }

    Qwt3DSurface* surface = new Qwt3DSurface();
    if (!title.isEmpty()) {
        surface->setTitle(title);
    }
    surface->loadFromData(data, columns, rows, minx, maxx, miny, maxy);
    surface->setPlotStyle(FILLEDMESH);
    surface->attach(this);
    return surface;
}

/**
 * @brief 添加3D柱状图（1D series）
 *
 * 参考 DAChartWidget::addBarChart()（DAChartWidget.cpp:200-226）。
 * @param samples 3D点数组（x,y 为底面中心，z 为高度）
 * @param title 标题
 * @return 创建的 Qwt3DBar 指针
 */
Qwt3DBar* DAChart3DWidget::addBar3D(const QVector< QwtPoint3D >& samples, const QString& title)
{
    if (samples.isEmpty()) {
        qWarning() << "Empty samples for 3D bar chart:" << title;
        return nullptr;
    }

    Qwt3DBar* bar = new Qwt3DBar();
    if (!title.isEmpty()) {
        bar->setTitle(title);
    }
    bar->setSamples(samples);
    bar->setBarStyle(Qwt3DBar::FilledMesh);
    bar->attach(this);
    return bar;
}

/**
 * @brief 添加3D柱状图（2D grid）
 * @param z z 值矩阵
 * @param columns/rows 网格尺寸
 * @param minX/maxX/minY/maxY x/y 域范围
 * @param title 标题
 * @return 创建的 Qwt3DBar 指针
 */
Qwt3DBar* DAChart3DWidget::addBar3D(double** z, int columns, int rows,
                                     double minX, double maxX, double minY, double maxY,
                                     const QString& title)
{
    if (!z || columns == 0 || rows == 0) {
        qWarning() << "Invalid data for 3D bar chart (grid):" << title;
        return nullptr;
    }

    Qwt3DBar* bar = new Qwt3DBar();
    if (!title.isEmpty()) {
        bar->setTitle(title);
    }
    bar->setSamples(z, columns, rows, minX, maxX, minY, maxY);
    bar->setBarStyle(Qwt3DBar::FilledMesh);
    bar->attach(this);
    return bar;
}

/**
 * @brief 添加3D线图
 *
 * 参考 DAChartWidget::addCurve()（DAChartWidget.cpp:137-149）。
 * @param samples 3D点数组
 * @param title 标题
 * @return 创建的 Qwt3DLine 指针
 */
Qwt3DLine* DAChart3DWidget::addLine3D(const QVector< QwtPoint3D >& samples, const QString& title)
{
    if (samples.isEmpty()) {
        qWarning() << "Empty samples for 3D line:" << title;
        return nullptr;
    }

    Qwt3DLine* line = new Qwt3DLine();
    if (!title.isEmpty()) {
        line->setTitle(title);
    }
    line->setSamples(samples);
    line->setLineStyle(Qwt3DLine::Tube);
    line->attach(this);
    return line;
}

/**
 * @brief 获取所有曲面 item
 *
 * 参考 DAChartWidget::getCurves()（DAChartWidget.cpp:166-178），
 * 遍历 itemList() 并按 RTTI 过滤。
 */
QList< Qwt3DSurface* > DAChart3DWidget::getSurfaces() const
{
    QList< Qwt3DSurface* > surfaces;
    const QList< Qwt3DPlotItem* > items = itemList();
    for (Qwt3DPlotItem* item : items) {
        if (item->rtti() == Rtti_Plot3DSurface) {
            surfaces.append(static_cast< Qwt3DSurface* >(item));
        }
    }
    return surfaces;
}

/**
 * @brief 获取所有3D柱状图 item
 */
QList< Qwt3DBar* > DAChart3DWidget::getBars3D() const
{
    QList< Qwt3DBar* > bars;
    const QList< Qwt3DPlotItem* > items = itemList();
    for (Qwt3DPlotItem* item : items) {
        if (item->rtti() == Rtti_Plot3DBar) {
            bars.append(static_cast< Qwt3DBar* >(item));
        }
    }
    return bars;
}

/**
 * @brief 获取所有3D线图 item
 */
QList< Qwt3DLine* > DAChart3DWidget::getLines3D() const
{
    QList< Qwt3DLine* > lines;
    const QList< Qwt3DPlotItem* > items = itemList();
    for (Qwt3DPlotItem* item : items) {
        if (item->rtti() == Rtti_Plot3DLine) {
            lines.append(static_cast< Qwt3DLine* >(item));
        }
    }
    return lines;
}

/**
 * @brief 删除指定的3D plot item
 *
 * 参考 DAChartWidget::removePlotItem()（DAChartWidget.cpp:422-430）。
 */
void DAChart3DWidget::removePlot3DItem(Qwt3DPlotItem* item)
{
    if (!item) {
        return;
    }
    item->detach();
    delete item;
}

/**
 * @brief 删除所有数据相关的3D item
 *
 * 参考 DAChartWidget::clearAllData()（DAChartWidget.cpp:432-444），
 * 基于 data3DRttis() 返回的 RTTI 列表过滤删除。
 */
void DAChart3DWidget::clearAllData3D()
{
    const QList< Qwt3DPlotItem* > items = itemList();
    const QList< int > rttis = data3DRttis();
    for (Qwt3DPlotItem* item : items) {
        if (rttis.contains(item->rtti())) {
            item->detach();
            delete item;
        }
    }
}

/**
 * @brief 获取所有 item 的包围盒
 *
 * 参考 DAChartWidget::getDataBounds()（DAChartWidget.cpp:446-449）。
 * 利用 Qwt3DPlot::hull() 返回所有 item 的并集包围盒。
 */
ParallelEpiped DAChart3DWidget::getDataBounds3D() const
{
    return hull();
}

/**
 * @brief 是否有数据 item
 *
 * 参考 DAChartWidget::hasData()（DAChartWidget.cpp:451-461）。
 */
bool DAChart3DWidget::hasData3D() const
{
    const QList< int > rttis = data3DRttis();
    const QList< Qwt3DPlotItem* > items = itemList();
    for (const Qwt3DPlotItem* item : items) {
        if (rttis.contains(item->rtti())) {
            return true;
        }
    }
    return false;
}

// ==================== 3D样式管理 ====================

/**
 * @brief 设置图表标题
 *
 * 参考 DAChartWidget::setChartTitle()（DAChartWidget.cpp:465-469）。
 * Qwt3DPlot::title()（plan-01 §2.11 添加）用于 getter，无需 PrivateData 缓存。
 */
void DAChart3DWidget::setChart3DTitle(const QString& title)
{
    setTitle(title);
    notifyPropertiesChanged(ChartTitleChanged);
}

/**
 * @brief 获取图表标题
 *
 * 使用 Qwt3DPlot::title()（const，plan-01 §2.11 添加）获取实时值。
 */
QString DAChart3DWidget::getChart3DTitle() const
{
    return title();
}

/**
 * @brief 设置背景色
 *
 * 参考 DAChartWidget::setBackgroundBrush()（DAChartWidget.cpp:476-484）。
 * Qwt3DPlot 使用 RGBA 而非 QBrush，通过 Qt2GL 转换。
 */
void DAChart3DWidget::set3DBackgroundColor(const QColor& color)
{
    setBackgroundColor(Qt2GL(color));
    notifyPropertiesChanged(BackgroundChanged);
}

/**
 * @brief 获取背景色
 *
 * 使用 Qwt3DPlot::backgroundRGBAColor()（const，qwt3d_plot.h:92）获取实时值，
 * 避免 apply3DTheme() 触发 Qwt3DTheme::apply() 绕过本类 setter 后缓存过期。
 */
QColor DAChart3DWidget::get3DBackgroundColor() const
{
    RGBA bg = backgroundRGBAColor();
    return GL2Qt(bg.r, bg.g, bg.b);
}

/**
 * @brief 设置轴标签
 *
 * 参考 DAChartWidget::setAxisLabel()（DAChartWidget.cpp:505-509）。
 * Qwt3DPlot 通过 coordinates()->axes[axis] 访问 Qwt3DAxis。
 * 同时缓存到 PrivateData，因为 coordinates() 非 const，
 * getter 无法在 const 上下文中调用。
 */
void DAChart3DWidget::set3DAxisLabel(AXIS axis, const QString& label)
{
    DA_D(d);
    d->axisLabels[ axis ] = label;
    Qwt3DCoordinateSystem* coords = coordinates();
    if (coords) {
        coords->axes[ axis ].setLabelString(label);
        notifyPropertiesChanged(AxisLabelChanged);
    }
}

/**
 * @brief 获取轴标签
 *
 * 从 PrivateData 缓存返回。coordinates() 非 const，无法在 const getter 中调用
 * coordinates()->axes[axis].labelString()。
 */
QString DAChart3DWidget::get3DAxisLabel(AXIS axis) const
{
    DA_DC(d);
    return d->axisLabels.value(axis);
}

/**
 * @brief 设置所有轴的颜色
 *
 * 参考 DAChartWidget::setAxisColor()（DAChartWidget.cpp:516-523）。
 */
void DAChart3DWidget::set3DAxesColor(const QColor& color)
{
    Qwt3DCoordinateSystem* coords = coordinates();
    if (coords) {
        coords->setAxesColor(Qt2GL(color));
        notifyPropertiesChanged(AxisColorChanged);
    }
}

/**
 * @brief 获取轴颜色
 *
 * 使用 Qwt3DCoordinateSystem::axesColor()（plan-01 §2.11 添加）获取实时值，
 * 避免 apply3DTheme() 后缓存过期。
 * 非 const：Qwt3DPlot::coordinates() 无 const 重载。
 */
QColor DAChart3DWidget::get3DAxesColor()
{
    Qwt3DCoordinateSystem* coords = coordinates();
    if (!coords) {
        return Qt::black;
    }
    RGBA ac = coords->axesColor();
    return GL2Qt(ac.r, ac.g, ac.b);
}

/**
 * @brief 设置轴刻度类型
 * @param axis 轴索引
 * @param type 刻度类型
 */
void DAChart3DWidget::set3DAxisScale(AXIS axis, SCALETYPE type)
{
    Qwt3DCoordinateSystem* coords = coordinates();
    if (coords) {
        coords->axes[ axis ].setScale(type);
        notifyPropertiesChanged(AxisScaleChanged);
    }
}

/**
 * @brief 设置是否自动缩放
 * @param on 是否启用
 */
void DAChart3DWidget::set3DAutoScale(bool on)
{
    Qwt3DCoordinateSystem* coords = coordinates();
    if (coords) {
        coords->setAutoScale(on);
        notifyPropertiesChanged(AxisScaleChanged);
    }
}

/**
 * @brief 设置坐标系样式
 * @param style 坐标系样式
 */
void DAChart3DWidget::setCoordSysStyle(COORDSTYLE style)
{
    Qwt3DCoordinateSystem* coords = coordinates();
    if (coords) {
        coords->setStyle(style);
        notifyPropertiesChanged(CoordSysStyleChanged);
    }
}

/**
 * @brief 获取坐标系样式
 * @return 坐标系样式
 */
// 非 const：Qwt3DPlot::coordinates() 无 const 重载
COORDSTYLE DAChart3DWidget::getCoordSysStyle()
{
    Qwt3DCoordinateSystem* coords = coordinates();
    return coords ? coords->style() : NOCOORD;
}

/**
 * @brief 启用/禁用网格线
 *
 * 参考 DAChartWidget::enableGrid()（DAChartWidget.cpp:531-544）。
 * Qwt3DCoordinateSystem 的网格通过 setGridLines() 控制。
 */
void DAChart3DWidget::enable3DGrid(bool majors, bool minors, int sides)
{
    Qwt3DCoordinateSystem* coords = coordinates();
    if (coords) {
        coords->setGridLines(majors, minors, sides);
        notifyPropertiesChanged(GridChanged);
    }
}

/**
 * @brief 网格线是否启用
 * @return 是否启用
 */
// 非 const：Qwt3DPlot::coordinates() 无 const 重载
bool DAChart3DWidget::is3DGridEnabled()
{
    Qwt3DCoordinateSystem* coords = coordinates();
    return coords ? (coords->grids() != 0) : false;
}

/**
 * @brief 启用/禁用内部网格线
 * @param majors 是否启用主网格
 * @param minors 是否启用次网格
 * @param directions 方向
 */
void DAChart3DWidget::enable3DInteriorGrid(bool majors, bool minors, int directions)
{
    Qwt3DCoordinateSystem* coords = coordinates();
    if (coords) {
        coords->setInteriorGridLines(majors, minors, directions);
        notifyPropertiesChanged(InteriorGridChanged);
    }
}

/**
 * @brief 内部网格线是否启用
 * @return 是否启用
 */
// 非 const：Qwt3DPlot::coordinates() 无 const 重载
bool DAChart3DWidget::is3DInteriorGridEnabled()
{
    Qwt3DCoordinateSystem* coords = coordinates();
    return coords ? (coords->interiorGrids() != 0) : false;
}

/**
 * @brief 设置网格线颜色
 * @param color 颜色
 */
void DAChart3DWidget::set3DGridLinesColor(const QColor& color)
{
    Qwt3DCoordinateSystem* coords = coordinates();
    if (coords) {
        coords->setGridLinesColor(Qt2GL(color));
        notifyPropertiesChanged(GridChanged);
    }
}

/**
 * @brief 设置内部网格线颜色
 * @param color 颜色
 */
void DAChart3DWidget::set3DInteriorGridLinesColor(const QColor& color)
{
    Qwt3DCoordinateSystem* coords = coordinates();
    if (coords) {
        coords->setInteriorGridLinesColor(Qt2GL(color));
        notifyPropertiesChanged(InteriorGridChanged);
    }
}

/**
 * @brief 启用/禁用颜色图例
 *
 * 参考 DAChartWidget::enableLegend()（DAChartWidget.cpp:649-664）。
 * 使用 Qwt3DPlot::isColorLegendShown()（const，plan-01 §2.11 添加）作为 getter，
 * 无需 PrivateData 缓存。
 */
void DAChart3DWidget::enable3DColorLegend(bool enable)
{
    showColorLegend(enable);
    notifyPropertiesChanged(ColorLegendEnabledChanged);
}

/**
 * @brief 颜色图例是否启用
 *
 * 使用 Qwt3DPlot::isColorLegendShown()（const，plan-01 §2.11 添加）获取实时值。
 */
bool DAChart3DWidget::is3DColorLegendEnabled() const
{
    return isColorLegendShown();
}

/**
 * @brief 设置颜色图例位置
 * @param pos 图例位置
 */
void DAChart3DWidget::set3DColorLegendPosition(Qwt3DColorLegend::Position pos)
{
    setLegendPosition(pos);
    notifyPropertiesChanged(ColorLegendPositionChanged);
}

/**
 * @brief 获取颜色图例位置
 * @return 图例位置
 */
// 非 const：Qwt3DPlot::legend() 无 const 重载
Qwt3DColorLegend::Position DAChart3DWidget::get3DColorLegendPosition()
{
    Qwt3DColorLegend* lg = legend();
    return lg ? lg->position() : Qwt3DColorLegend::PosTopRight;
}

/**
 * @brief 应用主题预设
 *
 * 委托给 Qwt3DPlot::applyTheme() → setTheme() → Qwt3DTheme::apply(this)
 * （qwt3d_theme.cpp:441-508）。
 * @note Qwt3DTheme::apply() 全量应用所有主题属性（背景色、轴色、网格色、
 *       标题字体/颜色、shininess、material 等），直接调用 plot->setBackgroundColor()
 *       / coords->setAxesColor() 等，绕过本类的 setter。因此
 *       get3DBackgroundColor() / get3DAxesColor() 使用实时 getter
 *       （backgroundRGBAColor() / coordinates()->axesColor()）而非 PrivateData 缓存，
 *       确保主题应用后 getter 返回正确值。
 */
void DAChart3DWidget::apply3DTheme(Qwt3DTheme::Preset preset)
{
    applyTheme(preset);
    notifyPropertiesChanged(ThemeChanged);
}

/**
 * @brief 通过名称应用主题预设
 * @param presetName 主题名称
 */
void DAChart3DWidget::apply3DTheme(const QString& presetName)
{
    applyTheme(presetName);
    notifyPropertiesChanged(ThemeChanged);
}

/**
 * @brief 获取当前主题
 * @return 主题对象
 */
Qwt3DTheme DAChart3DWidget::get3DTheme() const
{
    return theme();
}

// ==================== 3D交互管理 ====================

/**
 * @brief 重置视图到默认状态
 *
 * 参考 DAChartWidget::zoomToOriginal()（DAChartWidget.cpp:756-762）。
 */
void DAChart3DWidget::resetView()
{
    setRotation(0.0, 0.0, 0.0);
    setShift(0.0, 0.0, 0.0);
    setViewportShift(0.0, 0.0);
    setScale(1.0, 1.0, 1.0);
    setZoom(1.0);
    notifyPropertiesChanged(ViewRotationChanged | ViewShiftChanged | ViewScaleChanged | ViewZoomChanged);
}

/**
 * @brief 设置视图旋转角度
 * @param xVal X轴旋转角度
 * @param yVal Y轴旋转角度
 * @param zVal Z轴旋转角度
 */
void DAChart3DWidget::setViewRotation(double xVal, double yVal, double zVal)
{
    setRotation(xVal, yVal, zVal);
    notifyPropertiesChanged(ViewRotationChanged);
}

/**
 * @brief 设置视图平移
 * @param xVal X轴平移量
 * @param yVal Y轴平移量
 * @param zVal Z轴平移量
 */
void DAChart3DWidget::setViewShift(double xVal, double yVal, double zVal)
{
    setShift(xVal, yVal, zVal);
    notifyPropertiesChanged(ViewShiftChanged);
}

/**
 * @brief 设置视图缩放比例
 * @param xVal X轴缩放
 * @param yVal Y轴缩放
 * @param zVal Z轴缩放
 */
void DAChart3DWidget::setViewScale(double xVal, double yVal, double zVal)
{
    setScale(xVal, yVal, zVal);
    notifyPropertiesChanged(ViewScaleChanged);
}

/**
 * @brief 设置视图缩放
 * @param zoom 缩放比例
 */
void DAChart3DWidget::setViewZoom(double zoom)
{
    setZoom(zoom);
    notifyPropertiesChanged(ViewZoomChanged);
}

/**
 * @brief 设置投影模式
 * @param ortho 是否为正交投影
 */
void DAChart3DWidget::setProjection(bool ortho)
{
    setOrtho(ortho);
    notifyPropertiesChanged(ProjectionChanged);
}

/**
 * @brief 设置纵横比模式
 * @param mode 纵横比模式
 */
void DAChart3DWidget::setAspectRatioMode(ASPECTRATIOMODE mode)
{
    // 必须限定 Qwt3DPlot::，否则递归调用自身导致栈溢出
    Qwt3DPlot::setAspectRatioMode(mode);
    notifyPropertiesChanged(AspectRatioChanged);
}

/**
 * @brief 启用/禁用光照
 *
 * 参考 DAChartWidget::enableZoom()（DAChartWidget.cpp:734-749）。
 */
void DAChart3DWidget::enable3DLighting(bool enable)
{
    if (enable) {
        enableLighting();
    } else {
        disableLighting();
    }
    notifyPropertiesChanged(LightingStateChanged);
}

/**
 * @brief 光照是否启用
 * @return 是否启用
 */
bool DAChart3DWidget::is3DLightingEnabled() const
{
    return lightingEnabled();
}

/**
 * @brief 应用光照预设
 *
 * @note 此方法调用 setTheme() 触发 Qwt3DTheme::apply(this)（qwt3d_theme.cpp:441-508），
 *       该函数全量应用所有主题属性（背景色、轴色、网格色、标题字体/颜色、
 *       shininess、material 等），不仅仅是光照。如果用户此前调用了
 *       set3DBackgroundColor() 等单独属性修改，这些修改会被重置为 theme 对象
 *       中存储的值。这是 qwt3d 库 Qwt3DTheme::apply() 的设计特性（全量应用），
 *       本类无法绕过。
 */
void DAChart3DWidget::apply3DLightingPreset(Qwt3DTheme::LightingPreset preset)
{
    Qwt3DTheme theme = this->theme();
    theme.setLightingPreset(preset);
    setTheme(theme);
    notifyPropertiesChanged(LightingPresetChanged);
}

/**
 * @brief 启用/禁用鼠标交互
 * @param enable 是否启用
 */
void DAChart3DWidget::enable3DMouse(bool enable)
{
    if (enable) {
        enableMouse();
    } else {
        disableMouse();
    }
    notifyPropertiesChanged(MouseStateChanged);
}

/**
 * @brief 鼠标交互是否启用
 * @return 是否启用
 */
bool DAChart3DWidget::is3DMouseEnabled() const
{
    return mouseEnabled();
}

/**
 * @brief 启用/禁用键盘交互
 * @param enable 是否启用
 */
void DAChart3DWidget::enable3DKeyboard(bool enable)
{
    if (enable) {
        enableKeyboard();
    } else {
        disableKeyboard();
    }
    notifyPropertiesChanged(KeyboardStateChanged);
}

/**
 * @brief 键盘交互是否启用
 * @return 是否启用
 */
bool DAChart3DWidget::is3DKeyboardEnabled() const
{
    return keyboardEnabled();
}

// ==================== 工具函数 ====================

/**
 * @brief 获取所属 QwtFigure
 *
 * 参考 DAChartWidget::figure()（DAChartWidget.cpp:957-967），
 * 向上遍历 parentWidget() 链查找 QwtFigure。
 */
QwtFigure* DAChart3DWidget::figure() const
{
    QWidget* parent = const_cast< DAChart3DWidget* >(this);
    while (QWidget* grandParent = parent->parentWidget()) {
        if (QwtFigure* figure = qobject_cast< QwtFigure* >(grandParent)) {
            return figure;
        }
        parent = grandParent;
    }
    return nullptr;
}

/**
 * @brief 获取所属 DAFigureWidget
 *
 * 参考 DAChartWidget::figureWidget()（DAChartWidget.cpp:969-979），
 * 向上遍历 parentWidget() 链查找 DAFigureWidget。
 */
DAFigureWidget* DAChart3DWidget::figureWidget() const
{
    QWidget* parent = const_cast< DAChart3DWidget* >(this);
    while (QWidget* grandParent = parent->parentWidget()) {
        if (DAFigureWidget* figure = qobject_cast< DAFigureWidget* >(grandParent)) {
            return figure;
        }
        parent = grandParent;
    }
    return nullptr;
}

/**
 * @brief 通知属性变化
 *
 * 参考 DAChartWidget::notifyPropertiesChanged()（DAChartWidget.cpp:981-984）。
 */
void DAChart3DWidget::notifyPropertiesChanged(Chart3DPropertyChangeFlags flag)
{
    Q_EMIT chart3DPropertiesChanged(this, flag);
}

}  // namespace DA
