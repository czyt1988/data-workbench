#ifndef DAABSTRACTCHART3DITEMSETTINGWIDGET_H
#define DAABSTRACTCHART3DITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
// qwt3d
#include "qwt3d_plot.h"
#include "qwt3d_plotitem.h"

/**
 * @def DAAbstractChart3DItemSettingWidget_ReturnWhenItemNull
 * @brief 如果getPlot3DItem返回空则return，用于槽函数中判断是否设置了item
 */
#ifndef DAAbstractChart3DItemSettingWidget_ReturnWhenItemNull
#define DAAbstractChart3DItemSettingWidget_ReturnWhenItemNull                                                       \
    do {                                                                                                             \
        if (nullptr == getPlot3DItem()) {                                                                            \
            return;                                                                                                  \
        }                                                                                                            \
    } while (0)
#endif

namespace DA
{
/**
 * @brief 3D chart设置的基类，封装3D绘图项的基本操作
 */
class DAGUI_API DAAbstractChart3DItemSettingWidget : public QWidget
{
    Q_OBJECT
public:
    DAAbstractChart3DItemSettingWidget(QWidget* parent = nullptr);
    ~DAAbstractChart3DItemSettingWidget();
    // 设置3D plotitem
    void setPlot3DItem(Qwt3DPlotItem* item);
    Qwt3DPlotItem* getPlot3DItem() const;
    // 判断是否有item
    bool isHaveItem() const;
    // 判断当前item是否是对应的rtti，如果没有item也返回false
    bool checkItemRTTI(int rtti) const;

    Qwt3DPlot* getPlot3D() const;
    // 快捷dynamic_cast转换为别的item
    template< typename T >
    T d_cast()
    {
        return dynamic_cast< T >(mPlot3DItem);
    }
    // 快捷static_cast转换为别的item
    template< typename T >
    T s_cast()
    {
        return static_cast< T >(mPlot3DItem);
    }
    // setPlot3DItem之后调用的虚函数，子类重写以更新界面
    virtual void updateUI(Qwt3DPlotItem* item);
    // 重绘3D — 调用QWidget::update()触发Qwt3DPlot::paintGL()重绘
    void replot();

protected:
    // plot3D设置，此函数在setPlot3DItem中调用
    void setPlot3D(Qwt3DPlot* plot);
public Q_SLOTS:
    // item挂载/卸载槽函数，连接DAChart3DWidget::plot3DItemAttached信号
    virtual void plot3DItemAttached(Qwt3DPlotItem* plotItem, bool on);

protected:
    Qwt3DPlotItem* mPlot3DItem { nullptr };
    QPointer< Qwt3DPlot > mPlot3D { nullptr };
};
}  // namespace DA

#endif  // DAABSTRACTCHART3DITEMSETTINGWIDGET_H
