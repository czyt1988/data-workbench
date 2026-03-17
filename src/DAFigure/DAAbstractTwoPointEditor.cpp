#include "DAAbstractTwoPointEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QKeyEvent>
#include <QPainterPath>
#include "DAChartWidget.h"
#include "da_qt5qt6_compat.hpp"
#include "qwt_plot.h"
#include "qwt_scale_map.h"
namespace DA
{
class DAAbstractTwoPointEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAAbstractTwoPointEditor)
public:
    bool m_isDrawing { false };
    bool m_isFirstPointSet { false };
    QPointF m_firstPoint { 0, 0 };
    QPointF m_secondPoint { 0, 0 };
    bool m_isPlotEnableZoom { false };  ///< 记录绘图是否允许缩放，在结束的时候还原状态

public:
    PrivateData(DAAbstractTwoPointEditor* p) : q_ptr(p)
    {
    }

    ~PrivateData()
    {
    }

    void clear()
    {
        m_isDrawing       = false;
        m_isFirstPointSet = false;
        m_firstPoint      = QPointF();
        m_secondPoint     = QPointF();
    }
};

DAAbstractTwoPointEditor::DAAbstractTwoPointEditor(QwtPlot* parent) : DAAbstractChartEditor(parent), DA_PIMPL_CONSTRUCT
{
    setEnabled(true);
}

int DAAbstractTwoPointEditor::rtti() const
{
    return DAAbstractChartEditor::RTTITwoPointEditor;
}

DAAbstractTwoPointEditor::~DAAbstractTwoPointEditor()
{
}

bool DAAbstractTwoPointEditor::mousePressEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }

    QPoint p   = compat::eventPos(e);
    QPointF pf = invTransform(p);

    if (!d_ptr->m_isDrawing) {
        // 开始绘制
        d_ptr->m_isDrawing       = true;
        d_ptr->m_isFirstPointSet = false;

        // 禁用缩放
        DAChartWidget* chart = qobject_cast< DAChartWidget* >(parent());
        if (chart) {
            d_ptr->m_isPlotEnableZoom = chart->isZoomEnabled();
            if (d_ptr->m_isPlotEnableZoom) {
                chart->enableZoom(false);
            }
        }

        Q_EMIT beginEdit();
    }

    if (!d_ptr->m_isFirstPointSet) {
        // 第一个点，起点
        d_ptr->m_firstPoint      = pf;
        d_ptr->m_isFirstPointSet = true;
        return true;
    } else {
        // 第二个点，终点
        d_ptr->m_secondPoint = pf;

        // 恢复缩放
        if (d_ptr->m_isPlotEnableZoom) {
            DAChartWidget* chart = qobject_cast< DAChartWidget* >(parent());
            if (chart) {
                chart->enableZoom(true);
            }
        }

        // 发射信号
        Q_EMIT twoPointsSelected(d_ptr->m_firstPoint, d_ptr->m_secondPoint);
        Q_EMIT finishedEdit(false);

        // 重置状态
        d_ptr->m_isDrawing       = false;
        d_ptr->m_isFirstPointSet = false;

        return true;
    }
}

bool DAAbstractTwoPointEditor::mouseMoveEvent(const QMouseEvent* e)
{
    if (!d_ptr->m_isDrawing || !d_ptr->m_isFirstPointSet) {
        return false;
    }

    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }

    QPoint p   = compat::eventPos(e);
    QPointF pf = invTransform(p);

    // 更新预览
    updatePreviewItem(d_ptr->m_firstPoint, pf);
    return true;
}

bool DAAbstractTwoPointEditor::mouseReleaseEvent(const QMouseEvent* e)
{
    // 鼠标释放事件不需要特殊处理，因为我们在mousePressEvent中已经处理了
    return false;
}

bool DAAbstractTwoPointEditor::keyPressEvent(const QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        cancel();
        return true;
    }
    return DAAbstractChartEditor::keyPressEvent(e);
}

bool DAAbstractTwoPointEditor::keyReleaseEvent(const QKeyEvent* e)
{
    return DAAbstractChartEditor::keyReleaseEvent(e);
}

QPointF DAAbstractTwoPointEditor::getStartPoint() const
{
    return d_ptr->m_firstPoint;
}

QPointF DAAbstractTwoPointEditor::getEndPoint() const
{
    return d_ptr->m_secondPoint;
}

qreal DAAbstractTwoPointEditor::getDistance() const
{
    if (d_ptr->m_firstPoint.isNull() || d_ptr->m_secondPoint.isNull()) {
        return 0.0;
    }
    QLineF line(d_ptr->m_firstPoint, d_ptr->m_secondPoint);
    return line.length();
}

void DAAbstractTwoPointEditor::clear()
{
    d_ptr->clear();
}

bool DAAbstractTwoPointEditor::cancel()
{
    clear();
    Q_EMIT finishedEdit(true);
    return true;
}


/**
 * @brief 更新预览项
 *
 * 用于更新预览项，主要在鼠标移动事件中调用
 *
 * 在DAAbstractTwoPointEditor中此方法没有任何实现，如果需要实时更新预览项，需要在子类中实现此方法
 * @param startPoint 起点
 * @param currentPoint 当前点
 */
void DAAbstractTwoPointEditor::updatePreviewItem(const QPointF& startPoint, const QPointF& currentPoint)
{
    Q_UNUSED(startPoint);
    Q_UNUSED(currentPoint);
}

}  // End Of Namespace DA