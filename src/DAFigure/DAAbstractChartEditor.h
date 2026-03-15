#ifndef DAABSTRACTCHARTEDITOR_H
#define DAABSTRACTCHARTEDITOR_H
#include "DAFigureAPI.h"
#include <QObject>
#include "qwt_plot.h"

namespace DA
{
class DAChartWidget;

/**
 * @brief 绘图编辑器
 *
 * 此编辑器会屏蔽长按空格的keyPressEvent和keyReleaseEvent
 */
class DAFIGURE_API DAAbstractChartEditor : public QObject
{
    Q_OBJECT
    friend class DAFigureChartEditorWidgetOverlay;

public:
    DAAbstractChartEditor(QwtPlot* parent);
    virtual ~DAAbstractChartEditor();
    enum RTTI
    {
        RTTIRectRegionSelectEditor,     ///< 用于给图标添加矩形选框，对应DAChartRectRegionSelectEditor
        RTTIEllipseRegionSelectEditor,  ///< 用于给图标添加椭圆选框，对应DAChartEllipseRegionSelectEditor
        RTTIPolygonRegionSelectEditor,  ///< 用于给图标添加多边形选框，对应DAChartPolygonRegionSelectEditor
        RTTICreatInteractor,            ///< 用于创建图表项，对应DAChartItemCreatInteractor
        RTTIArrowEditor,                ///< 用于给图表添加箭头，对应DAChartArrowEditor
        RTTILineEditor,                 ///< 用于给图表添加线段，对应DAChartLineEditor
        RTTITwoPointEditor,             ///< 两点编辑器基类，对应DAAbstractTwoPointEditor
    };

    const QwtPlot* plot() const;
    QwtPlot* plot();
    const DAChartWidget* chart() const;
    DAChartWidget* chart();
    // 是否启用编辑器
    virtual void setEnabled(bool on);
    virtual bool isEnabled() const;
    virtual int rtti() const = 0;
    // 取消编辑
    virtual bool cancel();
    // 设置屏蔽按键，屏蔽按键将不触发keyPressEvent和keyReleaseEvent
    void setBlockKeys(const QList< int >& keys);
    const QList< int >& getBlockKeys() const;
    // 把绘图QwtPlot的坐标映射到canvas上面
    QPoint mapPlotPosToCanvasPos(const QPoint& pos) const;

    /**
     * @brief 获取当前编辑器对应的plot item
     *
     * 这是当前编辑器对应的plot item，用于在编辑过程中添加、删除、修改item
     *
     * 这个函数一般是编辑器结束后调用，用于获取临时项
     *
     * 此函数调用后编辑器维护的临时对象应该清理，代表所有权已经移交给绘图
     * @return QwtPlotItem*
     */
    virtual QwtPlotItem* takeItem() = 0;

        /**
     * @brief 屏幕坐标转换为数据坐标
     * @param pos 屏幕坐标
     * @return 数据坐标
     */
    QPointF invTransform(const QPointF& pos) const;

    /**
     * @brief 数据坐标转换为屏幕坐标
     * @param pos 数据坐标
     * @return 屏幕坐标
     */
    QPointF transform(const QPointF& pos) const;
Q_SIGNALS:
    /**
     * @brief 开始编辑
     */
    void beginEdit();

    /**
     * @brief 结束编辑
     * @param isCancel
     */
    void finishedEdit(bool isCancel);

protected:
    virtual bool eventFilter(QObject* object, QEvent* event);
    virtual bool mousePressEvent(const QMouseEvent* e);
    virtual bool mouseMoveEvent(const QMouseEvent* e);
    virtual bool mouseReleaseEvent(const QMouseEvent* e);
    virtual bool keyPressEvent(const QKeyEvent* e);
    virtual bool keyReleaseEvent(const QKeyEvent* e);

private:
    bool m_isEnable;           ///< 是否生效
    QList< int > m_blockKeys;  ///< 屏蔽按键
};
}  // End Of Namespace DA
#endif  // DAABSTRACTPLOTEDITOR_H
