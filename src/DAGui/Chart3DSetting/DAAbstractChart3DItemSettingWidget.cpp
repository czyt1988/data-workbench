#include "DAAbstractChart3DItemSettingWidget.h"
#include "DAChart3DWidget.h"
#include "qwt3d_plot.h"
#include "qwt3d_plotitem.h"

namespace DA
{
DAAbstractChart3DItemSettingWidget::DAAbstractChart3DItemSettingWidget(QWidget* parent) : QWidget(parent)
{
}

DAAbstractChart3DItemSettingWidget::~DAAbstractChart3DItemSettingWidget()
{
}

/**
 * @brief 设置3D plotitem
 *
 * 如果item有plot，则把plot设置进来
 * @param item
 */
void DAAbstractChart3DItemSettingWidget::setPlot3DItem(Qwt3DPlotItem* item)
{
    mPlot3DItem = item;
    if (item) {
        setPlot3D(item->plot());
    }
    updateUI(item);
}

/**
 * @brief 获取当前设置的3D plotitem
 */
Qwt3DPlotItem* DAAbstractChart3DItemSettingWidget::getPlot3DItem() const
{
    return mPlot3DItem;
}

/**
 * @brief 判断是否有item
 */
bool DAAbstractChart3DItemSettingWidget::isHaveItem() const
{
    return (mPlot3DItem != nullptr);
}

/**
 * @brief 判断当前item是否是对应的rtti
 * @param rtti 3D RTTI值（如Rtti_Plot3DSurface等）
 * @return 匹配返回true，没有item也返回false
 */
bool DAAbstractChart3DItemSettingWidget::checkItemRTTI(int rtti) const
{
    if (!mPlot3DItem) {
        return false;
    }
    return (mPlot3DItem->rtti() == rtti);
}

/**
 * @brief 设置3D plot，用于感知item所在的绘图窗口
 *
 * Qwt3DPlot本身没有itemAttached信号，但DAChart3DWidget
 * 重写了attach/detach并发出plot3DItemAttached信号。此处通过qobject_cast
 * 判断plot是否为DAChart3DWidget，如果是则连接其plot3DItemAttached信号，
 * 在item detach时自动清空mPlot3DItem，防止悬垂指针。
 */
void DAAbstractChart3DItemSettingWidget::setPlot3D(Qwt3DPlot* plot)
{
    Qwt3DPlot* oldPlot = mPlot3D.data();
    if (oldPlot != plot) {
        if (oldPlot) {
            disconnect(oldPlot, nullptr, this, nullptr);
        }
        mPlot3D = plot;
        if (plot) {
            // 尝试转换为DAChart3DWidget以连接plot3DItemAttached信号
            DAChart3DWidget* chart3DWidget = qobject_cast< DAChart3DWidget* >(plot);
            if (chart3DWidget) {
                connect(chart3DWidget, &DAChart3DWidget::plot3DItemAttached,
                        this, &DAAbstractChart3DItemSettingWidget::plot3DItemAttached);
            }
        }
    }
}

/**
 * @brief 获取当前3D绘图窗口
 */
Qwt3DPlot* DAAbstractChart3DItemSettingWidget::getPlot3D() const
{
    return mPlot3D;
}

/**
 * @brief setPlot3DItem之后调用的虚函数
 */
void DAAbstractChart3DItemSettingWidget::updateUI(Qwt3DPlotItem* item)
{
    Q_UNUSED(item);
}

/**
 * @brief 触发3D重绘
 *
 * 调用QWidget::update()触发Qwt3DPlot::paintGL()重绘
 */
void DAAbstractChart3DItemSettingWidget::replot()
{
    if (mPlot3D) {
        mPlot3D->update();
    }
}

/**
 * @brief item挂载/卸载槽函数
 *
 * 连接DAChart3DWidget::plot3DItemAttached信号，在item被detach时
 * 自动清空mPlot3DItem，防止悬垂指针（与2D的plotItemAttached行为一致）。
 * @param plotItem 发生挂载/卸载的item
 * @param on true表示attach，false表示detach
 */
void DAAbstractChart3DItemSettingWidget::plot3DItemAttached(Qwt3DPlotItem* plotItem, bool on)
{
    if (!on && plotItem == mPlot3DItem) {
        // item脱离plot，有可能会被delete
        setPlot3DItem(nullptr);
    }
}

}  // namespace DA
