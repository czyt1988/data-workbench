#ifndef DAPAINTERPROXY_H
#define DAPAINTERPROXY_H
#include "DAPyWorkFlowAPI.h"
#include "DAPybind11InQt.h"
#include <QRectF>

class QPainter;

namespace DA
{

/**
 * @brief QPainter的Python代理类
 *
 * 将QPainter基本绘制操作暴露给Python，使Python定义的节点可以自定义视觉渲染。
 * 此类不是QObject，仅持有QPainter指针的引用（不拥有所有权），
 * 所有操作直接转发到底层QPainter。
 *
 * @note paint_callback应在50ms内完成绘制，避免阻塞GUI线程
 * @note 此类仅在paintBody()调用期间有效，不应在paint回调外保存引用
 *
 * @see DAPyNodeGraphicsItem DAPyGILGuard
 */
class DAPYWORKFLOW_API DAPyPainterProxy
{
public:
    // 构造（绑定QPainter指针，不拥有所有权）
    explicit DAPyPainterProxy(QPainter* painter);
    ~DAPyPainterProxy();

    // 判断是否绑定有效的QPainter
    bool isValid() const;

    // 绘制矩形
    void drawRect(qreal x, qreal y, qreal w, qreal h);
    // 绘制文本
    void drawText(qreal x, qreal y, const std::string& text);
    // 绘制直线
    void drawLine(qreal x1, qreal y1, qreal x2, qreal y2);
    // 绘制椭圆
    void drawEllipse(qreal x, qreal y, qreal w, qreal h);
    // 填充矩形
    void fillRect(qreal x, qreal y, qreal w, qreal h, int r, int g, int b, int a = 255);

    // 设置画笔颜色（RGBA）
    void setPenColor(int r, int g, int b, int a = 255);
    // 设置画笔宽度
    void setPenWidth(qreal width);
    // 设置画刷颜色（RGBA）
    void setBrushColor(int r, int g, int b, int a = 255);
    // 设置字体
    void setFont(const std::string& family, qreal size);
    // 设置无画笔（Qt::NoPen）
    void setNoPen();
    // 设置无画刷（Qt::NoBrush）
    void setNoBrush();

private:
    QPainter* mPainter;  ///< 底层QPainter指针（不拥有所有权）
};

}  // namespace DA

#endif  // DAPAINTERPROXY_H