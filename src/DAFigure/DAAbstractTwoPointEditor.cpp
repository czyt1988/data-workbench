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
    bool mIsDrawing { false };
    bool mIsFirstPointSet { false };
    QPointF mFirstPoint { 0, 0 };
    QPointF mSecondPoint { 0, 0 };
    bool mIsPlotEnableZoom { false };  ///< 记录绘图是否允许缩放，在结束的时候还原状态

public:
    /**
     * @brief 构造函数
     * @param p 父对象指针
     */
    PrivateData(DAAbstractTwoPointEditor* p) : q_ptr(p)
    {
    }

    /**
     * @brief 析构函数
     */
    ~PrivateData()
    {
    }

    /**
     * @brief 清除所有状态，重置绘制状态
     */
    void clear()
    {
        mIsDrawing       = false;
        mIsFirstPointSet = false;
        mFirstPoint      = QPointF();
        mSecondPoint     = QPointF();
    }
};

/**
 * @brief 构造函数
 * @param parent 关联的QwtPlot对象
 */
DAAbstractTwoPointEditor::DAAbstractTwoPointEditor(QwtPlot* parent) : DAAbstractChartEditor(parent), DA_PIMPL_CONSTRUCT
{
    setEnabled(true);
}

/**
 * @brief 获取运行时类型标识
 * @return 返回两点编辑器的RTTI值
 */
int DAAbstractTwoPointEditor::rtti() const
{
    return DAAbstractChartEditor::RTTITwoPointEditor;
}

/**
 * @brief 析构函数
 */
DAAbstractTwoPointEditor::~DAAbstractTwoPointEditor()
{
}

/**
 * @brief 鼠标按下事件处理，处理两点选取逻辑
 *
 * 第一次按下记录起点并禁用缩放，第二次按下记录终点并发射信号完成编辑
 * @param e 鼠标事件
 * @return 是否处理了该事件
 */
bool DAAbstractTwoPointEditor::mousePressEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }

    QPoint p   = compat::eventPos(e);
    QPointF pf = invTransform(p);

    if (!d_ptr->mIsDrawing) {
        // 开始绘制
        d_ptr->mIsDrawing       = true;
        d_ptr->mIsFirstPointSet = false;

        // 禁用缩放
        DAChartWidget* chart = qobject_cast< DAChartWidget* >(parent());
        if (chart) {
            d_ptr->mIsPlotEnableZoom = chart->isZoomEnabled();
            if (d_ptr->mIsPlotEnableZoom) {
                chart->enableZoom(false);
            }
        }

        Q_EMIT beginEdit();
    }

    if (!d_ptr->mIsFirstPointSet) {
        // 第一个点，起点
        d_ptr->mFirstPoint      = pf;
        d_ptr->mIsFirstPointSet = true;
        return true;
    } else {
        // 第二个点，终点
        d_ptr->mSecondPoint = pf;

        // 恢复缩放
        if (d_ptr->mIsPlotEnableZoom) {
            DAChartWidget* chart = qobject_cast< DAChartWidget* >(parent());
            if (chart) {
                chart->enableZoom(true);
            }
        }

        // 发射信号
        Q_EMIT twoPointsSelected(d_ptr->mFirstPoint, d_ptr->mSecondPoint);
        Q_EMIT finishedEdit(false);

        // 重置状态
        d_ptr->mIsDrawing       = false;
        d_ptr->mIsFirstPointSet = false;

        return true;
    }
}

/**
 * @brief 鼠标移动事件处理，更新预览
 * @param e 鼠标事件
 * @return 是否处理了该事件
 */
bool DAAbstractTwoPointEditor::mouseMoveEvent(const QMouseEvent* e)
{
    if (!d_ptr->mIsDrawing || !d_ptr->mIsFirstPointSet) {
        return false;
    }

    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }

    QPoint p   = compat::eventPos(e);
    QPointF pf = invTransform(p);

    // 更新预览
    updatePreview({ d_ptr->mFirstPoint, pf });
    return true;
}

/**
 * @brief 鼠标释放事件处理
 *
 * 鼠标释放不需要特殊处理，因为按下事件中已完成处理
 * @param e 鼠标事件
 * @return 始终返回false
 */
bool DAAbstractTwoPointEditor::mouseReleaseEvent(const QMouseEvent* e)
{
    // 鼠标释放事件不需要特殊处理，因为我们在mousePressEvent中已经处理了
    return false;
}

/**
 * @brief 键盘按下事件处理，Escape键取消编辑
 * @param e 键盘事件
 * @return 是否处理了该事件
 */
bool DAAbstractTwoPointEditor::keyPressEvent(const QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        cancel();
        return true;
    }
    return DAAbstractChartEditor::keyPressEvent(e);
}

/**
 * @brief 键盘释放事件处理
 * @param e 键盘事件
 * @return 是否处理了该事件
 */
bool DAAbstractTwoPointEditor::keyReleaseEvent(const QKeyEvent* e)
{
    return DAAbstractChartEditor::keyReleaseEvent(e);
}

/**
 * @brief 获取起点坐标
 * @return 起点的坐标
 */
QPointF DAAbstractTwoPointEditor::getStartPoint() const
{
    return d_ptr->mFirstPoint;
}

/**
 * @brief 获取终点坐标
 * @return 终点的坐标
 */
QPointF DAAbstractTwoPointEditor::getEndPoint() const
{
    return d_ptr->mSecondPoint;
}

/**
 * @brief 获取起点和终点之间的距离
 * @return 两点之间的距离，若任一点为空则返回0
 */
qreal DAAbstractTwoPointEditor::getDistance() const
{
    if (d_ptr->mFirstPoint.isNull() || d_ptr->mSecondPoint.isNull()) {
        return 0.0;
    }
    QLineF line(d_ptr->mFirstPoint, d_ptr->mSecondPoint);
    return line.length();
}

/**
 * @brief 清除编辑状态，重置所有数据
 */
void DAAbstractTwoPointEditor::clear()
{
    d_ptr->clear();
}

/**
 * @brief 取消编辑，清除状态并发射完成信号
 * @return 始终返回true
 */
bool DAAbstractTwoPointEditor::cancel()
{
    clear();
    Q_EMIT finishedEdit(true);
    return true;
}


}  // End Of Namespace DA
