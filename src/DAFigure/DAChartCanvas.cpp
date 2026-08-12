#include "DAChartCanvas.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
namespace DA
{

/**
 * @brief 构造函数
 * @param p 关联的QwtPlot
 */
DAChartCanvas::DAChartCanvas(QwtPlot* p) : QwtPlotCanvas(p)
{
}

/**
 * @brief 析构函数
 */
DAChartCanvas::~DAChartCanvas()
{
}

}  // End Of Namespace DA
