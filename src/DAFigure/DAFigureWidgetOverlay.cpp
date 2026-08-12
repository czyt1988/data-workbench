#include "DAFigureWidgetOverlay.h"
#include <QDebug>
namespace DA
{
/**
 * @brief 构造函数
 * @param fig 关联的QwtFigure
 */
DAFigureWidgetOverlay::DAFigureWidgetOverlay(QwtFigure* fig) : QwtFigureWidgetOverlay(fig)
{
}

/**
 * @brief 析构函数
 */
DAFigureWidgetOverlay::~DAFigureWidgetOverlay()
{
}

}
