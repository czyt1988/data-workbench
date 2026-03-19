#ifndef DACHARTREGIONSELECTEDITOR_H
#define DACHARTREGIONSELECTEDITOR_H

#include "DAFigureAPI.h"
#include "DAAbstractRegionSelectEditor.h"
#include "DAChartSelectRegionShapeItem.h"
#include "DARegionShapeStrategy.h"
#include <QPolygonF>
#include <memory>

namespace DA
{

/**
 * \if ENGLISH
 * @brief Unified region selection editor supporting rect, ellipse and polygon shapes
 * @details This editor uses strategy pattern to handle different shape types,
 *          and state machine pattern for interaction logic. It consolidates
 *          the functionality of DAChartRectRegionSelectEditor, 
 *          DAChartEllipseRegionSelectEditor, and DAChartPolygonRegionSelectEditor
 *          into a single class.
 *
 * @par Shape Types:
 * - RectShape: Creates rectangular regions through drag interaction
 * - EllipseShape: Creates elliptical regions through drag interaction
 * - PolygonShape: Creates polygon regions through click interaction
 *
 * @par Interaction Modes:
 * - Drag-based (Rect, Ellipse): Press and drag to define the region
 * - Click-based (Polygon): Click to add points, close polygon to complete
 *
 * @par Selection Modes:
 * - SingleSelection: Replace existing selection
 * - AdditionalSelection: Add to existing selection (union)
 * - SubtractionSelection: Remove from existing selection (difference)
 * - IntersectionSelection: Intersect with existing selection
 *
 * @par Example Usage:
 * @code
 * // Create a rectangle region editor
 * auto* editor = new DAChartRegionSelectEditor(plot);
 * editor->setShapeType(DAChartRegionSelectEditor::RectShape);
 * editor->setEnabled(true);
 *
 * // Create an ellipse region editor
 * auto* editor = new DAChartRegionSelectEditor(plot);
 * editor->setShapeType(DAChartRegionSelectEditor::EllipseShape);
 * editor->setEnabled(true);
 *
 * // Create a polygon region editor
 * auto* editor = new DAChartRegionSelectEditor(plot);
 * editor->setShapeType(DAChartRegionSelectEditor::PolygonShape);
 * editor->setEnabled(true);
 * @endcode
 * \endif
 *
 * \if CHINESE
 * @brief 统一的区域选择编辑器，支持矩形、椭圆和多边形形状
 * @details 此编辑器使用策略模式处理不同形状类型，使用状态机模式处理交互逻辑。
 *          它将 DAChartRectRegionSelectEditor、DAChartEllipseRegionSelectEditor
 *          和 DAChartPolygonRegionSelectEditor 的功能整合到一个类中。
 *
 * @par 形状类型：
 * - RectShape：通过拖拽交互创建矩形区域
 * - EllipseShape：通过拖拽交互创建椭圆区域
 * - PolygonShape：通过点击交互创建多边形区域
 *
 * @par 交互模式：
 * - 拖拽模式（矩形、椭圆）：按下并拖拽以定义区域
 * - 点击模式（多边形）：点击添加点，闭合多边形完成
 *
 * @par 选择模式：
 * - SingleSelection：替换现有选区
 * - AdditionalSelection：添加到现有选区（并集）
 * - SubtractionSelection：从现有选区移除（差集）
 * - IntersectionSelection：与现有选区求交集
 *
 * @par 使用示例：
 * @code
 * // 创建矩形区域编辑器
 * auto* editor = new DAChartRegionSelectEditor(plot);
 * editor->setShapeType(DAChartRegionSelectEditor::RectShape);
 * editor->setEnabled(true);
 *
 * // 创建椭圆区域编辑器
 * auto* editor = new DAChartRegionSelectEditor(plot);
 * editor->setShapeType(DAChartRegionSelectEditor::EllipseShape);
 * editor->setEnabled(true);
 *
 * // 创建多边形区域编辑器
 * auto* editor = new DAChartRegionSelectEditor(plot);
 * editor->setShapeType(DAChartRegionSelectEditor::PolygonShape);
 * editor->setEnabled(true);
 * @endcode
 * \endif
 */
class DAFIGURE_API DAChartRegionSelectEditor : public DAAbstractRegionSelectEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartRegionSelectEditor)

public:
    /**
     * \if ENGLISH
     * @brief Shape type enumeration
     * @details Defines the available shape types for region selection.
     * \endif
     *
     * \if CHINESE
     * @brief 形状类型枚举
     * @details 定义区域选择可用的形状类型。
     * \endif
     */
    enum ShapeType
    {
        RectShape = 0,    ///< Rectangle shape (drag-based)
        EllipseShape = 1, ///< Ellipse shape (drag-based)
        PolygonShape = 2  ///< Polygon shape (click-based)
    };
    Q_ENUM(ShapeType)

    /**
     * \if ENGLISH
     * @brief Constructor
     * @param parent The parent QwtPlot widget
     * \endif
     *
     * \if CHINESE
     * @brief 构造函数
     * @param parent 父级 QwtPlot 控件
     * \endif
     */
    explicit DAChartRegionSelectEditor(QwtPlot* parent);

    /**
     * \if ENGLISH
     * @brief Constructor with shape type
     * @param parent The parent QwtPlot widget
     * @param type The initial shape type
     * \endif
     *
     * \if CHINESE
     * @brief 带形状类型的构造函数
     * @param parent 父级 QwtPlot 控件
     * @param type 初始形状类型
     * \endif
     */
    DAChartRegionSelectEditor(QwtPlot* parent, ShapeType type);

    virtual ~DAChartRegionSelectEditor();

    /**
     * \if ENGLISH
     * @brief Set the shape type
     * @param type The shape type to use
     * \endif
     *
     * \if CHINESE
     * @brief 设置形状类型
     * @param type 要使用的形状类型
     * \endif
     */
    void setShapeType(ShapeType type);

    /**
     * \if ENGLISH
     * @brief Get the current shape type
     * @return The current shape type
     * \endif
     *
     * \if CHINESE
     * @brief 获取当前形状类型
     * @return 当前形状类型
     * \endif
     */
    ShapeType getShapeType() const;

    // Get the selected region
    virtual QPainterPath getSelectRegion() const override;

    // Set the selection region
    virtual void setSelectRegion(const QPainterPath& shape) override;

    // Set the selection mode
    virtual void setSelectionMode(const SelectionMode& selectionMode) override;

    // Get the RTTI value
    virtual int rtti() const override;

    // Clear the current selection
    void clear();

    // Cancel the current operation
    virtual bool cancel() override;

    // Take ownership of the plot item
    virtual QwtPlotItem* takeItem() override;

private slots:
    void onItemAttached(QwtPlotItem* item, bool on);

protected:
    virtual bool mousePressEvent(const QMouseEvent* e) override;
    virtual bool mouseMoveEvent(const QMouseEvent* e) override;
    virtual bool mouseReleaseEvent(const QMouseEvent* e) override;
    virtual bool keyPressEvent(const QKeyEvent* e) override;

private:
    // Drag-based interaction handlers
    bool handleDragMousePress(const QMouseEvent* e);
    bool handleDragMouseMove(const QMouseEvent* e);
    bool handleDragMouseRelease(const QMouseEvent* e);

    // Polygon interaction handlers
    bool handlePolygonMousePress(const QMouseEvent* e);
    bool handlePolygonMouseMove(const QMouseEvent* e);

    // Complete the current region
    bool completeDragRegion(const QPointF& endPoint);
    bool completePolygonRegion();

    // Backspace for polygon
    bool backspacePolygonRegion();

    // Merge painter path based on selection mode
    void mergeSelectionPath(const QPainterPath& newPath);

    // Update strategy when shape type changes
    void updateStrategy();

    // Get the current strategy (cached)
    DARegionShapeStrategy* strategy() const;
};

}  // namespace DA

#endif  // DACHARTREGIONSELECTEDITOR_H
