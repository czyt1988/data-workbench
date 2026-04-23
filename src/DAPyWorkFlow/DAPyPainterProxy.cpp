#include "DAPyPainterProxy.h"
#include <QPainter>
#include <QColor>
#include <QFont>
#include <QDebug>

namespace DA
{

/**
 * @brief 构造DAPyPainterProxy
 *
 * 绑定QPainter指针，此代理不拥有QPainter的所有权。
 * 代理的生命周期应在paint回调函数内，回调结束后代理不再有效。
 *
 * @param[in] painter 要绑定的QPainter指针，可为nullptr（此时isValid()返回false）
 */
DAPyPainterProxy::DAPyPainterProxy(QPainter* painter) : mPainter(painter)
{
}

/**
 * @brief 析构函数
 *
 * 不释放QPainter，仅断开引用。
 */
DAPyPainterProxy::~DAPyPainterProxy()
{
    mPainter = nullptr;
}

/**
 * @brief 判断代理是否绑定有效的QPainter
 *
 * @return 如果mPainter非nullptr返回true，否则返回false
 */
bool DAPyPainterProxy::isValid() const
{
    return (mPainter != nullptr);
}

/**
 * @brief 绘制矩形
 *
 * 在指定位置绘制矩形轮廓，使用当前画笔样式。
 *
 * @param[in] x 矩形左上角x坐标
 * @param[in] y 矩形左上角y坐标
 * @param[in] w 矩形宽度
 * @param[in] h 矩形高度
 */
void DAPyPainterProxy::drawRect(qreal x, qreal y, qreal w, qreal h)
{
    if (!mPainter) {
        return;
    }
    mPainter->drawRect(QRectF(x, y, w, h));
}

/**
 * @brief 绘制文本
 *
 * 在指定位置绘制文本字符串，使用当前字体和画笔颜色。
 *
 * @param[in] x 文本左上角x坐标
 * @param[in] y 文本左上角y坐标
 * @param[in] text 要绘制的文本内容
 */
void DAPyPainterProxy::drawText(qreal x, qreal y, const std::string& text)
{
    if (!mPainter) {
        return;
    }
    mPainter->drawText(QPointF(x, y), QString::fromStdString(text));
}

/**
 * @brief 绘制直线
 *
 * 从起点到终点绘制直线，使用当前画笔样式。
 *
 * @param[in] x1 起点x坐标
 * @param[in] y1 起点y坐标
 * @param[in] x2 终点x坐标
 * @param[in] y2 终点y坐标
 */
void DAPyPainterProxy::drawLine(qreal x1, qreal y1, qreal x2, qreal y2)
{
    if (!mPainter) {
        return;
    }
    mPainter->drawLine(QPointF(x1, y1), QPointF(x2, y2));
}

/**
 * @brief 绘制椭圆
 *
 * 在指定矩形区域内绘制椭圆轮廓，使用当前画笔样式。
 *
 * @param[in] x 椭圆所在矩形左上角x坐标
 * @param[in] y 椭圆所在矩形左上角y坐标
 * @param[in] w 椭圆所在矩形宽度
 * @param[in] h 椭圆所在矩形高度
 */
void DAPyPainterProxy::drawEllipse(qreal x, qreal y, qreal w, qreal h)
{
    if (!mPainter) {
        return;
    }
    mPainter->drawEllipse(QRectF(x, y, w, h));
}

/**
 * @brief 填充矩形
 *
 * 使用指定颜色填充矩形区域，此操作会忽略当前画刷设置，
 * 直接用指定的RGBA颜色填充。
 *
 * @param[in] x 矩形左上角x坐标
 * @param[in] y 矩形左上角y坐标
 * @param[in] w 矩形宽度
 * @param[in] h 矩形高度
 * @param[in] r 红色分量（0-255）
 * @param[in] g 绿色分量（0-255）
 * @param[in] b 蓝色分量（0-255）
 * @param[in] a 透明度分量（0-255），默认255（完全不透明）
 */
void DAPyPainterProxy::fillRect(qreal x, qreal y, qreal w, qreal h, int r, int g, int b, int a)
{
    if (!mPainter) {
        return;
    }
    mPainter->fillRect(QRectF(x, y, w, h), QColor(r, g, b, a));
}

/**
 * @brief 设置画笔颜色
 *
 * 设置QPainter的画笔颜色为指定RGBA值，画笔宽度保持不变。
 *
 * @param[in] r 红色分量（0-255）
 * @param[in] g 绿色分量（0-255）
 * @param[in] b 蓝色分量（0-255）
 * @param[in] a 透明度分量（0-255），默认255
 */
void DAPyPainterProxy::setPenColor(int r, int g, int b, int a)
{
    if (!mPainter) {
        return;
    }
    QPen pen = mPainter->pen();
    pen.setColor(QColor(r, g, b, a));
    mPainter->setPen(pen);
}

/**
 * @brief 设置画笔宽度
 *
 * 设置QPainter的画笔宽度，画笔颜色保持不变。
 *
 * @param[in] width 画笔宽度（像素值）
 */
void DAPyPainterProxy::setPenWidth(qreal width)
{
    if (!mPainter) {
        return;
    }
    QPen pen = mPainter->pen();
    pen.setWidthF(width);
    mPainter->setPen(pen);
}

/**
 * @brief 设置画刷颜色
 *
 * 设置QPainter的画刷颜色为指定RGBA值，画刷样式自动设为SolidPattern。
 *
 * @param[in] r 红色分量（0-255）
 * @param[in] g 绿色分量（0-255）
 * @param[in] b 蓝色分量（0-255）
 * @param[in] a 透明度分量（0-255），默认255
 */
void DAPyPainterProxy::setBrushColor(int r, int g, int b, int a)
{
    if (!mPainter) {
        return;
    }
    mPainter->setBrush(QBrush(QColor(r, g, b, a)));
}

/**
 * @brief 设置字体
 *
 * 设置QPainter的字体族和字号。
 *
 * @param[in] family 字体族名称（如"Arial"、"Microsoft YaHei"）
 * @param[in] size 字体大小（磅值）
 */
void DAPyPainterProxy::setFont(const std::string& family, qreal size)
{
    if (!mPainter) {
        return;
    }
    QFont font(QString::fromStdString(family), size);
    mPainter->setFont(font);
}

/**
 * @brief 设置无画笔
 *
 * 设置QPainter的画笔为Qt::NoPen，后续绘制轮廓操作将不显示边框线。
 */
void DAPyPainterProxy::setNoPen()
{
    if (!mPainter) {
        return;
    }
    mPainter->setPen(Qt::NoPen);
}

/**
 * @brief 设置无画刷
 *
 * 设置QPainter的画刷为Qt::NoBrush，后续填充操作将不填充内部区域。
 */
void DAPyPainterProxy::setNoBrush()
{
    if (!mPainter) {
        return;
    }
    mPainter->setBrush(Qt::NoBrush);
}

}  // namespace DA