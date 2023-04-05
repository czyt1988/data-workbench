#ifndef DASHAPEEDITPANNELWIDGET_H
#define DASHAPEEDITPANNELWIDGET_H

#include <QWidget>
#include <QBrush>
#include <QPen>
#include "DACommonWidgetsAPI.h"
namespace Ui
{
class DAShapeEditPannelWidget;
}

namespace DA
{
/**
 * @brief  针对形状的编辑
 *
 * 此窗口会发射两个信号，一个是背景的画刷，一个是边框的画笔
 */
class DACOMMONWIDGETS_API DAShapeEditPannelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DAShapeEditPannelWidget(QWidget* parent = nullptr);
    ~DAShapeEditPannelWidget();
    //获取背景画刷
    QBrush getBackgroundBrush() const;
    //获取边框画笔
    QPen getBorderPen() const;
public slots:
    //设置背景画刷，会发射backgroundColorChanged
    void setBackgroundBrush(const QBrush& b);
    //设置边框画笔
    void setBorderPen(const QPen& v);
signals:
    /**
     * @brief 背景颜色发生变化
     * @param b
     * @param enable 是否允许背景，如果禁止背景，也会发射此信号，但此时此参数为false
     */
    void backgroundBrushChanged(const QBrush& b);
    /**
     * @brief 边框画笔发生变化
     * @param p
     * @param enable 是否允许边框，如果禁止背景，也会发射此信号，但此时此参数为false
     */
    void borderPenChanged(const QPen& p);

private:
    Ui::DAShapeEditPannelWidget* ui;
};
}
#endif  // DASHAPEEDITPANNELWIDGET_H
