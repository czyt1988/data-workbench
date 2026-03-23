#ifndef DAABSTRACTTWOPOINTEDITOR_H
#define DAABSTRACTTWOPOINTEDITOR_H
#include "DAFigureAPI.h"
#include "DAAbstractChartEditor.h"
#include <QPointF>
class QKeyEvent;
namespace DA
{
/**
 * @brief 两点绘图编辑器抽象基类
 *
 * 用于处理需要两个点的绘图操作，如箭头、线段等
 */
class DAFIGURE_API DAAbstractTwoPointEditor : public DAAbstractChartEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAbstractTwoPointEditor)
public:
    explicit DAAbstractTwoPointEditor(QwtPlot* parent);
    virtual ~DAAbstractTwoPointEditor();

    // rtti
    virtual int rtti() const override;

    // 获取起点
    QPointF getStartPoint() const;
    // 获取终点
    QPointF getEndPoint() const;
    // 获取两点之间的距离
    qreal getDistance() const;

    // 清理数据
    void clear();
    // 取消
    virtual bool cancel() override;
Q_SIGNALS:
    /**
     * @brief 两点选择完成信号
     * @param startPoint 起点
     * @param endPoint 终点
     */
    void twoPointsSelected(const QPointF& startPoint, const QPointF& endPoint);

protected:
    virtual bool mousePressEvent(const QMouseEvent* e) override;
    virtual bool mouseMoveEvent(const QMouseEvent* e) override;
    virtual bool mouseReleaseEvent(const QMouseEvent* e) override;
    virtual bool keyPressEvent(const QKeyEvent* e) override;
    virtual bool keyReleaseEvent(const QKeyEvent* e) override;
};
}  // End Of Namespace DA
#endif  // DAABSTRACTTWOPOINTEDITOR_H