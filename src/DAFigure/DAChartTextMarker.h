#ifndef DACHARTTEXTMARKER_H
#define DACHARTTEXTMARKER_H
#include "DAFigureAPI.h"
#include <QPointF>
#include <QString>
#include "qwt_plot_marker.h"
#include "qwt_text.h"

namespace DA
{
/**
 * @brief 文本标注图元
 *
 * 基于 QwtPlotMarker 的富文本标注，锚定在数据坐标上：
 * - setLineStyle(NoLine)：不绘制任何线条，仅渲染 label
 * - setLabelAlignment(AlignLeft|AlignTop)：qwt 对齐语义下文本渲染在锚点左上方
 *   （注意：QwtPlotMarker 的对齐语义与 Qt 控件相反，AlignLeft=文字在锚点左侧，
 *   AlignTop=文字在锚点上方，见 QwtPlotMarker::drawLabel）
 * - label 使用 QwtText::RichText 格式（QTextDocument 渲染），支持
 *   加粗/斜体/下划线/删除线/上下标/字体/文字颜色/文字背景色等
 *
 * @note QwtPlotMarker 不是 QObject 的子类，因此此类不能使用 Qt 信号槽机制。
 */
class DAFIGURE_API DAChartTextMarker : public QwtPlotMarker
{
public:
    // 静态RTTI值，用于序列化和属性面板注册
    static const int Rtti_TextMarker = QwtPlotItem::Rtti_PlotUserItem + 101;

    // 构造函数，锚点默认为(0,0)
    explicit DAChartTextMarker(const QString& title = QString());
    // 析构函数
    virtual ~DAChartTextMarker();

    // 运行时类型标识
    virtual int rtti() const override;

    // 设置富文本HTML内容，并标记为RichText格式
    void setHtmlText(const QString& html);
    // 获取富文本HTML内容
    QString htmlText() const;

    // 设置默认字体（HTML内未显式指定字体的文本使用）
    void setDefaultFont(const QFont& font);
    // 获取默认字体
    QFont defaultFont() const;

    // 设置默认文字颜色（HTML内未显式指定颜色的文本使用）
    void setDefaultTextColor(const QColor& color);
    // 获取默认文字颜色
    QColor defaultTextColor() const;

    // 设置整体背景画刷（QwtText::PaintBackground体系，与HTML内span背景色并存）
    void setBackgroundBrush(const QBrush& brush);
    // 获取整体背景画刷
    QBrush backgroundBrush() const;

    // 设置背景边框圆角半径
    void setBorderRadius(double radius);
    // 获取背景边框圆角半径
    double borderRadius() const;

    // 获取锚点位置（数据坐标）
    QPointF anchorPosition() const;
    // 设置锚点位置（数据坐标）
    void setAnchorPosition(const QPointF& pos);
};

}  // namespace DA

#endif  // DACHARTTEXTMARKER_H
