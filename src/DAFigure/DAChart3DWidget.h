#ifndef DACHART3DWIDGET_H
#define DACHART3DWIDGET_H

#include "DAFigureAPI.h"

// Qwt3D includes
#include "qwt3d_plot.h"
#include "qwt3d_plotitem.h"
#include "qwt3d_surface.h"
#include "qwt3d_bar.h"
#include "qwt3d_line3d.h"
#include "qwt3d_theme.h"
#include "qwt3d_types.h"
#include "qwt3d_coordsys.h"
#include "qwt3d_colorlegend.h"

// Qt includes
#include <QVector>
#include <QColor>
#include <QBrush>
#include <QString>

class QwtFigure;

namespace DA
{
class DAFigureWidget;

/**
 * @brief 3D图表控件，继承Qwt3DPlot
 */
class DAFIGURE_API DAChart3DWidget : public Qwt3DPlot
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChart3DWidget)
public:
    /**
     * @brief 3D图表属性变化标志
     */
    enum Chart3DPropertyChangeFlag
    {
        NoChange = 0x0,
        // 标题与背景
        ChartTitleChanged  = 0x1,
        BackgroundChanged  = 0x2,
        // 视图变换
        ViewRotationChanged = 0x10,
        ViewShiftChanged    = 0x20,
        ViewScaleChanged   = 0x40,
        ViewZoomChanged    = 0x80,
        ProjectionChanged  = 0x100,
        AspectRatioChanged = 0x200,
        // 坐标轴与坐标系
        AxisLabelChanged    = 0x1000,
        AxisColorChanged    = 0x2000,
        AxisScaleChanged    = 0x4000,
        CoordSysStyleChanged = 0x8000,
        // 网格
        GridChanged          = 0x10000,
        InteriorGridChanged  = 0x20000,
        // 颜色图例
        ColorLegendEnabledChanged  = 0x40000,
        ColorLegendPositionChanged = 0x80000,
        // 主题与光照
        ThemeChanged          = 0x100000,
        LightingStateChanged  = 0x200000,
        LightingPresetChanged = 0x400000,
        // 交互
        MouseStateChanged    = 0x1000000,
        KeyboardStateChanged = 0x2000000,
    };
    Q_ENUM(Chart3DPropertyChangeFlag)
    Q_DECLARE_FLAGS(Chart3DPropertyChangeFlags, Chart3DPropertyChangeFlag)
public:
    explicit DAChart3DWidget(QWidget* parent = nullptr);
    ~DAChart3DWidget() override;

    // ==================== 3D数据管理 ====================

    /// 获取所有数据相关的3D RTTI
    QList< int > data3DRttis() const;

    /// 添加曲面图
    Qwt3DSurface* addSurface(double** data, unsigned int columns, unsigned int rows,
                             double minx, double maxx, double miny, double maxy,
                             const QString& title = QString());

    /// 添加3D柱状图（1D series）
    Qwt3DBar* addBar3D(const QVector< QwtPoint3D >& samples,
                       const QString& title = QString());

    /// 添加3D柱状图（2D grid）
    Qwt3DBar* addBar3D(double** z, int columns, int rows,
                       double minX, double maxX, double minY, double maxY,
                       const QString& title = QString());

    /// 添加3D线图
    Qwt3DLine* addLine3D(const QVector< QwtPoint3D >& samples,
                         const QString& title = QString());

    /// 获取所有曲面item
    QList< Qwt3DSurface* > getSurfaces() const;

    /// 获取所有3D柱状图item
    QList< Qwt3DBar* > getBars3D() const;

    /// 获取所有3D线图item
    QList< Qwt3DLine* > getLines3D() const;

    /// 删除指定的3D plot item
    void removePlot3DItem(Qwt3DPlotItem* item);

    /// 删除所有数据相关的3D item
    void clearAllData3D();

    /// 获取所有item的包围盒
    ParallelEpiped getDataBounds3D() const;

    /// 是否有数据item
    bool hasData3D() const;

    // ==================== 3D样式管理 ====================

    // --- 标题与背景 ---
    void setChart3DTitle(const QString& title);
    QString getChart3DTitle() const;

    void set3DBackgroundColor(const QColor& color);
    QColor get3DBackgroundColor() const;

    // --- 坐标轴 ---
    void set3DAxisLabel(AXIS axis, const QString& label);
    QString get3DAxisLabel(AXIS axis) const;

    void set3DAxesColor(const QColor& color);
    QColor get3DAxesColor();

    void set3DAxisScale(AXIS axis, SCALETYPE type);
    void set3DAutoScale(bool on = true);

    // --- 坐标系 ---
    void setCoordSysStyle(COORDSTYLE style);
    COORDSTYLE getCoordSysStyle();

    // --- 网格 ---
    void enable3DGrid(bool majors = true, bool minors = false, int sides = NOSIDEGRID);
    bool is3DGridEnabled();

    void enable3DInteriorGrid(bool majors = true, bool minors = false, int directions = NO_INTERIOR);
    bool is3DInteriorGridEnabled();

    void set3DGridLinesColor(const QColor& color);
    void set3DInteriorGridLinesColor(const QColor& color);

    // --- 颜色图例 ---
    void enable3DColorLegend(bool enable = true);
    bool is3DColorLegendEnabled() const;
    void set3DColorLegendPosition(Qwt3DColorLegend::Position pos);
    Qwt3DColorLegend::Position get3DColorLegendPosition();

    // --- 主题 ---
    void apply3DTheme(Qwt3DTheme::Preset preset);
    void apply3DTheme(const QString& presetName);
    Qwt3DTheme get3DTheme() const;

    // ==================== 3D交互管理 ====================

    // --- 视图控制 ---
    void resetView();
    void setViewRotation(double xVal, double yVal, double zVal);
    void setViewShift(double xVal, double yVal, double zVal);
    void setViewScale(double xVal, double yVal, double zVal);
    void setViewZoom(double zoom);
    void setProjection(bool ortho);
    void setAspectRatioMode(ASPECTRATIOMODE mode);

    // --- 光照 ---
    void enable3DLighting(bool enable = true);
    bool is3DLightingEnabled() const;
    void apply3DLightingPreset(Qwt3DTheme::LightingPreset preset);

    // --- 鼠标/键盘 ---
    void enable3DMouse(bool enable = true);
    bool is3DMouseEnabled() const;
    void enable3DKeyboard(bool enable = true);
    bool is3DKeyboardEnabled() const;

    // ==================== attach/detach override（总纲§2.10） ====================
    void attach(Qwt3DPlotItem* item) override;
    void detach(Qwt3DPlotItem* item) override;

    // ==================== 工具函数 ====================
    QwtFigure* figure() const;
    DAFigureWidget* figureWidget() const;
    void notifyPropertiesChanged(Chart3DPropertyChangeFlags flag);

Q_SIGNALS:
    void chart3DPropertiesChanged(DA::DAChart3DWidget* chart3d,
                                  DA::DAChart3DWidget::Chart3DPropertyChangeFlags flag);
    // item挂载/卸载通知（on=true表示attach，false表示detach）
    void plot3DItemAttached(Qwt3DPlotItem* item, bool on);
};

}  // namespace DA

Q_DECLARE_OPERATORS_FOR_FLAGS(DA::DAChart3DWidget::Chart3DPropertyChangeFlags)

#endif  // DACHART3DWIDGET_H
