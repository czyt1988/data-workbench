﻿#ifndef DAABSTRACTREGIONSELECTEDITOR_H
#define DAABSTRACTREGIONSELECTEDITOR_H
#include "DAFigureAPI.h"
#include "DAAbstractPlotEditor.h"
#include <QPainterPath>
class QwtPlotShapeItem;
namespace DA
{
class DAFIGURE_API DAAbstractRegionSelectEditor : public DAAbstractPlotEditor
{
    Q_OBJECT

public:
    /**
     * @brief 选择模式
     */
    enum SelectionMode
    {
        SingleSelection,       ///< 单选
        AdditionalSelection,   ///< 合并选区
        SubtractionSelection,  ///< 减去选区
        IntersectionSelection  ///<  交集选区
    };
    Q_ENUM(SelectionMode)
public:
    DAAbstractRegionSelectEditor(QwtPlot* parent);
    virtual ~DAAbstractRegionSelectEditor();
    virtual SelectionMode getSelectionMode() const;
    virtual void setSelectionMode(const SelectionMode& selectionMode);
    //获取选择的数据区域
    virtual QPainterPath getSelectRegion() const = 0;
    //设置选区
    virtual void setSelectRegion(const QPainterPath& shape) = 0;
    //判断点是否在区域里 此算法频繁调用会耗时
    virtual bool isContains(const QPointF& p) const;
    //获取绑定的x轴
    int getXAxis() const;
    //获取绑定的y轴
    int getYAxis() const;
    //设置关联的坐标轴
    void setAxis(int xAxis, int yAxis);
    //屏幕坐标转换为数据坐标
    QPointF invTransform(const QPointF& pos) const;
    //数据坐标转换为屏幕坐标
    QPointF transform(const QPointF& pos) const;
    //把当前区域转换为其它轴系
    QPainterPath transformToOtherAxis(int axisX, int axisY) const;
signals:
    ///
    /// \brief 完成选择发出的信号
    /// \param shape 选区的区域
    ///
    void finishSelection(const QPainterPath& shape);

private:
    SelectionMode m_selectionMode;  ///< 选框类型
    int m_xAxis;
    int m_yAxis;
};
}  // End Of Namespace DA
#endif  // DAABSTRACTREGIONSELECTEDITOR_H
