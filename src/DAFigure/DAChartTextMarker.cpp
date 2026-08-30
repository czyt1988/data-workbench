#include "DAChartTextMarker.h"
#include <QBrush>
#include <QFont>

namespace DA
{
/**
 * @brief 构造函数
 * @param title 标题（用于绘图操作树节点显示）
 *
 * 默认配置：无线条(NoLine)、文本从锚点向右下展开(AlignLeft|AlignTop)、
 * label为RichText格式
 */
DAChartTextMarker::DAChartTextMarker(const QString& title) : QwtPlotMarker(title)
{
    setLineStyle(QwtPlotMarker::NoLine);
    setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);
    setLabelOrientation(Qt::Horizontal);
    // 默认无背景，由用户按需开启
    QwtText txt;
    txt.setText(QString(), QwtText::RichText);
    txt.setColor(Qt::black);
    setLabel(txt);
}

/**
 * @brief 析构函数
 */
DAChartTextMarker::~DAChartTextMarker()
{
}

/**
 * @brief 运行时类型标识
 * @return RTTI值
 */
int DAChartTextMarker::rtti() const
{
    return Rtti_TextMarker;
}

/**
 * @brief 设置富文本HTML内容
 * @param html 富文本HTML片段
 */
void DAChartTextMarker::setHtmlText(const QString& html)
{
    QwtText txt = label();
    txt.setText(html, QwtText::RichText);
    setLabel(txt);
}

/**
 * @brief 获取富文本HTML内容
 * @return 富文本HTML片段
 */
QString DAChartTextMarker::htmlText() const
{
    return label().text();
}

/**
 * @brief 设置默认字体
 * @param font 字体
 */
void DAChartTextMarker::setDefaultFont(const QFont& font)
{
    QwtText txt = label();
    txt.setFont(font);
    setLabel(txt);
}

/**
 * @brief 获取默认字体
 * @return 字体
 */
QFont DAChartTextMarker::defaultFont() const
{
    return label().font();
}

/**
 * @brief 设置默认文字颜色
 * @param color 颜色
 */
void DAChartTextMarker::setDefaultTextColor(const QColor& color)
{
    QwtText txt = label();
    txt.setColor(color);
    setLabel(txt);
}

/**
 * @brief 获取默认文字颜色
 * @return 颜色
 */
QColor DAChartTextMarker::defaultTextColor() const
{
    return label().color();
}

/**
 * @brief 设置整体背景画刷
 * @param brush 画刷
 */
void DAChartTextMarker::setBackgroundBrush(const QBrush& brush)
{
    QwtText txt = label();
    txt.setBackgroundBrush(brush);
    setLabel(txt);
}

/**
 * @brief 获取整体背景画刷
 * @return 画刷
 */
QBrush DAChartTextMarker::backgroundBrush() const
{
    return label().backgroundBrush();
}

/**
 * @brief 设置背景边框圆角半径
 * @param radius 半径
 */
void DAChartTextMarker::setBorderRadius(double radius)
{
    QwtText txt = label();
    txt.setBorderRadius(radius);
    setLabel(txt);
}

/**
 * @brief 获取背景边框圆角半径
 * @return 半径
 */
double DAChartTextMarker::borderRadius() const
{
    return label().borderRadius();
}

/**
 * @brief 获取锚点位置
 * @return 数据坐标
 */
QPointF DAChartTextMarker::anchorPosition() const
{
    return QPointF(xValue(), yValue());
}

/**
 * @brief 设置锚点位置
 * @param pos 数据坐标
 */
void DAChartTextMarker::setAnchorPosition(const QPointF& pos)
{
    setValue(pos);
}

}  // namespace DA
