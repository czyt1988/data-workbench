#include "DATriangleMarkSymbol.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
namespace DA
{
/**
 * @brief 构造三角形标记符号
 * @param clr 符号颜色
 * @param H 三角形高度
 * @param W 三角形宽度
 * @param isReversal 是否反转（倒三角形）
 */
DATriangleMarkSymbol::DATriangleMarkSymbol(QColor clr, int H, int W, bool isReversal) : DAAbstractMarkSymbol()
{
    QPen pen(clr, 1);
    pen.setJoinStyle(Qt::MiterJoin);
    setPen(pen);
    setBrush(QColor(clr.red(), clr.green(), clr.blue(), 30));
    QPainterPath path;
    if (isReversal) {
        path.moveTo(0, 0);
        path.lineTo(-W / 2, H);
        path.lineTo(W / 2, H);
        path.lineTo(0, 0);
    } else {
        path.moveTo(W / 2, -H);
        path.lineTo(-W / 2, -H);
        path.lineTo(0, 0);
        path.lineTo(W / 2, -H);
    }

    setPath(path);
    setPinPointEnabled(true);
    setSize(W, H);
    setPinPoint(QPointF(0, 0));  //设置文字的坐标点，-6是偏移
}

/**
 * @brief 析构函数
 */
DATriangleMarkSymbol::~DATriangleMarkSymbol()
{
}
}  // End Of Namespace DA
