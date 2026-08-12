#include "DAAbstractMarkSymbol.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
namespace DA
{
/**
 * @brief 默认构造函数
 */
DAAbstractMarkSymbol::DAAbstractMarkSymbol()
{
}

/**
 * @brief 构造函数，指定路径、画刷和画笔
 * @param path 绘图路径
 * @param brush 画刷
 * @param pen 画笔
 */
DAAbstractMarkSymbol::DAAbstractMarkSymbol(const QPainterPath& path, const QBrush& brush, const QPen& pen)
    : QwtSymbol(path, brush, pen)
{
}

/**
 * @brief 析构函数
 */
DAAbstractMarkSymbol::~DAAbstractMarkSymbol()
{
}
}  // End Of Namespace DA
