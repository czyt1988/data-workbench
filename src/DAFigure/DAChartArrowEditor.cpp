#include "DAChartArrowEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QKeyEvent>
#include <QPainterPath>
#include <QPen>
#include "DAChartWidget.h"
#include "da_qt5qt6_compat.hpp"
#include "qwt_plot.h"
#include "qwt_text.h"

#include <cmath>

namespace DA
{
class DAChartArrowEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartArrowEditor)
public:
    QPen mArrowPen { Qt::black };
    qreal mArrowSize { 8.0 };
    qreal mArrowLineWidth { 1.0 };
    QwtPlotArrowMarker::EndpointStyle mStartEndType { QwtPlotArrowMarker::NoEndpoint };
    QwtPlotArrowMarker::EndpointStyle mEndEndType { QwtPlotArrowMarker::Triangle };

    qreal mArrowLength { 20.0 };
    QwtPlotArrowMarker* mMarker { nullptr };

public:
    /**
     * @brief 构造函数
     * @param p 指向DAChartArrowEditor的指针
     */
    PrivateData(DAChartArrowEditor* p) : q_ptr(p)
    {
    }

    /**
     * @brief 析构函数，分离并释放箭头标记
     */
    ~PrivateData()
    {
        if (mMarker) {
            mMarker->detach();
            delete mMarker;
            mMarker = nullptr;
        }
    }
};

/**
 * @brief 构造函数
 * @param parent 关联的QwtPlot
 */
DAChartArrowEditor::DAChartArrowEditor(QwtPlot* parent) : DAAbstractTwoPointEditor(parent), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构函数
 */
DAChartArrowEditor::~DAChartArrowEditor()
{
}

/**
 * @brief 获取编辑器的RTTI类型标识
 * @return 返回RTTIArrowEditor
 */
int DAChartArrowEditor::rtti() const
{
    return DAAbstractChartEditor::RTTIArrowEditor;
}

/**
 * @brief 设置箭头线宽
 * @param width 线宽
 * @sa getArrowLineWidth
 */
void DAChartArrowEditor::setArrowLineWidth(qreal width)
{
    d_ptr->mArrowLineWidth = width;
}

/**
 * @brief 获取箭头线宽
 * @return 线宽
 * @sa setArrowLineWidth
 */
qreal DAChartArrowEditor::getArrowLineWidth() const
{
    return d_ptr->mArrowLineWidth;
}

/**
 * @brief 设置箭头线画笔
 * @param pen 画笔
 * @sa getArrowLinePen
 */
void DAChartArrowEditor::setArrowLinePen(const QPen& pen)
{
    d_ptr->mArrowPen = pen;
}

/**
 * @brief 获取箭头线画笔
 * @return 画笔
 * @sa setArrowLinePen
 */
QPen DAChartArrowEditor::getArrowLinePen() const
{
    return d_ptr->mArrowPen;
}

/**
 * @brief 设置箭头尺寸
 * @param size 尺寸
 * @sa getArrowSize
 */
void DAChartArrowEditor::setArrowSize(qreal size)
{
    d_ptr->mArrowSize = size;
}

/**
 * @brief 获取箭头尺寸
 * @return 尺寸
 * @sa setArrowSize
 */
qreal DAChartArrowEditor::getArrowSize() const
{
    return d_ptr->mArrowSize;
}

/**
 * @brief 设置箭头起点端点样式
 * @param type 端点样式
 * @sa getStartEndType
 */
void DAChartArrowEditor::setStartEndType(QwtPlotArrowMarker::EndpointStyle type)
{
    d_ptr->mStartEndType = type;
}

/**
 * @brief 获取箭头起点端点样式
 * @return 端点样式
 * @sa setStartEndType
 */
QwtPlotArrowMarker::EndpointStyle DAChartArrowEditor::getStartEndType() const
{
    return d_ptr->mStartEndType;
}

/**
 * @brief 设置箭头终点端点样式
 * @param type 端点样式
 * @sa getEndEndType
 */
void DAChartArrowEditor::setEndEndType(QwtPlotArrowMarker::EndpointStyle type)
{
    d_ptr->mEndEndType = type;
}

/**
 * @brief 获取箭头终点端点样式
 * @return 端点样式
 * @sa setEndEndType
 */
QwtPlotArrowMarker::EndpointStyle DAChartArrowEditor::getEndEndType() const
{
    return d_ptr->mEndEndType;
}

/**
 * @brief 提取箭头标记项，转移所有权给调用者
 * @return 箭头标记项指针
 */
QwtPlotItem* DAChartArrowEditor::takeItem()
{
    QwtPlotItem* item = d_ptr->mMarker;
    d_ptr->mMarker   = nullptr;
    return item;
}

/**
 * @brief 根据起点和终点创建绘图项
 * @param startPoint 起点
 * @param endPoint 终点
 * @return 绘图项指针
 * @sa createArrowMarker
 */
QwtPlotItem* DAChartArrowEditor::createPlotItem(const QPointF& startPoint, const QPointF& endPoint)
{
    return createArrowMarker(startPoint, endPoint);
}

/**
 * @brief 更新预览，根据点集合创建或更新箭头标记
 * @param points 点集合，至少包含起点和终点
 */
void DAChartArrowEditor::updatePreview(const QVector< QPointF >& points)
{
    DA_D(d);
    if (points.size() < 2) {
        return;
    }
    QPointF startPoint = points[ 0 ];
    QPointF endPoint   = points[ 1 ];
    if (!d->mMarker) {
        d->mMarker = createArrowMarker(startPoint, endPoint);
        d->mMarker->attach(plot());
    } else {
        // 更新标记位置
        d->mMarker->setPoints(startPoint, endPoint);
        if(QwtPlot* p = d->mMarker->plot()){
            p->replot();
        }
    }
}


/**
 * @brief 创建箭头标记
 * @param startPoint 起点
 * @param endPoint 终点
 * @return 箭头标记指针
 * @sa createPlotItem
 */
QwtPlotArrowMarker* DAChartArrowEditor::createArrowMarker(const QPointF& startPoint, const QPointF& endPoint)
{
    // 创建标记
    QwtPlotArrowMarker* marker = new QwtPlotArrowMarker();
    marker->setLinePen(getArrowLinePen());
    marker->setTailStyle(getStartEndType());
    marker->setHeadStyle(getEndEndType());
    marker->setTailSize(getArrowSize());
    marker->setPoints(startPoint, endPoint);

    // 返回标记（线条由图表管理）
    return marker;
}


}  // End Of Namespace DA
